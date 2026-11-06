/* SPDX-License-Identifier: GPL-3.0-or-later */
/* 動的メモリ確保を使わない，PicoNIX向けの最小Cライブラリ．
 * 標準入出力と文字列処理を小さな実行イメージ内で提供する． */

/* PIC対応の最小Cライブラリ．動的確保とSDK依存を持たない． */
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "unistd.h"
#include "fcntl.h"
#include <stdint.h>
#include <limits.h>

#define F_READ 1
#define F_WRITE 2
#define F_LINE 4
#define F_UNBUF 8
#define F_EOF 16
#define F_ERR 32
#define F_OWN 64
FILE __piconix_stdin = {.fd=0,.flags=F_READ};
FILE __piconix_stdout = {.fd=1,.flags=F_WRITE|F_LINE};
FILE __piconix_stderr = {.fd=2,.flags=F_WRITE|F_UNBUF};
/* ヒープを使わず，fopen可能なストリームは各プロセス最大4個とする． */
static FILE files[4];

/* NUL終端文字列の長さを数える．終端文字は長さに含めない． */
size_t strlen(const char *s) { const char *p=s; while (*p) ++p; return (size_t)(p-s); }

/* 文字列を終端文字まで宛先へコピーし，宛先を返す． */
char *strcpy(char *d,const char *s) { char *r=d; while ((*d++=*s++)) {} return r; }

/* 最大nバイトをコピーする．短い元文字列では残りをNULで埋める． */
char *strncpy(char *d,const char *s,size_t n) { char *r=d; while(n && *s) { *d++=*s++; --n; } while(n--) *d++=0; return r; }

/* 宛先文字列の末尾に別の文字列を追加する． */
char *strcat(char *d,const char *s) { strcpy(d+strlen(d),s); return d; }

/* 二つの文字列を辞書順で比較し，差の符号を返す． */
int strcmp(const char *a,const char *b) { while(*a && *a==*b) {++a;++b;} return (unsigned char)*a-(unsigned char)*b; }

/* 文字列を最大n文字比較し，最初に異なるバイトの差を返す． */
int strncmp(const char *a,const char *b,size_t n) { while(n--) { unsigned char x=(unsigned char)*a++,y=(unsigned char)*b++; if(x!=y)return (int)x-(int)y; if(!x)return 0; } return 0; }

/* 文字列から指定文字を探し，最初の位置またはNULLを返す． */
char *strchr(const char *s,int c) { for(;;++s) { if((unsigned char)*s==(unsigned char)c)return (char *)s; if(!*s)return 0; } }

/* 文字列から部分文字列を探し，最初の位置またはNULLを返す． */
char *strstr(const char *s,const char *sub) { if(!*sub)return (char *)s; for(;*s;++s) {const char *a=s,*b=sub;while(*a && *b && *a==*b){++a;++b;}if(!*b)return (char *)s;} return 0; }

/* バイト単位の処理にして，コンパイラが外部libc呼出しを生成しないようにする． */
void *memcpy(void *d,const void *s,size_t n) { unsigned char *a=d;const unsigned char *b=s;while(n--)*a++=*b++;return d; }

/* 領域が重なる場合も正しい順序でnバイト移動する． */
void *memmove(void *d,const void *s,size_t n) { unsigned char *a=d;const unsigned char *b=s;if(a<b){while(n--)*a++=*b++;}else if(a>b){a+=n;b+=n;while(n--)*--a=*--b;}return d; }

/* 指定領域のnバイトを同じ値で埋める． */
void *memset(void *d,int c,size_t n) { unsigned char *a=d;while(n--)*a++=(unsigned char)c;return d; }

/* Clang -Ozは構造体コピーなどをARM EABIのメモリ操作へ変換することがある．単独のPICイメージで動作するよう必要な補助関数をここに用意する．libgccが必ず提供するとは限らない． */
void __aeabi_memcpy(void *d, const void *s, size_t n) { (void)memcpy(d, s, n); }

/* ARM EABIの4バイト整列コピー要求を自前のmemcpyへ委譲する． */
void __aeabi_memcpy4(void *d, const void *s, size_t n) { __aeabi_memcpy(d, s, n); }

/* ARM EABIの8バイト整列コピー要求を自前のmemcpyへ委譲する． */
void __aeabi_memcpy8(void *d, const void *s, size_t n) { __aeabi_memcpy(d, s, n); }

/* ARM EABIの重複可能なメモリ移動を自前のmemmoveへ委譲する． */
void __aeabi_memmove(void *d, const void *s, size_t n) { (void)memmove(d, s, n); }

/* EABIのmemsetはISO Cのmemsetと異なり，引数順が宛先・長さ・値である． */
void __aeabi_memset(void *d, size_t n, int c) { (void)memset(d, c, n); }

/* ARM EABIの4バイト整列初期化を自前のmemsetへ委譲する．引数順に注意する． */
void __aeabi_memset4(void *d, size_t n, int c) { __aeabi_memset(d, n, c); }

/* ARM EABIの8バイト整列初期化を自前のmemsetへ委譲する．引数順に注意する． */
void __aeabi_memset8(void *d, size_t n, int c) { __aeabi_memset(d, n, c); }

/* 最大nバイトを検索し，最初に指定値と一致した位置を返す． */
void *memchr(const void *s,int c,size_t n) {
    const unsigned char *p=s;
    for(size_t i=0;i<n;++i)if(p[i]==(unsigned char)c)return (void *)(p+i);
    return 0;
}

/* 指定長の二つのメモリ領域を比較し，最初の不一致の差を返す． */
int memcmp(const void *a,const void *b,size_t n) {const unsigned char *x=a,*y=b;while(n--){int diff=(int)*x++-(int)*y++;if(diff)return diff;}return 0;}

/* 整数の絶対値を返す． */
int abs(int x) { return x<0?-x:x; } /* ISO Cと同様にabs(INT_MIN)の動作は未定義である． */

long strtol(const char *s,char **end,int base) {
    const char *p=s;while(*p==' '||(*p>='\t'&&*p<='\r'))++p;
    int negative=0;if(*p=='+'||*p=='-')negative=(*p++=='-');
    if((base==0||base==16)&&p[0]=='0'&&(p[1]=='x'||p[1]=='X')) {
        char c=p[2];if((c>='0'&&c<='9')||(c>='a'&&c<='f')||(c>='A'&&c<='F')){p+=2;base=16;}
    }
    if(base==0)base=(*p=='0')?8:10;
    if(base<2||base>36){if(end)*end=(char *)s;return 0;}
    const char *first=p;unsigned long value=0;
    unsigned long limit=negative?(unsigned long)LONG_MAX+1ul:(unsigned long)LONG_MAX;
    int overflow=0;
    for(;;++p){unsigned char c=(unsigned char)*p;unsigned digit;
        if(c>='0'&&c<='9')digit=c-'0';else if(c>='a'&&c<='z')digit=c-'a'+10u;
        else if(c>='A'&&c<='Z')digit=c-'A'+10u;else break;
        if(digit>=(unsigned)base)break;
        if(value>(limit-digit)/(unsigned)base){overflow=1;value=limit;}else if(!overflow)value=value*(unsigned)base+digit;
    }
    if(end)*end=(char *)(p==first?s:p);
    if(p==first)return 0;
    if(overflow)return negative?LONG_MIN:LONG_MAX;
    if(negative && value==(unsigned long)LONG_MAX+1ul)return LONG_MIN;
    return negative?-(long)value:(long)value;
}

/* 符号と基数接頭辞を解釈して整数へ変換する．endには走査終了位置を返し，範囲超過時はULONG_MAXへ飽和する． */
unsigned long strtoul(const char *s,char **end,int base) {
    const char *p=s;
    while(*p==' '||(*p>='\t'&&*p<='\r'))++p;
    int negative=0;
    if(*p=='+'||*p=='-')negative=(*p++=='-');
    if((base==0||base==16)&&p[0]=='0'&&(p[1]=='x'||p[1]=='X')) {
        char c=p[2];
        if((c>='0'&&c<='9')||(c>='a'&&c<='f')||(c>='A'&&c<='F')){p+=2;base=16;}
    }
    if(base==0)base=*p=='0'?8:10;
    if(base<2||base>36){if(end)*end=(char *)s;return 0;}
    const char *first=p;
    unsigned long value=0;
    int overflow=0;
    for(;*p;++p){
        unsigned char c=(unsigned char)*p;unsigned digit;
        if(c>='0'&&c<='9')digit=c-'0';
        else if(c>='a'&&c<='z')digit=c-'a'+10u;
        else if(c>='A'&&c<='Z')digit=c-'A'+10u;
        else break;
        if(digit>=(unsigned)base)break;
        if(value>(ULONG_MAX-digit)/(unsigned)base){overflow=1;value=ULONG_MAX;}
        else if(!overflow)value=value*(unsigned)base+digit;
    }
    if(end)*end=(char *)(p==first?s:p);
    if(p==first)return 0;
    return overflow?ULONG_MAX:negative?0ul-value:value;
}

/* 文字列の先頭を符号付き10進整数として読み取る． */
int atoi(const char *s) {return (int)strtol(s,0,10);}

/* 標準出力のバッファを部分書込みまで処理して送る． */
static int output(int fd,const char *s,size_t n) {
    size_t done=0;
    while(done<n){
        int r=write(fd,s+done,n-done);
        /* ネットワークTTYの送信キュー満杯時はアイドル処理へ譲り，未送信部分を再試行する．ANSI制御列や表示行を途中で失わない． */
        if(r==OS_EAGAIN || r==0){msleep(1);continue;}
        if(r<0)return -1;
        done+=(unsigned)r;
    }
    return (int)done;
}

/* 保留中の文字を送信する．失敗時には残りを保持し，次のfflushで再試行できる． */
int fflush(FILE *f){
    if(!f){
        int result=0;
        if(fflush(stdout)<0)result=EOF;
        if(fflush(stderr)<0)result=EOF;
        for(unsigned i=0;i<4;++i)if((files[i].flags&F_OWN)&&fflush(&files[i])<0)result=EOF;
        return result;
    }
    if(!(f->flags&F_WRITE))return 0;
    unsigned done=0;
    while(done<f->len){
        int r=write(f->fd,f->buf+done,f->len-done);
        if(r==OS_EAGAIN||r==0){msleep(1);continue;}
        if(r<0){
            if(done){memmove(f->buf,f->buf+done,f->len-done);f->len-=done;}
            f->flags|=F_ERR;return EOF;
        }
        done+=(unsigned)r;
    }
    f->len=0;return 0;
}

/* FILEストリームのバッファリング方式を設定する．利用できる方式は限定される． */
int setvbuf(FILE *f,char *buf,int mode,size_t size){
    (void)size;
    if(!f||buf||(mode!=_IOFBF&&mode!=_IOLBF&&mode!=_IONBF))return -1;
    if(fflush(f)<0)return -1;
    f->flags&=(unsigned char)~(F_LINE|F_UNBUF);
    if(mode==_IOLBF)f->flags|=F_LINE;
    if(mode==_IONBF)f->flags|=F_UNBUF;
    return 0;
}

/* FILEの出力条件を検査して1文字送る．必要に応じてフラッシュする． */
static int stream_put(FILE *f,unsigned char c){
    if(!f||!(f->flags&F_WRITE))return EOF;
    if(f->flags&F_UNBUF){if(output(f->fd,(const char *)&c,1)<0){f->flags|=F_ERR;return EOF;}return c;}
    if(f->len==64 && fflush(f)<0)return EOF;
    f->buf[f->len++]=c;
    if((f->flags&F_LINE)&&c=='\n'&&fflush(f)<0)return EOF;
    return c;
}

/* FILEの読出し位置とungetc用保存バイトを管理する．入力が尽きればFDから補充し，EOFとエラーを区別して記録する． */
static int stream_get(FILE *f){
    if(!f||!(f->flags&F_READ))return EOF;
    if(f->saved_count)return f->saved[--f->saved_count];
    if(f->pos==f->len){
        /* TTYでも入力前にプロンプトを表示させる． */
        if(f==stdin)fflush(stdout);
        int r=read(f->fd,f->buf,64);
        if(r<=0){f->flags|=r==0?F_EOF:F_ERR;return EOF;}
        f->pos=0;f->len=(unsigned char)r;
    }
    return f->buf[f->pos++];
}

/* 直前に読んだ1文字をFILEへ戻し，次回読出しへ備える． */
static void stream_unget(FILE *f,int c){
    if(c!=EOF&&f->saved_count<sizeof(f->saved)){
        f->saved[f->saved_count++]=(unsigned char)c;f->flags&=(unsigned char)~F_EOF;
    }
}

/* FILEストリームの終端到達フラグを返す． */
int feof(FILE *f){return f&&(f->flags&F_EOF)!=0;}

/* FILEストリームの入出力エラーフラグを返す． */
int ferror(FILE *f){return f&&(f->flags&F_ERR)!=0;}

/* openしたFDを小さなFILE管理表に関連付ける．失敗時はNULL． */
FILE *fopen(const char *path,const char *mode){
    if(!path||!mode||!mode[0])return 0;
    int flags,access;
    if(mode[0]=='r'){flags=O_RDONLY;access=F_READ;}
    else if(mode[0]=='w'){flags=O_WRONLY|O_CREAT|O_TRUNC;access=F_WRITE;}
    else if(mode[0]=='a'){flags=O_WRONLY|O_CREAT|O_APPEND;access=F_WRITE;}
    else return 0;
    /* テキストとバイナリを同じ扱いにし，更新モード（+）は意図的に省く． */
    if(mode[1] && !(mode[1]=='b' && !mode[2]))return 0;
    for(unsigned i=0;i<4;++i)if(!(files[i].flags&F_OWN)){
        int fd=open(path,flags);if(fd<0)return 0;
        FILE *f=&files[i];memset(f,0,sizeof(*f));f->fd=fd;f->flags=access|F_OWN;
        return f;
    }
    return 0;
}

/* 保留中の出力を流し，FDとFILE管理枠を解放する． */
int fclose(FILE *f){
    if(!f || !(f->flags&F_OWN))return EOF;
    int result=fflush(f);
    if(close(f->fd)<0)result=EOF;
    memset(f,0,sizeof(*f));return result;
}

/* 指定サイズ・個数で入力を読み取る．FILEの状態を更新し，完了した要素数を返す． */
size_t fread(void *ptr,size_t size,size_t count,FILE *f){
    if(!size||!count)return 0;
    if(!ptr||!f||!(f->flags&F_READ)||count>((size_t)-1)/size)return 0;
    unsigned char *p=ptr;size_t total=size*count,n=0;
    while(n<total){int c=stream_get(f);if(c==EOF)break;p[n++]=(unsigned char)c;}
    return n/size;
}

/* 指定サイズ・個数の出力を行う．短いwriteを扱い，完了した要素数を返す． */
size_t fwrite(const void *ptr,size_t size,size_t count,FILE *f){
    if(!size||!count)return 0;
    if(!ptr||!f||!(f->flags&F_WRITE)||count>((size_t)-1)/size)return 0;
    const unsigned char *p=ptr;size_t total=size*count,n=0;
    while(n<total && stream_put(f,p[n])!=EOF)++n;
    return n/size;
}

/* stdoutへ1文字出力する． */
int putchar(int c){return stream_put(stdout,(unsigned char)c);}

/* stdinから1文字取得する． */
int getchar(void){return stream_get(stdin);}

/* 文字列をstdoutへ出力し，末尾に改行を付ける． */
int puts(const char *s){
    size_t n=strlen(s);
    return fwrite(s,1,n,stdout)==n&&putchar('\n')!=EOF?(int)(n+1):EOF;
}

/* FILEから最大size-1文字を読み，改行またはEOFまでをNUL終端で格納する． */
char *fgets(char *s,int size,FILE *stream){
    if(!s||!stream||size<=0)return 0;
    int n=0;
    while(n<size-1){int c=stream_get(stream);if(c==EOF)break;s[n++]=(char)c;if(c=='\n')break;}
    if(!n&&size>1)return 0;
    s[n]=0;return s;
}

typedef struct {char *dst;size_t cap;size_t count;FILE *stream;int failed;} Format;
/* 書式出力の1文字を，ファイルまたは文字列バッファの出力先へ渡す． */
static void emit(Format *f,char c){
    if(!f->stream){if(f->dst && f->cap && f->count<f->cap-1)f->dst[f->count]=c;}
    else if(stream_put(f->stream,(unsigned char)c)==EOF)f->failed=1;
    ++f->count;
}

/* 指定基数で整数を桁の文字列へ変換する． */
static unsigned digits(char *buf,uint32_t n,unsigned base,int upper){
    unsigned i=0;
    do {unsigned d=n%base;buf[i++]=(char)(d<10?'0'+d:(upper?'A':'a')+d-10);n/=base;}while(n);
    return i;
}

/* 書式指定の最小幅に合わせて空白等を出力する． */
static void pad(Format *f,char c,int n){while(n-->0)emit(f,c);}

/* 限定的なprintf書式指定を解釈して文字を出力する．標準libcの全仕様には対応しない． */
static int format(Format *f,const char *fmt,va_list ap){
    while(*fmt){
        if(*fmt!='%'){emit(f,*fmt++);continue;}
        ++fmt;
        if(*fmt=='%'){emit(f,'%');++fmt;continue;}
        int left=0,zero=0,width=0,precision=-1;
        if(*fmt=='-'){left=1;fmt++;}
        if(*fmt=='0'){zero=1;fmt++;}
        while(*fmt>='0'&&*fmt<='9'){if(width<10000)width=width*10+(*fmt-'0');fmt++;}
        if(*fmt=='.'){
            fmt++;precision=0;
            while(*fmt>='0'&&*fmt<='9'){if(precision<10000)precision=precision*10+(*fmt-'0');fmt++;}
        }
        /* 32ビットCortex-M0+用の整数長修飾子を処理する． */
        if(*fmt=='l'){fmt++;if(*fmt=='l')fmt++;}
        else if(*fmt=='h'){fmt++;if(*fmt=='h')fmt++;}
        char spec=*fmt;if(!spec)break;fmt++;
        if(spec=='s'){
            const char *v=va_arg(ap,const char *);if(!v)v="(null)";
            int n=0;while(v[n]&&(precision<0||n<precision))n++;
            if(!left)pad(f,' ',width-n);
            for(int i=0;i<n;i++)emit(f,v[i]);
            if(left)pad(f,' ',width-n);
        }else if(spec=='c'){
            int c=va_arg(ap,int);
            if(!left)pad(f,' ',width-1);
            emit(f,(char)c);
            if(left)pad(f,' ',width-1);
        }else if(spec=='d'||spec=='i'||spec=='u'||spec=='x'||spec=='X'){
            uint32_t u;int negative=0;
            if(spec=='d'||spec=='i'){
                int32_t v=va_arg(ap,int);u=(uint32_t)v;
                if(v<0){negative=1;u=0u-u;}
            }else u=va_arg(ap,unsigned);
            char buf[32];unsigned n=digits(buf,u,(spec=='x'||spec=='X')?16:10,spec=='X');
            if(precision==0&&u==0)n=0;
            int zeros=precision>(int)n?precision-(int)n:0;
            int spaces=width-(int)n-zeros-negative;
            if(zero&&!left&&precision<0){zeros+=spaces>0?spaces:0;spaces=0;}
            if(!left)pad(f,' ',spaces);
            if(negative)emit(f,'-');
            pad(f,'0',zeros);
            while(n)emit(f,buf[--n]);
            if(left)pad(f,' ',spaces);
        }else{emit(f,'%');emit(f,spec);}
    }
    if(!f->stream&&f->dst&&f->cap)f->dst[f->count<f->cap?f->count:f->cap-1]=0;

    return f->failed?-1:(int)f->count;
}

/* 簡易書式解析でstdoutへ整形出力する．対応形式は実装範囲に限る． */
int printf(const char *fmt,...){Format f={0};f.stream=stdout;va_list a;va_start(a,fmt);int r=format(&f,fmt,a);va_end(a);return r;}

/* 簡易書式解析で指定FILEへ整形出力する． */
int fprintf(FILE *stream,const char *fmt,...){if(!stream||!(stream->flags&F_WRITE))return EOF;Format f={0};f.stream=stream;va_list a;va_start(a,fmt);int r=format(&f,fmt,a);va_end(a);return r;}

/* 最大サイズを守って整形文字列を生成する． */
int snprintf(char *s,size_t n,const char *fmt,...){Format f={0};f.dst=s;f.cap=n;va_list a;va_start(a,fmt);int r=format(&f,fmt,a);va_end(a);return r;}

/* サイズ指定のない書式出力を行う．呼出し側が十分な領域を確保する． */
int sprintf(char *s,const char *fmt,...){Format f={0};f.dst=s;f.cap=(size_t)-1;va_list a;va_start(a,fmt);int r=format(&f,fmt,a);va_end(a);return r;}

/* 小さなscanf群．浮動小数点・動的確保・ロケールには対応しない． */
typedef struct { const char *text; size_t pos; FILE *stream; } Scan;
/* 書式入力処理で次の1文字を取り出す． */
static int scan_get(Scan *s) {
    if (!s->stream) {
        unsigned char c=(unsigned char)s->text[s->pos];
        if (!c)return EOF;
        ++s->pos;
        return c;
    }
    int c=stream_get(s->stream);
    if(c!=EOF)++s->pos;
    return c;
}

/* 書式入力処理で読んだ1文字を入力元へ戻す． */
static void scan_unget(Scan *s,int c) {
    if(c==EOF)return;
    if(s->stream)stream_unget(s->stream,c);
    else --s->pos;
    if(s->stream)--s->pos;
}

/* 書式入力で先頭の空白を読み飛ばす． */
static int scan_space(int c){return c==' '||(c>='\t'&&c<='\r');}

/* 書式入力で1桁の数字を指定基数として解釈する． */
static int scan_digit(int c){
    if(c>='0'&&c<='9')return c-'0';
    if(c>='a'&&c<='z')return c-'a'+10;
    if(c>='A'&&c<='Z')return c-'A'+10;
    return -1;
}

/* 限定的なscanf書式を解析する．対象入力と形式に明示的な上限がある． */
static int scan_run(Scan *s,const char *fmt,va_list ap){
    int assigned=0;
    while(*fmt){
        if(scan_space((unsigned char)*fmt)){
            do{++fmt;}while(scan_space((unsigned char)*fmt));
            int c;do{c=scan_get(s);}while(scan_space(c));
            scan_unget(s,c);
            continue;
        }
        if(*fmt!='%'){
            int c=scan_get(s);
            if(c!=(unsigned char)*fmt){scan_unget(s,c);return c==EOF&&!assigned?EOF:assigned;}
            ++fmt;continue;
        }
        ++fmt;
        if(*fmt=='%'){
            int c=scan_get(s);++fmt;
            if(c!='%'){scan_unget(s,c);return c==EOF&&!assigned?EOF:assigned;}
            continue;
        }
        int suppress=0,width=0;
        if(*fmt=='*'){suppress=1;++fmt;}
        while(*fmt>='0'&&*fmt<='9'){
            if(width<10000)width=width*10+(*fmt-'0');
            ++fmt;
        }
        char spec=*fmt;
        if(!spec)return assigned;
        ++fmt;
        if(spec!='d'&&spec!='i'&&spec!='u'&&spec!='x'&&spec!='X'&&spec!='s'&&spec!='c')return assigned;
        if(spec!='c'){
            int c;do{c=scan_get(s);}while(scan_space(c));
            scan_unget(s,c);
        }
        if(spec=='s'){
            char *dst=suppress?0:va_arg(ap,char *);
            int n=0,c;
            while((!width||n<width)&&(c=scan_get(s))!=EOF){
                if(scan_space(c)){scan_unget(s,c);break;}
                if(dst)dst[n]=(char)c;
                ++n;
            }
            if(!n){int next=scan_get(s);scan_unget(s,next);return next==EOF&&!assigned?EOF:assigned;}
            if(dst){dst[n]=0;++assigned;}
            continue;
        }
        if(spec=='c'){
            char *dst=suppress?0:va_arg(ap,char *);
            int n=width?width:1;
            for(int i=0;i<n;++i){
                int c=scan_get(s);
                if(c==EOF)return assigned?assigned:EOF;
                if(dst)dst[i]=(char)c;
            }
            if(dst)++assigned;
            continue;
        }
        int base=spec=='x'||spec=='X'?16:spec=='i'?0:10;
        int max=width?width:10000,n=0,negative=0,have=0,c=scan_get(s);
        if(c==EOF)return assigned?assigned:EOF;
        if((c=='+'||c=='-')&&max>0){negative=c=='-';++n;c=scan_get(s);}
        if(c=='0'&&n<max){
            have=1;++n;
            c=scan_get(s);
            if((base==0||base==16)&&(c=='x'||c=='X')&&n<max){
                int next=scan_get(s);
                if(scan_digit(next)>=0&&scan_digit(next)<16&&n+1<max){
                    n+=2;base=16;c=next;have=0;
                }else{
                    scan_unget(s,next);scan_unget(s,c);
                    c=EOF;
                }
            }
            if(base==0)base=8;
        }
        if(base==0)base=10;
        unsigned long value=0;
        while(c!=EOF&&n<max){
            int d=scan_digit(c);
            if(d<0||d>=base)break;
            have=1;++n;
            value=value*(unsigned)base+(unsigned)d;
            c=scan_get(s);
        }
        scan_unget(s,c);
        if(!have)return assigned;
        if(!suppress){
            if(spec=='d'||spec=='i')*va_arg(ap,int *)=negative?-(int)value:(int)value;
            else *va_arg(ap,unsigned *)=negative?0u-(unsigned)value:(unsigned)value;
            ++assigned;
        }
    }
    return assigned;
}

/* stdinから簡易書式に従って値を読み取る． */
int scanf(const char *fmt,...){
    Scan s={0};s.stream=stdin;
    va_list ap;va_start(ap,fmt);int r=scan_run(&s,fmt,ap);va_end(ap);return r;
}

/* 指定FILEから簡易書式に従って値を読み取る． */
int fscanf(FILE *stream,const char *fmt,...){
    if(!stream)return EOF;
    Scan s={0};s.stream=stream;
    va_list ap;va_start(ap,fmt);int r=scan_run(&s,fmt,ap);va_end(ap);return r;
}

/* 入力文字列から簡易書式に従って値を読み取る． */
int sscanf(const char *text,const char *fmt,...){
    Scan s={0};s.text=text;
    va_list ap;va_start(ap,fmt);int r=scan_run(&s,fmt,ap);va_end(ap);return r;
}

/* 一部のコンパイラがローカル構造体のゼロ初期化で生成するARM EABI補助関数． */
void __aeabi_memclr(void *dst, size_t n) {
    volatile unsigned char *p = (volatile unsigned char *)dst;
    while (n--)
        *p++ = 0;
}

/* ARM EABIの4バイト整列ゼロクリア要求をmemsetへ委譲する． */
void __aeabi_memclr4(void *dst, size_t n) { __aeabi_memclr(dst, n); }

/* ARM EABIの8バイト整列ゼロクリア要求をmemsetへ委譲する． */
void __aeabi_memclr8(void *dst, size_t n) { __aeabi_memclr(dst, n); }
