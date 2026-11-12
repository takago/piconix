/* SPDX-License-Identifier: GPL-3.0-or-later */
/* 小さな対話シェル．コマンド行の編集，履歴，リダイレクト，
 * 最大3段のパイプ，前景・背景実行と起動時rcの実行を扱う． */

/* PicoNIXのシェルは独立したPICイメージとして0x2000E000にリンクする．SDK・カーネルRAMへ直接依存せず，SVC ABIではr0が番号，r1〜r3が引数である． */
#include <stdint.h>
#include <stddef.h>

#include "unistd.h"
#include "fcntl.h"
#include "string.h"
#include "stdio.h"
/* 非対話実行ではTTYのフォアグラウンド所有者を変更しない． */
static int batch_mode;
/* sh -cが最後に実行したコマンドの終了結果． */
static int command_status;

/* 二つの文字列を完全一致で比較する． */
static int eq(const char *a,const char *b) { return strcmp(a,b)==0; }

/* 指定した接頭辞で文字列が始まるか判定する． */
static int prefix(const char *a,const char *b) { return strncmp(a,b,strlen(b))==0; }

/* カーネルのTXキューが満杯なら書込みが途中で終わる．アイドルへ処理を譲り，未送信部分を破棄せず再試行する． */
static void write_all(int fd, const char *p, size_t n) {
    while(n) {
        int rc=write(fd, p, n);
        if(rc<0)
            return;
        if(rc==0) { msleep(1); continue; }
        p+=(unsigned)rc; n-=(unsigned)rc;
    }
}

/* 標準出力へNUL終端文字列を表示する． */
static void put(const char *s) {
    char b[48];
    while(*s) {
        size_t n=0;
        while(n<sizeof b && *s)
            b[n++]=*s++;
        write_all(1,b,n);
    }
}

/* 作業ディレクトリ基準でパスを解決する．`.`，`..`，連続スラッシュを整理する． */
static int resolve(const char *cwd, const char *input, char out[128]) {
    char combined[256];
    unsigned n=0;
    if(*input!='/') {
        for(const char *q=cwd;*q;q++) { if(n>=sizeof(combined)-2) return -1; combined[n++]=*q; }
        if(n==0 || combined[n-1]!='/')
            combined[n++]='/';
    }
    for(const char *q=input;*q;q++) { if(n>=sizeof(combined)-1) return -1; combined[n++]=*q; }
    combined[n]=0;
    unsigned o=1; out[0]='/'; out[1]=0;
    for(unsigned i=0;i<n;) {
        while(combined[i]=='/')
            i++;
        if(!combined[i])
            break;
        unsigned start=i;
        while(combined[i] && combined[i]!='/')
            i++;
        unsigned len=i-start;
        if(len==1 && combined[start]=='.')
            continue;
        if(len==2 && combined[start]=='.' && combined[start+1]=='.') {
            if(o>1) { while(o>1 && out[o-1]!='/') o--; if(o>1) o--; }
            out[o]=0;
            continue;
        }
        if(o>1) { if(o>=127) return -1; out[o++]='/'; }
        if(o+len>=128)
            return -1;
        for(unsigned k=0;k<len;k++)
            out[o++]=combined[start+k];
        out[o]=0;
    }
    return 0;
}

/* リダイレクト先の名前に空白を含めず，周囲の空白を構文として扱う． */
static char *trim(char *s) {
    while(*s==' ' || *s=='\t')
        s++;
    char *end=s+strlen(s);
    while(end>s && (end[-1]==' ' || end[-1]=='\t'))
        *--end=0;
    return s;
}

/* 一時的に変更した標準FDを復元し，退避用FDを閉じる． */
static void restore_fd(int target, int saved) {
    if(saved>=0) {
        dup2(saved,target);
        close(saved);
    }
}

/* シェルで一時変更した標準入出力FDを元の設定へ戻す． */
static void restore_stdio(int saved_in, int saved_out) {
    restore_fd(0,saved_in);
    restore_fd(1,saved_out);
}

/* ファイルまたはデバイスへ標準FDを付け替える．戻すために元のFDを退避し，組込みコマンドにも同じ処理を使う． */
static int redirect(char *s, const char *cwd, int *save_in, int *save_out) {
    char *first=0;
    for(char *q=s;*q;q++) if(*q=='<' || *q=='>') {
        if(!first)
            first=q;
        if(*q=='>' && q[1]=='>')
            q++;
    }
    if(!first)
        return 0;
    /* リダイレクト先は引用符なしの一語とし，二箇所まで認める． */
    char *q=first;
    int seen=0;
    while(*q) {
        if(*q!='<' && *q!='>')
            return -1;
        int output=*q=='>';
        q++;
        int append=output && *q=='>';
        if(append)
            q++;
        while(*q==' ' || *q=='\t')
            q++;
        char *name=q;
        while(*q && *q!=' ' && *q!='\t' && *q!='<' && *q!='>')
            q++;
        if(name==q)
            return -1;
        char terminator=*q;
        *q=0;
        char path[128];
        int valid=resolve(cwd,name,path);
        *q=terminator;
        if(valid<0)
            return -1;
        /* デバイスノードは通常ファイルではないため，作成・切り詰めを要求しない． */
        int flag=output?(1|(prefix(path,"/dev/")?0:O_CREAT|(append?O_APPEND:O_TRUNC))):0;
        int target=output?1:0, saved=-1;
        if(seen & (1<<target))
            return -1;
        seen|=1<<target;
        int fd=open(path, flag);
        if(fd<0)
            return -1;
        saved=dup(target);
        if(saved<0 || dup2(fd, target)<0) {
            if(saved>=0)
                close(saved);
            close(fd); return -1;
        }
        close(fd);
        if(output) *save_out=saved; else *save_in=saved;
        while(*q==' ' || *q=='\t')
            q++;
        if(*q && *q!='<' && *q!='>')
            return -1;
    }
    *first=0;
    return 0;
}

/* 登録済み外部コマンドをIDで起動する．子へ引数と標準入出力FDを継承させる． */
static int run_external(char *cmd,const char *cwd,int background,int *bg_pid,const char **bg_name) {
    char *args[8];int argc=0;
    char *q=cmd;
    while(*q) {
        while(*q==' ' || *q=='\t')
            q++;
        if(!*q)
            break;
        if(argc==8) {put("too many arguments (max 8)\r\n");command_status=1;return 1;}
        args[argc++]=q;
        while(*q && *q!=' ' && *q!='\t')
            q++;
        if(*q)
            *q++=0;
    }
    if(!argc)
        return 1;
    const char *name=args[0];
    if(prefix(name,"/bin/"))
        name+=5;
    if(prefix(name,"./") || prefix(name,"/tmp/")) {
        char path[128];
        if(argc!=1 || resolve(cwd,name,path)<0) { put("invalid executable path\r\n"); command_status=1; return 1; }
        int pid=(background || batch_mode)?spawn_tmp(path):spawn_tmp_foreground(path);
        if(pid<0) { put("cannot execute image\r\n"); command_status=1; return 1; }
        if(background) {*bg_pid=pid;*bg_name="a.out";}
        else { int status=0; command_status=waitpid(pid,&status,0)<0?1:status; if(!batch_mode) set_foreground(0); }
        return 1;
    }
    int id=program_id(name);
    if(id<0)
        return 0;
    args[0]=(char *)name;
    /* 引数は変えずに渡し，各コマンドまたはVFSがそのパスを解釈する． */
    unsigned total=0;
    for(int i=0;i<argc;i++)
        total+=(unsigned)strlen(args[i])+1;
    if(total>128) {put("arguments too long (max 128 bytes)\r\n");command_status=1;return 1;}
    int pid=(background || batch_mode)?spawn(id,argc,(const char *const *)args):spawn_foreground(id,argc,(const char *const *)args);
    if(pid<0) {put("cannot start program\r\n");command_status=1;return 1;}
    if(background) {*bg_pid=pid;*bg_name=name;}
    else {
        int status=0;
        command_status=waitpid(pid,&status,0)<0?1:status;
        if(!batch_mode) set_foreground(0);
        /* 全画面アプリは終了処理前にkillされ得るため，画面復旧が必要となる． */
        if(eq(name,"mb") || eq(name,"gpio") || eq(name,"pong") || eq(name,"reversi") ||
           eq(name,"fbed") || eq(name,"edit") || eq(name,"mem") || eq(name,"fd"))
            put("\033[?1049l");
    }
    return 1;
}

/* パイプの一段を起動し，標準FDの0〜2だけを継承して入出力と前景状態を設定する． */
static int pipeline_spawn(char *cmd, const char *cwd) {
    char *args[8]; int argc=0;
    for(char *q=cmd;*q;) {
        while(*q==' ' || *q=='\t')
            q++;
        if(!*q)
            break;
        if(argc==8)
            return -1;
        args[argc++]=q;
        while(*q && *q!=' ' && *q!='\t')
            q++;
        if(*q)
            *q++=0;
    }
    if(!argc)
        return -1;
    const char *name=args[0];
    if(prefix(name,"/bin/"))
        name+=5;
    if(prefix(name,"./") || prefix(name,"/tmp/")) {
        char path[128];
        if(argc!=1 || resolve(cwd,name,path)<0)
            return -1;
        return spawn_tmp(path);
    }
    int id=program_id(name);
    if(id<0)
        return -1;
    args[0]=(char *)name;
    /* パイプの各段へ通常コマンドと同様の引数を渡す． */
    unsigned total=0;
    for(int i=0;i<argc;i++)
        total+=(unsigned)strlen(args[i])+1;
    if(total>128)
        return -1;
    return spawn(id,argc,(const char *const *)args);
}

/* 最大3段のパイプを組み立てる．全段を起動してから待機し，未使用端を閉じてEOFを伝える． */
static void run_pipeline(char *stage[3], int count, const char *cwd) {
    int saved_in=-1, saved_out=-1, previous_read=-1;
    int pid[3]={-1,-1,-1}, failed=0;
    saved_in=dup(0);
    if(saved_in<0) {failed=1;goto cleanup;}
    saved_out=dup(1);
    if(saved_out<0) {failed=1;goto cleanup;}
    /* 段の起動に合わせて次のパイプを作り，不要な端点は直ちにcloseして最後の書き手の消滅をEOFとして伝える． */
    for(int i=0;i<count;i++) {
        int next[2]={-1,-1};
        if(i<count-1 && pipe(next)<0) {failed=1;break;}
        int input=i ? previous_read : saved_in;
        int output=i<count-1 ? next[1] : saved_out;
        if(dup2(input,0)<0 || dup2(output,1)<0) {
            if(next[0]>=0)
                close(next[0]);
            if(next[1]>=0)
                close(next[1]);
            failed=1;break;
        }
        pid[i]=pipeline_spawn(stage[i],cwd);
        if(next[1]>=0)
            close(next[1]);
        if(pid[i]<0) {
            if(next[0]>=0)
                close(next[0]);
            failed=1;break;
        }
        /* 前景パイプの各PIDを登録し，Ctrl+Cで一括終了できるようにする． */
        if(!batch_mode) set_foreground(pid[i]);
        if(previous_read>=0)
            close(previous_read);
        previous_read=next[0];
    }
cleanup:
    restore_stdio(saved_in,saved_out);
    if(previous_read>=0)
        close(previous_read);
    if(failed) put("pipe: cannot start command\r\n");
    int status=0;
    command_status=failed?1:0;
    for(int i=0;i<count;i++)
        if(pid[i]>0) {
            int rc=waitpid(pid[i],&status,0);
            if(i==count-1 && !failed) command_status=rc<0?1:status;
        }
    if(!batch_mode) set_foreground(0);
}

/* 入力行を解釈し，組込みコマンド／外部コマンドへ振り分ける． */
static void command(char *s, char cwd[128], int requested_bg, int *bg_pid, const char **bg_name) {
    while(*s==' ')
        s++;
    if(!*s)
        return;
    command_status=0;
    if(eq(s,"logout") || eq(s,"exit")) {
        put("logout\r\nPicoNIX: Session closed.\r\n");
        _exit(0);
        for(;;)
            msleep(100);
    }
    else if(eq(s,"cd") || prefix(s,"cd ")) {
        const char *arg=eq(s,"cd")?"/":s+3;
        while(*arg==' ')
            arg++;
        if(!*arg)
            arg="/";
        char path[128];
        if(resolve(cwd,arg,path)<0 || chdir(path)<0) { put("cd: no such directory\r\n"); command_status=1; return; }
        strcpy(cwd,path);
    } else if(prefix(s,"getty ")) {
        const char *arg=s+6;
        int tty=0;
        if(prefix(arg,"/dev/ttyS1 ")) {tty=2;arg+=11;}
        else if(prefix(arg,"/dev/ttyV0 ")) {tty=1;arg+=11;}
        if(!tty || !(eq(arg,"sh") || eq(arg,"python"))) {
            command_status=1;
            put("usage: getty /dev/ttyS1 sh|python [&]\r\n");return;
        }
        int pid=tty_spawn(tty, eq(arg,"sh")?2:4);
        if(pid<0) {put("getty: terminal busy or no free slot\r\n");command_status=1;return;}
        if(requested_bg) {*bg_pid=pid;*bg_name=eq(arg,"sh")?"sh":"python";}
        else {int status=0;command_status=waitpid(pid, &status, 0)<0?1:status;}
    } else if(!run_external(s,cwd,requested_bg,bg_pid,bg_name)) {
        put("unknown command (help)\r\n");
        command_status=127;
    }
}
/* シェルごとの小さな履歴を固定長で管理する．ヒープもカーネル変更も不要である． */
#define HISTORY_SIZE 8
#define SHELL_PROMPT "piconix# "
static char history[HISTORY_SIZE][128];
static unsigned history_count, history_next;

/* 固定長のコマンド履歴へ追加する．ヒープは使わず，古い項目を順に入れ替える． */
static void history_add(const char *line) {
    if(!*line)
        return;
    if(history_count && eq(history[(history_next+HISTORY_SIZE-1)%HISTORY_SIZE],line))
        return;
    strcpy(history[history_next],line);
    history_next=(history_next+1)%HISTORY_SIZE;
    if(history_count<HISTORY_SIZE)
        history_count++;
}

/* 現在位置からn桁だけ左へ移動する． */
static void shell_move_left(unsigned n) {
    if(!n)
        return;
    char digits[4];unsigned used=0;
    do {digits[used++]=(char)('0'+n%10);n/=10;} while(n);
    put("\033[");
    while(used)
        write_all(1,&digits[--used],1);
    put("D");
}

/* プロンプトも含め，端末のTAB位置は4桁刻みとする． */
static unsigned shell_columns(const char *line,unsigned len) {
    unsigned col=sizeof(SHELL_PROMPT)-1;
    for(unsigned i=0;i<len;i++)col+=line[i]=='\t'?4-(col&3u):1;
    return col;
}

/* シェル入力にタブが含まれるか調べ，補完処理の分岐に使う． */
static int shell_has_tab(const char *line,unsigned len) {
    for(unsigned i=0;i<len;i++)if(line[i]=='\t')return 1;
    return 0;
}

/* 補完候補や展開後の入力行を端末に表示する． */
static void shell_print_expanded(const char *line,unsigned len) {
    unsigned col=sizeof(SHELL_PROMPT)-1;
    for(unsigned i=0;i<len;i++) {
        if(line[i]=='\t') {
            unsigned n=4-(col&3u);
            for(unsigned j=0;j<n;j++)put(" ");
            col+=n;
        }else {write_all(1,line+i,1);col++;}
    }
}

/* TABを4桁位置へ展開して一行を再描画する． */
static void shell_redraw(const char *line, unsigned len, unsigned cursor) {
    put("\r" SHELL_PROMPT);
    shell_print_expanded(line,len);
    put("\033[K");
    shell_move_left(shell_columns(line,len)-shell_columns(line,cursor));
}

/* 入力行の変更部分だけ再描画し，プロンプトと前半はそのままにする． */
static void shell_redraw_tail(const char *line, unsigned len, unsigned cursor) {
    write_all(1,line+cursor,len-cursor);
    put("\033[K");
    shell_move_left(len-cursor);
}

/* 行末のファイル名または/binコマンドを補完する．引用・ワイルドカード等は対象にせずRAM使用を抑える． */
static int shell_complete(char line[128], unsigned *len, unsigned *cursor,
                          const char cwd[128]) {
    if(*cursor!=*len)
        return 0;
    unsigned start=*len;
    while(start && line[start-1]!=' ' && line[start-1]!='\t')
        start--;
    const char *word=line+start;
    if(!*word)
        return 0;
    unsigned slash=*len;
    while(slash>start && line[slash-1]!='/')
        slash--;
    char directory[128], name_prefix[128];
    unsigned prefix_len=*len-slash;
    if(prefix_len>=sizeof name_prefix)
        return 0;
    for(unsigned i=0;i<prefix_len;i++)
        name_prefix[i]=line[slash+i];
    name_prefix[prefix_len]=0;
    if(slash>start) {
        char path[128];
        unsigned n=slash-start;
        if(n>=sizeof path)
            return 0;
        for(unsigned i=0;i<n;i++)path[i]=word[i];
        path[n]=0;
        if(resolve(cwd,path,directory)<0)
            return 0;
    } else if(start==0) strcpy(directory,"/bin");
    else strcpy(directory,cwd);
    int fd=open(directory,O_RDONLY);
    if(fd<0)
        return 0;
    piconix_dirent_t entry;
    char common[32]={0};
    int matches=0, is_dir=0, status;
    while((status=getdents(fd,&entry,sizeof entry))>0) {
        if((entry.name[0]=='.' && !prefix_len) ||
           strncmp(entry.name,name_prefix,prefix_len)!=0) continue;
        if(matches==0) {
            strcpy(common,entry.name);
            is_dir=entry.is_dir;
        } else {
            unsigned i=0;
            while(common[i] && entry.name[i] && common[i]==entry.name[i])
                i++;
            common[i]=0;
        }
        matches++;
    }
    close(fd);
    if(!matches)
        return 0;
    unsigned common_len=(unsigned)strlen(common);
    unsigned append=common_len>prefix_len?common_len-prefix_len:0;
    if(*len+append+(matches==1 && is_dir?1u:0u)>127)
        return 0;
    if(append) {
        memcpy(line+*len,common+prefix_len,append);
        *len+=append;
    }
    if(matches==1 && is_dir && *len<127)
        line[(*len)++]='/';
    line[*len]=0;
    *cursor=*len;
    return matches;
}

/* 候補を画面に表示して入力行を再描画する．候補一覧を保存せずディレクトリを再走査する． */
static void shell_list_matches(const char line[128], unsigned len,
                               const char cwd[128]) {
    unsigned start=len;
    while(start && line[start-1]!=' ' && line[start-1]!='\t')start--;
    if(start==len)return;
    unsigned slash=len;
    while(slash>start && line[slash-1]!='/')slash--;
    char dir[128];
    if(slash>start) {
        char input[128];unsigned n=slash-start;
        if(n>=sizeof input)return;
        memcpy(input,line+start,n);input[n]=0;
        if(resolve(cwd,input,dir)<0)return;
    } else if(!start)strcpy(dir,"/bin");
    else strcpy(dir,cwd);
    int fd=open(dir,O_RDONLY);
    if(fd<0)return;
    unsigned nmatch=0, prefix_len=len-slash;
    piconix_dirent_t ent;
    while(getdents(fd,&ent,sizeof ent)>0) {
        if(ent.name[0]=='.' && (!prefix_len || line[slash]!='.'))continue;
        if(strncmp(ent.name,line+slash,prefix_len))continue;
        if(!nmatch++)put("\r\n");
        if(nmatch<=48) {
            put(ent.name);
            if(ent.is_dir)put("/");
            put("  ");
            if(nmatch%3==0)put("\r\n");
        }
    }
    close(fd);
    if(!nmatch)return;
    if(nmatch>48)put("... (more matches)");
    put("\r\n");
}

/* 一行入力中だけTTYをraw／エコーなしへ切り替え，編集後に元へ戻す．端末の占有に失敗した場合は行入力へ戻る． */
static int shell_readline(char line[128], const char cwd[128]) {
    int canon=ioctl(0,OS_TTY_GET_CANON,0);
    int echo=ioctl(0,OS_TTY_GET_ECHO,0);
    if(canon<0 || echo<0 || ioctl(0,OS_TTY_SET_CANON,0)<0 ||
       ioctl(0,OS_TTY_SET_ECHO,0)<0) {
        if(canon>=0)
            ioctl(0,OS_TTY_SET_CANON,canon);
        if(echo>=0)
            ioctl(0,OS_TTY_SET_ECHO,echo);
        unsigned used=0; int previous_cr=0;
        for(;;) {
            char c;int n=read(0,&c,1);
            if(n<=0) {if(n==0 && get_tty()==1) {put("\r\n" SHELL_PROMPT);used=0;} else msleep(1);continue;}
            if(c=='\n' && previous_cr) {previous_cr=0;continue;}
            previous_cr=(c=='\r');
            if(c=='\r'||c=='\n') {line[used]=0;return 1;}
            if(used<127)
                line[used++]=c;
        }
    }
    char draft[128]={0};
    line[0]=0;
    unsigned len=0,cursor=0;
    unsigned selected=history_count; /* history_countは現在編集中の下書き位置を示す． */
    int escape=0, tab_pending=0;
    for(;;) {
        char c;
        int n=read(0,&c,1);
        if(n<=0) {if(n<0) msleep(1);else msleep(1);continue;}
        if(escape==1) {escape=(c=='[' || c=='O')?2:0;continue;}
        if(escape==2) {
            escape=0;
            if(c=='A' || c=='B') {
                if(c=='A' && selected==0)
                    continue;
                if(c=='B' && selected==history_count)
                    continue;
                if(selected==history_count)
                    strcpy(draft,line);
                if(c=='A')
                    selected--;
                else selected++;
                if(selected==history_count)
                    strcpy(line,draft);
                else strcpy(line,history[(history_next+HISTORY_SIZE-history_count+selected)%HISTORY_SIZE]);
                len=(unsigned)strlen(line);cursor=len;
            } else if(c=='C') {
                if(cursor<len) {cursor++;if(shell_has_tab(line,len))shell_redraw(line,len,cursor);else put("\033[C");}
                continue;
            } else if(c=='D') {
                if(cursor) {cursor--;if(shell_has_tab(line,len))shell_redraw(line,len,cursor);else put("\033[D");}
                continue;
            } else continue;
            shell_redraw(line,len,cursor);
            continue;
        }
        if(c==1) { /* Ctrl+A：行頭へ移動する． */
            cursor=0;
            shell_redraw(line,len,cursor);
            continue;
        }
        if(c==5) { /* Ctrl+E：行末へ移動する． */
            cursor=len;
            shell_redraw(line,len,cursor);
            continue;
        }
        if(c==11) { /* Ctrl+K：カーソルから行末まで削除する． */
            len=cursor;
            line[len]=0;
            shell_redraw(line,len,cursor);
            continue;
        }
        if(c==23) { /* Ctrl+W：直前の空白区切りの単語を削除する． */
            unsigned end=cursor;
            while(cursor && (line[cursor-1]==' ' || line[cursor-1]=='\t'))
                cursor--;
            while(cursor && line[cursor-1]!=' ' && line[cursor-1]!='\t')
                cursor--;
            if(cursor!=end) {
                memmove(line+cursor,line+end,len-end+1);
                len-=end-cursor;
                shell_redraw(line,len,cursor);
            }
            continue;
        }
        if(c=='\t') {
            if(tab_pending) {
                shell_list_matches(line,len,cwd);
                shell_redraw(line,len,cursor);
                tab_pending=0;
            } else {
                int matches=shell_complete(line,&len,&cursor,cwd);
                if(matches)
                    shell_redraw(line,len,cursor);
                else put("\a");
                tab_pending=matches>1;
            }
            continue;
        }
        tab_pending=0;
        if(c==12) { /* Ctrl+L：画面を消去し，編集中の文字列とカーソルを復元する． */
            put("\033[2J\033[H");
            shell_redraw(line,len,cursor);
            continue;
        }
        if(c==27) {escape=1;continue;}
        if(c=='\r' || c=='\n') {
            line[len]=0;
            put("\r\n");
            history_add(line);
            ioctl(0,OS_TTY_SET_ECHO,echo);
            ioctl(0,OS_TTY_SET_CANON,canon);
            return 1;
        }
        if(c==8 || c==127) {
            if(!cursor)
                continue;
            int had_tab=shell_has_tab(line,len);
            for(unsigned i=cursor;i<len;i++)line[i-1]=line[i];
            len--;cursor--;line[len]=0;
            if(had_tab)
                shell_redraw(line,len,cursor);
            else {
                put("\b");
                if(cursor==len) put(" \b");
                else shell_redraw_tail(line,len,cursor);
            }
        } else if((c>=32 && c<=126) && len<127) {
            int at_end=(cursor==len);
            for(unsigned i=len;i>cursor;i--)line[i]=line[i-1];
            line[cursor++]=c;line[++len]=0;
            if(shell_has_tab(line,len))
                shell_redraw(line,len,cursor);
            else if(at_end) write_all(1,&c,1);
            else {
                /* 端末上のカーソルは引き続き挿入位置にある． */
                shell_redraw_tail(line,len,cursor-1);
                put("\033[C");
            }
        }
    }
}

/* 最初の起動シェルで設定ファイルの各行を通常のコマンド解析器へ渡す．長すぎる行は実行しない． */
static void run_boot_rc(char *cwd) {
    int fd=open("/etc/rc",O_RDONLY);
    if(fd<0)
        return;
    char line[128]; unsigned len=0; char c;
    while(read(fd,&c,1)==1) {
        if(c=='\r')
            continue;
        if(c!='\n' && len<sizeof(line)-1) {line[len++]=c;continue;}
        if(c!='\n') { /* 長すぎるrc行を途中まで実行しないよう拒否する． */
            while(read(fd,&c,1)==1 && c!='\n') {}
            len=0;continue;
        }
        line[len]=0;len=0;
        char *cmd=trim(line);
        if(!*cmd || *cmd=='#')
            continue;
        unsigned n=(unsigned)strlen(cmd);int bg=0;
        if(n && cmd[n-1]=='&') {cmd[n-1]=0;cmd=trim(cmd);bg=1;}
        if(strchr(cmd,'|') || strchr(cmd,'<') || strchr(cmd,'>'))
            continue;
        int pid=0;const char *label=0;
        command(cmd,cwd,bg,&pid,&label);
    }
    close(fd);
}

/* MCPなどからの一回限りの非対話実行．端末入力，起動バナー，rcは使用しない．
 * シェルの通常のコマンド処理とパイプ処理を再利用する． */
static int run_one_command(const char *input, char cwd[128]) {
    char line[128];
    size_t n = strlen(input);
    if(n == 0 || n >= sizeof line) return 2;
    memcpy(line, input, n + 1);
    char *cmd = trim(line);
    if(strchr(cmd, '&')) { put("sh -c: background disabled\n"); return 2; }
    if(strchr(cmd, '|')) {
        char *stage[3]; int count=0, invalid=0;
        char *start=cmd;
        for(char *q=cmd;;q++) {
            if(*q!='|' && *q) continue;
            char sep=*q; *q=0;
            char *part=trim(start);
            if(!*part || strchr(part,'<') || strchr(part,'>') || count==3)
                invalid=1;
            else stage[count++]=part;
            if(!sep) break;
            start=q+1;
        }
        if(invalid || count<2) { put("sh -c: invalid pipeline\n"); return 2; }
        run_pipeline(stage,count,cwd);
        return command_status;
    }
    int saved_in=-1,saved_out=-1,bg_pid=0;
    const char *bg_name=0;
    int rc=redirect(cmd,cwd,&saved_in,&saved_out);
    if(rc==0) command(trim(cmd),cwd,0,&bg_pid,&bg_name);
    restore_stdio(saved_in,saved_out);
    if(rc<0) { put("sh -c: invalid redirection\n"); return 2; }
    return command_status;
}

/* 端末の用意と起動時処理を終えた後，入力・解析・実行・子の回収を繰り返す． */
int main(int argc, char *argv[]) {
    if(argc!=1 && !(argc==3 && !strcmp(argv[1],"-c"))) {
        put("usage: sh [-c command]\r\n"); return 1;
    }
    char line[128];
    char cwd[128];
    if(chdir("/tmp")<0) {
        put("sh: cannot cd to /tmp\r\n");
        _exit(1);
    }
    strcpy(cwd,"/tmp");
    if(argc==3) { batch_mode=1; return run_one_command(argv[2],cwd); }
    /* シェル起動後にUSB CDCが接続されることがあるため，起動バナーを捨てない． */
    while(!console_ready())
        msleep(10);
    char id_line[] = "FlashID: 00000000\r\n";
    static const char hex[] = "0123456789abcdef";
    unsigned id = flash_id();
    for (unsigned i=0; i<8; ++i)
        id_line[9+i] = hex[(id >> (28-4*i)) & 15u];
    put("\r\n--------------------------------------------------------\r\n"
        "PicoNIX-2026-10-30 (TAKAGO_LAB.)\r\n");
    put(id_line);
    put("Type 'help' for commands.\r\n"
        "--------------------------------------------------------\r\n");
    if(getpid()==1)
        run_boot_rc(cwd);
    put("\033[0m\033[?25h" SHELL_PROMPT);
    for(;;) {
        shell_readline(line,cwd);
        /* 完了した背景子プロセスを回収する．終了状態の表示はまだ行わない． */
        int bg_status;
        while(waitpid(-1, &bg_status, 1)>0) {}
        /* リダイレクトを解析する前に，コマンド末尾の&を一度だけ解釈する． */
        char *cmd=trim(line);
        int requested_bg=0;
        size_t len=strlen(cmd);
        if(len && cmd[len-1]=='&') {
            cmd[len-1]=0;
            cmd=trim(cmd);
            requested_bg=1;
        }
        if(strchr(cmd,'|')) {
            char *stage[3]; int count=0, invalid=requested_bg;
            char *start=cmd;
            for(char *q=cmd;;q++) {
                if(*q!='|' && *q)
                    continue;
                char sep=*q;
                *q=0;
                char *part=trim(start);
                if(!*part || strchr(part,'<') || strchr(part,'>') || count==3)
                    invalid=1;
                else stage[count++]=part;
                if(!sep)
                    break;
                start=q+1;
            }
            if(invalid || count<2)
                put("pipe: expected 2 or 3 commands (no redirects/background)\r\n");
            else run_pipeline(stage,count,cwd);
            put("\033[0m\033[?25h" SHELL_PROMPT);
            continue;
        }
        int saved_in=-1,saved_out=-1, bg_pid=0;
        const char *bg_name=0;
        int redir_ok=redirect(cmd,cwd,&saved_in,&saved_out);
        if(redir_ok==0)
            command(trim(cmd),cwd,requested_bg,&bg_pid,&bg_name);
        restore_stdio(saved_in,saved_out);
        if(redir_ok<0) put("redirect: invalid path or cannot open file\r\n");
        /* 背景起動の通知はリダイレクト先でなく端末へ出す． */
        if(bg_pid>0) {
            char pid_text[16];
            snprintf(pid_text,sizeof pid_text,"%d",bg_pid);
            put("["); put(pid_text);
            put("] "); put(bg_name?bg_name:"program"); put(" started\r\n");
        }
        put("\033[0m\033[?25h" SHELL_PROMPT);
    }
}
