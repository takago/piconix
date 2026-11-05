/* SPDX-License-Identifier: GPL-3.0-or-later */
/* PicoNIX用の小さなPython風処理系．
 * 24 KiBのタスク領域で動くため，値領域・構文・入出力に明示的な上限を設ける． */

/* PicoNIXの小さなPython風処理系．ファイル実行と簡易REPLに対応する． */
#include <stdint.h>
#include <limits.h>
#include <stddef.h>
#include "unistd.h"
#include "fcntl.h"
#include "string.h"
#include "stdio.h"
#include "piconix_i2c.h"
#include "piconix.h"
/* 標準出力へ指定長を書き切る．一時的な部分書込みでも残りを送る． */
static void outn(const char *s,unsigned n) {
    while(n) { int r=write(1, s, n); if(r<0) return;
        if(!r) { msleep(1); continue; } s+=r;n-=(unsigned)r; }
}

/* NUL終端文字列を標準出力へ書き込む． */
static void outs(const char *s){outn(s,(unsigned)strlen(s));}

/* 二つの識別子やキーワードを文字列として比較する． */
static int same(const char *a,const char *b){return strcmp(a,b)==0;}

/* 文字が英数字または下線なら真を返す． */
static int idchar(char c){return (c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='_';}
#define SOURCE_MAX 2048
static char source[SOURCE_MAX+1];
static uint16_t offsets[64],levels[64];
static unsigned count;
static struct {char name[16];int32_t val;} vars[16];
#define STR_MAX 64
static uint8_t is_string[16];
static int nvars,error, evaluating=1;
static unsigned error_line;
#define SEQ_MAX 64
#define POOL_WORDS 256
/* 文字列と配列で共有する，ワード境界で詰め直し可能な固定長プール． */
static int32_t pool[POOL_WORDS];
static uint16_t pool_used;
static struct { uint8_t used, immutable, len; uint16_t pos, words; } seq[16];
/* 固定長プール内の指定配列が格納される先頭ポインタを得る． */
static int32_t *items(int k){return pool+seq[k].pos;}

/* 固定長プール内の指定文字列が格納される先頭ポインタを得る． */
static char *string_at(int k){return (char *)(pool+seq[k].pos);}

/* 変数名に対応する既存の固定スロット番号を探す．なければ-1． */
static int seqslot(const char *name){for(int i=0;i<nvars;i++)if(strcmp(vars[i].name,name)==0)return i;return -1;}

/* 変数名のスロットを検索・確保する．最大16個を超える場合はエラー． */
static int slot_for(const char *name){
    int k=seqslot(name);
    if(k>=0)return k;
    if(nvars==16){error=1;return -1;}
    k=nvars++;unsigned j=0;
    while(name[j]&&j<15){vars[k].name[j]=name[j];j++;}vars[k].name[j]=0;
    return k;
}

/* 配列・文字列を固定長プール内で置き換える．領域が足りなければ既存値を壊さず失敗する． */
static int replace_data(int k,const void *data,unsigned words){
    unsigned available=pool_used-seq[k].words;
    if(available+words>POOL_WORDS){error=1;return 0;}
    /* 代入元が同じプールに存在する場合（例：b=a）は，移動前にデータを退避する． */
    int32_t scratch[SEQ_MAX];
    if(words){memcpy(scratch,data,words*4);}
    unsigned dst=0;
    /* 変数番号順と物理格納順が異なり得るので，アドレス順で処理する． */
    for(unsigned pos=0;pos<pool_used;){
        int owner=-1;
        for(int j=0;j<nvars;j++)if(seq[j].words&&seq[j].pos==pos){owner=j;break;}
        if(owner<0){error=1;return 0;}
        unsigned n=seq[owner].words;
        if(owner!=k){
            if(dst!=pos)memmove(pool+dst,pool+pos,n*4);
            seq[owner].pos=(uint16_t)dst;dst+=n;
        }
        pos+=n;
    }
    seq[k].pos=(uint16_t)dst;
    if(words)memcpy(pool+dst,scratch,words*4);
    seq[k].words=(uint16_t)words;
    pool_used=(uint16_t)(dst+words);
    return 1;
}

/* 固定長プールに配列・タプル相当の要素列を格納し，変数へ関連付ける． */
static int seqnew(const char *name,int immutable,const int32_t *values,int len){
    if(len<0||len>SEQ_MAX){error=1;return 0;}
    int k=slot_for(name);if(k<0)return 0;
    if(!replace_data(k,values,(unsigned)len))return 0;
    is_string[k]=0;seq[k].used=1;seq[k].immutable=immutable;seq[k].len=(uint8_t)len;
    return 1;
}

/* 最大64文字の文字列を固定長プールに格納する．容量不足はエラー． */
static void setstring(const char *name,const char *value){
    unsigned n=(unsigned)strlen(value);
    if(n>STR_MAX){error=1;return;}
    int k=slot_for(name);if(k<0)return;
    /* 最後のワードを埋め，コピーで値の終端外を読まないようにする． */
    int32_t scratch[(STR_MAX+4)/4]={0};
    memcpy(scratch,value,n+1);
    if(!replace_data(k,scratch,(n+4)/4))return;
    is_string[k]=1;seq[k].used=0;seq[k].len=(uint8_t)n;
}

/* 呼出し元が開いたI²C FDで転送する．FDの所有権は呼出し元にある． */
static int transfer(int fd,int addr,const int32_t *values,int n,int r,int32_t *output){
    if(fd<0||addr<8||addr>0x77||n<0||n>32||r<0||r>32){error=1;return 0;}
    piconix_i2c_transfer_t req={0};req.address=(uint8_t)addr;
    req.write_len=(uint8_t)n;req.read_len=(uint8_t)r;
    for(int i=0;i<n;i++){if(values[i]<0||values[i]>255){error=1;return 0;}
        req.write_data[i]=(uint8_t)values[i];}
    int rc=ioctl(fd,OS_I2C_TRANSFER,(int)(uintptr_t)&req);
    if(rc<0)return -1;
    for(int i=0;i<r;i++)output[i]=req.read_data[i];return n;
}
static uint32_t last_peer_ip;
static unsigned last_peer_port;
static const char *cursor;
/* 解析カーソルを空白・タブの後まで進める． */
static void ws(void){while(*cursor==' '||*cursor=='\t')cursor++;}

/* 空白を飛ばした位置で指定1文字を消費する．一致しなければ消費しない． */
static int eat(char c){ws();if(*cursor==c){cursor++;return 1;}return 0;}

/* キーワードを単語境界も含めて照合し，成功時だけカーソルを進める． */
static int word(const char *s){ws();const char *p=cursor;while(*s&&*p==*s){s++;p++;}if(*s||idchar(*p))return 0;cursor=p;return 1;}

/* 名前で整数変数を検索する．文字列・配列との型不一致はエラーにする． */
static int32_t getvar(const char *name){for(int i=0;i<nvars;i++)if(same(vars[i].name,name))return (seq[i].used||is_string[i])?(error=1,0):vars[i].val;error=1;return 0;}

/* 整数変数へ代入し，旧配列等があればプール領域を解放する． */
static void setvar(const char *name,int32_t val){for(int i=0;i<nvars;i++)if(same(vars[i].name,name)){vars[i].val=val;replace_data(i,0,0);seq[i].used=0;is_string[i]=0;seq[i].len=0;return;}
    if(nvars==16){error=1;return;}unsigned i=0;while(name[i]&&i<15){vars[nvars].name[i]=name[i];i++;}vars[nvars].name[i]=0;vars[nvars].val=val;seq[nvars].used=0;is_string[nvars]=0;seq[nvars].len=0;seq[nvars].words=0;nvars++;}

/* 符号付き32ビットの整数除算を行い，ゼロ除算と最小値の境界を明示的に扱う． */
static int32_t divide(int32_t a,int32_t b,int mod) {
    if(!evaluating)return 0;
    if(!b) { error=1; return 0; }
    /* 数学的な商2147483648はint32_tに収まらないため，従来の32ビット折返し結果を維持する． */
    if(a==INT32_MIN && b==-1) return mod ? 0 : INT32_MIN;
    return mod ? a%b : a/b;
}
static int32_t expr(void);
/* FDはPicoNIXの整数であり，Pythonのファイルオブジェクトではない． */
static void decimal_string(int32_t v,char *dst);
static unsigned parsed_string_len;
/* 文字列リテラルを読み，エスケープと領域上限を検査する． */
static int parse_string(char *dst,unsigned cap){
    ws();char q=*cursor;if(q!='"'&&q!='\''){error=1;return 0;}
    cursor++;unsigned n=0;
    while(*cursor&&*cursor!=q){
        unsigned char c=(unsigned char)*cursor++;
        if(c=='\\'){
            c=(unsigned char)*cursor++;
            if(!c){error=1;return 0;}
            if(c=='n')c='\n';else if(c=='r')c='\r';else if(c=='t')c='\t';
            else if(c=='0')c=0;
            else if(c=='x'){
                unsigned v=0;
                for(int i=0;i<2;i++){
                    char h=*cursor++;unsigned d;
                    if(h>='0'&&h<='9')d=(unsigned)(h-'0');
                    else if(h>='a'&&h<='f')d=(unsigned)(h-'a'+10);
                    else if(h>='A'&&h<='F')d=(unsigned)(h-'A'+10);
                    else {error=1;return 0;}
                    v=v*16+d;
                }
                c=(unsigned char)v;
            }else if(c!='\\'&&c!='"'&&c!='\''){error=1;return 0;}
        }
        if(n+1>=cap){error=1;return 0;}dst[n++]=(char)c;
    }
    if(!eat(q)){error=1;return 0;}dst[n]=0;parsed_string_len=n;return 1;
}

/* ソケットの送信データは文字列・バイト列・str(整数)から作れる． */
static int payload(char *buf,unsigned cap){
    ws();
    if(*cursor=='"'||*cursor=='\''){
        if(!parse_string(buf,cap+1))return -1;
        /* 文字列の内部にNULを含む場合は，テキストとして送信できない． */
        return (int)parsed_string_len;
    }
    if(word("str")){
        if(!eat('(')){error=1;return -1;}
        int32_t v=expr();if(!eat(')')){error=1;return -1;}
        char tmp[12];decimal_string(v,tmp);
        unsigned n=(unsigned)strlen(tmp);
        if(n>cap){error=1;return -1;}memcpy(buf,tmp,n);return (int)n;
    }
    char key[16];unsigned z=0;
    while(idchar(*cursor)&&z<15)key[z++]=*cursor++;
    key[z]=0;int k=z?seqslot(key):-1;
    if(k<0){error=1;return -1;}
    if(is_string[k]){
        unsigned n=(unsigned)strlen(string_at(k));
        if(n>cap){error=1;return -1;}memcpy(buf,string_at(k),n);return (int)n;
    }
    if(!seq[k].used||seq[k].len>cap){error=1;return -1;}
    for(int i=0;i<seq[k].len;i++){
        int32_t v=items(k)[i];if(v<0||v>255){error=1;return -1;}
        buf[i]=(char)v;
    }
    return seq[k].len;
}

/* IPv4のドット区切り表記をncと共通のlwIPアドレス表現へ変換する． */
static int parse_ip(const char *s,uint32_t *out){
    unsigned parts[4]={0},i=0,digits=0;
    for(;*s;s++){
        if(*s>='0'&&*s<='9'){
            if(++digits>3)return 0;
            parts[i]=parts[i]*10u+(unsigned)(*s-'0');
            if(parts[i]>255)return 0;
        }else if(*s=='.'&&i<3&&digits){i++;digits=0;}
        else return 0;
    }
    if(i!=3||!digits)return 0;
    uint8_t *p=(uint8_t *)out;
    for(i=0;i<4;i++)p[i]=(uint8_t)parts[i];
    return 1;
}

/* 通信APIに使うIPv4アドレスとポートを解析する． */
static int socket_address(struct sockaddr_in *addr){
    char ip[20];
    if(!parse_string(ip,sizeof ip))return 0;
    if(!eat(','))error=1;
    int32_t port=expr();
    addr->sin_family=AF_INET;
    if(same(ip,"peer")){
        addr->sin_addr.s_addr=last_peer_ip;
        if(port==0)port=(int32_t)last_peer_port;
    }else if(!parse_ip(ip,&addr->sin_addr.s_addr))error=1;
    if(port<1||port>65535)error=1;
    addr->sin_port=htons((uint16_t)port);
    return !error;
}

/* 通信先のポート番号が扱える範囲か検査する． */
static int valid_port(int32_t port){return port>0&&port<=65535;}

/* 引数で受け取ったFD番号を検証・変換する． */
static int parse_fd(void){int fd=expr();if(fd<0)error=1;return fd;}

/* UART/USBのバイト列から長さ制限のある一行を組み立てる． */
static int read_line(char *buf,unsigned cap){
    static int skip_lf;
    unsigned n=0;
    for(;;){
        char c;
        int r=read(0, &c, 1);
        if(r<=0)return n?(int)n:-1;
        if(skip_lf){skip_lf=0;if(c=='\n')continue;}
        if(c=='\r'){skip_lf=1;break;}
        if(c=='\n')break;
        if(n+1<cap)buf[n++]=c;
    }
    buf[n]=0;
    return (int)n;
}

/* input()の文字列を固定領域へ取得する． */
static int input_text(char *dst){
    if(!eat('(')){error=1;return 0;}
    ws();
    if(*cursor=='"'||*cursor=='\''){
        char prompt[STR_MAX+1];if(!parse_string(prompt,sizeof prompt))return 0;
        if(evaluating)outs(prompt);
    }else if(*cursor!=')'){
        char key[16];unsigned n=0;
        while(idchar(*cursor)){if(n>=15){error=1;return 0;}key[n++]=*cursor++;}
        key[n]=0;int k=n?seqslot(key):-1;
        if(k<0||!is_string[k]){error=1;return 0;}
        if(evaluating)outs(string_at(k));
    }
    if(!eat(')')){error=1;return 0;}
    if(!evaluating){dst[0]=0;return 1;}
    int n=read_line(dst,STR_MAX+1);
    if(n<0){error=1;return 0;}
    return 1;
}

/* 十進文字列を符号付き32ビット整数へ変換する．空文字・余分な文字・範囲外値は0を返し，成功時のみ*outへ格納する． */
static int parse_decimal(const char *s,int32_t *out){
    while(*s==' '||*s=='\t')s++;
    int neg=0;if(*s=='+'||*s=='-')neg=(*s++=='-');
    uint32_t v=0,limit=neg?2147483648u:2147483647u;int digits=0;
    while(*s>='0'&&*s<='9'){
        unsigned d=(unsigned)(*s++-'0');
        if(v>(limit-d)/10u)return 0;
        v=v*10u+d;digits++;
    }
    while(*s==' '||*s=='\t')s++;
    if(!digits||*s)return 0;
    *out=neg?(int32_t)(0u-v):(int32_t)v;return 1;
}

/* 32ビット整数を表示用の10進文字列へ変換する． */
static void decimal_string(int32_t v,char *dst){
    char reversed[12];unsigned n=0;uint32_t a=v<0?0u-(uint32_t)v:(uint32_t)v;
    do{reversed[n++]=(char)('0'+a%10u);a/=10u;}while(a);
    if(v<0)reversed[n++]='-';
    unsigned j=0;while(n)dst[j++]=reversed[--n];dst[j]=0;
}

/* int()で整数式，文字列変数，input()の結果を整数化する． */
static int32_t integer_conversion(void){
    if(!eat('(')){error=1;return 0;}
    ws();char value[STR_MAX+1];int string_arg=0;
    if(*cursor=='"'||*cursor=='\''){
        string_arg=parse_string(value,sizeof value);
    }else if(word("input")){
        string_arg=input_text(value);
    }else{
        const char *save=cursor;char name[16];unsigned n=0;
        while(idchar(*cursor)&&n<15)name[n++]=*cursor++;
        name[n]=0;int k=n?seqslot(name):-1;ws();
        if(k>=0&&is_string[k]&&*cursor==')'){
            memcpy(value,string_at(k),strlen(string_at(k))+1);string_arg=1;
        }else cursor=save;
    }
    if(string_arg){
        if(!eat(')'))error=1;
        int32_t v=0;if(evaluating&&!parse_decimal(value,&v))error=1;
        return v;
    }
    int32_t v=expr();if(!eat(')'))error=1;return v;
}
static int32_t expr(void);
/* 整数式の最小単位を解析する．括弧，変数，文字列などの型と範囲を確認する． */
static int32_t atom(void){ws();if(eat('(')){int32_t x=expr();if(!eat(')'))error=1;return x;}
    if(eat('-'))return (int32_t)(0u-(uint32_t)atom());
    if(eat('+'))return atom();
    if(eat('~'))return ~atom();
    if(*cursor>='0'&&*cursor<='9'){
        unsigned base=10;uint32_t v=0;int digits=0;
        if(cursor[0]=='0'&&(cursor[1]=='x'||cursor[1]=='X'||cursor[1]=='b'||cursor[1]=='B')){
            base=(cursor[1]=='x'||cursor[1]=='X')?16:2;cursor+=2;
        }
        for(;;){unsigned d;char c=*cursor;
            if(c>='0'&&c<='9')d=(unsigned)(c-'0');
            else if(c>='a'&&c<='f')d=(unsigned)(c-'a'+10);
            else if(c>='A'&&c<='F')d=(unsigned)(c-'A'+10);
            else break;
            if(d>=base)break;
            if(v>(UINT32_MAX-d)/base){error=1;return 0;}
            v=v*base+d;cursor++;digits++;
        }
        if(!digits||idchar(*cursor))error=1;
        return (int32_t)v;
    }
    char name[16];unsigned n=0;
    while(idchar(*cursor)&&n<15)name[n++]=*cursor++;
    name[n]=0;if(!n){error=1;return 0;}
    if(same(name,"True"))return 1;
    if(same(name,"False"))return 0;
    /* fbctlと同じフレームバッファioctl ABIを共有する． */
    if(same(name,"FB_ENABLE"))return OS_FB_ENABLE;
    if(same(name,"FB_DISABLE"))return OS_FB_DISABLE;
    if(same(name,"FB_STATUS"))return OS_FB_STATUS;
    if(same(name,"SEEK_SET"))return SEEK_SET;
    if(same(name,"SEEK_CUR"))return SEEK_CUR;
    if(same(name,"SEEK_END"))return SEEK_END;
    if(same(name,"int"))return integer_conversion();
    /* input()単独の戻り値は文字列なので，そのまま整数式の値にはならない． */
    if(same(name,"input")){error=1;return 0;}
    if(same(name,"len")&&eat('(')){
        char target[16];unsigned z=0;ws();while(idchar(*cursor)&&z<15)target[z++]=*cursor++;
        target[z]=0;int k=seqslot(target);if(!eat(')')||k<0||(!seq[k].used&&!is_string[k])){error=1;return 0;}
        return is_string[k]?(int32_t)strlen(string_at(k)):seq[k].len;
    }
    /* 整数FDを使う簡易ソケットAPI．操作失敗は-1に統一する． */
    if(same(name,"socket")&&eat('(')){
        int32_t type=expr();if(!eat(')'))error=1;
        if(type!=SOCK_STREAM&&type!=SOCK_DGRAM)error=1;
        if(error||!evaluating)return 0;
        int rc=socket(AF_INET,type,0);return rc<0?-1:rc;
    }
    if(same(name,"connect")&&eat('(')){
        int fd=parse_fd();if(!eat(','))error=1;
        struct sockaddr_in addr={0};
        if(!socket_address(&addr))return 0;
        if(!eat(')'))error=1;
        if(error||!evaluating)return 0;
        int rc=connect(fd,(const struct sockaddr *)&addr,sizeof addr);return rc<0?-1:rc;
    }
    if(same(name,"bind")&&eat('(')){
        int fd=parse_fd();if(!eat(','))error=1;
        int port=expr();if(!eat(')')||!valid_port(port))error=1;
        if(error||!evaluating)return 0;int rc=bind_udp(fd,(unsigned)port);return rc<0?-1:rc;
    }
    if(same(name,"listen")&&eat('(')){
        int fd=parse_fd();if(!eat(','))error=1;
        int32_t port=expr();if(!eat(')')||!valid_port(port))error=1;
        if(error||!evaluating)return 0;
        int rc=tcp_listen(fd,(unsigned)port);return rc<0?-1:rc;
    }
    if((same(name,"accept")||same(name,"status"))&&eat('(')){
        int accepting=same(name,"accept");int fd=parse_fd();
        if(!eat(')'))error=1;
        if(error||!evaluating)return 0;
        int rc=accepting?tcp_accept(fd):tcp_status(fd);return rc<0?-1:rc;
    }
    if(same(name,"peerport")&&eat('(')){
        if(!eat(')'))error=1;return (int32_t)last_peer_port;
    }
    if(same(name,"open")&&eat('(')){
        char path[96];if(!parse_string(path,sizeof path))return 0;
        if(!eat(',')){error=1;return 0;}int flags=expr();
        if(!eat(')'))error=1;
        if(error||!evaluating)return 0;
        int rc=open(path,flags);return rc<0?-1:rc;
    }
    if(same(name,"close")&&eat('(')){
        int fd=parse_fd();if(!eat(')'))error=1;
        if(error||!evaluating)return 0;
        int rc=close(fd);return rc<0?-1:rc;
    }
    if(same(name,"lseek")&&eat('(')){
        int fd=parse_fd();if(!eat(','))error=1;
        int32_t offset=expr();if(!eat(','))error=1;
        int32_t whence=expr();if(!eat(')'))error=1;
        if(error||!evaluating)return 0;
        int32_t rc=(int32_t)lseek(fd,offset,whence);return rc<0?-1:rc;
    }
    if(same(name,"ioctl")&&eat('(')){
        int fd=parse_fd();if(!eat(','))error=1;int cmd=expr();
        if(!eat(','))error=1;int arg=expr();if(!eat(')'))error=1;
        if(error||!evaluating)return 0;
        int rc=ioctl(fd,cmd,arg);return rc<0?-1:rc;
    }
    if((same(name,"write")||same(name,"send")||same(name,"sendto"))&&eat('(')){
        int udp=same(name,"sendto"),tcp=same(name,"send");
        int fd=parse_fd();if(!eat(','))error=1;
        char buf[SEQ_MAX+1];int len=payload(buf,SEQ_MAX);
        struct sockaddr_in addr={0};
        if(udp){if(!eat(','))error=1;socket_address(&addr);}
        if(!eat(')'))error=1;
        if(error||!evaluating||len<0)return 0;
        int rc=udp?sendto(fd,buf,(unsigned)len,0,(struct sockaddr *)&addr,sizeof addr):
               tcp?send(fd,buf,(unsigned)len,0):write(fd,buf,(unsigned)len);
        return rc<0?-1:rc;
    }

    if(eat('[')){int32_t index=expr();if(!eat(']'))error=1;
        int k=seqslot(name);if(k<0||!seq[k].used||index<0||index>=seq[k].len){error=1;return 0;}
        return items(k)[index];}
    return evaluating?getvar(name):0;
}

/* 乗除算の優先順位を処理する． */
static int32_t product(void){int32_t a=atom();for(;;){if(eat('*'))a=(int32_t)((uint32_t)a*(uint32_t)atom());
    else if(eat('/')){ /* この限定版では/と//のどちらも整数除算を行う． */
        eat('/');a=divide(a,atom(),0);
    }else if(eat('%'))a=divide(a,atom(),1);else break;}return a;}

/* 加減算を含む式を評価し，次の構文解析位置を更新する． */
static int32_t sum(void){int32_t a=product();for(;;){if(eat('+'))a=(int32_t)((uint32_t)a+(uint32_t)product());
    else if(eat('-'))a=(int32_t)((uint32_t)a-(uint32_t)product());else break;}return a;}

/* 再帰下降で演算子の優先順位を扱う．論理演算・ビット演算・シフト・四則演算を段階別に解析する． */
static int32_t bits(int level){
    int32_t a=level==3?sum():bits(level+1);
    for(;;){
        ws();int op=0;
        if(level==0&&cursor[0]=='|'&&cursor[1]!='|')op='|';
        if(level==1&&*cursor=='^')op='^';
        if(level==2&&*cursor=='&')op='&';
        if(level==3&&cursor[0]=='<'&&cursor[1]=='<')op='<';
        if(level==3&&cursor[0]=='>'&&cursor[1]=='>')op='>';
        if(!op)break;
        cursor+=op=='<'||op=='>'?2:1;
        int32_t b=level==3?sum():bits(level+1);
        if(op=='|')a|=b;
        else if(op=='^')a^=b;
        else if(op=='&')a&=b;
        else {
            if(b<0||b>=32){error=1;return 0;}
            a=op=='<'?(int32_t)((uint32_t)a<<b):(a>>b);
        }
    }
    return a;
}

/* 比較演算を解析する． */
static int32_t comparison(void){
    /* 配列と整数の等値比較は常に偽．read/recv/I²Cの -1 を検査できる． */
    const char *save=cursor;ws();char name[16];unsigned n=0;
    while(idchar(*cursor)&&n<15)name[n++]=*cursor++;
    name[n]=0;int k=n?seqslot(name):-1;ws();
    if(k>=0&&seq[k].used&&((cursor[0]=='='&&cursor[1]=='=')||(cursor[0]=='!'&&cursor[1]=='='))){
        int neq=cursor[0]=='!';cursor+=2;
        (void)bits(0);return neq;
    }
    cursor=save;
    int32_t a=bits(0);ws();int op=0;
    if(cursor[0]=='='&&cursor[1]=='='){op=1;cursor+=2;}
    else if(cursor[0]=='!'&&cursor[1]=='='){op=2;cursor+=2;}
    else if(cursor[0]=='<'&&cursor[1]=='='){op=3;cursor+=2;}
    else if(cursor[0]=='>'&&cursor[1]=='='){op=4;cursor+=2;}
    else if(eat('<'))op=5;else if(eat('>'))op=6;
    if(op){int32_t b=bits(0);
        if(op==1)return a==b;
        if(op==2)return a!=b;
        if(op==3)return a<=b;
        if(op==4)return a>=b;
        if(op==5)return a<b;
        return a>b;
    }
    return a;
}

/* 比較演算よりnot，さらにand，orの順に優先順位が下がる．短絡評価で読み飛ばす式は副作用を起こさず構文だけ解析する． */
static int32_t logical_not(void){if(word("not"))return !logical_not();return comparison();}

/* 論理AND演算を含む式を解析・評価する． */
static int32_t logical_and(void){int32_t v=logical_not();
    while(word("and")){int saved=evaluating;evaluating=saved&&(v!=0);
        int32_t b=logical_not();evaluating=saved;
        if(saved&&v!=0)v=b; /* この処理系でもand/orの結果は単なる真偽値でなくオペランドになる． */
    }return v;
}

/* 演算子の優先順位に従って式を評価する入口． */
static int32_t expr(void){int32_t v=logical_and();
    while(word("or")){int saved=evaluating;evaluating=saved&&(v==0);
        int32_t b=logical_and();evaluating=saved;
        if(saved&&v==0)v=b;
    }return v;
}

/* 数値リテラルを解析し，整数値を返す． */
static void number(int32_t v){char b[12];unsigned n=0,x=v<0?0u-(uint32_t)v:(uint32_t)v;
    do{uint32_t r=x%10u;x/=10u;b[n++]=(char)('0'+r);}while(x&&n<11);
    if(v<0)b[n++]='-';while(n){char c=b[--n];outn(&c,1);}}

/* 値を出力する．無制限長の中間文字列を確保せずに表示する． */
static void print_value(void){
    ws();
    if(*cursor=='"'||*cursor=='\''){
        char b[STR_MAX+1];if(parse_string(b,sizeof b))outs(b);return;
    }
    if(word("str")){
        if(!eat('(')){error=1;return;}
        int32_t v=expr();if(!eat(')'))error=1;
        if(!error)number(v);return;
    }
    if(word("input")){
        char b[STR_MAX+1];if(input_text(b)&&!error)outs(b);return;
    }
    if(eat('[')){
        outs("[");int first=1;
        if(!eat(']')){
            do{if(!first)outs(", ");first=0;number(expr());}while(!error&&eat(','));
            if(!eat(']'))error=1;
        }
        outs("]");return;
    }
    const char *start=cursor;char key[16];unsigned z=0;
    while(idchar(*cursor)&&z<15)key[z++]=*cursor++;
    key[z]=0;int k=z?seqslot(key):-1;
    ws();
    if(k>=0&&(*cursor==','||*cursor==')')){
        if(is_string[k]){outs(string_at(k));return;}
        if(seq[k].used){
            outs(seq[k].immutable?"b[":"[");
            for(int j=0;j<seq[k].len;j++){
                if(j)outs(", ");number(items(k)[j]);
            }
            outs("]");return;
        }
    }
    cursor=start;number(expr());
}
static int repl_mode;
/* I²C書込みに渡すFD，アドレス，バイト列を解析する． */
static int parse_i2c_write_args(int *fd,int32_t *addr,int32_t *payload,int *len){
    *fd=parse_fd();if(!eat(',')){error=1;return 0;}
    *addr=expr();if(!eat(',')){error=1;return 0;}
    *len=0;
    if(word("bytes")){
        if(!eat('(')||!eat('[')){error=1;return 0;}
        if(!eat(']')){
            do{if(*len==32){error=1;return 0;}payload[(*len)++]=expr();}while(eat(','));
            if(!eat(']')){error=1;return 0;}
        }
        if(!eat(')')){error=1;return 0;}
    }else{
        char srcname[16];unsigned z=0;ws();while(idchar(*cursor)&&z<15)srcname[z++]=*cursor++;
        srcname[z]=0;int k=seqslot(srcname);
        if(k<0||!seq[k].used||seq[k].len>32){error=1;return 0;}
        *len=seq[k].len;for(int j=0;j<*len;j++)payload[j]=items(k)[j];
    }
    if(!eat(')')){error=1;return 0;}
    ws();if(*cursor&&*cursor!='#')error=1;
    return !error;
}
/* 一文の構文と処理を実行する．ファイル，GPIO，通信等の小さな組込み機能へ分岐する． */
static void statement(unsigned line){cursor=source+offsets[line];ws();
    if(word("msleep")&&eat('(')){
        int32_t ms=expr();if(!eat(')'))error=1;
        ws();if(*cursor&&*cursor!='#')error=1;
        if(ms<0||ms>60000)error=1;
        if(!error)msleep(ms);
        return;
    }
    cursor=source+offsets[line];ws();
    if(word("i2c_write")&&eat('(')){
        int fd,len;int32_t addr,payload[32],result[32];
        if(parse_i2c_write_args(&fd,&addr,payload,&len))
            (void)transfer(fd,addr,payload,len,0,result);
        return;
    }
    cursor=source+offsets[line];ws();
    if(word("print")&&eat('(')){
        if(!eat(')')){
            do{
                if(cursor[-1]==',')outs(" ");
                print_value();
            }while(!error&&eat(','));
            if(!eat(')'))error=1;
        }
        ws();if(*cursor&&*cursor!='#')error=1;
        if(!error)outs("\r\n");return;
    }
    cursor=source+offsets[line];ws();
    if(word("write")||word("close")||word("ioctl")||word("connect")||word("listen")||word("bind")||word("send")||word("sendto")){
        cursor=source+offsets[line];(void)expr();
        ws();if(*cursor&&*cursor!='#')error=1;return;
    }
    cursor=source+offsets[line];ws();char name[16];unsigned n=0;
    if(!((*cursor>='a'&&*cursor<='z')||(*cursor>='A'&&*cursor<='Z')||*cursor=='_')){
        if(repl_mode){cursor=source+offsets[line];number(expr());outs("\r\n");return;}
        error=1;return;
    }
    while(idchar(*cursor)&&n<15)name[n++]=*cursor++;name[n]=0;
    ws();if(*cursor=='['){cursor++;int32_t index=expr();if(!eat(']')||!eat('=')){error=1;return;}
        int32_t v=expr();int k=seqslot(name);if(k<0||!seq[k].used||seq[k].immutable||index<0||index>=seq[k].len)error=1;
        ws();if(*cursor&&*cursor!='#')error=1;
        if(!error)items(k)[index]=v;return;}
    if(!eat('=')){
        if(repl_mode){cursor=source+offsets[line];int32_t v=expr();
            ws();if(*cursor&&*cursor!='#')error=1;
            if(!error){number(v);outs("\r\n");}return;}
        error=1;return;
    }
    /* 成功時は送信バイト数，転送失敗時は-1を返す． */
    if(word("i2c_write")&&eat('(')){
        int fd,len;int32_t addr,payload[32],result[32];
        if(parse_i2c_write_args(&fd,&addr,payload,&len)){
            int rc=transfer(fd,addr,payload,len,0,result);
            if(!error)setvar(name,rc);
        }
        return;
    }
    if(word("input")){
        char value[STR_MAX+1];if(!input_text(value))return;
        ws();if(*cursor&&*cursor!='#')error=1;
        if(!error)setstring(name,value);return;
    }
    if(word("str")){
        if(!eat('(')){error=1;return;}
        int32_t v=expr();if(!eat(')'))error=1;
        ws();if(*cursor&&*cursor!='#')error=1;
        if(!error){char value[12];decimal_string(v,value);setstring(name,value);}return;
    }
    ws();if(*cursor=='"'||*cursor=='\''){
        char value[STR_MAX+1];if(!parse_string(value,sizeof value))return;
        ws();if(*cursor&&*cursor!='#')error=1;
        if(!error)setstring(name,value);return;
    }
    /* 文字列同士の代入であり，整数を自動で文字列化しない． */
    {const char *save=cursor;char from[16];unsigned j=0;
        while(idchar(*cursor)&&j<15)from[j++]=*cursor++;
        from[j]=0;int k=j?seqslot(from):-1;ws();
        if(k>=0&&is_string[k]&&(!*cursor||*cursor=='#')){
            setstring(name,string_at(k));return;
        }
        cursor=save;
    }
    ws();if(*cursor=='[' || (cursor[0]=='b'&&cursor[1]=='y'&&cursor[2]=='t'&&cursor[3]=='e'&&cursor[4]=='s')){
        int immutable=0;if(word("bytes")){immutable=1;if(!eat('(')){error=1;return;}}
        if(!eat('[')){error=1;return;}int32_t values[SEQ_MAX];int len=0;
        if(!eat(']')){do{if(len==SEQ_MAX){error=1;break;}values[len++]=expr();}while(eat(','));
            if(!eat(']'))error=1;}
        if(immutable&&!eat(')'))error=1;
        ws();if(*cursor&&*cursor!='#')error=1;
        if(immutable)for(int j=0;j<len;j++)if(values[j]<0||values[j]>255)error=1;
        if(!error)seqnew(name,immutable,values,len);return;
    }
    if(word("read")||word("recv")||word("recvfrom")){
        int is_udp=cursor[-1]=='m';
        int nonblocking=cursor[-1]=='v'||is_udp;
        if(!eat('(')){error=1;return;}
        int fd=parse_fd();if(!eat(','))error=1;
        int32_t size=expr();if(!eat(')')||size<0||size>SEQ_MAX)error=1;
        ws();if(*cursor&&*cursor!='#')error=1;
        if(error)return;
        uint8_t buf[SEQ_MAX];int32_t values[SEQ_MAX];
        struct sockaddr_in from={0};socklen_t fromlen=sizeof from;
        int rc=is_udp?recvfrom(fd,buf,(unsigned)size,0,(struct sockaddr *)&from,&fromlen):
               nonblocking?recv(fd,buf,(unsigned)size,0):read(fd,buf,(unsigned)size);
        if(is_udp&&rc>=0){last_peer_ip=from.sin_addr.s_addr;last_peer_port=ntohs(from.sin_port);}
        /* 空配列はEOFまたは長さ0の受信を意味する．未到着(EAGAIN)も-1を返す． */
        if(rc<0){setvar(name,-1);return;}
        if(rc>size){error=1;return;}
        for(int i=0;i<rc;i++)values[i]=buf[i];
        seqnew(name,1,values,rc);return;
    }
    int is_read=word("i2c_read"),is_xfer=0;
    if(!is_read)is_xfer=word("i2c_xfer");
    if(is_read||is_xfer){
        int xfer=is_xfer;
        if(!eat('(')){error=1;return;}
        int fd=parse_fd();if(!eat(',')){error=1;return;}
        int32_t addr=expr();if(!eat(',')){error=1;return;}
        int32_t values[SEQ_MAX]={0},result[32]={0};int len=0;
        if(xfer){char srcname[16];unsigned z=0;ws();while(idchar(*cursor)&&z<15)srcname[z++]=*cursor++;
            srcname[z]=0;int k=seqslot(srcname);if(k<0||!seq[k].used){error=1;return;}
            len=seq[k].len;if(len>32){error=1;return;}
            for(int j=0;j<len;j++)values[j]=items(k)[j];if(!eat(',')){error=1;return;}}
        int32_t r=expr();if(!eat(')')||r<0||r>32){error=1;return;}
        ws();if(*cursor&&*cursor!='#')error=1;
        if(!error){
            int rc=transfer(fd,addr,values,len,r,result);
            if(rc<0)setvar(name,-1);
            else seqnew(name,1,result,r);
        }
        return;
    }
    int32_t v=expr();ws();if(*cursor&&*cursor!='#')error=1;if(!error)setvar(name,v);
}

/* インデントに従って現在の複合文の終端行を探す． */
static unsigned block_end(unsigned start){unsigned i=start+1;while(i<count&&levels[i]>levels[start])i++;return i;}
/* ブロックからのループ制御は直近の外側ループへ伝える． */
enum {FLOW_NORMAL, FLOW_BREAK, FLOW_CONTINUE};
/* インデントで表される文ブロックを実行し，ループ制御を呼出し元へ伝える． */
static int execute(unsigned first,unsigned last,unsigned depth,unsigned in_loop){
    if(depth>8){error=1;return FLOW_NORMAL;}
    for(unsigned i=first;i<last&&!error;){error_line=i+1;cursor=source+offsets[i];ws();
        if(word("break")||word("continue")){
            int flow=source[offsets[i]]=='b'?FLOW_BREAK:FLOW_CONTINUE;
            ws();if((*cursor&&*cursor!='#')||!in_loop)error=1;
            return flow;
        }
        cursor=source+offsets[i];ws();
        int is_if=word("if"),is_while=0,is_for=0;
        if(!is_if){is_while=word("while");if(!is_while)is_for=word("for");}
        if(is_if||is_while||is_for){
            int32_t condition=0,start=0,stop=0,step=1;
            char target[16]={0};
            if(is_for){
                ws();unsigned n=0;
                if(!((*cursor>='a'&&*cursor<='z')||(*cursor>='A'&&*cursor<='Z')||*cursor=='_'))error=1;
                while(idchar(*cursor)){
                    if(n<15)target[n++]=*cursor;
                    else error=1;
                    cursor++;
                }
                if(!word("in")||!word("range")||!eat('('))error=1;
                int32_t a=expr();
                if(eat(',')){
                    start=a;stop=expr();
                    if(eat(','))step=expr();
                }else stop=a;
                if(!eat(')')||step==0)error=1;
            }else condition=expr();
            if(!eat(':'))error=1;
            ws();if(*cursor&&*cursor!='#')error=1;
            unsigned end=block_end(i);
            if(i+1==end||levels[i+1]<=levels[i])error=1;
            if(error)break;
            unsigned other=end;
            if(is_if&&end<last&&levels[end]==levels[i]){
                cursor=source+offsets[end];if(word("else")&&eat(':')){
                    ws();if(*cursor&&*cursor!='#')error=1;
                    other=block_end(end);
                }
            }
            if(is_while){
                unsigned iterations=0;
                while(condition&&!error){
                    if(++iterations>10000){error=1;break;}
                    int flow=execute(i+1,end,depth+1,1);
                    if(flow==FLOW_BREAK)break;
                    if(error)break;
                    cursor=source+offsets[i];word("while");condition=expr();
                }
            }else if(is_for){
                unsigned iterations=0;
                int64_t v=start;
                while(!error&&(step>0?v<(int64_t)stop:v>(int64_t)stop)){
                    if(++iterations>10000){error=1;break;}
                    setvar(target,(int32_t)v);if(error)break;
                    int flow=execute(i+1,end,depth+1,1);
                    if(flow==FLOW_BREAK)break;
                    v+=(int64_t)step;
                }
            }else if(condition){
                int flow=execute(i+1,end,depth+1,in_loop);
                if(flow!=FLOW_NORMAL)return flow;
            }else if(other>end){
                int flow=execute(end+1,other,depth+1,in_loop);
                if(flow!=FLOW_NORMAL)return flow;
            }
            i=other;
        }else{statement(i);i++;}
    }
    return FLOW_NORMAL;
}

/* リストやbytes等の未閉鎖の括弧に含まれる複数行を，インデントを維持して結合する．既存ソースバッファの上限内に収める． */
static unsigned join_continuations(unsigned n){
    unsigned out=0;int nesting=0;char quote=0;
    for(unsigned i=0;i<n;i++){
        char c=source[i];
        if(quote){if(c==quote)quote=0;}
        else if(c=='\"'||c=='\'')quote=c;
        else if(c=='['||c=='(')nesting++;
        else if((c==']'||c==')')&&nesting>0)nesting--;
        if((c=='\n'||c=='\r')&&nesting>0){
            if(out && source[out-1]!=' ')source[out++]=' ';
            while(i+1<n&&(source[i+1]==' '||source[i+1]=='\t'||source[i+1]=='\n'||source[i+1]=='\r'))i++;
        }else source[out++]=c;
    }
    source[out]=0;return out;
}

/* 与えられたソースの先頭から文を順に実行する． */
static void run_source(unsigned n){
    n=join_continuations(n);
    count=0;error=0;error_line=0;
    unsigned off=0;
    while(off<n&&count<64){unsigned begin=off,indent=0;
        while(off<n && source[off]==' '){off++;indent++;}
        if(off<n && source[off]=='\t'){error=1;break;}
        unsigned content=off;
        while(off<n && source[off]!='\n' && source[off]!='\r')off++;
        unsigned end=off;
        while(end>content&&source[end-1]==' ')end--;
        if(content<end&&source[content]!='#'){
            offsets[count]=content;levels[count]=indent;count++;
        }
        while(off<n&&(source[off]=='\n'||source[off]=='\r'))off++;
        source[end]=0;
        if(off==begin)break;
    }
    if(!error)execute(0,count,0,0);
    if(error){outs("python: syntax/runtime error at statement ");number((int32_t)error_line);outs("\r\n");}
}

/* REPL入力に未閉鎖の括弧が残っているか調べる． */
static int pending_brackets(const char *s,unsigned n){
    int depth=0;char quote=0;
    for(unsigned i=0;i<n;i++){
        char c=s[i];
        if(quote){if(c=='\\' && i+1<n){i++;continue;}if(c==quote)quote=0;}
        else if(c=='"'||c=='\'')quote=c;
        else if(c=='['||c=='('||c=='{')depth++;
        else if(c==']'||c==')'||c=='}')depth--;
    }
    return depth;
}

/* REPL入力を読み，継続行を処理して一文ずつ実行する． */
static void repl(void){
    outs("PicoNIX Tiny Python (Python subset)\r\n");
    outs("Type exit() to quit.\r\n");
    repl_mode=1;
    for(;;){
        unsigned n=0;int block=0;
        for(;;){
            outs(block?"... ":">>> ");
            char line[128];
            int m=read_line(line,sizeof(line));
            if(m<0){n=0;msleep(10);break;}
            while(m>0&&(line[m-1]=='\n'||line[m-1]=='\r'))m--;
            line[m]=0;
            if(!block && same(line,"exit()")){
                if(get_tty()==1)outs("PicoNIX: Session closed.\r\n");
                return;
            }
            if(!block && !m)break;
            if(block && !m && pending_brackets(source,n)<=0)break;
            if(n+(unsigned)m+1>=sizeof(source)){
                outs("python: input too long\r\n");n=0;break;
            }
            for(int i=0;i<m;i++)source[n++]=line[i];
            source[n++]='\n';
            if(pending_brackets(source,n)>0){block=1;continue;}
            if(!block){
                if(m && line[m-1]==':'){block=1;continue;}
                break;
            }
            /* インデントブロックでない式は完成した時点で即座に実行する． */
            unsigned i=0;while(i<n && (source[i]==' '||source[i]=='\t'))i++;
            int suite=0;
            while(i<n && source[i]!='\n' && source[i]!='\r')i++;
            if(i>0 && source[i-1]==':')suite=1;
            if(!suite)break;
        }
        if(n){source[n]=0;run_source(n);}
    }
}

/* ファイル実行と対話実行を引数に応じて切り替える． */
int main(int argc, char *argv[]){
    if(argc>2) {outs("usage: python [file]\r\n");_exit(1);}
    if(argc<2 || !argv[1]){repl();goto done;}
    const char *path=argv[1];
    int fd=open(path, 0);
    if(fd<0){outs("python: script not found\r\n");goto done;}
    int n=read(fd, source, SOURCE_MAX);
    if(n<0){close(fd);outs("python: read error\r\n");goto done;}
    char extra;
    int more=read(fd,&extra,1);
    close(fd);
    if(more<0){outs("python: read error\r\n");goto done;}
    if(more>0){outs("python: script exceeds 2048 bytes\r\n");error=1;goto done;}
    source[n]=0;run_source((unsigned)n);
done:
    _exit(error?1:0);
    for(;;)msleep(1000);
}
