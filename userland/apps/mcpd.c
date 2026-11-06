/* PicoNIX用の小さなMCP Streamable HTTP実験サーバ．
 * 2026-07-28版のJSON応答のみ．SSEと旧版のinitializeは対象外． */
#include "unistd.h"
#include "fcntl.h"
#include "stdio.h"
#include "string.h"
#include <stdint.h>

#define PORT 8080
#define REQUEST_SIZE 3072
#define BODY_LIMIT 1536
#define RESPONSE_SIZE 1400

/* 要求処理は一接続ずつ行う．大きなバッファは4 KiBスタックに置かない． */
static char request[REQUEST_SIZE];
static char response[RESPONSE_SIZE];

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

/* MCPのJSON要求を解析して処理する． */
static int process_json(const char *body, unsigned body_len, const char *http_method,
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
        rpc_result(id, "{\"resultType\":\"complete\",\"tools\":[{\"name\":\"get_temperature\",\"description\":\"Read the RP2040 temperature sensor\",\"inputSchema\":{\"type\":\"object\",\"properties\":{},\"additionalProperties\":false}},{\"name\":\"get_wifi_info\",\"description\":\"Read Pico W Wi-Fi status\",\"inputSchema\":{\"type\":\"object\",\"properties\":{},\"additionalProperties\":false}}],\"ttlMs\":0,\"cacheScope\":\"private\"}");
        return 200;
    }
    if (!strcmp(method, "tools/call")) {
        v = params ? member(params, params_end, "name", &v_end) : 0;
        const char *path = 0;
        if (json_equals(v, v_end, "get_temperature"))
            path = "/dev/adc/temp";
        if (json_equals(v, v_end, "get_wifi_info"))
            path = "/proc/net/wifi";
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
    status = process_json(request + body_offset, body_length, method, name, version);
    http_reply(fd, status, response);
}

/* MCPサーバを起動して接続要求を受け付ける． */
int main(int argc, char **argv) {
    (void)argv;
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
    puts("mcpd: listening on TCP 8080 (experimental)");
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
