/* PicoNIX用の小さなMCP Streamable HTTP実験サーバ．
 * 2026-07-28版のJSON応答のみ．SSEと旧版のinitializeは対象外． */
#include "unistd.h"
#include "fcntl.h"
#include "stdio.h"
#include "string.h"
#include <stdint.h>

#define PORT 8080
#define REQUEST_SIZE 3600
#define BODY_LIMIT 3200
#define RESPONSE_SIZE 1400
#define COMMAND_SIZE 120
#define COMMAND_OUTPUT_LIMIT 4096
#define COMMAND_TIMEOUT_MS 3000

/* 要求処理は一接続ずつ行う．大きなバッファは4 KiBスタックに置かない． */
static char request[REQUEST_SIZE];
static char response[RESPONSE_SIZE];
/* 応答用の比較的大きいバッファは4 KiBスタックを圧迫しないよう静的確保． */
static int allow_shell; /* 明示的な --shell のときだけ任意コマンドを公開する． */

/* JSON文字列とコンテナを走査し，指定された値の終端を探す． */
static const char *space(const char *p, const char *end) {
    while (p < end && (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n'))
        p++;
    return p;
}

/* JSON文字列の終端位置を検証しながら取得する． */
static const char *json_string_end(const char *p, const char *end) {
    if (p == end || *p++ != '"')
        return 0;
    while (p < end) {
        if (*p == '"')
            return p + 1;
        if ((unsigned char)*p < 32)
            return 0;
        if (*p++ == '\\') {
            if (p == end)
                return 0;
            p++;
        }
    }
    return 0;
}

/* JSON値の終端位置を調べる． */
static const char *json_end(const char *p, const char *end) {
    p = space(p, end);
    if (p == end)
        return 0;
    if (*p == '"')
/* JSON文字列の終端位置を検証しながら取得する． */
        return json_string_end(p, end);
    if (*p == '{' || *p == '[') {
        int object = *p == '{';
        char closing = object ? '}' : ']';
        p = space(p + 1, end);
        if (p < end && *p == closing)
            return p + 1;
        while (p < end) {
            if (object) {
                p = json_string_end(p, end);
                if (!p)
                    return 0;
                p = space(p, end);
                if (p == end || *p != ':')
                    return 0;
                p = space(p + 1, end);
            }
            p = json_end(p, end);
            if (!p)
                return 0;
            p = space(p, end);
            if (p < end && *p == closing)
                return p + 1;
            if (p == end || *p != ',')
                return 0;
            p = space(p + 1, end);
        }
        return 0;
    }
    while (p < end && *p != ',' && *p != '}' && *p != ']' &&
           *p != ' ' && *p != '\t' && *p != '\r' && *p != '\n') p++;
    return p;
}

/* 単純なJSONオブジェクトから，直下のキーのみを探す． */
static const char *member(const char *obj, const char *end, const char *key,
                          const char **value_end) {
    const char *p = space(obj, end);
    if (p == end || *p++ != '{')
        return 0;
    while ((p = space(p, end)) < end && *p != '}') {
        const char *k = p;
        p = json_string_end(p, end);
        if (!p)
            return 0;
        unsigned len = (unsigned)strlen(key);
        int match = (unsigned)(p - k) == len + 2 && !memcmp(k + 1, key, len);
        p = space(p, end);
        if (p == end || *p != ':')
            return 0;
        const char *v = space(p + 1, end);
        p = json_end(v, end);
        if (!p)
            return 0;
        if (match) {
            *value_end = p;
            return v;
        }
        p = space(p, end);
        if (p < end && *p == ',')
            p++;
        else if (p < end && *p == '}') break;
        else return 0;
    }
    return 0;
}

/* JSON文字列が指定文字列と一致するか調べる． */
static int json_equals(const char *p, const char *end, const char *s) {
    unsigned n = (unsigned)strlen(s);
    return p && end - p == (int)n + 2 && *p == '"' &&
           p[n + 1] == '"' && !memcmp(p + 1, s, n);
}

/* 小さなASCIIテキストをJSON文字列に安全に入れる． */
static int append_escaped(char *dst, unsigned cap, const char *value) {
    unsigned n = 0;
    while (*value) {
        unsigned char c = (unsigned char)*value++;
        const char *seq = 0;
        if (c == '"') seq = "\\\"";
        else if (c == '\\') seq = "\\\\";
        else if (c == '\n') seq = "\\n";
        else if (c == '\r') seq = "\\r";
        else if (c == '\t') seq = "\\t";
        else if (c < 32 || c > 126) return -1;
        if (seq) {
            while (*seq) {
                if (n + 1 >= cap)
                    return -1;
                dst[n++] = *seq++;
            }
        } else {
            if (n + 1 >= cap)
                return -1;
            dst[n++] = (char)c;
        }
    }
    if (n >= cap)
        return -1;
    dst[n] = 0;
    return (int)n;
}

/* デバイスから現在の情報を読み取る． */
static int read_device(const char *path, char *out, unsigned cap) {
    int fd = open(path, O_RDONLY);
    if (fd < 0)
        return -1;
    int n = read(fd, out, cap - 1);
    close(fd);
    if (n < 0)
        return -1;
    out[n] = 0;
    return n;
}

/* 指定されたデータをソケットへ送信する． */
static int send_all(int fd, const char *p, unsigned size) {
    unsigned retries = 0;
    while (size) {
        int n = send(fd, p, size, 0);
        if (n == OS_EAGAIN || n == 0) {
            if (++retries > 300)
                return -1;
            msleep(2);
            continue;
        }
        if (n < 0)
            return -1;
        p += n;
        size -= (unsigned)n;
        retries = 0;
    }
    return 0;
}

/* HTTPレスポンスを組み立てて返す． */
static void http_reply(int fd, int status, const char *body) {
    const char *reason = status == 200 ? "200 OK" :
                         status == 400 ? "400 Bad Request" :
                         status == 403 ? "403 Forbidden" :
                         status == 404 ? "404 Not Found" :
                         status == 405 ? "405 Method Not Allowed" :
                         status == 413 ? "413 Content Too Large" : "500 Internal Server Error";
    char header[192];
    unsigned len = (unsigned)strlen(body);
    int n = snprintf(header, sizeof header,
                     "HTTP/1.1 %s\r\nContent-Type: application/json\r\nContent-Length: %u\r\nConnection: close\r\nCache-Control: no-store\r\n\r\n",
                     reason, len);
    if (n > 0 && (unsigned)n < sizeof header && send_all(fd, header, (unsigned)n) == 0)
        send_all(fd, body, len);
}

/* ヘッダは比較に用いる名前と値だけを取得し，重複を拒否する． */
static int header_value(const char *start, const char *end, const char *name,
                        char *out, unsigned capacity) {
    unsigned matches = 0;
    const char *p = start;
    while (p < end) {
        const char *line = p;
        while (p + 1 < end && !(p[0] == '\r' && p[1] == '\n'))
            p++;
        if (p + 1 >= end)
            return -1;
        const char *line_end = p;
        p += 2;
        if (line_end == line)
            break;
        const char *colon = line;
        while (colon < line_end && *colon != ':')
            colon++;
        if (colon == line_end)
            continue;
        unsigned k = (unsigned)(colon - line);
        if (k != strlen(name))
            continue;
        unsigned i;
        for (i = 0; i < k; i++) {
            char a = line[i], b = name[i];
            if (a >= 'A' && a <= 'Z')
                a += 'a' - 'A';
            if (b >= 'A' && b <= 'Z')
                b += 'a' - 'A';
            if (a != b)
                break;
        }
        if (i != k)
            continue;
        if (++matches > 1)
            return -1;
        const char *v = colon + 1;
        while (v < line_end && (*v == ' ' || *v == '\t'))
            v++;
        while (line_end > v && (line_end[-1] == ' ' || line_end[-1] == '\t'))
            line_end--;
        unsigned n = (unsigned)(line_end - v);
        if (n >= capacity)
            return -1;
        memcpy(out, v, n);
        out[n] = 0;
    }
    return matches ? 1 : 0;
}

/* JSON-RPCエラー応答を作成する． */
static void rpc_error(const char *id, int code, const char *message) {
    snprintf(response, sizeof response,
             "{\"jsonrpc\":\"2.0\",\"id\":%s,\"error\":{\"code\":%d,\"message\":\"%s\"}}",
             id, code, message);
}

/* JSON-RPC正常応答を作成する． */
static void rpc_result(const char *id, const char *result) {
    snprintf(response, sizeof response,
             "{\"jsonrpc\":\"2.0\",\"id\":%s,\"result\":%s}", id, result);
}

/* MCP tool: send plain ASCII over UDP. Successful sendto() does not
 * guarantee delivery, queueing, or completion of LED rendering. */
static int scroll_message(const char *v, const char *end) {
    if(!v || end-v<3 || *v!='"' || end[-1]!='"')return -1;
    unsigned n=(unsigned)(end-v-2);
    if(n==0 || n>128)return -1;
    unsigned char packet[128];
    for(unsigned i=0;i<n;i++){
        unsigned char c=(unsigned char)v[i+1];
        if(c<32 || c>126 || c=='\\' || c=='"')return -1;
        packet[i]=c;
    }
    int fd=socket(AF_INET,SOCK_DGRAM,0);
    if(fd<0)return -2;
    struct sockaddr_in dst={0};dst.sin_family=AF_INET;dst.sin_port=htons(2001);
    unsigned char *ip=(unsigned char *)&dst.sin_addr.s_addr;
    ip[0]=127;ip[1]=0;ip[2]=0;ip[3]=1;
    int sent=sendto(fd,packet,n,0,(struct sockaddr *)&dst,sizeof dst);
    close(fd);
    return sent==(int)n?0:-2;
}

/* JSON文字列からコマンドを読み取る．改行と制御文字は拒否する． */
static int command_argument(const char *v, const char *end, char out[COMMAND_SIZE]) {
    if (!v || end-v<3 || *v!='"' || end[-1]!='"') return -1;
    unsigned n=0;
    for (const char *p=v+1; p<end-1; p++) {
        unsigned char c=(unsigned char)*p;
        if(c=='\\') {
            if(++p==end-1 || (*p!='\\' && *p!='"')) return -1;
            c=(unsigned char)*p;
        }
        if(c<32 || c>126 || n+1>=COMMAND_SIZE) return -1;
        out[n++]=(char)c;
    }
    if(!n) return -1;
    out[n]=0;
    return 0;
}

/* JSONの文字列を逐次復号する．fd<0なら長さと書式だけを検査する．
 * 作業バッファを持たずに最大2 KiBのソースをRAMファイルへ書ける． */
static int decode_file_text(const char *v, const char *end, int fd) {
    if (!v || end-v<2 || *v!='"' || end[-1]!='"') return -1;
    unsigned count=0;
    char part[128]; unsigned used=0;
    for (const char *p=v+1; p<end-1; p++) {
        unsigned char c=(unsigned char)*p;
        if (c=='\\') {
            if (++p>=end-1) return -1;
            switch (*p) {
            case 'n': c='\n'; break;
            case 'r': c='\r'; break;
            case 't': c='\t'; break;
            case '"': c='"'; break;
            case '\\': c='\\'; break;
            case '/': c='/'; break;
            case 'b': c='\b'; break;
            case 'f': c='\f'; break;
            case 'u': {
                if (end-1-p<4 || p[1]!='0' || p[2]!='0') return -1;
                int hi=-1, lo=-1;
                char a=p[3], b=p[4];
                if (a>='0'&&a<='9') hi=a-'0';
                else if (a>='a'&&a<='f') hi=a-'a'+10;
                else if (a>='A'&&a<='F') hi=a-'A'+10;
                if (b>='0'&&b<='9') lo=b-'0';
                else if (b>='a'&&b<='f') lo=b-'a'+10;
                else if (b>='A'&&b<='F') lo=b-'A'+10;
                if (hi<0 || lo<0) return -1;
                c=(unsigned char)((hi<<4)|lo);
                p+=4;
                break;
            }
            default: return -1;
            }
        } else if (c<32 || c>126) return -1;
        /* 初期版はASCIIテキスト専用．NULと非ASCIIは扱わない． */
        if (!c || c>127 || ++count>2048) return -1;
        part[used++]=(char)c;
        if (used==sizeof part) {
            if (fd>=0 && write(fd,part,used)!=(int)used) return -2;
            used=0;
        }
    }
    if (fd>=0 && used && write(fd,part,used)!=(int)used) return -2;
    return (int)count;
}

/* 書込先は/tmp直下の短いファイル名だけに制限する． */
static int file_path_argument(const char *v,const char *end,char path[32]) {
    if (!v || end-v<8 || *v!='"' || end[-1]!='"') return -1;
    unsigned n=(unsigned)(end-v-2);
    if (n<6 || n>=32 || memcmp(v+1,"/tmp/",5)) return -1;
    for (unsigned i=5;i<n;i++) {
        char c=v[1+i];
        if (!((c>='a'&&c<='z') || (c>='A'&&c<='Z') ||
              (c>='0'&&c<='9') || c=='_' || c=='-' || c=='.')) return -1;
    }
    memcpy(path,v+1,n); path[n]=0;
    return 0;
}

/* HTTP/1.1のchunked転送．HTTP本文をRAMに貯めず送信する． */
static int send_chunk(int fd, const char *data, unsigned len) {
    char header[16];
    int n=snprintf(header,sizeof header,"%x\r\n",len);
    if(n<=0 || (unsigned)n>=sizeof header) return -1;
    return send_all(fd,header,(unsigned)n)<0 ||
           (len && send_all(fd,data,len)<0) || send_all(fd,"\r\n",2)<0 ? -1:0;
}

/* バイト列をJSON文字列の内容として送り出す．非ASCIIは\u00XXにする． */
static int send_escaped_chunk(int fd, const char *src, unsigned len) {
    char out[256];
    unsigned used=0;
    const char hex[]="0123456789abcdef";
    for(unsigned i=0;i<len;i++) {
        unsigned char c=(unsigned char)src[i];
        char esc[6];
        const char *part=esc;
        unsigned count=1;
        if(c=='"' || c=='\\') { esc[0]='\\'; esc[1]=(char)c; count=2; }
        else if(c=='\n' || c=='\r' || c=='\t') {
            esc[0]='\\'; esc[1]=c=='\n'?'n':c=='\r'?'r':'t'; count=2;
        } else if(c<32 || c>126) {
            esc[0]='\\'; esc[1]='u'; esc[2]='0'; esc[3]='0';
            esc[4]=hex[c>>4]; esc[5]=hex[c&15]; count=6;
        } else { esc[0]=(char)c; }
        if(used+count>sizeof out) {
            if(send_chunk(fd,out,used)<0) return -1;
            used=0;
        }
        for(unsigned j=0;j<count;j++) out[used++]=part[j];
    }
    return used ? send_chunk(fd,out,used) : 0;
}

/* シェルを起動し，最大4 KiBの出力を即時にHTTPへ流す．
 * 子プロセスの出力は上限到達後も読み捨て，パイプ詰まりを避ける． */
static int execute_command_stream(int fd, const char *id, const char *cmd) {
    int fds[2]={-1,-1}, old_out=-1,old_err=-1,pid=-1;
    int done=0,eof=0,status=0,error=0,truncated=0;
    unsigned used=0;
    if(pipe(fds)<0) error=1;
    if(!error) {
        old_out=dup(1); old_err=dup(2);
        if(old_out<0 || old_err<0 || dup2(fds[1],1)<0 || dup2(fds[1],2)<0)
            error=1;
        else {
            const char *args[]={"sh","-c",cmd};
            pid=spawn(program_id("sh"),3,args);
            if(pid<0) error=1;
        }
        if(old_out>=0) { dup2(old_out,1);close(old_out); }
        if(old_err>=0) { dup2(old_err,2);close(old_err); }
        close(fds[1]);fds[1]=-1;
    }
    if(!error && fcntl(fds[0],F_SETFL,O_NONBLOCK)<0) error=1;

    /* ステータスや出力サイズに依存せず，正常なJSON応答を閉じる． */
    const char *headers="HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nTransfer-Encoding: chunked\r\nConnection: close\r\nCache-Control: no-store\r\n\r\n";
    if(send_all(fd,headers,(unsigned)strlen(headers))<0) goto finish;
    char prefix[160];
    int plen=snprintf(prefix,sizeof prefix,
        "{\"jsonrpc\":\"2.0\",\"id\":%s,\"result\":{\"resultType\":\"complete\",\"content\":[{\"type\":\"text\",\"text\":\"",id);
    if(plen<0 || (unsigned)plen>=sizeof prefix || send_chunk(fd,prefix,(unsigned)plen)<0) goto finish;

    if(!error) {
        for(unsigned elapsed=0;elapsed<COMMAND_TIMEOUT_MS;elapsed+=10) {
            char raw[64];
            int n=read(fds[0],raw,sizeof raw);
            if(n>0) {
                unsigned copy=(unsigned)n;
                if(used>=COMMAND_OUTPUT_LIMIT) copy=0;
                else if(copy>COMMAND_OUTPUT_LIMIT-used) copy=COMMAND_OUTPUT_LIMIT-used;
                if(copy && send_escaped_chunk(fd,raw,copy)<0) goto finish;
                used+=copy;
                if(copy<(unsigned)n) truncated=1;
            } else if(n==0) eof=1;
            else if(n!=OS_EAGAIN) {error=1;break;}
            if(!done && waitpid(pid,&status,WNOHANG)==pid) done=1;
            if(done && eof) break;
            msleep(10);
        }
        if(!done) {
            kill(pid);
            waitpid(pid,&status,0);
            pid=-1;
            error=2;
        } else if(!eof) error=2;
    }
    const char *suffix=error==2?"\\n[timeout]":error?"\\n[execution error]":"";
    if(truncated && send_chunk(fd,"\\n[output truncated]",20)<0) goto finish;
    if(*suffix && send_chunk(fd,suffix,(unsigned)strlen(suffix))<0) goto finish;
    const char *end=error?"\"}],\"isError\":true}}":"\"}],\"isError\":false}}";
    if(send_chunk(fd,end,(unsigned)strlen(end))<0) goto finish;
    send_all(fd,"0\r\n\r\n",5);
finish:
    if(fds[0]>=0) close(fds[0]);
    if(fds[1]>=0) close(fds[1]);
    if(pid>0 && !done) { kill(pid);waitpid(pid,&status,0); }
    return 0;
}

/* MCPのJSON要求を解析して処理する． */
static int process_json(int fd, const char *body, unsigned body_len, const char *http_method,
                        const char *http_name, const char *http_version) {
    const char *end = body + body_len;
    /* 深いネストによる再帰スタック枯渇を防ぐ． */
    unsigned depth = 0;
    int quoted = 0, escaped_char = 0;
    for (const char *scan = body; scan < end; scan++) {
        if (quoted) {
            if (escaped_char)
                escaped_char = 0;
            else if (*scan == '\\') escaped_char = 1;
            else if (*scan == '"') quoted = 0;
        } else if (*scan == '"') quoted = 1;
        else if (*scan == '{' || *scan == '[') {
            if (++depth > 8) { rpc_error("null", -32700, "JSON nesting limit"); return 400; }
        } else if (*scan == '}' || *scan == ']') {
            if (!depth) { rpc_error("null", -32700, "Invalid JSON"); return 400; }
            depth--;
        }
    }
    if (depth || quoted) { rpc_error("null", -32700, "Invalid JSON"); return 400; }
    const char *v_end, *v;
    v = member(body, end, "jsonrpc", &v_end);
    if (!json_equals(v, v_end, "2.0")) { rpc_error("null", -32600, "Invalid Request"); return 400; }
    v = member(body, end, "id", &v_end);
    char id[48] = "null";
    if (!v || v_end - v >= (int)sizeof id || v_end == v) {
        rpc_error("null", -32600, "Request id required"); return 400;
    }
    /* 数値IDまたは文字列IDだけを許可する． */
    if (*v != '"') {
        const char *p = v;
        if (p < v_end && *p == '-')
            p++;
        if (p == v_end) { rpc_error("null", -32600, "Invalid id"); return 400; }
        while (p < v_end && *p >= '0' && *p <= '9')
            p++;
        if (p != v_end) { rpc_error("null", -32600, "Invalid id"); return 400; }
    } else if (json_string_end(v, v_end) != v_end) {
        rpc_error("null", -32600, "Invalid id"); return 400;
    }
    memcpy(id, v, (unsigned)(v_end - v));
    id[v_end - v] = 0;
    v = member(body, end, "method", &v_end);
    if (!v || *v != '"' || json_string_end(v, v_end) != v_end) {
        rpc_error(id, -32600, "Invalid method"); return 400;
    }
    char method[40];
    unsigned mlen = (unsigned)(v_end - v - 2);
    if (mlen >= sizeof method) { rpc_error(id, -32600, "Invalid method"); return 400; }
    memcpy(method, v + 1, mlen);
    method[mlen] = 0;
    if (strcmp(method, http_method)) { rpc_error(id, -32020, "HeaderMismatch"); return 400; }
    const char *params_end, *params = member(body, end, "params", &params_end);
    const char *meta_end, *meta = params ? member(params, params_end, "_meta", &meta_end) : 0;
    v = meta ? member(meta, meta_end, "io.modelcontextprotocol/protocolVersion", &v_end) : 0;
    if (!json_equals(v, v_end, http_version)) {
        rpc_error(id, -32020, "HeaderMismatch"); return 400;
    }
    if (strcmp(http_version, "2026-07-28")) {
        snprintf(response, sizeof response,
                 "{\"jsonrpc\":\"2.0\",\"id\":%s,\"error\":{\"code\":-32022,\"message\":\"Unsupported protocol version\",\"data\":{\"supported\":[\"2026-07-28\"]}}}", id);
        return 400;
    }
    const char *client_end;
    if (!member(meta, meta_end, "io.modelcontextprotocol/clientInfo", &client_end) ||
        !member(meta, meta_end, "io.modelcontextprotocol/clientCapabilities", &client_end)) {
        rpc_error(id, -32600, "Missing client metadata"); return 400;
    }
    if (!strcmp(method, "server/discover")) {
        rpc_result(id, "{\"resultType\":\"complete\",\"supportedVersions\":[\"2026-07-28\"],\"capabilities\":{\"tools\":{}},\"ttlMs\":0,\"cacheScope\":\"private\"}");
        return 200;
    }
    if (!strcmp(method, "tools/list")) {
        if(allow_shell) {
            rpc_result(id, "{\"resultType\":\"complete\",\"tools\":[{\"name\":\"get_temperature\",\"description\":\"Read the RP2040 temperature sensor\",\"inputSchema\":{\"type\":\"object\",\"properties\":{},\"additionalProperties\":false}},{\"name\":\"get_wifi_info\",\"description\":\"Read Pico W Wi-Fi status\",\"inputSchema\":{\"type\":\"object\",\"properties\":{},\"additionalProperties\":false}},{\"name\":\"display_message\",\"description\":\"Send ASCII text to LED ticker via UDP\",\"inputSchema\":{\"type\":\"object\",\"properties\":{\"text\":{\"type\":\"string\"}},\"required\":[\"text\"],\"additionalProperties\":false}},{\"name\":\"write_file\",\"description\":\"Overwrite an ASCII text file under /tmp (max 2048 bytes)\",\"inputSchema\":{\"type\":\"object\",\"properties\":{\"path\":{\"type\":\"string\"},\"content\":{\"type\":\"string\"}},\"required\":[\"path\",\"content\"],\"additionalProperties\":false}},{\"name\":\"execute_command\",\"description\":\"Run one PicoNIX sh -c command (experimental; limited output/time; no isolation)\",\"inputSchema\":{\"type\":\"object\",\"properties\":{\"command\":{\"type\":\"string\"}},\"required\":[\"command\"],\"additionalProperties\":false}}],\"ttlMs\":0,\"cacheScope\":\"private\"}");
        } else {
        rpc_result(id, "{\"resultType\":\"complete\",\"tools\":[{\"name\":\"get_temperature\",\"description\":\"Read the RP2040 temperature sensor\",\"inputSchema\":{\"type\":\"object\",\"properties\":{},\"additionalProperties\":false}},{\"name\":\"get_wifi_info\",\"description\":\"Read Pico W Wi-Fi status\",\"inputSchema\":{\"type\":\"object\",\"properties\":{},\"additionalProperties\":false}}, {\"name\":\"display_message\",\"description\":\"Send ASCII text to the LED ticker over UDP (no delivery guarantee)\",\"inputSchema\":{\"type\":\"object\",\"properties\":{\"text\":{\"type\":\"string\"}},\"required\":[\"text\"],\"additionalProperties\":false}}],\"ttlMs\":0,\"cacheScope\":\"private\"}");
        }
        return 200;
    }
    if (!strcmp(method, "tools/call")) {
        v = params ? member(params, params_end, "name", &v_end) : 0;
        const char *path = 0;
        if (json_equals(v, v_end, "get_temperature"))
            path = "/dev/adc/temp";
        if (json_equals(v, v_end, "get_wifi_info"))
            path = "/proc/net/wifi";
        if (json_equals(v, v_end, "display_message")) {
            if(strcmp(http_name,"display_message")) {rpc_error(id,-32020,"HeaderMismatch");return 400;}
            const char *args_end,*args=params?member(params,params_end,"arguments",&args_end):0;
            const char *txt_end,*txt=args?member(args,args_end,"text",&txt_end):0;
            int result=scroll_message(txt,txt_end);
            const char *message=result==0?"UDP send succeeded (display not confirmed)":result==-2?"UDP send failed":"Invalid ASCII message (1..128 bytes, no escapes)";
            char output[256];
            snprintf(output,sizeof output,"{\"resultType\":\"complete\",\"content\":[{\"type\":\"text\",\"text\":\"%s\"}],\"isError\":%s}",message,result==0?"false":"true");
            rpc_result(id,output);return 200;
        }
        if (allow_shell && json_equals(v,v_end,"write_file")) {
            if (strcmp(http_name,"write_file")) {
                rpc_error(id,-32020,"HeaderMismatch"); return 400;
            }
            const char *args_end,*args=params?member(params,params_end,"arguments",&args_end):0;
            const char *path_end,*path_value=args?member(args,args_end,"path",&path_end):0;
            const char *content_end,*content=args?member(args,args_end,"content",&content_end):0;
            char path[32];
            int bytes=decode_file_text(content,content_end,-1);
            if (file_path_argument(path_value,path_end,path)<0 || bytes<0) {
                rpc_result(id,"{\"resultType\":\"complete\",\"content\":[{\"type\":\"text\",\"text\":\"Invalid path/content: /tmp/NAME, ASCII text up to 2048 bytes\"}],\"isError\":true}");
                return 200;
            }
            int out=open(path,O_WRONLY|O_CREAT|O_TRUNC);
            if (out<0) {
                rpc_result(id,"{\"resultType\":\"complete\",\"content\":[{\"type\":\"text\",\"text\":\"Cannot open RAM file (slot unavailable?)\"}],\"isError\":true}");
                return 200;
            }
            int written=decode_file_text(content,content_end,out);
            close(out);
            if (written!=bytes) {
                rpc_result(id,"{\"resultType\":\"complete\",\"content\":[{\"type\":\"text\",\"text\":\"Write failed; file may be incomplete\"}],\"isError\":true}");
                return 200;
            }
            char result[192];
            snprintf(result,sizeof result,"{\"resultType\":\"complete\",\"content\":[{\"type\":\"text\",\"text\":\"Wrote %d bytes to %s\"}],\"isError\":false}",written,path);
            rpc_result(id,result);
            return 200;
        }
        if (allow_shell && json_equals(v,v_end,"execute_command")) {
            if(strcmp(http_name,"execute_command")) {
                rpc_error(id,-32020,"HeaderMismatch"); return 400;
            }
            const char *args_end,*args=params?member(params,params_end,"arguments",&args_end):0;
            const char *cmd_end,*value=args?member(args,args_end,"command",&cmd_end):0;
            char cmd[COMMAND_SIZE];
            if(command_argument(value,cmd_end,cmd)<0) {
                rpc_error(id,-32602,"Invalid command (ASCII, up to 119 bytes)"); return 200;
            }
            execute_command_stream(fd,id,cmd);
            return -1; /* 応答は既に送信済み． */
        }
        if (!path) { rpc_error(id, -32602, "Unknown tool"); return 200; }
        char tool_name[20];
        unsigned count = (unsigned)(v_end - v - 2);
        if (count >= sizeof tool_name) { rpc_error(id, -32602, "Invalid name"); return 400; }
        memcpy(tool_name, v + 1, count);
        tool_name[count] = 0;
        if (strcmp(http_name, tool_name)) { rpc_error(id, -32020, "HeaderMismatch"); return 400; }
        char raw[256], escaped[560];
        if (read_device(path, raw, sizeof raw) < 0 ||
            append_escaped(escaped, sizeof escaped, raw) < 0) {
            rpc_result(id, "{\"resultType\":\"complete\",\"content\":[{\"type\":\"text\",\"text\":\"Device unavailable\"}],\"isError\":true}");
            return 200;
        }
        char result[640];
        snprintf(result, sizeof result,
                 "{\"resultType\":\"complete\",\"content\":[{\"type\":\"text\",\"text\":\"%s\"}],\"isError\":false}", escaped);
        rpc_result(id, result);
        return 200;
    }
    rpc_error(id, -32601, "Method not found");
    return 404;
}

/* ヘッダ，Content-Length，本文を上限付きで受信する． */
static int read_request(int fd, unsigned *body_offset, unsigned *body_length) {
    unsigned used = 0, header = 0, waits = 0;
    while (waits < 600) {
        fd_set set = 0;
        FD_SET(fd, &set);
        struct timeval timeout = {0, 5000};
        int ready = select(fd + 1, &set, 0, 0, &timeout);
        if (!ready) { waits++; continue; }
        if (ready < 0)
            return -1;
        if (used == sizeof request - 1)
            return 413;
        int n = recv(fd, request + used, sizeof request - 1 - used, 0);
        if (n == OS_EAGAIN) { waits++; continue; }
        if (n <= 0)
            return -1;
        used += (unsigned)n;
        request[used] = 0;
        if (!header) {
            for (unsigned i = 3; i < used; i++) {
                if (request[i - 3] == '\r' && request[i - 2] == '\n' &&
                    request[i - 1] == '\r' && request[i] == '\n') {
                    header = i + 1;
                    break;
                }
            }
            if (!header)
                continue;
            char length[16];
            int has = header_value(request, request + header, "Content-Length", length, sizeof length);
            if (has != 1 || !length[0])
                return 400;
            unsigned value = 0;
            for (const char *p = length; *p; p++) {
                if (*p < '0' || *p > '9')
                    return 400;
                value = value * 10 + (unsigned)(*p - '0');
                if (value > BODY_LIMIT)
                    return 413;
            }
            if (value == 0 || header + value >= sizeof request)
                return 400;
            *body_offset = header;
            *body_length = value;
        }
        if (header && used >= header + *body_length) {
            request[header + *body_length] = 0;
            return 200;
        }
    }
    return 400;
}

/* MCPの接続要求を処理する． */
static void handle(int fd) {
    unsigned body_offset = 0, body_length = 0;
    int status = read_request(fd, &body_offset, &body_length);
    if (status != 200) {
        http_reply(fd, status == 413 ? 413 : 400, "{\"error\":\"Invalid HTTP request\"}");
        return;
    }
    if (strncmp(request, "POST /mcp HTTP/1.", 16)) {
        http_reply(fd, 405, "{\"error\":\"POST /mcp required\"}");
        return;
    }
    char origin[96], method[48], version[24], name[64], type[64], accept[96];
    const char *headers_end = request + body_offset;
    int got_origin = header_value(request, headers_end, "Origin", origin, sizeof origin);
    if (got_origin != 0) {
        /* ブラウザからのアクセスは許可しない．Originなしの検証クライアントのみ． */
        http_reply(fd, got_origin < 0 ? 400 : 403, "{\"error\":\"Origin not allowed\"}");
        return;
    }
    if (header_value(request, headers_end, "Mcp-Method", method, sizeof method) != 1 ||
        header_value(request, headers_end, "Mcp-Name", name, sizeof name) < 0 ||
        header_value(request, headers_end, "MCP-Protocol-Version", version, sizeof version) != 1 ||
        header_value(request, headers_end, "Content-Type", type, sizeof type) != 1 ||
        header_value(request, headers_end, "Accept", accept, sizeof accept) != 1 ||
        !strstr(accept, "application/json") || !strstr(accept, "text/event-stream") ||
        strcmp(type, "application/json")) {
        http_reply(fd, 400, "{\"error\":\"Missing MCP headers\"}");
        return;
    }
    if (header_value(request, headers_end, "Mcp-Name", name, sizeof name) == 0)
        name[0] = 0;
    status = process_json(fd, request + body_offset, body_length, method, name, version);
    if(status>=0) http_reply(fd, status, response);
}

/* MCPサーバを起動して接続要求を受け付ける． */
int main(int argc, char **argv) {
    if (argc != 1 && !(argc==2 && !strcmp(argv[1],"--shell"))) {
        puts("usage: mcpd [--shell]"); return 1;
    }
    allow_shell = argc==2;
    int server = socket(AF_INET, SOCK_STREAM, 0);
    if (server < 0)
        return 1;
    struct sockaddr_in address = {0};
    address.sin_family = AF_INET;
    address.sin_port = htons(PORT);
    if (bind(server, (const struct sockaddr *)&address, sizeof address) < 0 ||
        listen(server, 1) < 0) {
        puts("mcpd: cannot listen on TCP 8080");
        close(server);
        return 1;
    }
    puts("mcpd: listening on TCP 8080 (experimental)");
    if(allow_shell) puts("mcpd: WARNING shell tool enabled; trusted isolated network ONLY");
    for (;;) {
        int client = accept(server, 0, 0);
        if (client == OS_EAGAIN) { msleep(5); continue; }
        if (client < 0)
            break;
        if (fcntl(client, F_SETFL, O_NONBLOCK) >= 0)
            handle(client);
        close(client);
    }
    close(server);
    return 0;
}
