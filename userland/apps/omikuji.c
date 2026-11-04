/* SPDX-License-Identifier: GPL-3.0-or-later */
/* 引数・標準入力・ループの扱いを学ぶ小プログラム． */

/* 引数・標準入力・反復処理の練習用おみくじ． */
#include <stdint.h>
#include "unistd.h"
#include "fcntl.h"
#include "stdio.h"
#include "stdlib.h"

/* /dev/randから32ビットの値を読む．読出しに失敗した場合は0を返す． */
static unsigned random_from_device(void) {
    uint32_t value=0;
    int fd=open("/dev/rand",O_RDONLY);
    if(fd<0)return 0;
    int n=read(fd,&value,sizeof(value));
    close(fd);
    return n==(int)sizeof(value)?value:0;
}

/* 乱数値をおみくじの結果文字列へ対応付ける． */
static void fortune(int seconds) {
    /* PICイメージ内のポインタ表を避けるため，固定幅文字列を使う． */
    static const char fortunes[][9] = {
        "DAIKICHI", "CHUKICHI", "SHOKICHI", "KICHI", "SUEKICHI", "KYO"
    };
    for(int n=seconds;n>=1;n--) {
        printf("%d...",n);
        fflush(stdout); /* スリープ前にカウントダウンを表示する． */
        msleep(1000);
    }
    printf(" (*^_^*) %s!\n",fortunes[random_from_device()%6u]);
}

/* 擬似乱数で運勢を選び，結果を表示する． */
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
            /* 不正な入力は読み捨てなければ同じトークンを再解析してしまう． */
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
