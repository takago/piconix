/* SPDX-License-Identifier: GPL-3.0-or-later */
/* VFS上のファイルとディレクトリを閲覧する全画面ブラウザ．
 * 端末とメモリの上限を考慮し，表示に必要な範囲を読み出す． */

/* fdはコンパクトなファイルブラウザである．動的確保，書込みを伴うデバイスread，カーネル変更を必要としない． */
#include "unistd.h"
#include "fcntl.h"
#include "stdio.h"
#include "string.h"
#include "../lib/screen.h"

#define MAX_ENTRIES 48
#define PATH_CAP 128
#define VIEW_CAP 2048
#define MIN_ROWS 12
#define INDEX_STRIDE 16
#define INDEX_CAP 128

typedef struct { char name[32]; unsigned char dir; } entry_t;
static entry_t entries[MAX_ENTRIES];
static char current[PATH_CAP]="/tmp";
static char path[PATH_CAP];
static char file_data[VIEW_CAP+1], old_data[VIEW_CAP+1];
static int count, selected, scroll, total_entries, rows=24, cols=80;
static int original_canon=-1, original_echo=-1, original_flags=-1;
static int mode_view, old_len=-1, old_error=1;
static int view_top, view_lines, view_more, old_more, old_top=-1;
/* 文字列アドレスで初期化されたポインタは絶対再配置を生むので避ける． */
static const char *notice;
/* シンボルテキストのようにseek可能で内容が安定したファイルだけ，疎な行オフセット索引を持つ．/proc状態は更新ごとに変わるため索引化しない． */
static long line_offsets[INDEX_CAP];
static int indexed_lines;
/* ファイル表示用の疎な行索引を先頭位置で初期化する． */
static void reset_view_index(void) {
    indexed_lines=1;
    line_offsets[0]=0;
}

/* 表示範囲内のファイル内容を行ごとに索引化する．全ファイルを保持しない． */
static void index_line(int line, long offset) {
    if(line>0 && line%INDEX_STRIDE==0) {
        int idx=line/INDEX_STRIDE;
        if(idx<INDEX_CAP && idx==indexed_lines) {
            line_offsets[idx]=offset;
            indexed_lines++;
        }
    }
}

/* 指定幅以内で文字列を切り出し，必ず終端文字を付ける． */
static void clipped(char out[112],const char *in,int max){
    int i=0;
    while(i<max&&in[i]){out[i]=in[i];i++;}
    out[i]=0;
}

/* ディレクトリとエントリ名から絶対パスを組み立てる．固定長上限を確認する． */
static int join(const char *dir,const char *name,char out[PATH_CAP]) {
    unsigned n=(unsigned)strlen(dir),m=(unsigned)strlen(name);
    unsigned slash=n>1?1u:0u;
    if(n+slash+m>=PATH_CAP)return -1;
    memcpy(out,dir,n);
    if(slash)out[n++]='/';
    memcpy(out+n,name,m+1);
    return 0;
}

/* 現在の閲覧パスを親ディレクトリへ移動する．ルートより上には進めない． */
static void parent_dir(void) {
    int n=(int)strlen(current);
    while(n>1 && current[n-1]!='/')--n;
    if(n>1)--n;
    current[n]=0;
}

/* getdentsでディレクトリを列挙し，表示用の項目を保持する． */
static int directory_load(void) {
    int fd=open(current,O_RDONLY);
    if(fd<0)return -1;
    int n=0;
    piconix_dirent_t e;
    total_entries=0;
    while((n=getdents(fd,&e,sizeof e))>0) {
        if(!strcmp(e.name,".")) continue;
        if(!strcmp(e.name,"..") && !strcmp(current,"/")) continue;
        ++total_entries;
        if(count<MAX_ENTRIES) {
            int k=count++;
            snprintf(entries[k].name,sizeof(entries[k].name),"%s",e.name);
            entries[k].dir=e.is_dir;
        }
    }
    close(fd);
    if(n<0)return -1;
    if(selected>=count)selected=count?count-1:0;
    return 0;
}

/* 現在ディレクトリのエントリをVFSから読み直し，選択位置を更新する． */
static void refresh_dir(void){count=0;selected=0;scroll=0;notice="";if(directory_load()<0)notice="Cannot open directory";}

/* ファイル一覧と選択位置を端末画面へ表示する． */
static void draw_dir(void) {
    char title[112];clipped(title,current,cols-15);
    printf("\033[?25l\033[H\033[2J\033[7m PicoNIX fd  %s\033[K\033[0m",title);
    screen_move(2,1);printf(" Enter: open   e: edit   Backspace: parent   r: reload   q: quit\033[K");
    int visible=rows-5;
    if(selected<scroll)scroll=selected;
    if(selected>=scroll+visible)scroll=selected-visible+1;
    for(int i=0;i<visible;i++) {
        int k=scroll+i;
        screen_move(i+3,1);
        if(k<count) {
            if(k==selected)printf("\033[7m");
            char name[112];clipped(name,entries[k].name,cols-6);
            if(!strcmp(entries[k].name,".."))printf(" ../\033[K");
            else printf(" %c %s\033[K",entries[k].dir?'/':' ',name);
            if(k==selected)printf("\033[0m");
        } else printf("\033[K");
    }
    screen_move(rows-1,1);
    if(total_entries>MAX_ENTRIES)printf(" Showing first %d of %d entries\033[K",count-(strcmp(current,"/")!=0),total_entries);
    else printf(" %d entries\033[K",count);
    screen_move(rows,1);printf(" %s\033[K",notice);
}

/* 既知のテキスト系ファイルだけを閲覧する．/devのreadは消費的・待機的・バイナリの場合があるため実行しない． */
static int safe_view(const char *p) {
    return !strncmp(p,"/tmp/",5) || !strncmp(p,"/etc/",5) ||
           !strncmp(p,"/proc/",6);
}

/* ファイルの対象範囲を読み取り，現在のページ表示を準備する． */
static int view_load(void){
    int fd=open(path,O_RDONLY);
    if(fd<0)return -1;
    int visible=rows-5;
    int limit=VIEW_CAP/(cols>1?cols:80);
    if(visible>limit)visible=limit;
    if(visible<1)visible=1;
    view_lines=0;
    view_more=0;
    int line=0, x=0, n=0, error=0, done=0;
    long byte_offset=0;
    int indexed=!strcmp(path,"/proc/sym/kernel") && indexed_lines>0;
    if(indexed) {
        /* 古いカーネルはseekに未対応の場合があるので，実際に試して判定する． */
        if(lseek(fd,0,SEEK_CUR)<0) indexed=0;
        else {
            int anchor=view_top/INDEX_STRIDE;
            if(anchor>=indexed_lines)anchor=indexed_lines-1;
            if(anchor<0)anchor=0;
            byte_offset=line_offsets[anchor];
            if(lseek(fd,byte_offset,SEEK_SET)<0) {
                indexed=0;
                byte_offset=0;
            } else line=anchor*INDEX_STRIDE;
        }
    }
    char block[256];
    while(!done){
        int count=read(fd,block,sizeof(block));
        if(count<0){error=1;break;}
        if(!count)break;
        for(int i=0;i<count;i++){
            unsigned char c=(unsigned char)block[i];
            byte_offset++;
            if(line<view_top){
                if(c=='\n') {line++;if(indexed)index_line(line,byte_offset);}
                continue;
            }
            if(line>=view_top+visible){view_more=1;done=1;break;}
            if(c=='\n'){
                if(n<VIEW_CAP)file_data[n++]='\n';
                line++;x=0;
                if(indexed)index_line(line,byte_offset);
                view_lines++;
                continue;
            }
            if(c=='\r')continue;
            if(c=='\t'){
                int spaces=4-(x&3);
                while(spaces--){
                    if(x<cols-1 && n<VIEW_CAP){file_data[n++]=' ';x++;}
                }
            } else if(x<cols-1 && n<VIEW_CAP){
                file_data[n++]=c>=32&&c<127?(char)c:'.';
                x++;
            }
        }
    }
    close(fd);
    if(error)return -1;
    if(n>0 && file_data[n-1]!='\n')view_lines++;
    file_data[n]=0;
    return n;
}

/* 現在のファイルページを端末へ描画する． */
static void paint_view(int len,int error){
    char title[112];clipped(title,path,cols-15);
    printf("\033[?25l\033[H\033[2J\033[7m PicoNIX fd: %s\033[K\033[0m",title);
    screen_move(2,1);
    printf(" Up/Down: scroll   PgUp/PgDn: page   e: edit   q/Esc: back\033[K");
    int y=3;
    if(error){screen_move(y,1);printf(" Cannot read file\033[K");}
    else {
        int start=0;
        for(int i=0;i<=len;i++){
            if(i==len || file_data[i]=='\n'){
                if(i>start || i<len){
                    char saved=file_data[i];file_data[i]=0;
                    screen_move(y++,1);printf("%s\033[K",file_data+start);
                    file_data[i]=saved;
                } else if(i==len && len==0) { /* 空ファイルまたはEOFを越えた位置． */ }
                start=i+1;
            }
        }
    }
    while(y<rows-1){screen_move(y++,1);printf("\033[K");}
    screen_move(rows-1,1);
    printf(" Line %d%s   (%d lines shown)\033[K",view_top+1,view_more?" +":" (end)",view_lines);
    screen_move(rows,1);printf(" Live refresh: 1 s   Read-only; /dev is not previewed\033[K");
}

/* ファイルの表示領域を読み直す．固定長バッファ内だけを表示に使う． */
static void refresh_view(void) {
    int n=view_load();
    int err=n<0;
    if(n!=old_len||err!=old_error||view_top!=old_top||view_more!=old_more||
       (!err&&memcmp(file_data,old_data,(size_t)n))) {
        paint_view(n,err);
        if(!err)memcpy(old_data,file_data,(size_t)n);
        old_len=n;old_error=err;old_top=view_top;old_more=view_more;
    }
}

/* 選択したRAMファイルに対する編集コマンドを呼び出す． */
static int edit_selected(void){
    const char *edit_path=mode_view?path:0;
    if(!edit_path) {
        if(!count||entries[selected].dir||join(current,entries[selected].name,path)<0)return -1;
        edit_path=path;
    }
    if((strncmp(edit_path,"/tmp/",5)&&strncmp(edit_path,"/etc/",5))||
       !edit_path[5]||strchr(edit_path+5,'/'))return -1;
    int editor=program_id("edit");
    if(editor<0)return -1;
    /* 子は標準入力のフラグを継承する．fdはイベント処理でO_NONBLOCKを使うが，editにはブロッキングreadが必要なので起動前に解除する． */
    if(fcntl(0,F_SETFL,original_flags & ~O_NONBLOCK)<0)return -1;
    screen_leave();
    const char *args[]={"edit",edit_path};
    int pid=spawn_foreground(editor,2,args);
    if(pid>=0){
        int status=0;
        waitpid(pid,&status,0);
        set_foreground(0);
    } else notice="Cannot start editor";
    /* 復帰後はブラウザのraw・ノンブロッキングモードへ戻す． */
    ioctl(0,OS_TTY_SET_CANON,0);
    ioctl(0,OS_TTY_SET_ECHO,0);
    fcntl(0,F_SETFL,original_flags|O_NONBLOCK);
    screen_enter();
    old_len=-1;
    if(mode_view)refresh_view();
    else {refresh_dir();if(pid<0)notice="Cannot start editor";draw_dir();}
    return pid<0?-1:0;
}

/* キー入力からディレクトリ移動・閲覧等の操作を選択する． */
static int key_event(int c,int *state){
    if(*state==0){if(c==27){*state=1;return 0;}return c;}
    if(*state==1){*state=0;if(c=='['){*state=2;return 0;}return 27;}
    if(*state==3||*state==4){int pg=*state;*state=0;return c=='~'?(pg==3?1005:1006):0;}
    *state=0;
    if(c=='A')return 1001;
    if(c=='B')return 1002;
    if(c=='C')return 1003;
    if(c=='D')return 1004;
    if(c=='5'){*state=3;return 0;}
    if(c=='6'){*state=4;return 0;}
    return 0;
}

/* 端末を設定してファイルブラウザの操作ループを実行する． */
int main(int argc,char **argv){
    if(argc>2){puts("usage: fd [directory]");return 1;}
    if(argc==2){
        if(argv[1][0]=='/'){
            if(strlen(argv[1])>=PATH_CAP){puts("fd: path too long");return 1;}
            strcpy(current,argv[1]);
        } else {
            char cwd[PATH_CAP];
            if(!getcwd(cwd,sizeof cwd)||join(cwd,argv[1],current)<0){puts("fd: invalid path");return 1;}
        }
        while(strlen(current)>1&&current[strlen(current)-1]=='/')current[strlen(current)-1]=0;
    }
    original_canon=ioctl(0,OS_TTY_GET_CANON,0);
    original_echo=ioctl(0,OS_TTY_GET_ECHO,0);
    original_flags=fcntl(0,F_GETFL,0);
    if(original_canon<0||original_echo<0||original_flags<0||
       ioctl(0,OS_TTY_SET_CANON,0)<0||ioctl(0,OS_TTY_SET_ECHO,0)<0||
       fcntl(0,F_SETFL,original_flags|O_NONBLOCK)<0) {
        if(original_flags>=0)fcntl(0,F_SETFL,original_flags);
        if(original_echo>=0)ioctl(0,OS_TTY_SET_ECHO,original_echo);
        if(original_canon>=0)ioctl(0,OS_TTY_SET_CANON,original_canon);
        puts("fd: terminal required");return 1;
    }
    screen_size_t size={24,80};
    screen_measure(&size);
    rows=size.rows>40?40:size.rows;
    cols=size.cols>110?110:size.cols;
    if(rows<MIN_ROWS)rows=MIN_ROWS;
    if(cols<40)cols=40;
    screen_enter();
    refresh_dir();draw_dir();
    int state=0, idle=0, active=1;
    while(active){
        char c;
        int n=read(0,&c,1);
        if(n==OS_EAGAIN||n==0){
            msleep(20);
            if(state==1&&++idle>=3){state=0;if(mode_view){mode_view=0;draw_dir();}else {parent_dir();refresh_dir();draw_dir();}}
            else if(n==OS_EAGAIN||n==0){
                static unsigned elapsed;
                if(mode_view&&++elapsed>=50){elapsed=0;refresh_view();}
            }
            continue;
        }
        idle=0;
        if(n<0)break;
        int key=key_event((unsigned char)c,&state);
        if(!key)continue;
        if(key==3||key=='Q'){active=0;break;}
        if(mode_view){
            if(key==1001 && view_top>0){view_top--;refresh_view();continue;}
            if(key==1002 && (view_more || view_lines>1)){
                if(view_more){view_top++;refresh_view();}
                continue;
            }
            if(key==1005 && view_top>0){
                view_top-=view_lines?view_lines:1;
                if(view_top<0)view_top=0;
                refresh_view();continue;
            }
            if(key==1006 && view_more){
                view_top+=view_lines?view_lines:1;
                refresh_view();continue;
            }
            if(key=='q' ||key==27||key==8||key==127){mode_view=0;draw_dir();}
            else if(key=='e'){if(edit_selected()<0){/* ファイル閲覧画面を表示したままにする． */}}
            else if(key=='r'){reset_view_index();old_len=-1;refresh_view();}
            continue;
        }
        if(key=='q'){active=0;break;}
        if(key=='r'){refresh_dir();draw_dir();continue;}
        if(key==8||key==127||key==27||key==1004){parent_dir();refresh_dir();draw_dir();continue;}
        if(key==1001&&selected>0){selected--;draw_dir();continue;}
        if(key==1002&&selected+1<count){selected++;draw_dir();continue;}
        if(key=='e'){if(edit_selected()<0){notice="Only /tmp or /etc files can be edited";draw_dir();}continue;}
        if(key=='\r'||key=='\n'||key==1003){
            if(count==0)continue;
            if(join(current,entries[selected].name,path)<0){notice="Path too long";draw_dir();continue;}
            if(entries[selected].dir){
                if(!strcmp(entries[selected].name,"..")){parent_dir();refresh_dir();draw_dir();continue;}
                char previous[PATH_CAP];strcpy(previous,current);
                strcpy(current,path);refresh_dir();
                if(*notice){strcpy(current,previous);refresh_dir();notice="Cannot open directory";}
                draw_dir();
            } else if(safe_view(path)){
                mode_view=1;view_top=0;reset_view_index();old_len=-1;old_error=-1;refresh_view();
            } else {notice="Preview: /tmp, /etc, /proc only";draw_dir();}
        }
    }
    screen_leave();
    fcntl(0,F_SETFL,original_flags);
    ioctl(0,OS_TTY_SET_ECHO,original_echo);
    ioctl(0,OS_TTY_SET_CANON,original_canon);
    return 0;
}
