/* カーネルの固定長トレースレコードを読み，
 * システムコールの入口・出口やタスク切替を表示する． */
/* ユーザ空間でカーネルトレースを読み取るコマンド． */
#include "unistd.h"
#include "fcntl.h"
#include "stdio.h"
#include "string.h"
#include "../../include/piconix_trace.h"
#include "../../include/device_names.h"
#include "../../include/piconix_syscall.h"

/* トレース中のシステムコール番号を表示名へ変換する． */
static const char *syscall_name(unsigned n) {
    switch(n) {
        case 0: return "?";
#define PICONIX_TRACE_SYSCALL_CASE(name, number, label) case name: return label;
        PICONIX_SYSCALL_TABLE(PICONIX_TRACE_SYSCALL_CASE)
#undef PICONIX_TRACE_SYSCALL_CASE
        default: return "unknown";
    }
}
/* /binの列挙はプログラムID順である．終了したタスクでも記録済みIDを名前に変換できるよう，表示時に一度解決する． */
#define PROGRAM_COUNT 65  /* カーネルと同じプログラムID容量（0〜64）にする． */
#define NAMES_CAP 768
static char names[NAMES_CAP];
static const char *program_names[PROGRAM_COUNT];

/* 登録プログラムのIDと名前をVFSから読み，記録表示に使う． */
static void load_program_names(void) {
    for(int i=0;i<PROGRAM_COUNT;i++)
        program_names[i]="?";
    program_names[0]="idle";
    int fd=open("/bin",O_RDONLY);
    if(fd<0)
        return;
    unsigned pos=0;
    piconix_dirent_t ent;
    int id=1, n;
    while(id<PROGRAM_COUNT && (n=getdents(fd,&ent,sizeof ent))>0) {
        unsigned len=(unsigned)strlen(ent.name)+1u;
        if(pos+len>sizeof names)
            break;
        memcpy(names+pos,ent.name,len);
        int actual_id=program_id(ent.name);
        if(actual_id>0 && actual_id<PROGRAM_COUNT)
            program_names[actual_id]=names+pos;
        id++;
        pos+=len;
    }
    close(fd);
}

/* プログラムIDから登録名を取得する．不明なIDなら?を返す． */
static const char *program_name(unsigned id) {
    return id<PROGRAM_COUNT ? program_names[id] : "?";
}

/* openに成功したFDの小さな履歴を保持する． */
#define FD_NAMES 24
static struct { unsigned pid; unsigned fd; char name[17]; } fd_names[FD_NAMES];
static unsigned fd_next;
/* 追跡対象プロセスのFDに付けた表示名を探す． */
static const char *find_fd(unsigned pid, unsigned fd) {
    for(unsigned i=0;i<FD_NAMES;i++)
        if(fd_names[i].name[0] && fd_names[i].pid==pid && fd_names[i].fd==fd)
            return fd_names[i].name;
    return NULL;
}

/* 終了・closeされたFDの表示名キャッシュを削除する． */
static void forget_fd(unsigned pid, unsigned fd) {
    for(unsigned i=0;i<FD_NAMES;i++)
        if(fd_names[i].name[0] && fd_names[i].pid==pid && fd_names[i].fd==fd)
            fd_names[i].name[0]=0;
}

/* openしたFDと表示パスの組を小さな履歴に保存する． */
static void save_fd(unsigned pid, unsigned fd, const uint8_t *name) {
    forget_fd(pid,fd);
    unsigned i=fd_next++;
    if(fd_next==FD_NAMES)
        fd_next=0;
    fd_names[i].pid=pid; fd_names[i].fd=fd;
    for(unsigned j=0;j<16;j++)
        fd_names[i].name[j]=(char)name[j];
    fd_names[i].name[16]=0;
}

/* 列幅を固定してトレース表示を小さく予測しやすく保つ． */
static void col(const char *s, unsigned width) {
    unsigned i=0;
    while(i<width && s[i]) {putchar(s[i]); i++;}
    while(i++<width)
        putchar(' ');
    putchar(' ');
}

/* 記録内の数値フィールドを表示用に書式化する． */
static void numeric(char *s, int n) {sprintf(s,"%d",n);}

/* トレース時刻を短い固定桁の表示へ変換する． */
static void format_short_time(char *out, uint32_t ms) {
    char digits[16];
    sprintf(digits,"%u",(unsigned)ms);
    unsigned len=(unsigned)strlen(digits), pos=0;
    if(len<=3) {
        out[pos++]='0'; out[pos++]='.';
        for(unsigned i=len;i<3;i++)
            out[pos++]='0';
        for(unsigned i=0;i<len;i++)
            out[pos++]=digits[i];
    } else {
        for(unsigned i=0;i<len-3;i++)
            out[pos++]=digits[i];
        out[pos++]='.';
        for(unsigned i=len-3;i<len;i++)
            out[pos++]=digits[i];
    }
    out[pos]=0;
}

/* 文字列バッファ末尾に表示項目を追加し，上限を越えないようにする． */
static void append(char *out, const char *s) {
    unsigned n=(unsigned)strlen(out);
    while(*s && n<sizeof(((trace_event_t *)0)->data)*4u-1u)
        out[n++]=*s++;
    out[n]=0;
}

/* デバイスIDの名称はカーネルVFSと共通の定義から求める． */
static const char *device_path(int id) {
#define DEVICE_CASE(path, dev) if(id==dev) return path;
    PICONIX_FIXED_DEVICES(DEVICE_CASE)
    PICONIX_WIFI_DEVICES(DEVICE_CASE)
#undef DEVICE_CASE
    return NULL;
}

/* システムコールのFD／対象を人間が読みやすい短い文字列へ整形する． */
static void format_target(char *out, unsigned pid, unsigned fd, int8_t id) {
    const char *name=find_fd(pid,fd);
    sprintf(out,"%u",fd);
    if(name) {append(out,":"); append(out,name);}
    else if(id>=0 && device_path(id)) {append(out,":");append(out,device_path(id));}
    else if(id>=0) {char part[16];sprintf(part,":id=%d",(int)id);append(out,part);}
}

/* read／writeの先頭データを印字可能な範囲で表示する． */
static void print_preview(const uint8_t *data, unsigned len, int shortened) {
    unsigned written=0;
    putchar('"'); written++;
    unsigned i=0;
    for(;i<len && written<9;i++) {
        unsigned c=data[i];
        if(c=='\n' || c=='\r' || c=='\t') {
            putchar('\\');putchar(c=='\n'?'n':c=='\r'?'r':'t'); written+=2;
        } else if(c=='"'||c=='\\') {putchar('\\');putchar((int)c);written+=2;}
        else if(c>=32 && c<127) {putchar((int)c);written++;}
        else {putchar('.');written++;}
    }
    putchar('"');
    if(shortened || i<len)
        printf("...");
}

/* ヘッダとPID切替時の区切りには同じASCII線を使用する． */
#define TRACE_SEPARATOR "------------------------------------------------------------------------------------------------"
static void header(void) {
    puts("Time   PID Command TTY     Type     Function                           Arg              Ret Info");
    puts(TRACE_SEPARATOR);
}
static unsigned close_pid, close_fd;
static struct { unsigned pid, call; char target[64]; } pending[8];
static unsigned pending_next;
/* PIDごとに表示上の呼び出し深さを保持する．実行時のカーネルスタックは変更しない． */
#define TRACE_DEPTH_SLOTS 16
static struct { unsigned pid; unsigned depth; int used; } trace_depths[TRACE_DEPTH_SLOTS];
static unsigned trace_depth_next;
static trace_event_t display_pending;
static int display_has_pending;
/* 直前に実際に表示した行のPID．フィルタで除外したイベントは数えない． */
static unsigned display_previous_pid;
static int display_previous_valid;
/* PIDが同じイベントだけが同一の深さを共有する． */
static unsigned *trace_depth(unsigned pid) {
    for(unsigned i=0;i<TRACE_DEPTH_SLOTS;i++)
        if(trace_depths[i].used && trace_depths[i].pid==pid)
            return &trace_depths[i].depth;
    for(unsigned i=0;i<TRACE_DEPTH_SLOTS;i++)
        if(!trace_depths[i].used) {
            trace_depths[i].used=1; trace_depths[i].pid=pid;
            trace_depths[i].depth=0;
            return &trace_depths[i].depth;
        }
    unsigned i=trace_depth_next++ % TRACE_DEPTH_SLOTS;
    trace_depths[i].pid=pid; trace_depths[i].depth=0;
    return &trace_depths[i].depth;
}
/* 入口をname，復帰をname/で表す．記録形式は変更しない． */
static void function_label(char *out, const char *name, unsigned depth, int entering) {
    unsigned pos=0;
    if(depth>8)
        depth=8;
    for(unsigned i=0;i<depth*2;i++)
        out[pos++]=' ';
    while(*name && pos<(entering ? 39u : 38u))
        out[pos++]=*name++;
    if(!entering)
        out[pos++]='/';
    out[pos]=0;
}
/* 未完了システムコールの対象を取得する．同じPID・番号の古い履歴は残さない． */
static const char *pending_find(unsigned pid, unsigned call) {
    for(unsigned i=0;i<8;i++)
        if(pending[i].pid==pid && pending[i].call==call)
            return pending[i].target;
    return NULL;
}

/* システムコール終了時に対応付けを消し，次回の同一呼び出しへ持ち越さない． */
static void pending_forget(unsigned pid, unsigned call) {
    for(unsigned i=0;i<8;i++)
        if(pending[i].pid==pid && pending[i].call==call)
            pending[i].pid=0;
}

/* 同一PID・同一システムコールの入口は上書きする． */
static void pending_save(unsigned pid, unsigned call, const char *target) {
    for(unsigned i=0;i<8;i++)
        if(pending[i].pid==pid && pending[i].call==call) {
            strcpy(pending[i].target,target);
            return;
        }
    unsigned i=pending_next++ & 7u;
    pending[i].pid=pid; pending[i].call=call;
    strcpy(pending[i].target,target);
}

/* 対象PIDやFD履歴等のトレース表示状態を初期化する． */
static void reset_view_state(void) {
    memset(fd_names,0,sizeof fd_names);
    memset(pending,0,sizeof pending);
    fd_next=0;
    pending_next=0;
    memset(trace_depths,0,sizeof trace_depths);
    trace_depth_next=0;
    close_pid=0;
    close_fd=0;
    display_has_pending=0;
    display_previous_pid=0;
    display_previous_valid=0;
}

/* 表示上のPIDが変わる直前に，ASCIIの区切り線を1行出す． */
static void print_pid_separator(unsigned pid) {
    if(display_previous_valid && display_previous_pid!=pid)
        puts(TRACE_SEPARATOR);
    display_previous_pid=pid;
    display_previous_valid=1;
}

/* 表示は Function（入口name／復帰name/の階層），Arg，Ret，Info に統一する． */
static void print_event(const trace_event_t *e, int compact) {
    char time[20],pid[16],arg[64]="",ret[16]="--",label[48];
    unsigned kind=e->kind&TRACE_KIND_MASK;
    unsigned shown_pid=kind==TRACE_SWITCH ? e->value : e->pid;
    format_short_time(time,e->tick);
    if(!shown_pid && (kind==TRACE_FUNC_ENTER || kind==TRACE_FUNC_EXIT))
        strcpy(pid,"-");
    else numeric(pid,(int)shown_pid);
    const char *tty=shown_pid==0 ? "-" :
        (TRACE_TTY(e->kind)==1 ? "ttyS1" : TRACE_TTY(e->kind)==2 ? "ttyN0" : "console");
    const char *name=shown_pid==0 && (kind==TRACE_FUNC_ENTER || kind==TRACE_FUNC_EXIT) ? "kernel" :
        (e->program_id==0 && shown_pid!=0 ? "a.out" : program_name(e->program_id));
    unsigned func_depth=0;
    int func_enter=kind==TRACE_FUNC_ENTER;
    if(kind==TRACE_FUNC_ENTER || kind==TRACE_FUNC_EXIT) {
        unsigned *depth=trace_depth(e->pid);
        if(!compact && !func_enter && *depth)
            --*depth;
        func_depth=*depth;
        if(!compact && func_enter && *depth<255)
            ++*depth;
    }
    /* SWITCH行では切替先PIDを表示する．区切りはイベント種別に依存しない． */
    print_pid_separator(shown_pid);
    col(time,7);col(pid,3);col(name,7);col(tty,7);
    if(kind==TRACE_SWITCH) {
        col("SWITCH",9);col("SWITCH",34);col("--",16);col("--",3);
        printf("%u -> %u\n",e->pid,e->value);
        return;
    }
    if(kind==TRACE_TICK1K) {
        col("TIMER",9);col("TICKx1K",34);col("--",16);col("--",3);
        puts("SysTick");return;
    }
    if(kind==TRACE_FUNC_ENTER || kind==TRACE_FUNC_EXIT) {
        char func[17];
        memcpy(func,e->data,16);func[16]=0;
        function_label(label,func,func_depth,func_enter);
        if(compact) {
            unsigned i=0;
            while(label[i]==' ')
                i++;
            unsigned j=i;
            while(label[j])
                j++;
            if(j<33) {
                for(unsigned k=j+1;k>i;k--)
                    label[k]=label[k-1];
                label[i]='('; label[j+1]=')'; label[j+2]=0;
            }
        }
        col("KERNEL",9);col(label,34);col("--",16);col("--",3);puts("--");
        return;
    }
    if(kind!=TRACE_SVC_ENTER && kind!=TRACE_SVC_EXIT) {
        col("OTHER",9);col("UNKNOWN",34);col("--",16);col("--",3);puts("--");return;
    }
    int enter=kind==TRACE_SVC_ENTER;
    unsigned *depth=trace_depth(e->pid);
    /* システムコールは現在の関数階層内に表示する．
     * os_svc_dispatchが記録されていればその一段内側になる．
     * トレースが途中から始まった場合でも階層は0から始められる． */
    unsigned syscall_depth=*depth;
    if(enter && *depth<255)
        ++*depth;
    unsigned call=e->value;
    if(!enter) {
        numeric(ret,(int)e->detail);
        if(call==SYS_OPEN && (int)e->detail>=0) {
            save_fd(e->pid,e->detail,e->data);
            format_target(arg,e->pid,e->detail,e->file_id);
        } else {
            const char *prior=pending_find(e->pid,call);
            if(prior)
                strcpy(arg,prior);
        }
    } else {
        if(call==SYS_CLOSE || call==SYS_READ || call==SYS_WRITE || call==SYS_IOCTL ||
           call==SYS_LSEEK || call==SYS_GETDENTS || call==SYS_BIND ||
           call==SYS_SENDTO || call==SYS_RECVFROM || call==SYS_LISTEN_TCP ||
           call==SYS_CONNECT_TCP || call==SYS_TCP_STATUS || call==SYS_ACCEPT_TCP)
            format_target(arg,e->pid,e->detail,e->file_id);
        else if(call==SYS_OPEN) {
            /* openの引数はENTERイベント内の先頭16バイト．終端は自分で付ける． */
            memcpy(arg,e->data,sizeof e->data);
            arg[sizeof e->data]=0;
            if(!arg[0])
                strcpy(arg,"path");
        }
        else if(call==SYS_WAITPID) numeric(arg,(int32_t)e->detail);
        else if(call==SYS_SPAWN) {const char *prog=program_name(e->detail);strcpy(arg,prog);}
        else numeric(arg,(int32_t)e->detail);
        pending_save(e->pid,call,arg);
        if(call==SYS_CLOSE) {close_pid=e->pid;close_fd=e->detail;}
    }
    if(!enter && *depth)
        --*depth;
    function_label(label,syscall_name(call),enter ? syscall_depth : *depth,enter);
    col("SYSCALL",9);col(label,34);col(arg[0]?arg:"--",16);col(ret,3);
    if(!enter && (call==SYS_READ || call==SYS_WRITE) && (int)e->detail>0 && e->file_id!=99) {
        unsigned length=(unsigned)e->detail;
        if(length>16)
            length=16;
        print_preview(e->data,length,(unsigned)e->detail>16);
    } else printf("--");
    putchar('\n');
    if(!enter)
        pending_forget(e->pid,call);
    if(!enter && call==SYS_CLOSE && (int)e->detail==0 && close_pid==e->pid)
        forget_fd(close_pid,close_fd);
}

/* 関数ENTERは次のレコードまで表示を保留する．ほかのイベントを挟んだ場合は統合しない． */
/* ミリ秒表示が同一で，連続した同じ関数・実行コンテキストの入退場だけを統合する． */
static int compact_pair(const trace_event_t *a, const trace_event_t *b) {
    return (a->kind&TRACE_KIND_MASK)==TRACE_FUNC_ENTER &&
           (b->kind&TRACE_KIND_MASK)==TRACE_FUNC_EXIT &&
           a->tick==b->tick && a->pid==b->pid &&
           a->program_id==b->program_id && a->kind==b->kind-1 &&
           a->value==b->value && !memcmp(a->data,b->data,16);
}

/* PIDフィルタは表示のみに適用する．統合判定には生の連続レコードを使う． */
static int display_matches(const trace_event_t *e, int filter_pid) {
    unsigned event_pid=(e->kind&TRACE_KIND_MASK)==TRACE_SWITCH ? e->value : e->pid;
    return filter_pid<0 || event_pid==(unsigned)filter_pid;
}

/* トレースイベントを表示し，関数の入口と出口を可能なら統合する． */
static int display_event(const trace_event_t *e, int filter_pid) {
    int shown=0;
    if(display_has_pending) {
        if(compact_pair(&display_pending,e)) {
            if(display_matches(&display_pending,filter_pid)) {
                print_event(&display_pending,1);
                shown++;
            }
            display_has_pending=0;
            return shown;
        }
        if(display_matches(&display_pending,filter_pid)) {
            print_event(&display_pending,0);
            shown++;
        }
        display_has_pending=0;
    }
    if((e->kind&TRACE_KIND_MASK)==TRACE_FUNC_ENTER) {
        display_pending=*e;
        display_has_pending=1;
    } else if(display_matches(e,filter_pid)) {
        print_event(e,0);
        shown++;
    }
    return shown;
}

/* 表示待ちの関数入口イベントを出力する． */
static int display_flush(int filter_pid) {
    if(!display_has_pending)
        return 0;
    display_has_pending=0;
    if(!display_matches(&display_pending,filter_pid))
        return 0;
    print_event(&display_pending,0);
    return 1;
}

/* トレースデバイスから固定長レコードを読み，表示へ順次流す． */
static int trace_drain(int fd) {
    trace_event_t e;
    int n;
    while((n=read(fd,&e,sizeof e))==(int)sizeof e) {}
    return n;
}

/* コマンドの実行中だけ記録範囲を切り替え，既存の記録状態を復元する． */
static int trace_run(int fd, int argc, char *argv[]) {
    int command_index=2;
    int mode=TRACE_MODE_SVC;
    if(argc>2 && (!strcmp(argv[2],"-a") || !strcmp(argv[2],"-s"))) {
        mode=!strcmp(argv[2],"-a") ? TRACE_MODE_ALL : TRACE_MODE_SYSCALL;
        command_index++;
    }
    if(argc<command_index+1 || argc-command_index>8) {
        puts("usage: trace run [-s|-a] command [args...]");return 1;
    }
    load_program_names();
    reset_view_state();
    /* 現在ディレクトリ内の./image実行はシェルと同じ形式で解釈する． */
    char image_path[128];
    const char *name=argv[command_index];
    const char *image=name;
    int is_tmp=!strncmp(name,"/tmp/",5) || !strncmp(name,"./",2);
    if(!strncmp(name,"./",2)) {
        char cwd[128];
        if(!getcwd(cwd,sizeof cwd)) {puts("trace: cannot get cwd");return 1;}
        unsigned a=(unsigned)strlen(cwd), b=(unsigned)strlen(name+2);
        if(a+b+2>sizeof image_path) {puts("trace: executable path too long");return 1;}
        strcpy(image_path,cwd);
        if(a && image_path[a-1]!='/')
            image_path[a++]='/';
        strcpy(image_path+a,name+2);
        image=image_path;
    }
    int id=is_tmp ? -1 : program_id(name);
    if(is_tmp && argc!=command_index+1) {puts("trace: /tmp programs do not accept arguments");return 1;}
    if(!is_tmp && id<0) {puts("trace: unknown command");return 1;}
    unsigned bytes=0;
    for(int i=command_index;i<argc;i++)
        bytes+=(unsigned)strlen(argv[i])+1;
    if(bytes>128) {puts("trace: arguments too long");return 1;}
    int enabled=ioctl(fd,TRACE_STATUS,0);
    if(enabled<0) {puts("trace: cannot query tracing status");return 1;}
    int previous_mode=ioctl(fd,TRACE_CONTEXT_GET,0);
    if(previous_mode<0) {puts("trace: cannot query tracing context");return 1;}
    if(enabled) {
        if(ioctl(fd,TRACE_CONTEXT_SET,mode)<0)
            return 1;
    } else if(ioctl(fd,TRACE_ON,mode)<0) {
        puts("trace: cannot enable tracing");return 1;
    }
    /* 既に記録中なら取得済みデータを維持する． */
    if(trace_drain(fd)<0) {
        puts("trace: cannot seek to latest event");
        if(enabled)
            ioctl(fd,TRACE_CONTEXT_SET,previous_mode);
        else ioctl(fd,TRACE_OFF,0);
        return 1;
    }
    int pid=is_tmp ? spawn_tmp(image) : spawn(id,argc-command_index,(const char *const *)(argv+command_index));
    if(pid<0) {
        puts("trace: cannot start command");
        if(enabled)
            ioctl(fd,TRACE_CONTEXT_SET,previous_mode);
        else ioctl(fd,TRACE_OFF,0);
        return 1;
    }
    /* Pythonなどの入力待ちプログラムが通常のTTYを利用できるようにする． */
    set_foreground(pid);
    int status=0;
    int waited=waitpid(pid,&status,0);
    set_foreground(0);
    if(enabled)
        ioctl(fd,TRACE_CONTEXT_SET,previous_mode);
    else ioctl(fd,TRACE_OFF,0);
    printf("\n--- trace: %s (PID %d) ---\n",argv[command_index],pid);
    header();
    trace_event_t e;
    int count=0,n;
    while((n=read(fd,&e,sizeof e))==(int)sizeof e) {
        count+=display_event(&e,pid);
    }
    count+=display_flush(pid);
    if(n<0)
        puts("trace: read error");
    else if(!count) puts("trace: no retained events");
    int lost=ioctl(fd,TRACE_LOST,0);
    if(lost>0) printf("trace: %d events overwritten\n",lost);
    puts("--- end of trace ---");
    if(waited<0)
        puts("trace: waitpid failed");
    return n<0 || waited<0 ? 1 : 0;
}

/* トレース表示・起動操作をコマンド引数に基づいて選ぶ． */
int main(int argc, char *argv[]) {
    if(argc<2) {
        puts("usage: trace on [-s|-a]|off|show [PID]|follow [PID]|run [-s|-a] command [args...]");return 1;
    }
    if(!strcmp(argv[1],"run")) {
        int fd=open("/dev/trace",O_RDONLY);
        if(fd<0) {puts("trace: cannot open /dev/trace");return 1;}
        int rc=trace_run(fd,argc,argv);
        close(fd);
        return rc;
    }
    int on_mode=TRACE_MODE_SVC;
    int on_option=argc==3 && !strcmp(argv[1],"on") &&
                  (!strcmp(argv[2],"-a") || !strcmp(argv[2],"-s"));
    if(on_option)
        on_mode=!strcmp(argv[2],"-a") ? TRACE_MODE_ALL : TRACE_MODE_SYSCALL;
    if((argc>3 || (argc==3 && !on_option && strcmp(argv[1],"show") && strcmp(argv[1],"follow"))) ||
       (strcmp(argv[1],"on") && strcmp(argv[1],"off") &&
                   strcmp(argv[1],"show") && strcmp(argv[1],"follow"))) {
        puts("usage: trace on [-s|-a]|off|show [PID]|follow [PID]|run [-s|-a] command [args...]");return 1;
    }
    int filter_pid=-1;
    if(argc==3 && !on_option) {
        if(strcmp(argv[1],"show") && strcmp(argv[1],"follow")) {
            puts("trace: PID filter requires show or follow"); return 1;
        }
        const char *p=argv[2];
        if(!*p) {puts("trace: invalid PID");return 1;}
        int pid=0;
        while(*p) {
            if(*p<'0' || *p>'9' || pid>100000) {
                puts("trace: invalid PID");return 1;
            }
            pid=pid*10+(*p++-'0');
        }
        filter_pid=pid;
    }
    int fd=open("/dev/trace",O_RDONLY);
    if(fd<0) {puts("trace: cannot open /dev/trace"); return 1;}
    int show=!strcmp(argv[1],"show"), follow=!strcmp(argv[1],"follow");
    if(!show && !follow) {
        int on=!strcmp(argv[1],"on");
        if(ioctl(fd,on?TRACE_ON:TRACE_OFF,on_mode)<0) {puts("trace: ioctl failed");close(fd);return 1;}
        puts(on?(on_mode==TRACE_MODE_SYSCALL ? "trace enabled (syscalls only)" :
                 on_mode==TRACE_MODE_ALL ? "trace enabled (all contexts)" :
                 "trace enabled (SVC functions if built)") : "trace disabled");
        close(fd);return 0;
    }
    if(show && ioctl(fd,TRACE_OFF,0)<0) {puts("trace: ioctl failed");close(fd);return 1;}
    /* followモードでは保持中の最古イベントでなく，起動時点から表示する． */
    if(follow && trace_drain(fd)<0) {puts("trace: cannot seek to latest event");close(fd);return 1;}
    load_program_names();
    reset_view_state();
    header();
    trace_event_t event;
    int count=0,n;
    for(;;) {
        if(follow) {
            int lost=ioctl(fd,TRACE_LOST,0);
            if(lost>0) printf("trace: %d events overwritten\n",lost);
        }
        n=read(fd,&event,sizeof event);
        if(n==(int)sizeof event) {
            count+=display_event(&event,filter_pid);
            continue;
        }
        if(n<0) {count+=display_flush(filter_pid);puts("trace: read error");close(fd);return 1;}
        if(!follow)
            break;
        /* 新規イベントが途切れたら保留中のENTERを表示し，followの表示遅延を抑える． */
        count+=display_flush(filter_pid);
        if(ioctl(fd,TRACE_STATUS,0)==0)
            break;
        msleep(50);
    }
    count+=display_flush(filter_pid);
    close(fd);
    if(!count)
        puts("trace: no events (use trace on first)");
    return 0;
}
