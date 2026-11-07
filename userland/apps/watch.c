/* SPDX-License-Identifier: GPL-3.0-or-later */
/* コマンドを定期実行し，表示の変化を観察する． */

/* watchはコマンドの出力を記録し，変化した行だけ画面を書き換える． */
#include "unistd.h"
#include "fcntl.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"

/* 24 KiBスロット内に収まるよう二つの画面スナップショットを小さくする．24行端末の見出しと省略表示分も確保する．TTYから端末サイズは取得できない． */
#define WATCH_ROWS 20
#define WATCH_COLS 78

typedef struct {
    char line[WATCH_ROWS][WATCH_COLS+1];
    unsigned rows, row, col;
    unsigned char esc, cr, utf8, truncated;
} watch_frame_t;
static watch_frame_t frames[2];

/* 子プロセスの出力1バイトを画面スナップショットへ反映する．ANSI制御列は読み飛ばす． */
static void capture_byte(watch_frame_t *f, unsigned char c) {
    /* 出力中の端末制御列は無視する．全画面プログラムではなく行指向コマンド用である． */
    if(f->esc) {
        if(f->esc==1 && c=='[') {f->esc=2;return;}
        if(f->esc==2 && c>=0x20 && c<0x40)
            return;
        f->esc=0;
        return;
    }
    if(c==27) {f->esc=1;return;}
    if(f->cr) {
        f->cr=0;
        if(c!='\n') {
            f->col=0;
            if(f->row<WATCH_ROWS)
                f->line[f->row][0]=0;
        }
    }
    if(c=='\r') {f->cr=1;f->utf8=0;return;}
    if(c=='\n') {
        f->utf8=0;
        if(f->row>=WATCH_ROWS)
            f->truncated=1;
        else {
            if(f->rows<=f->row)
                f->rows=f->row+1;
            f->row++;
        }
        f->col=0;
        return;
    }
    if(c=='\b') {
        if(f->col)
            f->col--;
        if(f->row<WATCH_ROWS)
            f->line[f->row][f->col]=0;
        return;
    }
    if(c=='\t') {
        unsigned count=4-(f->col&3u);
        while(count--)
            capture_byte(f,' ');
        return;
    }
    /* 長い行の切詰め時にUTF-8の途中で分断しない．上限は文字数でなくバイト数であり，80桁より小さく設定する． */
    if(c>=128) {
        if((c&0xc0)==0x80) {
            if(!f->utf8)
                return;
            f->utf8--;
        } else {
            unsigned tail=(c>=0xc2 && c<=0xdf)?1u:
                          (c>=0xe0 && c<=0xef)?2u:
                          (c>=0xf0 && c<=0xf4)?3u:0u;
            if(!tail)
                return;
            f->utf8=(unsigned char)tail;
            if(f->row>=WATCH_ROWS || f->col+tail>=WATCH_COLS) {
                f->utf8=0;f->truncated=1;return;
            }
        }
    } else {
        f->utf8=0;
        if(c<32)
            return;
    }
    if(f->row>=WATCH_ROWS || f->col>=WATCH_COLS) {f->truncated=1;return;}
    f->line[f->row][f->col++]=(char)c;
    f->line[f->row][f->col]=0;
    if(f->rows<=f->row)
        f->rows=f->row+1;
}

/* 対象コマンドの出力を再描画し，画面を更新する． */
static void redraw(const watch_frame_t *old, const watch_frame_t *next, int first) {
    for(unsigned i=0;i<WATCH_ROWS;i++) {
        if(!first && !strcmp(old->line[i],next->line[i]))
            continue;
        /* まず上書きし，未使用の末尾だけ消す．全消去によるちらつきを防ぐ． */
        printf("\033[?7l\033[%u;1H%s\033[K\033[?7h", i+3, next->line[i]);
    }
    if(first || old->truncated!=next->truncated)
        printf("\033[?7l\033[23;1H%s\033[K\033[?7h",next->truncated?"[watch: output truncated]":"");
}

/* 256バイトのカーネルパイプは子が動いている間に排出する．先にwaitpidすると子が満杯のパイプで停止しデッドロックする． */
static int capture_command(int id, int count, const char *const args[], watch_frame_t *f) {
    int pair[2]={-1,-1}, saved_out=-1, saved_err=-1;
    int child=-1, error=0;
    if(pipe(pair)<0)
        return -1;
    saved_out=dup(1);
    saved_err=dup(2);
    if(saved_out<0 || saved_err<0) {error=1;goto restore;}
    if(dup2(pair[1],1)<0 || dup2(pair[1],2)<0) {error=1;goto restore;}
    child=spawn(id,count,args);
restore:
    if(saved_out>=0) {if(dup2(saved_out,1)<0) error=1;close(saved_out);}
    if(saved_err>=0) {if(dup2(saved_err,2)<0) error=1;close(saved_err);}
    close(pair[1]);
    if(error || child<0) {close(pair[0]);return -1;}
    char buf[64];
    for(;;) {
        int n=read(pair[0],buf,sizeof buf);
        if(n==0)
            break;
        if(n==OS_EAGAIN) {msleep(1);continue;}
        if(n<0) {error=1;break;}
        for(int j=0;j<n;j++)
            capture_byte(f,(unsigned char)buf[j]);
    }
    close(pair[0]);
    if(waitpid(child,0,0)<0)
        error=1;
    return error?-1:0;
}

/* 指定コマンドを一定間隔で再実行し，結果を端末へ表示する． */
int main(int argc, char *argv[]) {
    unsigned interval=1;
    int first=1;
    if(argc>1 && argv[1][0]>='0' && argv[1][0]<='9') {
        char *end;
        long n=strtol(argv[1],&end,10);
        if(*end || n<1 || n>60) {puts("watch: interval must be 1..60 seconds");return 1;}
        interval=(unsigned)n;
        first=2;
    }
    if(argc<=first) {puts("usage: watch [seconds] command [args...]");return 1;}
    const char *name=argv[first];
    if(!strncmp(name,"/bin/",5))
        name+=5;
    int id=program_id(name);
    if(id<0 || !strcmp(name,"watch")) {puts("watch: unknown or recursive command");return 1;}
    const char *args[8]; int count=argc-first;
    if(count>8) {puts("watch: too many arguments");return 1;}
    for(int i=0;i<count;i++)
        args[i]=argv[first+i];
    args[0]=name;

    char header[WATCH_COLS+1];
    snprintf(header,sizeof header,"Every %us:",interval);
    for(int i=0;i<count;i++) {
        unsigned len=(unsigned)strlen(header);
        if(len>=WATCH_COLS)
            break;
        snprintf(header+len,sizeof header-len," %s",args[i]);
    }
    int old=0, next=1, first_frame=1;
    for(;;) {
        memset(&frames[next],0,sizeof frames[next]);
        if(capture_command(id,count,args,&frames[next])<0) {
            puts("watch: cannot capture command");return 1;
        }
        if(first_frame) printf("\033[?7l\033[H\033[2J%s\033[?7h",header);
        redraw(&frames[old],&frames[next],first_frame);
        /* ANSIのカーソル移動には改行がない．差分が少なくても直ちに端末へ反映する． */
        if(fflush(stdout)<0) return 1;
        first_frame=0;
        int tmp=old;old=next;next=tmp;
        msleep(interval*1000u);
    }
}
