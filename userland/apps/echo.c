/* SPDX-License-Identifier: GPL-3.0-or-later */
/* 引数を標準出力へ書き出す． */

#include "unistd.h"
#include "string.h"

/* 短い文字列は1回のwriteで出力する．部分書込み時は残りを送る． */
typedef struct {
    char data[128];
    size_t used;
} echo_buffer_t;

/* 蓄積した標準出力を部分書込みに対応して送出する． */
static int flush(echo_buffer_t *out) {
    size_t sent=0;
    while(sent<out->used) {
        int n=write(1,out->data+sent,out->used-sent);
        if(n<0) return -1;
        if(!n) { msleep(1); continue; }
        sent+=(size_t)n;
    }
    out->used=0;
    return 0;
}

/* エスケープ展開後の1バイトを蓄積する．満杯になったら送信する． */
static int emit(echo_buffer_t *out,char c) {
    if(out->used==sizeof out->data && flush(out)<0) return -1;
    out->data[out->used++]=c;
    return 0;
}

/* エスケープの16進数字を数値へ変換する．不正値なら負値を返す． */
static int digit(char c,int base) {
    int d=(c>='0'&&c<='9')?c-'0':(c>='a'&&c<='f')?c-'a'+10:
          (c>='A'&&c<='F')?c-'A'+10:-1;
    return d>=0&&d<base?d:-1;
}

/* エスケープ指定を考慮し，echoの引数文字列を出力する． */
static int output(echo_buffer_t *out,const char *s,size_t n,int escapes) {
    for(size_t i=0;i<n;i++) {
        char c=s[i];
        if(escapes && c=='\\' && i+1<n) {
            char e=s[++i];
            if(e=='n') c='\n';
            else if(e=='r') c='\r';
            else if(e=='t') c='\t';
            else if(e=='\\') c='\\';
            else if(e=='x') {
                int v=0,k=0,d;
                while(k<2 && i+1<n && (d=digit(s[i+1],16))>=0) {
                    v=v*16+d;i++;k++;
                }
                if(k)
                    c=(char)v;
                else {if(emit(out,'\\')<0)return -1;c='x';}
            } else if(e>='0'&&e<='7') {
                int v=e-'0',k=1,d;
                while(k<3 && i+1<n && (d=digit(s[i+1],8))>=0) {
                    v=v*8+d;i++;k++;
                }
                c=(char)v;
            } else {if(emit(out,'\\')<0)return -1;c=e;}
        }
        if(emit(out,c)<0)return -1;
    }
    return 0;
}

/* 引数を空白区切りで表示し，オプションに応じて改行・エスケープを扱う． */
int main(int argc,char *argv[]) {
    int newline=1,escapes=0,i=1;
    /* -n，-e，-ne，-enを受け付け，--でオプション解析を終える． */
    for(;i<argc;i++) {
        const char *p=argv[i];
        if(!strcmp(p,"--")) {i++;break;}
        if(*p!='-' || !p[1])
            break;
        const char *q=p+1;
        for(;*q && (*q=='n'||*q=='e');q++);
        if(*q)
            break;
        for(q=p+1;*q;q++) if(*q=='n')newline=0;else escapes=1;
    }
    /* 簡易シェルは現在，引用符を文字として保持する． */
    echo_buffer_t out={0};
    char quote=0;
    for(int j=i;j<argc;j++) {
        const char *p=argv[j];size_t n=strlen(p),start=0;
        if(j>i && emit(&out,' ')<0)return 1;
        if(!quote && n && (p[0]=='\'' || p[0]=='"')) {quote=p[0];start=1;}
        if(quote && n>start && p[n-1]==quote) {n--;quote=0;}
        if(output(&out,p+start,n-start,escapes)<0)return 1;
    }
    if(newline && emit(&out,'\n')<0) return 1;
    return flush(&out)<0 ? 1 : 0;
}
