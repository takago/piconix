/* SPDX-License-Identifier: GPL-3.0-or-later */
/* 画面セル・ANSI解析・PTYの画面スナップショット・キー名変換．
 * TTY入力キュー，VFS，USB送信，プロセス管理には依存しない． */
#include "screen.h"
#include <string.h>
#include <stdio.h>
#define NO_TRACE __attribute__((no_instrument_function))

/* CDC二画面の表示選択状態．TTYの切替に必要な最小情報だけ公開する． */
uint8_t cdc_active;
/* Dual mode displays both 80x24 screens vertically on a >=80x51 host terminal. */
uint8_t cdc_dual_mode;
static uint32_t cdc_shown_revision[2];
/* In-progress repaint suppresses interleaved foreground writes to the USB queue. */
static struct {
    uint8_t phase;           /* 再描画の進行段階． */
    uint8_t row;             /* 再描画中の行． */
    uint8_t col;             /* 再描画中の列． */
    uint8_t reverse;         /* 出力中の反転表示状態． */
    uint8_t pending;         /* 未送信の制御列があることを示す状態． */
    uint8_t seq_at;          /* 制御列の送信位置． */
    uint8_t panel;           /* 再描画対象の画面． */
    uint32_t revision;       /* 対象画面の更新世代． */
    uint32_t revision_other; /* もう一方の画面の更新世代． */
    char seq[80];            /* 送信途中の画面制御列． */
} cdc_repaint;

/* A switch first drops queued bytes from the previous console; repaint is
 * incremental so the scheduler is never blocked by USB backpressure. */
NO_TRACE void screen_cdc_begin_repaint(void) {
    cdc_repaint.phase=1;
    cdc_repaint.row=cdc_repaint.col=cdc_repaint.reverse=cdc_repaint.panel=0;
    cdc_repaint.pending=cdc_repaint.seq_at=0;
    cdc_repaint.revision=cdc_screen[cdc_active].revision;
    cdc_repaint.revision_other=cdc_screen[cdc_active^1u].revision;
}

/* Queue an ANSI control sequence; its cursor is retained across poll cycles. */
NO_TRACE static void cdc_repaint_seq(const char *s) {
    unsigned n=(unsigned)strlen(s);
    if(n>=sizeof cdc_repaint.seq) n=sizeof cdc_repaint.seq-1u;
    memcpy(cdc_repaint.seq,s,n);
    cdc_repaint.pending=(uint8_t)n;
    cdc_repaint.seq_at=0;
}

/* Generate at most 96 bytes per idle poll, without a 2-KiB scratch buffer. */
NO_TRACE static void screen_cdc_draw(screen_cdc_emit_t emit) {
    unsigned budget=96;
    vt_screen_t *screen=&cdc_screen[cdc_dual_mode ? cdc_repaint.panel : cdc_active];
    while(cdc_repaint.phase && budget) {
        if(cdc_repaint.seq_at<cdc_repaint.pending) {
            if(!emit((const uint8_t *)&cdc_repaint.seq[cdc_repaint.seq_at],1)) return;
            cdc_repaint.seq_at++;
            budget--;
            continue;
        }
        cdc_repaint.pending=cdc_repaint.seq_at=0;
        if(cdc_repaint.phase==1) {
            /* Redraw without changing the host's alternate-screen mode. */
            cdc_repaint_seq("\033[?25l\033[?7l\033[0m\033[2J\033[H");
            cdc_repaint.phase=cdc_dual_mode ? 7 : 2;
        } else if(cdc_repaint.phase==2) {
            if(cdc_repaint.row>=VT_ROWS) {cdc_repaint.phase=4;continue;}
            snprintf(cdc_repaint.seq,sizeof cdc_repaint.seq,
                     "\033[%u;1H",(unsigned)cdc_repaint.row+1u+
                     (cdc_dual_mode ? (cdc_repaint.panel ? 26u : 1u) : 0u));
            cdc_repaint.pending=(uint8_t)strlen(cdc_repaint.seq);
            cdc_repaint.phase=3;
        } else if(cdc_repaint.phase==3) {
            if(cdc_repaint.col>=VT_COLS) {
                cdc_repaint.col=0;
                cdc_repaint.row++;
                cdc_repaint.phase=2;
                continue;
            }
            unsigned row=cdc_repaint.row,col=cdc_repaint.col;
            unsigned inv=(screen->inverse[row][col>>3]>>(col&7u))&1u;
            if(inv!=cdc_repaint.reverse) {
                cdc_repaint_seq(inv?"\033[7m":"\033[0m");
                cdc_repaint.reverse=(uint8_t)inv;
                continue;
            }
            if(!emit((const uint8_t *)&screen->cells[row][col],1)) return;
            cdc_repaint.col++;
            budget--;
        } else if(cdc_repaint.phase==11) {
            /* Hide cursor before refreshing either panel. */
            cdc_repaint_seq("\033[?25l\033[0m");
            cdc_repaint.phase=7;
        } else if(cdc_repaint.phase==7) {
            /* Full-width reverse-video title bar; never touch the saved screen. */
            unsigned active=cdc_repaint.panel==cdc_active;
            snprintf(cdc_repaint.seq,sizeof cdc_repaint.seq,
                     "\033[%u;1H\033[7m%s[/dev/ttyV%u]%s",
                     cdc_repaint.panel ? 26u : 1u,
                     active ? "\033[1m" : "",
                     (unsigned)cdc_repaint.panel,active ? " ACTIVE" : "");
            cdc_repaint.pending=(uint8_t)strlen(cdc_repaint.seq);
            cdc_repaint.col=(uint8_t)(12u+(active ? 7u : 0u));
            cdc_repaint.phase=8;
        } else if(cdc_repaint.phase==8 || cdc_repaint.phase==9) {
            /* Fill the rest of the 80-column bar without another screen buffer. */
            if(cdc_repaint.col<VT_COLS) {
                const uint8_t space=' ';
                if(!emit(&space,1)) return;
                cdc_repaint.col++;
                budget--;
                continue;
            }
            cdc_repaint_seq("\033[0m");
            if(cdc_repaint.phase==8) {
                cdc_repaint.row=cdc_repaint.col=cdc_repaint.reverse=0;
                cdc_repaint.phase=2;
            } else cdc_repaint.phase=5;
        } else if(cdc_repaint.phase==4) {
            if(cdc_dual_mode && !cdc_repaint.panel) {
                cdc_repaint.panel=1;
                cdc_repaint.row=cdc_repaint.col=cdc_repaint.reverse=0;
                screen=&cdc_screen[1];
                cdc_repaint.phase=7;
                continue;
            }
            if(cdc_dual_mode) {
                cdc_repaint_seq("\033[51;1H\033[7mC-Left/Right: V0/V1/Dual  C-Up/Down: Focus (Dual)  C-T/G: Legacy");
                cdc_repaint.col=(uint8_t)(sizeof("C-Left/Right: V0/V1/Dual  C-Up/Down: Focus (Dual)  C-T/G: Legacy")-1u);
                cdc_repaint.phase=9;
            } else {
                cdc_repaint_seq("\033[0m\033[?7h");
                cdc_repaint.phase=5;
            }
        } else if(cdc_repaint.phase==5) {
            /* Keep the cursor hidden while further repaints are necessary. */
            screen=&cdc_screen[cdc_active];
            if(screen->revision!=cdc_repaint.revision ||
               (cdc_dual_mode && cdc_screen[cdc_active^1u].revision!=cdc_repaint.revision_other)) {
                cdc_repaint.phase=cdc_dual_mode ? 7 : 1;
                cdc_repaint.row=cdc_repaint.col=cdc_repaint.reverse=cdc_repaint.panel=0;
                cdc_repaint.revision=screen->revision;
                cdc_repaint.revision_other=cdc_screen[cdc_active^1u].revision;
                continue;
            }
            cdc_repaint.phase=10;
        } else if(cdc_repaint.phase==10) {
            screen=&cdc_screen[cdc_active];
            snprintf(cdc_repaint.seq,sizeof cdc_repaint.seq,
                     "\033[%u;%uH\033[?25%c%s",
                     (unsigned)screen->row+1u+(cdc_dual_mode ? (cdc_active ? 26u : 1u) : 0u),
                     (unsigned)screen->col+1u,
                     screen->cursor_hidden?'l':'h',
                     screen->reverse?"\033[7m":"");
            cdc_repaint.pending=(uint8_t)strlen(cdc_repaint.seq);
            cdc_repaint.phase=6;
        } else {
            /* Cursor restoration is the last queued operation in this redraw. */
            cdc_repaint.phase=0;
            if(cdc_dual_mode) {
                cdc_shown_revision[0]=cdc_screen[0].revision;
                cdc_shown_revision[1]=cdc_screen[1].revision;
            }
        }
    }
}

/* アイドル周期でバックグラウンド更新を検出し，非ブロッキングで描画する． */
NO_TRACE void screen_cdc_poll(screen_cdc_emit_t emit) {
    if(cdc_dual_mode && !cdc_repaint.phase &&
       (cdc_shown_revision[0]!=cdc_screen[0].revision ||
        cdc_shown_revision[1]!=cdc_screen[1].revision)) {
        /* Background screen changes need no full-screen erase. */
        cdc_repaint.phase=11;
        cdc_repaint.row=cdc_repaint.col=cdc_repaint.reverse=cdc_repaint.panel=0;
        cdc_repaint.pending=cdc_repaint.seq_at=0;
        cdc_repaint.revision=cdc_screen[cdc_active].revision;
        cdc_repaint.revision_other=cdc_screen[cdc_active^1u].revision;
    }
    if(cdc_repaint.phase) screen_cdc_draw(emit);
}
NO_TRACE int screen_cdc_repainting(void) { return cdc_repaint.phase!=0; }
NO_TRACE void screen_cdc_init(void) {
    cdc_active=cdc_dual_mode=0;
    screen_cdc_key_reset();
    memset(&cdc_repaint,0,sizeof cdc_repaint);
    memset(cdc_shown_revision,0,sizeof cdc_shown_revision);
}

/* CDCのCtrl+矢印などを識別するための短いESC列．端末入力キューには依存しない． */
static uint8_t cdc_key_bytes[6], cdc_key_len, cdc_key_tty;
static uint32_t cdc_key_time;

NO_TRACE static void cdc_key_result_reset(screen_cdc_key_result_t *result) {
    memset(result,0,sizeof *result);
}
NO_TRACE static void cdc_key_result_flush(screen_cdc_key_result_t *result) {
    result->flushed_count=cdc_key_len;
    result->flushed_tty=cdc_key_tty;
    if(cdc_key_len) memcpy(result->flushed,cdc_key_bytes,cdc_key_len);
    cdc_key_len=0;
}
/* 1バイトの入力から，キー操作・通常文字・保留ESC列のいずれかを生成する． */
NO_TRACE void screen_cdc_key_feed(uint8_t ch,uint32_t now,uint8_t tty,
                                 screen_cdc_key_result_t *result) {
    static const char prefix[]="\033[1;5";
    cdc_key_result_reset(result);
    if(cdc_key_len) {
        if(cdc_key_len<5u && ch==(uint8_t)prefix[cdc_key_len]) {
            cdc_key_bytes[cdc_key_len++]=ch;
            return;
        }
        if(cdc_key_len==5u && ch>='A' && ch<='D') {
            cdc_key_len=0;
            if(ch=='A') result->action=SCREEN_CDC_FOCUS_UP;
            else if(ch=='B') result->action=SCREEN_CDC_FOCUS_DOWN;
            else if(ch=='C') result->action=SCREEN_CDC_CYCLE_RIGHT;
            else result->action=SCREEN_CDC_CYCLE_LEFT;
            return;
        }
        cdc_key_result_flush(result);
    }
    if(ch==0x1b) {
        cdc_key_bytes[0]=ch;
        cdc_key_len=1;
        cdc_key_tty=tty;
        cdc_key_time=now;
    } else if(ch==0x14) result->action=SCREEN_CDC_SWITCH;
    else if(ch==0x07) result->action=SCREEN_CDC_DUAL_TOGGLE;
    else {
        result->pass=1;
        result->pass_tty=tty;
        result->pass_byte=ch;
    }
}
/* ESC列の受信が止まったら元のTTYへそのまま戻す．uint32_t時刻の周回にも対応． */
NO_TRACE int screen_cdc_key_timeout(uint32_t now,screen_cdc_key_result_t *result) {
    cdc_key_result_reset(result);
    if(!cdc_key_len || (uint32_t)(now-cdc_key_time)<=100000u) return 0;
    cdc_key_result_flush(result);
    return 1;
}
NO_TRACE void screen_cdc_key_reset(void) {
    cdc_key_len=cdc_key_tty=0;
    cdc_key_time=0;
}

vt_screen_t pty_screen[SCREEN_PTY_COUNT];
vt_screen_t cdc_screen[SCREEN_CDC_COUNT];

NO_TRACE void screen_reset(vt_screen_t *screen) {
    memset(screen,0,sizeof *screen);
}

/* 仮想画面の最小ANSI解釈．通常のPTY出力は加工せず，画面だけを更新する． */
NO_TRACE void screen_clear(vt_screen_t *screen) {
    memset(screen->cells,' ',sizeof screen->cells);
    memset(screen->inverse,0,sizeof screen->inverse);
}
/* 最下行を空けながら仮想画面を1行スクロールする． */
NO_TRACE static void vt_scroll(vt_screen_t *screen) {
    memmove(screen->cells[0],screen->cells[1],(VT_ROWS-1u)*VT_COLS);
    memmove(screen->inverse[0],screen->inverse[1],(VT_ROWS-1u)*sizeof screen->inverse[0]);
    memset(screen->cells[VT_ROWS-1u],' ',VT_COLS);
    memset(screen->inverse[VT_ROWS-1u],0,sizeof screen->inverse[0]);
}
/* 改行時に下端を超えた場合は1行スクロールする． */
NO_TRACE static void vt_lf(vt_screen_t *screen) {
    if(++screen->row>=VT_ROWS) { vt_scroll(screen); screen->row=VT_ROWS-1u; }
}
/* ANSIのパラメータが省略された場合に既定値を返す． */
NO_TRACE static unsigned vt_arg(vt_screen_t *screen,unsigned i,unsigned fallback) {
    return i<3u && screen->arg[i] ? screen->arg[i] : fallback;
}
/* TTYとは独立した画面エミュレータ．幅を超えた文字は改行まで捨てる． */
NO_TRACE int screen_byte(vt_screen_t *screen, int is_pty, uint8_t c) {
    int cursor_report=0;
    if(screen->esc==1) {
        screen->esc=0;
        if(c=='[') {
            screen->esc=2; screen->params=0; screen->private_csi=0;
            memset(screen->arg,0,sizeof screen->arg);
        } else if(c=='c') { screen_clear(screen); screen->row=screen->col=0; screen->clipped=0; screen->cursor_hidden=0; }
        return cursor_report;
    }
    if(screen->esc==2) {
        if(c=='?' && !screen->params && !screen->arg[0]) {screen->private_csi=1;return cursor_report;}
        if(c>='0' && c<='9') {
            unsigned i=screen->params;
            if(i<3 && screen->arg[i]<1000u) screen->arg[i]=(uint16_t)(screen->arg[i]*10u+c-'0');
            return cursor_report;
        }
        if(c==';') {if(screen->params<2) screen->params++;return cursor_report;}
        unsigned n=vt_arg(screen,0,1);
        if(c=='H'||c=='f') {
            unsigned r=n-1u,col=vt_arg(screen,1,1)-1u;
            screen->row=(uint8_t)(r<VT_ROWS?r:VT_ROWS-1u);
            screen->col=(uint8_t)(col<VT_COLS?col:VT_COLS-1u);
            screen->clipped=0;
        } else if(c=='A') { screen->clipped=0; screen->row=(uint8_t)(n>screen->row?0:screen->row-n); }
        else if(c=='B') { screen->clipped=0; screen->row=(uint8_t)(screen->row+n<VT_ROWS?screen->row+n:VT_ROWS-1u); }
        else if(c=='C') { screen->clipped=0; screen->col=(uint8_t)(screen->col+n<VT_COLS?screen->col+n:VT_COLS-1u); }
        else if(c=='D') { screen->clipped=0; screen->col=(uint8_t)(n>screen->col?0:screen->col-n); }
        else if(c=='G') { screen->clipped=0; screen->col=(uint8_t)(n-1u<VT_COLS?n-1u:VT_COLS-1u); }
        else if(c=='J' && (screen->arg[0]==2 || screen->arg[0]==3)) screen_clear(screen);
        else if(c=='K') {
            unsigned start=screen->arg[0]==1 ? 0 : screen->col;
            unsigned end=screen->arg[0]==1 ? (unsigned)screen->col+1u : VT_COLS;
            if(screen->arg[0]==2) {start=0;end=VT_COLS;}
            memset(screen->cells[screen->row]+start,' ',end-start);
            for(unsigned j=start;j<end;j++) screen->inverse[screen->row][j>>3]&=(uint8_t)~(1u<<(j&7u));
        } else if(c=='m') {
            for(unsigned i=0;i<=screen->params;i++) {
                if(screen->arg[i]==0 || screen->arg[i]==27) screen->reverse=0;
                if(screen->arg[i]==7) screen->reverse=1;
            }
        } else if(c=='n' && screen->arg[0]==6) cursor_report=1;
        else if(screen->private_csi && (c=='h'||c=='l') && screen->arg[0]==25) {
            screen->cursor_hidden=(c=='l');
        } else if(screen->private_csi && (c=='h'||c=='l') && screen->arg[0]==1049) {
            if(c=='h' || (c=='l' && !is_pty)) {
                screen_clear(screen);screen->row=screen->col=screen->clipped=0;
            }
        }
        screen->esc=0;
        screen->revision++;
        return cursor_report;
    }
    if(c==27) {screen->esc=1;return cursor_report;}
    if(c=='\r') screen->col=screen->clipped=0;
    else if(c=='\n') {screen->clipped=0;vt_lf(screen);}
    else if(c=='\b') {if(screen->clipped) screen->clipped=0;else if(screen->col) screen->col--;}
    else if(c=='\t' && !screen->clipped) {unsigned col=((unsigned)screen->col+8u)&~7u;screen->col=(uint8_t)(col<VT_COLS?col:VT_COLS-1u);if(col>=VT_COLS) screen->clipped=1;}
    else if(c>=32 && c<127 && !screen->clipped) {
        screen->cells[screen->row][screen->col]=(char)c;
        uint8_t *attr=&screen->inverse[screen->row][screen->col>>3];
        uint8_t bit=(uint8_t)(1u<<(screen->col&7u));
        if(screen->reverse) *attr|=bit; else *attr&=(uint8_t)~bit;
        if(screen->col==VT_COLS-1u) screen->clipped=1;
        else screen->col++;
    }
    screen->revision++;
    return cursor_report;
}
/* 80x24の反転表示を再生成し，可視カーソルだけをSGR 4/24で示す． */
/* カーソル属性は画面セルへ保存せず，FD位置は可変長ストリーム上のバイト位置とする． */
NO_TRACE int screen_read(const vt_screen_t *screen, uint32_t *position, void *p, size_t n) {
    unsigned pos=*position,at=0,out=0;
    char *dst=p;
    unsigned reverse=0;
#define VT_EMIT(ch) do { \
    if(at>=pos && out<n) dst[out++]=(char)(ch); \
    at++; \
} while(0)
    for(unsigned row=0;row<VT_ROWS && out<n;row++) {
        for(unsigned col=0;col<VT_COLS && out<n;col++) {
            unsigned inv=(screen->inverse[row][col>>3]>>(col&7u))&1u;
            if(inv!=reverse) {
                const char *seq=inv ? "\033[7m" : "\033[0m";
                for(unsigned i=0;seq[i];i++) VT_EMIT(seq[i]);
                reverse=inv;
            }
            if(!screen->cursor_hidden && row==screen->row && col==screen->col) {
                const char *seq="\033[4m";
                for(unsigned i=0;seq[i];i++) VT_EMIT(seq[i]);
            }
            VT_EMIT(screen->cells[row][col]);
            if(!screen->cursor_hidden && row==screen->row && col==screen->col) {
                const char *seq="\033[24m";
                for(unsigned i=0;seq[i];i++) VT_EMIT(seq[i]);
            }
        }
        if(out<n) VT_EMIT('\n');
    }
    if(reverse && out<n) {
        const char *seq="\033[0m";
        for(unsigned i=0;seq[i];i++) VT_EMIT(seq[i]);
    }
#undef VT_EMIT
    *position=pos+out;
    return (int)out;
}
/* .keyの1 writeは1命令．echoの末尾LFのみを命令終端として除く．
 * すべて検証してから呼出側に入力バイト列を返し，不正入力をPTYへ送らない． */
NO_TRACE int screen_key_decode(const void *p, size_t n, char out[128], size_t *length) {
    const uint8_t *src=p;
    size_t end=n, used=0;
    *length=0;
    if(!n) return 0;
    if(end && src[end-1]=='\n') --end;
    if(end && src[end-1]=='\r') --end;
    if(end>=4 && !memcmp(src,"key:",4)) {
        const char *key=(const char *)src+4;
        size_t size=end-4;
        const char *seq=0;
        char control[2]={0,0};
        if(size==2 && !memcmp(key,"UP",2)) seq="\033[A";
        else if(size==4 && !memcmp(key,"DOWN",4)) seq="\033[B";
        else if(size==5 && !memcmp(key,"RIGHT",5)) seq="\033[C";
        else if(size==4 && !memcmp(key,"LEFT",4)) seq="\033[D";
        else if(size==5 && !memcmp(key,"ENTER",5)) seq="\r";
        else if((size==3 && !memcmp(key,"ESC",3)) ||
                (size==6 && !memcmp(key,"ESCAPE",6))) seq="\033";
        else if(size==3 && !memcmp(key,"TAB",3)) seq="\t";
        else if((size==5 && !memcmp(key,"SPACE",5)) ||
                (size==2 && !memcmp(key,"SP",2))) seq=" ";
        else if((size==9 && !memcmp(key,"BACKSPACE",9)) ||
                (size==2 && !memcmp(key,"BS",2))) seq="\177";
        else if(size==6 && !memcmp(key,"DELETE",6)) seq="\033[3~";
        else if(size==6 && !memcmp(key,"CTRL_",5) &&
                key[5]>='A' && key[5]<='Z') {
            control[0]=(char)(key[5]-'A'+1); seq=control;
        } else if(size==1 && (unsigned char)key[0]>=32 &&
                (unsigned char)key[0]<=126) {
            out[0]=key[0]; *length=1; return (int)n;
        }
        if(!seq) return -1;
        used=strlen(seq);
        memcpy(out,seq,used);
    } else if(end>=5 && !memcmp(src,"text:",5)) {
        /* 最大127バイトの入力．未知のエスケープは一切送らない． */
        for(size_t i=5;i<end;i++) {
            uint8_t c=src[i];
            if(c=='\\') {
                if(++i==end) return -1;
                if(src[i]=='n') c='\n';
                else if(src[i]=='\\') c='\\';
                else return -1;
            }
            if(used>=127) return -1;
            out[used++]=(char)c;
        }
    } else return -1;
    *length=used;
    return (int)n;
}
