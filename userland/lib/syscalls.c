/* ユーザ空間の薄いシステムコールラッパー．
 * svc #0を発行し，r0に番号，r1〜r3に引数を渡す． */
/* ホストlibcに依存しない最小のユーザAPI． */
#include "unistd.h"
#include "../../include/piconix_syscall.h"
#include <stdint.h>
/* r0にシステムコール番号n，r1〜r3に引数a〜cを置いてSVCを発行する．戻り値は更新されたr0から取り出す． */
static inline int call(int n, uintptr_t a, uintptr_t b, uintptr_t c) {
    register uintptr_t r0 __asm("r0") = (uintptr_t)n;
    register uintptr_t r1 __asm("r1") = a;
    register uintptr_t r2 __asm("r2") = b;
    register uintptr_t r3 __asm("r3") = c;
    __asm volatile("svc #0" : "+r"(r0) : "r"(r1), "r"(r2), "r"(r3) : "memory");
    return (int)r0;
}

/* SVCを発行して現在の作業ディレクトリを変更する． */
int chdir(const char *path) { return call(SYS_CHDIR,(uintptr_t)path,0,0); }

/* SVCから現在ディレクトリのパスを受け取る．失敗時はNULLを返す． */
char *getcwd(char *buf,size_t size) { return call(SYS_GETCWD,(uintptr_t)buf,size,0)<0 ? 0 : buf; }

/* SVCから現在タスクのPIDを取得する． */
int getpid(void) { return call(SYS_GETPID,0,0,0); }

/* SVCからFlash固有IDの下位32ビットを取得する． */
unsigned flash_id(void) { return (unsigned)call(SYS_FLASH_ID,0,0,0); }

/* SVCで指定PIDのタスクに終了を要求する． */
int kill(int pid) { return call(SYS_KILL,(uintptr_t)pid,0,0); }

/* SVCで現在タスクを終了する．終了後はユーザ処理へ戻らない． */
void _exit(int status) { call(SYS_EXIT,(uintptr_t)status,0,0); for(;;) {} }

/* SVCで指定ミリ秒だけタスクを睡眠させる． */
int msleep(unsigned ms) { return call(SYS_SLEEP,ms,0,0); }

/* SVCで実行権をほかのREADYタスクへ譲る． */
int yield(void) { return call(SYS_YIELD,0,0,0); }

/* SVCでVFS上のパスを開き，FDまたはエラーを返す． */
int open(const char *path,int flags) { return call(SYS_OPEN,(uintptr_t)path,(uintptr_t)flags,0); }

/* SVCでFDを閉じる．パイプやソケットの参照も解放対象となる． */
int close(int fd) { return call(SYS_CLOSE,(uintptr_t)fd,0,0); }

/* SVCでFDのファイル位置を変更する．対応する仮想ファイルは限定される． */
long lseek(int fd,long offset,int whence) { return call(SYS_LSEEK,(uintptr_t)fd,(uintptr_t)offset,(uintptr_t)whence); }

/* FDマスクと待機時間を一つの要求にまとめてSVCへ渡す． */
int select(int nfds, fd_set *readfds, fd_set *writefds,
           fd_set *exceptfds, const struct timeval *timeout) {
    if (nfds<0 || nfds>PICONIX_FD_SETSIZE || exceptfds)
        return OS_EINVAL;
    int32_t ms=-1;
    if(timeout) {
        if(timeout->tv_sec<0 || timeout->tv_usec<0 || timeout->tv_usec>=1000000L ||
           timeout->tv_sec>2147482L) return OS_EINVAL;
        ms=(int32_t)(timeout->tv_sec*1000L+(timeout->tv_usec+999L)/1000L);
    }
    piconix_select_request_t r={readfds?*readfds:0,writefds?*writefds:0,0,0,ms};
    int n=call(SYS_SELECT,(uintptr_t)nfds,(uintptr_t)&r,0);
    if(n>=0) {
        if(readfds)
            *readfds=r.ready_read;
        if(writefds)
            *writefds=r.ready_write;
    }
    return n;
}

/* SVCで指定FDから最大nバイトを読み込む． */
int read(int fd,void *p,size_t n) { return call(SYS_READ,(uintptr_t)fd,(uintptr_t)p,n); }

/* SVCで指定FDへ最大nバイトを書き込む． */
int write(int fd,const void *p,size_t n) { return call(SYS_WRITE,(uintptr_t)fd,(uintptr_t)p,n); }

/* SVCでTTYやデバイスの固有制御を要求する． */
int ioctl(int fd,int cmd,int value) { return call(SYS_IOCTL,(uintptr_t)fd,(uintptr_t)cmd,(uintptr_t)value); }

/* SVCでFDのフラグ・非ブロック設定等を操作する． */
int fcntl(int fd,int cmd,int value) { return call(SYS_FCNTL,(uintptr_t)fd,(uintptr_t)cmd,(uintptr_t)value); }

/* SVCで指定タスクの終了を待ち，必要なら終了状態を受け取る． */
int waitpid(int pid,int *status,int options) { return call(SYS_WAITPID,(uintptr_t)pid,(uintptr_t)status,(uintptr_t)options); }

/* SVCでディレクトリエントリを一件ずつ読み取る． */
int getdents(int fd,piconix_dirent_t *entry,size_t size) { return call(SYS_GETDENTS,(uintptr_t)fd,(uintptr_t)entry,size); }

/* SVCでVFS上のRAMファイルを削除する． */
int unlink(const char *path) { return call(SYS_UNLINK,(uintptr_t)path,0,0); }

/* SVCでパイプの読取FD・書込FDを配列へ返す． */
int pipe(int fds[2]) { return call(SYS_PIPE,(uintptr_t)fds,0,0); }

/* SVCで既存FDを未使用番号へ複製する． */
int dup(int oldfd) { return call(SYS_DUP,(uintptr_t)oldfd,0,0); }

/* SVCで既存FDを指定FD番号へ複製する．リダイレクトに使う． */
int dup2(int oldfd,int newfd) { return call(SYS_DUP2,(uintptr_t)oldfd,(uintptr_t)newfd,0); }

/* 名前に対応する登録済み実行プログラムのIDを取得する． */
int program_id(const char *name) { return call(SYS_PROGRAM_ID,(uintptr_t)name,0,0); }

/* プログラムIDと引数で子タスクを起動し，PIDを返す． */
int spawn(int program,int argc,const char *const argv[]) { return call(SYS_SPAWN,(uintptr_t)program,(uintptr_t)argc,(uintptr_t)argv); }

/* 子タスクを前景指定付きで起動する．argcのフラグでカーネルへ通知する． */
int spawn_foreground(int program,int argc,const char *const argv[]) { return call(SYS_SPAWN,(uintptr_t)program,(uintptr_t)(argc|0x100),(uintptr_t)argv); }

/* 指定TTY向けに登録プログラムを起動する． */
int tty_spawn(int tty,int program) { return call(SYS_TTY_SPAWN,(uintptr_t)tty,(uintptr_t)program,0); }

/* TTYの前景プロセスを指定PIDへ切り替える． */
int set_foreground(int pid) { return call(SYS_SET_FG,(uintptr_t)pid,0,0); }

/* コンソールが利用可能な状態かSVCで問い合わせる． */
int console_ready(void) { return call(SYS_CONSOLE_READY,0,0,0); }

/* 現在のタスクが所属するTTY番号を取得する． */
int get_tty(void) { return call(SYS_GET_TTY,0,0,0); }

/* /tmpの実行イメージを読み出し，子タスクとして起動する． */
int spawn_tmp(const char *path) { return call(SYS_SPAWN_TMP,(uintptr_t)path,0,0); }

/* /tmpの実行イメージをTTY前景タスクとして起動する． */
int spawn_tmp_foreground(const char *path) { return call(SYS_SPAWN_TMP,(uintptr_t)path,1,0); }

/* ドメイン・種別・プロトコルを検査するカーネルへソケット作成を要求する． */
int socket(int domain,int type,int protocol) { return call(SYS_SOCKET,domain,type,protocol); }

/* UDPソケットを指定ポートへ束縛する． */
int bind_udp(int fd,unsigned port) { return call(SYS_BIND,fd,port,0); }

/* 宛先情報を含むPicoNIX独自UDP構造体で送信する． */
int sendto_udp(int fd,const piconix_udp_packet_t *packet) { return call(SYS_SENDTO,fd,(uintptr_t)packet,0); }

/* PicoNIX独自UDP構造体へ送信元と受信内容を取得する． */
int recvfrom_udp(int fd,piconix_udp_packet_t *packet) { return call(SYS_RECVFROM,fd,(uintptr_t)packet,0); }

/* システムコール番号はkernel/kernel.cと共通の定義を使う． */
int tcp_listen(int fd,unsigned port) { return call(SYS_LISTEN_TCP,fd,port,0); }

/* 接続先構造体を渡し，TCP接続を非同期に開始する． */
int tcp_connect(int fd,const piconix_tcp_endpoint_t *ep) { return call(SYS_CONNECT_TCP,fd,(uintptr_t)ep,0); }

/* TCPソケットの接続状態をカーネルへ問い合わせる． */
int tcp_status(int fd) { return call(SYS_TCP_STATUS,fd,0,0); }

/* TCP待受FDから確立済み接続を取り出す． */
int tcp_accept(int fd) { return call(SYS_ACCEPT_TCP,fd,0,0); }

/* BSD風ソケットのアドレスをIPv4に限定して検査する．受理できた場合だけ変換後ポインタを渡す． */
static int ipv4_addr(const struct sockaddr *addr, socklen_t len,
                     const struct sockaddr_in **out) {
    if (!addr || len < sizeof(struct sockaddr_in))
        return OS_EINVAL;
    *out = (const struct sockaddr_in *)addr;
    return (*out)->sin_family == AF_INET ? 0 : OS_EINVAL;
}

/* bindのIPv4アドレスとポートを検証し，PicoNIXのUDP bindまたはTCP listenの呼出しに変換する． */
int bind(int fd, const struct sockaddr *addr, socklen_t len) {
    const struct sockaddr_in *in;
    int r = ipv4_addr(addr, len, &in);
    if (r < 0)
        return r;
    unsigned port = ntohs(in->sin_port);
    if (!port || in->sin_addr.s_addr != INADDR_ANY)
        return OS_EINVAL;
    /* 下位のTCPシステムコールではbindとlistenが一体化している． */
    r = bind_udp(fd, port);
    if (r == OS_EBADF || r == OS_EINVAL)
        return tcp_listen(fd, port);
    return r;
}

/* PicoNIXではTCPのbind時点で待受を開始するため，ここでは引数の簡易検証のみ行う． */
int listen(int fd, int backlog) {
    (void)fd;
    return backlog < 0 ? OS_EINVAL : 0; /* TCPはbind時点で待ち受けを始めている． */
}

/* 未対応の接続元アドレス取得を拒否して，TCP接続FDを受け取る． */
int accept(int fd, struct sockaddr *addr, socklen_t *len) {
    if (addr || len) return OS_EINVAL; /* 接続元アドレスの取得にはまだ対応しない． */
    return tcp_accept(fd);
}

/* IPv4のsockaddr_inをPicoNIXのエンドポイントへ変換してTCP接続する． */
int connect(int fd, const struct sockaddr *addr, socklen_t len) {
    const struct sockaddr_in *in;
    int r = ipv4_addr(addr, len, &in);
    if (r < 0)
        return r;
    piconix_tcp_endpoint_t ep = {in->sin_addr.s_addr, ntohs(in->sin_port)};
    return tcp_connect(fd, &ep);
}

/* 送信側だけの半閉鎖（SHUT_WR）をサポートする． */
int shutdown(int fd,int how) {
    if(how!=SHUT_WR)
        return OS_EINVAL;
    return call(SYS_SHUTDOWN_TCP,(uintptr_t)fd,0,0);
}

/* 送信フラグとバッファを検証し，writeへ委譲する． */
int send(int fd, const void *buf, size_t len, int flags) {
    if (flags || (len && !buf)) return OS_EINVAL;
    return write(fd, buf, len);
}

/* 受信フラグとバッファを検証し，readへ委譲する． */
int recv(int fd, void *buf, size_t len, int flags) {
    if (flags || (len && !buf)) return OS_EINVAL;
    return read(fd, buf, len);
}

/* IPv4宛先を独自UDP送信構造体へ変換し，データを送信する． */
int sendto(int fd, const void *buf, size_t len, int flags,
           const struct sockaddr *to, socklen_t tolen) {
    const struct sockaddr_in *in;
    int r = ipv4_addr(to, tolen, &in);
    if (r < 0)
        return r;
    if (flags || len > UINT32_MAX || (len && !buf))
        return OS_EINVAL;
    piconix_udp_packet_t pkt = {buf, (uint32_t)len, in->sin_addr.s_addr,
                                ntohs(in->sin_port), 0};
    return sendto_udp(fd, &pkt);
}

/* UDPデータを受信し，必要なら送信元をsockaddr_inへ戻す． */
int recvfrom(int fd, void *buf, size_t len, int flags,
             struct sockaddr *from, socklen_t *fromlen) {
    if (flags || len > UINT32_MAX || (len && !buf))
        return OS_EINVAL;
    if (from && (!fromlen || *fromlen < sizeof(struct sockaddr_in)))
        return OS_EINVAL;
    piconix_udp_packet_t pkt = {buf, (uint32_t)len, 0, 0, 0};
    int r = recvfrom_udp(fd, &pkt);
    if (r >= 0 && from) {
        struct sockaddr_in *in = (struct sockaddr_in *)from;
        *in = (struct sockaddr_in){0};
        in->sin_family = AF_INET;
        in->sin_port = htons(pkt.peer_port);
        in->sin_addr.s_addr = pkt.peer_ip;
        *fromlen = sizeof(*in);
    }
    return r;
}
