/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Omikuji: a small example of arguments, standard input, and loops. */
#include <stdint.h>
#include "unistd.h"
#include "fcntl.h"
#include "stdio.h"
#include "stdlib.h"

static unsigned random_from_device(void) {
    uint32_t value=0;
    int fd=open("/dev/rand",O_RDONLY);
    if(fd<0)return 0;
    int n=read(fd,&value,sizeof(value));
    close(fd);
    return n==(int)sizeof(value)?value:0;
}

static void fortune(int seconds) {
    /* Fixed-width strings avoid a pointer table in the PIC image. */
    static const char fortunes[][9] = {
        "DAIKICHI", "CHUKICHI", "SHOKICHI", "KICHI", "SUEKICHI", "KYO"
    };
    for(int n=seconds;n>=1;n--) {
        printf("%d...",n);
        fflush(stdout); /* Show countdown before sleeping. */
        msleep(1000);
    }
    printf(" (*^_^*) %s!\n",fortunes[random_from_device()%6u]);
}

int main(int argc,char *argv[]) {
    if(argc>2) {puts("usage: omikuji [-i | seconds]");return 1;}
    if(argc==2 && argv[1][0]=='-' && argv[1][1]=='i' && !argv[1][2]) {
        for(;;) {
            int seconds;
            printf("Seconds (1-30): ");
            int result=scanf("%d",&seconds);
            if(result==EOF)break;
            if(result==1 && seconds>=1 && seconds<=30) {
                fortune(seconds);
                continue;
            }
            /* Invalid tokens cannot be scanned again without consuming them. */
            int c;
            do {c=getchar();} while(c!=EOF && c!=' ' && c!='\t' && c!='\n' && c!='\r');
            if(c==EOF)break;
        }
        return 0;
    }
    int seconds=10;
    if(argc==2) {
        char *end;
        long value=strtol(argv[1],&end,10);
        if(end==argv[1] || *end || value<1 || value>30) {
            puts("omikuji: seconds must be 1..30");
            return 1;
        }
        seconds=(int)value;
    }
    fortune(seconds);
    return 0;
}
