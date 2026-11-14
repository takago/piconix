/* PicoNIX用の小さなMCP Streamable HTTP実験サーバ．
 * 2026-07-28版のJSON応答のみ．SSEと旧版のinitializeは対象外． */
#include "unistd.h"
#include "fcntl.h"
#include "stdio.h"
#include "string.h"
#include <stdint.h>

#define PORT 8080
#define REQUEST_SIZE 2800
#define BODY_LIMIT 2400
#define RESPONSE_SIZE 900
#define COMMAND_SIZE 120
#define COMMAND_OUTPUT_LIMIT 4096
#define COMMAND_TIMEOUT_MS 3000

/* 要求処理は一接続ずつ行う．大きなバッファは4 KiBスタックに置かない． */
static char request[REQUEST_SIZE];
static char response[RESPONSE_SIZE];
/* 応答用の比較的大きいバッファは4 KiBスタックを圧迫しないよう静的確保． */
/* mcpdの起動時に生成し，HTTP要求とは独立して保持するシェルPTY． */
static int term_index=-1;
static int term_master=-1,term_pid=-1;
/* 通信失敗をHTTP応答とは別のコンソールで観測する．1要求中の最初の失敗を保持する． */
static int send_fail_reason, send_fail_value;



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

/* FDの読書き可能状態を待つ．timeout_ms<0なら無期限待機する．
 * PicoNIXのselectはTCP listenソケット，パイプ，PTYにも対応する． */
static int wait_fd(int fd, int writing, int timeout_ms) {
    fd_set r=0,w=0;
    if(fd>=16 || fd < -1 || (fd<0 && timeout_ms<0)) return -1;
    if(fd>=0) {
        if(writing) FD_SET(fd,&w);
        else FD_SET(fd,&r);
    }
    if(timeout_ms<0) return select(fd+1,writing?0:&r,writing?&w:0,0,0);
    struct timeval tv={timeout_ms/1000,(timeout_ms%1000)*1000};
    return select(fd+1,fd<0?0:(writing?0:&r),fd<0?0:(writing?&w:0),0,&tv);
}

/* 非ブロッキング送信は部分成功と一時的な送信キュー満杯を区別する．
 * 実際の受信完了を保証するものではない（tcp_closeがACK待ちを引き継ぐ）． */
static int send_all(int fd, const char *p, unsigned size) {
    unsigned retries = 0;
    while (size) {
        int n = send(fd, p, size, 0);
        if (n == OS_EAGAIN || n == 0) {
            if (++retries > 1500) {
                if (!send_fail_reason) { send_fail_reason=1; send_fail_value=(int)size; }
                return -1;
            }
            if(wait_fd(fd,1,2)<0) {
                if(!send_fail_reason) { send_fail_reason=2; send_fail_value=-1; }
                return -1;
            }
            continue;
        }
        if (n < 0) {
            if (!send_fail_reason) { send_fail_reason=2; send_fail_value=n; }
            return -1;
        }
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
/* 1チャンクを一つの送信バッファへ組み立てる．
 * 小さなtcp_writeを3回発行するとTCP送信キュー（8要素）が枯渇しやすい．
 * 768バイト以下をまとめ，それ以上は従来の分割経路で処理する． */
static int send_chunk(int fd, const char *data, unsigned len) {
    char header[16];
    int n=snprintf(header,sizeof header,"%x\r\n",len);
    if(n<=0 || (unsigned)n>=sizeof header) return -1;
    if(len<=768) {
        /* ストリーム中は通常応答に未使用の静的response[]を再利用する． */
        char *frame=response;
        memcpy(frame,header,(unsigned)n);
        if(len) memcpy(frame+n,data,len);
        frame[n+len]='\r'; frame[n+len+1]='\n';
        return send_all(fd,frame,(unsigned)n+len+2);
    }
    return send_all(fd,header,(unsigned)n)<0 ||
           (len && send_all(fd,data,len)<0) || send_all(fd,"\r\n",2)<0 ? -1:0;
}

/* バイト列をJSON文字列の内容として送り出す．非ASCIIは\u00XXにする． */
static int send_escaped_chunk(int fd, const char *src, unsigned len) {
    char out[768];
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
            char raw[256];
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
            /* EOFは常にreadableなので，子の終了待ちは時間付きselect(0)で行う． */
            if(wait_fd(eof?-1:fds[0],0,10)<0) { error=1; break; }
        }
        if(!done) {
            kill(pid);
            waitpid(pid,&status,0);
            pid=-1;
            error=2;
        } else if(!eof) error=2;
    }
    if(!error && done && status!=0) error=1;
    const char *suffix=error==2?"\\n[timeout]":error?"\\n[execution error]":"";
    if(truncated && send_chunk(fd,"\\n[output truncated]",20)<0) goto finish;
    if(*suffix && send_chunk(fd,suffix,(unsigned)strlen(suffix))<0) goto finish;
    if(error==1 && done && status!=0) {
        char code_text[32];
        int n=snprintf(code_text,sizeof code_text,"\\n[exit code: %d]",status);
        if(n<=0 || (unsigned)n>=sizeof code_text || send_chunk(fd,code_text,(unsigned)n)<0) goto finish;
    }
    const char *end=error?"\"}],\"isError\":true}}":"\"}],\"isError\":false}}";
    if(send_chunk(fd,end,(unsigned)strlen(end))<0) goto finish;
    if(send_all(fd,"0\r\n\r\n",5)<0) goto finish;
finish:
    if(fds[0]>=0) close(fds[0]);
    if(fds[1]>=0) close(fds[1]);
    if(pid>0 && !done) { kill(pid);waitpid(pid,&status,0); }
    return 0;
}

/* 応答の総サイズをRAMに載せずMCPツール一覧を送信する． */
static int list_tools_stream(int fd,const char *id) {
    const char *headers="HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nTransfer-Encoding: chunked\r\nConnection: close\r\n\r\n";
    char prefix[128];
    int n=snprintf(prefix,sizeof prefix,"{\"jsonrpc\":\"2.0\",\"id\":%s,\"result\":{\"resultType\":\"complete\",\"cacheScope\":\"private\",\"ttlMs\":0,\"tools\":[",id);
    if(n<=0 || (unsigned)n>=sizeof prefix || send_all(fd,headers,(unsigned)strlen(headers))<0 || send_chunk(fd,prefix,(unsigned)n)<0) return -1;
    const char *base="{\"name\":\"get_temperature\",\"description\":\"Read temperature\",\"inputSchema\":{\"type\":\"object\",\"properties\":{},\"additionalProperties\":false}}";
    if(send_chunk(fd,base,(unsigned)strlen(base))<0) return -1;
    {
        /* 定数ポインタ配列はRAMリンクアドレス依存を生むため使わない． */
#define MCP_ADD(fragment) do { const char *part=(fragment); if(send_chunk(fd,part,(unsigned)strlen(part))<0) return -1; } while(0)
        MCP_ADD(",{\"name\":\"get_manual\",\"description\":\"Read PicoNIX manual. Call get_manual({}) or topic=index for contents.\",\"inputSchema\":{\"type\":\"object\",\"properties\":{\"topic\":{\"type\":\"string\",\"description\":\"Omit or use index for contents\"}}}}");
        MCP_ADD(",{\"name\":\"write_file\",\"description\":\"Write /tmp file\",\"inputSchema\":{\"type\":\"object\",\"properties\":{\"path\":{\"type\":\"string\"},\"content\":{\"type\":\"string\"}},\"required\":[\"path\",\"content\"]}}");
        MCP_ADD(",{\"name\":\"execute_command\",\"description\":\"Run shell command (noninteractive); do not stop mcpd\",\"inputSchema\":{\"type\":\"object\",\"properties\":{\"command\":{\"type\":\"string\"}},\"required\":[\"command\"]}}");
        MCP_ADD(",{\"name\":\"terminal_send\",\"description\":\"Send text or named key. Keys: UP DOWN LEFT RIGHT ENTER ESC ESCAPE TAB SPACE SP BACKSPACE BS DELETE CTRL_A..CTRL_Z. Use text for characters; key for special keys. Returns accepted byte count only; call terminal_read to inspect screen.\",\"inputSchema\":{\"type\":\"object\",\"properties\":{\"text\":{\"type\":\"string\"},\"key\":{\"type\":\"string\"}}}}");
        MCP_ADD(",{\"name\":\"terminal_read\",\"description\":\"Read ANSI screen: SGR 4/24 marks cursor; SGR 7/0 marks inverse\",\"inputSchema\":{\"type\":\"object\",\"properties\":{}}}");
#undef MCP_ADD
    }
    if(send_chunk(fd,"]}}",3)<0) return -1;
    return send_all(fd,"0\r\n\r\n",5);
}

/* mcpd専用PTYに対話シェルを起動する．親の標準FDは復元する． */
static int terminal_start_shell(void) {
    if(term_master>=0) return -1;
    int master=open("/dev/ptmx",O_RDWR|O_NONBLOCK);
    if(master<0) return -1;
    if(ioctl(master,OS_PTY_SCREEN_ONLY,1)<0) {close(master);return -1;}
    int slot=ioctl(master,OS_PTY_GET_INDEX,0);
    if(slot<0) {close(master);return -1;}
    char path[24];snprintf(path,sizeof path,"/dev/pts/%d",slot);
    int slave=open(path,O_RDWR);
    int saved[3]={-1,-1,-1},pid=-1,ok=slave>=0;
    if(ok) for(int k=0;k<3;k++) if((saved[k]=dup(k))<0) ok=0;
    if(ok) for(int k=0;k<3;k++) if(dup2(slave,k)<0) ok=0;
    if(ok) {
        const char *args[1]={"sh"};
        pid=spawn_foreground(program_id("sh"),1,args);
    }
    for(int k=0;k<3;k++) if(saved[k]>=0) {(void)dup2(saved[k],k);close(saved[k]);}
    if(slave>=0)close(slave);
    if(pid<=0) {close(master);return -1;}
    term_master=master;term_pid=pid;term_index=slot;
    return 0;
}
/* 現在のPTYセッションを終了し，Masterと子プロセスを解放する． */
static void terminal_close_session(void) {
    if(term_pid>0) {
        int status=0;
        if(waitpid(term_pid,&status,WNOHANG)==0) {kill(term_pid);(void)waitpid(term_pid,&status,0);}
    }
    if(term_master>=0)close(term_master);
    term_master=term_pid=term_index=-1;
}
/* シェルがexit等で終了したらPTYを新しく確保する．
 * 入力側では復旧直後のキー入力を破棄し，意図しないコマンド実行を防ぐ． */
static int terminal_ensure_shell(void) {
    if(term_pid>0) {
        int status=0;
        if(waitpid(term_pid,&status,WNOHANG)==0) return 0;
    }
    terminal_close_session();
    return terminal_start_shell()<0 ? -1 : 1;
}
/* /dev/pts/N.screenを1回読み，現在の画面をchunkedで返す．
 * 描画と並行した場合に一貫したスナップショットになる保証はない． */
static int terminal_screen_stream(int fd,const char *id) {
    if(terminal_ensure_shell()<0) {rpc_error(id,-32030,"PTY shell unavailable");return 200;}
    char path[32];snprintf(path,sizeof path,"/dev/pts/%d.screen",term_index);
    int screen=open(path,O_RDONLY);
    if(screen<0) {rpc_error(id,-32030,"Screen unavailable");return 200;}
    const char *headers="HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nTransfer-Encoding: chunked\r\nConnection: close\r\n\r\n";
    char prefix[160];
    int n=snprintf(prefix,sizeof prefix,"{\"jsonrpc\":\"2.0\",\"id\":%s,\"result\":{\"resultType\":\"complete\",\"content\":[{\"type\":\"text\",\"text\":\"",id);
    if(n<0 || (unsigned)n>=sizeof prefix || send_all(fd,headers,(unsigned)strlen(headers))<0 || send_chunk(fd,prefix,(unsigned)n)<0) goto done;
    char row[81];
    while((n=read(screen,row,sizeof row))>0)
        if(send_escaped_chunk(fd,row,(unsigned)n)<0) goto done;
    if(n<0) goto done;
    {
        const char *suffix="\"}],\"isError\":false}}";
        if(send_chunk(fd,suffix,(unsigned)strlen(suffix))<0) goto done;
        if(send_all(fd,"0\r\n\r\n",5)<0) goto done;
    }
done:
    close(screen);
    return -1;
}

/* ROM内の/manファイルをMCPへストリーミング転送する．
 * 引数なしはREADME(目次)，topic指定は該当ページ．大きなRAM配列を使わない． */
static int send_full_manual(int fd,const char *id,const char *topic) {
    char path[80];
    snprintf(path,sizeof path,"/man/%s",topic);
    int file=open(path,O_RDONLY);
    if(file<0) {
        rpc_result(id,"{\"resultType\":\"complete\",\"content\":[{\"type\":\"text\",\"text\":\"Unknown topic. Use index.\"}],\"isError\":true}");
        return 200;
    }
    const char *headers="HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nTransfer-Encoding: chunked\r\nConnection: close\r\nCache-Control: no-store\r\n\r\n";
    char prefix[160];
    int n=snprintf(prefix,sizeof prefix,
        "{\"jsonrpc\":\"2.0\",\"id\":%s,\"result\":{\"resultType\":\"complete\",\"content\":[{\"type\":\"text\",\"text\":\"",id);
    if(n<0 || (unsigned)n>=sizeof prefix) { close(file); return -1; }
    if(send_all(fd,headers,(unsigned)strlen(headers))<0 || send_chunk(fd,prefix,(unsigned)n)<0) goto finish;
    char part[192];
    int got;
    while((got=read(file,part,sizeof part))>0)
        if(send_escaped_chunk(fd,part,(unsigned)got)<0) goto finish;
    if(got<0) goto finish;
    const char *suffix="\"}],\"isError\":false}}";
    if(send_chunk(fd,suffix,(unsigned)strlen(suffix))<0) goto finish;
    if(send_all(fd,"0\r\n\r\n",5)<0) goto finish;
finish:
    close(file);
    return -1;
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
        list_tools_stream(fd,id);
        return -1;
    }
    if (!strcmp(method, "tools/call")) {
        v = params ? member(params, params_end, "name", &v_end) : 0;
        const char *path = 0;
        if (json_equals(v, v_end, "get_temperature"))
            path = "/dev/adc/temp";
        if(json_equals(v,v_end,"terminal_send") || json_equals(v,v_end,"terminal_read")) {
            const char *expected=json_equals(v,v_end,"terminal_send") ? "terminal_send" : "terminal_read";
            if(strcmp(http_name,expected)) {rpc_error(id,-32020,"HeaderMismatch");return 400;}
            if(!strcmp(expected,"terminal_read")) return terminal_screen_stream(fd,id);
            int state=terminal_ensure_shell();
            if(state!=0) {
                rpc_result(id,state<0 ?
                    "{\"resultType\":\"complete\",\"content\":[{\"type\":\"text\",\"text\":\"PTY shell unavailable\"}],\"isError\":true}" :
                    "{\"resultType\":\"complete\",\"content\":[{\"type\":\"text\",\"text\":\"Shell restarted; read terminal before sending input\"}],\"isError\":true}");
                return 200;
            }
            const char *args_end,*args=params?member(params,params_end,"arguments",&args_end):0;
            const char *v2_end,*value=args?member(args,args_end,"key",&v2_end):0;
            int written=-1;
            if(value) {
                char key[20];
                unsigned len=(unsigned)(v2_end-value-2);
                if(*value=='"' && len>0 && len<sizeof key) {
                    memcpy(key,value+1,len);key[len]=0;
                    char path[32];snprintf(path,sizeof path,"/dev/pts/%d.key",term_index);
                    char command[24]="key:";
                    memcpy(command+4,key,len);
                    int fdkey=open(path,O_WRONLY);
                    if(fdkey>=0) {
                        written=write(fdkey,command,len+4);
                        close(fdkey);
                        if(written>=0) written=(int)len;
                    }
                }
            } else {
                const char *text_end,*content=args?member(args,args_end,"text",&text_end):0;
                if(content) written=decode_file_text(content,text_end,term_master);
            }
            if(written<0) {
                if(written==OS_EINVAL && value)
                    rpc_result(id,"{\"resultType\":\"complete\",\"content\":[{\"type\":\"text\",\"text\":\"Unknown key. See terminal_send key list.\"}],\"isError\":true}");
                else rpc_result(id,"{\"resultType\":\"complete\",\"content\":[{\"type\":\"text\",\"text\":\"PTY send failed; partial input possible, do not retry blindly\"}],\"isError\":true}");
                return 200;
            }
            /* 入力受付のみを返す．描画完了と画面確認はterminal_readの責務． */
            char reply[140];
            snprintf(reply,sizeof reply,
                     "{\"resultType\":\"complete\",\"content\":[{\"type\":\"text\",\"text\":\"accepted: %d bytes\"}]}",written);
            rpc_result(id,reply);
            return 200;
        }
        if (json_equals(v,v_end,"get_manual")) {
            if (strcmp(http_name, "get_manual")) {
                rpc_error(id, -32020, "HeaderMismatch"); return 400;
            }
            const char *args_end,*args=params?member(params,params_end,"arguments",&args_end):0;
            const char *topic_end,*topic_value=args?member(args,args_end,"topic",&topic_end):0;
            char topic[64]="README";
            if(topic_value) {
                int len=(int)(topic_end-topic_value-2);
                if(*topic_value!='"' || len<1 || len>=(int)sizeof topic) {
                    rpc_result(id,"{\"resultType\":\"complete\",\"content\":[{\"type\":\"text\",\"text\":\"Invalid topic. Use index.\"}],\"isError\":true}");
                    return 200;
                }
                for(int i=0;i<len;i++) {
                    char c=topic_value[i+1];
                    if(!((c>='a'&&c<='z') || (c>='A'&&c<='Z') ||
                         (c>='0'&&c<='9') || c=='/' || c=='_' || c=='-')) {
                        rpc_result(id,"{\"resultType\":\"complete\",\"content\":[{\"type\":\"text\",\"text\":\"Invalid topic. Use index.\"}],\"isError\":true}");
                        return 200;
                    }
                    topic[i]=c;
                }
                topic[len]=0;
                /* LLMが目次を探しやすいよう，indexをREADMEの別名にする． */
                if(!strcmp(topic,"index") || !strcmp(topic,"INDEX")) strcpy(topic,"README");
            }
            return send_full_manual(fd,id,topic); /* 成功時はchunked応答を送信済み． */
        }
        if (json_equals(v,v_end,"write_file")) {
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
        if (json_equals(v,v_end,"execute_command")) {
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
        if (used == sizeof request - 1)
            return 413;
        int ready=wait_fd(fd,0,5);
        if(ready<0) return -1;
        if(!ready) { waits++; continue; }
        int n = recv(fd, request + used, sizeof request - 1 - used, 0);
        if(n == OS_EAGAIN) { waits++; continue; }
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
    if (argc != 1) { puts("usage: mcpd"); return 1; }
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
    if(terminal_start_shell()<0) {
        puts("mcpd: cannot start PTY shell");
        close(server);
        return 1;
    }
    puts("mcpd: listening on TCP 8080 (experimental)");
    puts("mcpd: WARNING remote commands enabled; trusted isolated network ONLY");
    for (;;) {
        int ready=wait_fd(server,0,-1);
        if(ready<0) break;
        if(!ready) continue;
        int client = accept(server, 0, 0);
        if (client == OS_EAGAIN) continue;
        if (client < 0)
            break;
        send_fail_reason=send_fail_value=0;
        if (fcntl(client, F_SETFL, O_NONBLOCK) >= 0)
            handle(client);
        if (send_fail_reason==1)
            printf("mcpd: HTTP send timeout (bytes left=%d)\n",send_fail_value);
        else if (send_fail_reason==2)
            printf("mcpd: HTTP send socket error (%d)\n",send_fail_value);
        close(client);
    }
    if(term_master>=0)terminal_close_session();
    close(server);
    return 0;
}
