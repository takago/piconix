/* SPDX-License-Identifier: GPL-3.0-or-later */
/* ファイルまたは標準入力を表示する． */

/* ファイルまたは標準入力を表示し，行番号付けにも対応するcat． */
#include "unistd.h"
#include "fcntl.h"
#include "stdio.h"
#include "string.h"

/* 標準出力への書込みを繰り返し，部分書込みと一時的な待機を処理する． */
static int output(const char *buf, int n) {
    int done=0;
    while(done<n) {
        int k=write(1,buf+done,(unsigned)(n-done));
        if(k==OS_EAGAIN || k==0) { msleep(1); continue; }
        if(k<0)
            return -1;
        done+=k;
    }
    return 0;
}

/* 32バイト単位のreadをまたいでも，空行を含む行頭位置を保持する． */
static int numbered_output(const char *buf, int n, unsigned *line, int *at_start) {
    for(int i=0;i<n;) {
        if(*at_start) {
            char label[20];
            int len=snprintf(label,sizeof label,"%6u\t",(*line)++);
            if(output(label,len)<0)
                return -1;
            *at_start=0;
        }
        int end=i;
        while(end<n && buf[end++]!='\n') {}
        if(output(buf+i,end-i)<0)
            return -1;
        *at_start=buf[end-1]=='\n';
        i=end;
    }
    return 0;
}

/* 読み取ったデータ片を表示する．-n指定時は複数のreadをまたぐ行番号状態を維持する． */
static int display(const char *buf, int n, int numbered,
                   unsigned *line, int *at_start) {
    return numbered ? numbered_output(buf,n,line,at_start) : output(buf,n);
}

/* 指定ファイルまたは標準入力の内容を順番に表示する． */
int main(int argc, char *argv[]) {
    int numbered=argc>1 && !strcmp(argv[1],"-n");
    if(argc>2+numbered) {
        puts("usage: cat [-n] [PATH]");
        return 1;
    }
    const char *path=argc>1+numbered ? argv[1+numbered] : 0;
    if(path && (!strcmp(path,"/dev/console") || !strcmp(path,"/dev/ttyV0") || !strcmp(path,"/dev/ttyV1"))) {
        puts("cat: console reserved for shell");
        return 1;
    }
    int uart=path && !strcmp(path,"/dev/ttyS1");
    int fd=path ? open(path,uart?O_NONBLOCK:O_RDONLY) : 0;
    if(fd<0) {
        puts("cat: cannot open");
        return 1;
    }

    char buf[32];
    int n=0, rc=0, at_start=1;
    unsigned line=1;
    if(uart) {
        /* UART入力にはEOFがないので，入力が200 ms途絶えたら終了する． */
        unsigned idle=0;
        while(idle<200) {
            n=read(fd,buf,sizeof buf);
            if(n<0) { rc=1; break; }
            if(n>0) {
                if(display(buf,n,numbered,&line,&at_start)<0) { rc=1; break; }
                idle=0;
            } else {
                msleep(10);
                idle+=10;
            }
        }
    } else {
        while((n=read(fd,buf,sizeof buf))>0) {
            if(display(buf,n,numbered,&line,&at_start)<0) { rc=1; break; }
        }
        if(n<0)
            rc=1;
    }
    if(path)
        close(fd);
    if(rc)
        puts("cat: read/write error");
    return rc;
}
