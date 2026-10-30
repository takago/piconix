/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "unistd.h"
#include "string.h"

static int emit(char c) {
    for (;;) {
        int n=write(1,&c,1);
        if(n<0) return -1;
        if(n) return 0;
        msleep(1);
    }
}
static int digit(char c,int base) {
    int d=(c>='0'&&c<='9')?c-'0':(c>='a'&&c<='f')?c-'a'+10:
          (c>='A'&&c<='F')?c-'A'+10:-1;
    return d>=0&&d<base?d:-1;
}
static int output(const char *s,size_t n,int escapes) {
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
                if(k) c=(char)v;
                else {if(emit('\\')<0)return -1;c='x';}
            } else if(e>='0'&&e<='7') {
                int v=e-'0',k=1,d;
                while(k<3 && i+1<n && (d=digit(s[i+1],8))>=0) {
                    v=v*8+d;i++;k++;
                }
                c=(char)v;
            } else {if(emit('\\')<0)return -1;c=e;}
        }
        if(emit(c)<0)return -1;
    }
    return 0;
}
int main(int argc,char *argv[]) {
    int newline=1,escapes=0,i=1;
    /* Accept -n, -e, -ne, -en; -- ends option parsing. */
    for(;i<argc;i++) {
        const char *p=argv[i];
        if(!strcmp(p,"--")) {i++;break;}
        if(*p!='-' || !p[1]) break;
        const char *q=p+1;
        for(;*q && (*q=='n'||*q=='e');q++);
        if(*q) break;
        for(q=p+1;*q;q++) if(*q=='n')newline=0;else escapes=1;
    }
    /* The tiny PicoNIX shell currently keeps quote marks verbatim. */
    char quote=0;
    for(int j=i;j<argc;j++) {
        const char *p=argv[j];size_t n=strlen(p),start=0;
        if(j>i && emit(' ')<0)return 1;
        if(!quote && n && (p[0]=='\'' || p[0]=='"')) {quote=p[0];start=1;}
        if(quote && n>start && p[n-1]==quote) {n--;quote=0;}
        if(output(p+start,n-start,escapes)<0)return 1;
    }
    return newline ? emit('\n')<0 : 0;
}
