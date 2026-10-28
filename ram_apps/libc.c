/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Minimal PIC-safe user C library. No heap and no SDK dependencies. */
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "unistd.h"
#include <stdint.h>
#include <limits.h>

FILE __piconix_stdin = {0}, __piconix_stdout = {1}, __piconix_stderr = {2};

size_t strlen(const char *s) { const char *p=s; while (*p) ++p; return (size_t)(p-s); }
char *strcpy(char *d,const char *s) { char *r=d; while ((*d++=*s++)) {} return r; }
char *strncpy(char *d,const char *s,size_t n) { char *r=d; while(n && *s) { *d++=*s++; --n; } while(n--) *d++=0; return r; }
char *strcat(char *d,const char *s) { strcpy(d+strlen(d),s); return d; }
int strcmp(const char *a,const char *b) { while(*a && *a==*b) {++a;++b;} return (unsigned char)*a-(unsigned char)*b; }
int strncmp(const char *a,const char *b,size_t n) { while(n--) { unsigned char x=(unsigned char)*a++,y=(unsigned char)*b++; if(x!=y)return (int)x-(int)y; if(!x)return 0; } return 0; }
char *strchr(const char *s,int c) { for(;;++s) { if((unsigned char)*s==(unsigned char)c)return (char *)s; if(!*s)return 0; } }
char *strstr(const char *s,const char *sub) { if(!*sub)return (char *)s; for(;*s;++s) {const char *a=s,*b=sub;while(*a && *b && *a==*b){++a;++b;}if(!*b)return (char *)s;} return 0; }
/* Byte-at-a-time loops prevent accidental compiler-generated libc calls. */
void *memcpy(void *d,const void *s,size_t n) { unsigned char *a=d;const unsigned char *b=s;while(n--)*a++=*b++;return d; }
void *memmove(void *d,const void *s,size_t n) { unsigned char *a=d;const unsigned char *b=s;if(a<b){while(n--)*a++=*b++;}else if(a>b){a+=n;b+=n;while(n--)*--a=*--b;}return d; }
void *memset(void *d,int c,size_t n) { unsigned char *a=d;while(n--)*a++=(unsigned char)c;return d; }
int memcmp(const void *a,const void *b,size_t n) {const unsigned char *x=a,*y=b;while(n--){int diff=(int)*x++-(int)*y++;if(diff)return diff;}return 0;}
int abs(int x) { return x<0?-x:x; } /* As in ISO C, abs(INT_MIN) is undefined. */
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
int atoi(const char *s) {return (int)strtol(s,0,10);}

static int output(int fd,const char *s,size_t n) {
    size_t done=0;
    while(done<n){int r=write(fd,s+done,n-done);if(r<0)return -1;
        if(!r){msleep(1);continue;}done+=(unsigned)r;}
    return (int)done;
}
int putchar(int c){unsigned char ch=(unsigned char)c;return output(1,(const char *)&ch,1)<0?EOF:ch;}
int getchar(void){unsigned char c;return read(0,&c,1)==1?c:EOF;}
int puts(const char *s){size_t n=strlen(s);if(output(1,s,n)<0||output(1,"\n",1)<0)return EOF;return (int)(n+1);}
char *fgets(char *s,int size,FILE *stream){
    if(!s||!stream||size<=0)return 0;
    int n=0;while(n<size-1){char c;int r=read(stream->fd,&c,1);if(r!=1)break;s[n++]=c;if(c=='\n')break;}
    if(!n && size>1)return 0;s[n]=0;return s;
}

typedef struct {char *dst;size_t cap;size_t count;int fd;char out[48];unsigned used;int failed;} Format;
static void emit(Format *f,char c){
    if(f->fd<0){if(f->dst && f->cap && f->count<f->cap-1)f->dst[f->count]=c;}
    else {f->out[f->used++]=c;if(f->used==sizeof(f->out)){
        if(output(f->fd,f->out,f->used)<0)f->failed=1;f->used=0;}}
    ++f->count;
}
static void unsigned_value(Format *f,uint32_t n,unsigned base,int upper){
    char buf[32];unsigned i=0;
    /* Compiler runtime provides Cortex-M0+ integer division. */
    do {
        unsigned d=n%base;
        buf[i++]=(char)(d<10 ? '0'+d : (upper?'A':'a')+d-10);
        n/=base;
    } while(n);
    while(i)emit(f,buf[--i]);
}
static int format(Format *f,const char *fmt,va_list ap){
    while(*fmt){if(*fmt!='%'){emit(f,*fmt++);continue;}++fmt;
        if(*fmt=='%'){emit(f,'%');++fmt;continue;}
        /* Minimal subset: no width, precision, length modifiers or floats. */
        char spec=*fmt;if(!spec)break;++fmt;
        if(spec=='s'){const char *s=va_arg(ap,const char *);if(!s)s="(null)";while(*s)emit(f,*s++);}
        else if(spec=='c')emit(f,(char)va_arg(ap,int));
        else if(spec=='d'||spec=='i'){int v=va_arg(ap,int);uint32_t u=(uint32_t)v;
            if(v<0){emit(f,'-');u=0u-u;}unsigned_value(f,u,10,0);}
        else if(spec=='u')unsigned_value(f,va_arg(ap,unsigned),10,0);
        else if(spec=='x'||spec=='X')unsigned_value(f,va_arg(ap,unsigned),16,spec=='X');
        else {emit(f,'%');emit(f,spec);}
    }
    if(f->fd<0 && f->dst && f->cap)f->dst[f->count<f->cap?f->count:f->cap-1]=0;
    if(f->fd>=0 && f->used && output(f->fd,f->out,f->used)<0)f->failed=1;
    return f->failed?-1:(int)f->count;
}
int printf(const char *fmt,...){Format f={0};f.fd=1;va_list a;va_start(a,fmt);int r=format(&f,fmt,a);va_end(a);return r;}
int snprintf(char *s,size_t n,const char *fmt,...){Format f={0};f.dst=s;f.cap=n;f.fd=-1;va_list a;va_start(a,fmt);int r=format(&f,fmt,a);va_end(a);return r;}
int sprintf(char *s,const char *fmt,...){Format f={0};f.dst=s;f.cap=(size_t)-1;f.fd=-1;va_list a;va_start(a,fmt);int r=format(&f,fmt,a);va_end(a);return r;}

/* ARM EABI zero-fill routines emitted by some compilers for local structs. */
void __aeabi_memclr(void *dst, size_t n) {
    volatile unsigned char *p = (volatile unsigned char *)dst;
    while (n--) *p++ = 0;
}
void __aeabi_memclr4(void *dst, size_t n) { __aeabi_memclr(dst, n); }
void __aeabi_memclr8(void *dst, size_t n) { __aeabi_memclr(dst, n); }
