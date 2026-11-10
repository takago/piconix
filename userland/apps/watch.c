/* SPDX-License-Identifier: GPL-3.0-or-later */
/* コマンドを定期実行し，表示の変化を観察する． */

/* watchはコマンドの出力を記録し，変化した行だけ画面を書き換える． */
#include "unistd.h"
#include "fcntl.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"

/* 80×24の監視領域．上部2行と下部1行は案内表示用．TTYから端末サイズは取得できない． */
#define WATCH_ROWS 24
#define WATCH_COLS 80
#define WATCH_TOP 3
#define WATCH_FOOTER (WATCH_TOP+WATCH_ROWS)

typedef struct {
    char line[WATCH_ROWS][WATCH_COLS+1];
    unsigned char attr[WATCH_ROWS][WATCH_COLS]; /* bit 0: underline, bit 1: reverse */
    unsigned rows, row, col;
    unsigned char esc, cr, utf8, truncated, sgr;
    unsigned csi_arg;
    unsigned char csi_params[4], csi_count;
} watch_frame_t;
static watch_frame_t frames[2];

/* SGR 4/7のみ保持する．その他のANSI制御列は従来どおり無視する． */
static void watch_sgr(watch_frame_t *f, unsigned n) {
    if(n==0) f->sgr=0;
    else if(n==4) f->sgr|=1u;
    else if(n==7) f->sgr|=2u;
    else if(n==24) f->sgr&=(unsigned char)~1u;
    else if(n==27) f->sgr&=(unsigned char)~2u;
}

/* 子プロセスの出力1バイトを画面スナップショットへ反映する． */
static void capture_byte(watch_frame_t *f, unsigned char c) {
    if(f->esc==1) {
        f->esc=(c=='[')?2:0;
        f->csi_arg=0;f->csi_count=0;
        return;
    }
    if(f->esc==2) {
        if(c>='0' && c<='9') {
            if(f->csi_arg<1000) f->csi_arg=f->csi_arg*10u+(unsigned)(c-'0');
            return;
        }
        if(c==';') {
            if(f->csi_count<4) f->csi_params[f->csi_count++]=(unsigned char)f->csi_arg;
            f->csi_arg=0;return;
        }
        if(c=='m') {
            for(unsigned i=0;i<f->csi_count;i++) watch_sgr(f,f->csi_params[i]);
            watch_sgr(f,f->csi_arg);
        }
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
            if(f->row<WATCH_ROWS)
                memset(f->attr[f->row],0,WATCH_COLS);
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
        if(f->row<WATCH_ROWS) {
            f->line[f->row][f->col]=0;
            f->attr[f->row][f->col]=0;
        }
        return;
    }
    if(c=='\t') {
        unsigned count=4-(f->col&3u);
        while(count--)
            capture_byte(f,' ');
        return;
    }
    /* 長い行の切詰め時にUTF-8の途中で分断しない．上限は文字数でなくバイト数． */
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
    f->attr[f->row][f->col]=f->sgr;
    f->line[f->row][f->col++]=(char)c;
    f->line[f->row][f->col]=0;
    if(f->rows<=f->row)
        f->rows=f->row+1;
}

/* 対象コマンドの出力を再描画し，画面を更新する． */
static void redraw(const watch_frame_t *old, const watch_frame_t *next, int first) {
    for(unsigned i=0;i<WATCH_ROWS;i++) {
        if(!first && !strcmp(old->line[i],next->line[i]) &&
           !memcmp(old->attr[i],next->attr[i],WATCH_COLS))
            continue;
        /* まず上書きし，未使用の末尾だけ消す．全消去によるちらつきを防ぐ． */
        unsigned len=(unsigned)strlen(next->line[i]);
        unsigned has_attr=0;
        for(unsigned j=0;j<len;j++) has_attr|=next->attr[i][j];
        if(!has_attr) {
            printf("\033[0m\033[?7l\033[%u;1H%s\033[K\033[?7h", i+WATCH_TOP, next->line[i]);
        } else {
            printf("\033[?7l\033[%u;1H\033[0m",i+WATCH_TOP);
            unsigned pos=0, mode=0;
            while(pos<len) {
                unsigned next_mode=next->attr[i][pos];
                if(next_mode!=mode) {
                    printf("\033[0m");
                    if(next_mode&1u) printf("\033[4m");
                    if(next_mode&2u) printf("\033[7m");
                    mode=next_mode;
                }
                unsigned end=pos+1;
                while(end<len && next->attr[i][end]==mode) end++;
                char part[WATCH_COLS+1];
                memcpy(part,next->line[i]+pos,end-pos);
                part[end-pos]=0;
                printf("%s",part);
                pos=end;
            }
            printf("\033[0m\033[K\033[?7h");
        }
    }
    if(first || old->truncated!=next->truncated)
        printf("\033[0m\033[?7l\033[%u;1H%s\033[K\033[?7h", WATCH_FOOTER,
               next->truncated?"[watch: output truncated]  Ctrl+C: stop":"Ctrl+C: stop");
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
