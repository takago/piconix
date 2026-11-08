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
#define FILTER_CAP 32
#define TEXT_FIND_CAP 32
#define PATH_CAP 128
#define VIEW_CAP 4096 /* 40x110 terminal: 35 body rows x 109 chars < 4096 */
#define MIN_ROWS 12
#define INDEX_STRIDE 16
#define INDEX_CAP 128

typedef struct { char name[32]; unsigned char dir; } entry_t;
static entry_t entries[MAX_ENTRIES];
static char current[PATH_CAP];
static char path[PATH_CAP];
static char file_data[VIEW_CAP+1];
/* 前画面は全文を複製せず，軽量な指紋で内容の変化を判定する． */
static unsigned old_view_hash;
static int count, selected, scroll, total_entries, list_offset, rows=24, cols=80;
static char filter[FILTER_CAP], filter_saved[FILTER_CAP];
static int searching;
static char text_find[TEXT_FIND_CAP], text_saved[TEXT_FIND_CAP];
static int text_searching, found_line=-1;
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

/* ASCII大文字・小文字を区別せず，名前の一部との一致を判定する． */
static int name_matches(const char *name) {
    if(!filter[0])return 1;
    for(int i=0;name[i];i++){
        int j=0;
        while(filter[j]&&name[i+j]){
            char a=name[i+j],b=filter[j];
            if(a>='A'&&a<='Z')a=(char)(a+('a'-'A'));
            if(b>='A'&&b<='Z')b=(char)(b+('a'-'A'));
            if(a!=b)break;
            j++;
        }
        if(!filter[j])return 1;
    }
    return 0;
}

/* ASCII大文字・小文字を区別せずに文字を比較する． */
static unsigned char lower_ascii(unsigned char c) {
    return c>='A' && c<='Z' ? (unsigned char)(c+('a'-'A')) : c;
}

/* テキストを逐次走査して検索する．全体をRAMへ展開しない．
 * direction=1 は現在行から前方，-1 は現在行より前方の最後の一致を探す．
 * 最後まで一致しなければ先頭へ折り返す．検索は行単位の部分一致． */
static int find_text_line(int from,int direction) {
    int len=(int)strlen(text_find);
    if(!len)return -1;
    int fd=open(path,O_RDONLY);
    if(fd<0)return -1;
    char block[128], window[TEXT_FIND_CAP];
    int line=0, used=0, first=-1, last=-1, first_after=-1, last_before=-1;
    int n;
    while((n=read(fd,block,sizeof(block)))>0) {
        for(int i=0;i<n;i++) {
            unsigned char c=(unsigned char)block[i];
            if(c=='\n') {line++;used=0;continue;}
            if(c=='\r')continue;
            if(used<len)window[used++]=(char)lower_ascii(c);
            else {
                for(int j=1;j<len;j++)window[j-1]=window[j];
                window[len-1]=(char)lower_ascii(c);
            }
            if(used==len) {
                int ok=1;
                for(int j=0;j<len;j++)
                    if(window[j]!=lower_ascii((unsigned char)text_find[j])){ok=0;break;}
                if(ok) {
                    if(first<0)first=line;
                    last=line;
                    if(line>=from && first_after<0)first_after=line;
                    if(line<from)last_before=line;
                    /* 同一行の一致を重複して記録しても結果は変わらない． */
                }
            }
        }
    }
    close(fd);
    if(n<0)return -1;
    if(direction>0)return first_after>=0?first_after:first;
    return last_before>=0?last_before:last;
}

static void refresh_view(void);

/* ビューア内の検索結果へ移動する．見つからない場合は現在位置を保持する． */
static void go_to_match(int from,int direction) {
    int line=find_text_line(from,direction);
    if(line>=0){view_top=line;found_line=line;old_len=-1;}
    else found_line=-1;
    refresh_view();
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
        if(!strcmp(e.name,"..") && !strcmp(current,"/"))
            continue;
        if(filter[0] && (!strcmp(e.name,"..") || !name_matches(e.name)))continue;
        if(total_entries++ < list_offset)continue;
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
static void refresh_dir(void){
    count=0;selected=0;scroll=0;notice="";
    if(directory_load()<0){notice="Cannot open directory";return;}
    if(!count && list_offset>0){list_offset=0;directory_load();}
}

/* ファイル一覧と選択位置を端末画面へ表示する． */
static void draw_dir(void) {
    char title[112];clipped(title,current,cols-15);
    printf("\033[?25l\033[H\033[2J\033[7m PicoNIX fd  %s\033[K\033[0m",title);
    screen_move(2,1);printf(" Enter: open  /: find  e: edit  Backspace: parent  q: quit\033[K");
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
    if(total_entries>MAX_ENTRIES || list_offset)
        printf(" %d-%d / %d entries (Up/Down: next page)\033[K",count?list_offset+1:0,list_offset+count,total_entries);
    else printf(" %d entries\033[K",count);
    screen_move(rows,1);
    if(searching)printf(" Find: %s_  (Enter: apply  Esc: cancel)\033[K",filter);
    else if(filter[0])printf(" Find: %s  (/: edit  c: clear)\033[K",filter);
    else printf(" %s\033[K",notice);
}

/* 既知のテキスト系ファイルだけを閲覧する．/devのreadは消費的・待機的・バイナリの場合があるため実行しない． */
static int safe_view(const char *p) {
    return !strncmp(p,"/tmp/",5) || !strncmp(p,"/etc/",5) ||
           !strncmp(p,"/proc/",6) || !strncmp(p,"/man/",5);
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
    int indexed=(!strcmp(path,"/proc/sym/kernel") || !strncmp(path,"/man/",5)) && indexed_lines>0;
    if(indexed) {
        /* 古いカーネルはseekに未対応の場合があるので，実際に試して判定する． */
        if(lseek(fd,0,SEEK_CUR)<0)
            indexed=0;
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

/* 表示行の検索語だけを反転表示する． */
static void print_matched_line(char *line) {
    int len=(int)strlen(text_find);
    if(!len){printf("%s",line);return;}
    char *start=line, *p=line;
    while(*p) {
        int j=0;
        while(j<len && p[j] && lower_ascii((unsigned char)p[j])==
              lower_ascii((unsigned char)text_find[j]))j++;
        if(j==len) {
            char saved=*p;*p=0;printf("%s",start);*p=saved;
            saved=p[len];p[len]=0;
            printf("\033[7m%s\033[0m",p);
            p[len]=saved;p+=len;start=p;
        } else p++;
    }
    printf("%s",start);
}

/* 現在のファイルページを端末へ描画する． */
static void paint_view(int len,int error){
    char title[112];clipped(title,path,cols-15);
    printf("\033[?25l\033[H\033[2J\033[7m PicoNIX fd: %s\033[K\033[0m",title);
    screen_move(2,1);
    printf(" Up/Down: scroll  /: search  n/N: next/prev  q/Esc: back\033[K");
    int y=3;
    if(error){screen_move(y,1);printf(" Cannot read file\033[K");}
    else {
        int start=0;
        for(int i=0;i<=len;i++){
            if(i==len || file_data[i]=='\n'){
                if(i>start || i<len){
                    char saved=file_data[i];file_data[i]=0;
                    screen_move(y++,1);print_matched_line(file_data+start);printf("\033[K");
                    file_data[i]=saved;
                } else if(i==len && len==0) { /* 空ファイルまたはEOFを越えた位置． */ }
                start=i+1;
            }
        }
    }
    while(y<rows-1){screen_move(y++,1);printf("\033[K");}
    screen_move(rows-1,1);
    printf(" Line %d%s   (%d lines shown)\033[K",view_top+1,view_more?" +":" (end)",view_lines);
    screen_move(rows,1);
    if(text_searching)printf(" Search text: %s_  (Enter: find  Esc: cancel)\033[K",text_find);
    else if(text_find[0])printf(" Search: %s   %s   (n: next  N: previous)\033[K",text_find,
                                found_line>=0?"match":"not found");
    else if(!strncmp(path,"/man/",5))printf(" ROM manual (read-only)\033[K");
    else printf(" Live refresh: 1 s   Read-only; /dev not previewed\033[K");
}

/* 画面本文のFNV-1a指紋．再描画判定のためであり，内容保持用ではない． */
static unsigned view_hash(int n) {
    unsigned hash=2166136261u;
    for(int i=0;i<n;i++) {
        hash^=(unsigned char)file_data[i];
        hash*=16777619u;
    }
    return hash;
}

/* ファイルの表示領域を読み直す．前画面の複製を持たずに再描画を抑制する． */
static void refresh_view(void) {
    int n=view_load();
    int err=n<0;
    unsigned hash=err?0:view_hash(n);
    if(n!=old_len||err!=old_error||view_top!=old_top||view_more!=old_more||
       (!err&&hash!=old_view_hash)) {
        paint_view(n,err);
        old_view_hash=hash;
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
    if(argc==1){
        if(!getcwd(current,sizeof current)){puts("fd: cannot get cwd");return 1;}
    } else {
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
            if(state==1&&++idle>=3){
                state=0;
                if(text_searching){strcpy(text_find,text_saved);text_searching=0;old_len=-1;refresh_view();}
                else if(searching){strcpy(filter,filter_saved);searching=0;list_offset=0;refresh_dir();draw_dir();}
                else if(mode_view){mode_view=0;draw_dir();}
                else {parent_dir();list_offset=0;filter[0]=0;refresh_dir();draw_dir();}
            }
            else if(n==OS_EAGAIN||n==0){
                static unsigned elapsed;
                if(mode_view && !text_searching && strncmp(path,"/man/",5) && ++elapsed>=50){elapsed=0;refresh_view();}
            }
            continue;
        }
        idle=0;
        if(n<0)break;
        int key=key_event((unsigned char)c,&state);
        if(!key)continue;
        if(key==3||(!searching && !text_searching && !mode_view && key=='Q')){active=0;break;}
        if(text_searching){
            if(key==27){strcpy(text_find,text_saved);text_searching=0;old_len=-1;refresh_view();}
            else if(key=='\r'||key=='\n'){
                text_searching=0;
                if(text_find[0])go_to_match(view_top,1);
                else {found_line=-1;old_len=-1;refresh_view();}
            } else if(key==8||key==127){int k=(int)strlen(text_find);if(k>0){text_find[k-1]=0;old_len=-1;refresh_view();}}
            else if(key>=32&&key<=126 && strlen(text_find)<TEXT_FIND_CAP-1){int k=(int)strlen(text_find);text_find[k]=(char)key;text_find[k+1]=0;old_len=-1;refresh_view();}
            continue;
        }
        if(searching){
            if(key==27){strcpy(filter,filter_saved);searching=0;list_offset=0;refresh_dir();draw_dir();}
            else if(key=='\r'||key=='\n'){searching=0;list_offset=0;refresh_dir();draw_dir();}
            else if(key==8||key==127){int n=(int)strlen(filter);if(n>0){filter[n-1]=0;list_offset=0;refresh_dir();draw_dir();}}
            else if(key>=32&&key<=126 && strlen(filter)<FILTER_CAP-1){int n=(int)strlen(filter);filter[n]=(char)key;filter[n+1]=0;list_offset=0;refresh_dir();draw_dir();}
            continue;
        }
        if(mode_view){
            if(key=='/') {strcpy(text_saved,text_find);text_searching=1;old_len=-1;refresh_view();continue;}
            if(key=='n'&&text_find[0]){go_to_match(view_top+1,1);continue;}
            if(key=='N'&&text_find[0]){go_to_match(view_top,-1);continue;}
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
        if(key=='/'){strcpy(filter_saved,filter);searching=1;draw_dir();continue;}
        if(key=='c' && filter[0]){filter[0]=0;list_offset=0;refresh_dir();draw_dir();continue;}
        if(key=='r'){refresh_dir();draw_dir();continue;}
        if(key==8||key==127||key==27||key==1004){parent_dir();list_offset=0;filter[0]=0;refresh_dir();draw_dir();continue;}
        if(key==1001){
            if(selected>0)selected--;
            else if(list_offset>0){list_offset-=MAX_ENTRIES;if(list_offset<0)list_offset=0;refresh_dir();selected=count?count-1:0;}
            draw_dir();continue;
        }
        if(key==1002){
            if(selected+1<count)selected++;
            else if(list_offset+count<total_entries){list_offset+=count;refresh_dir();}
            draw_dir();continue;
        }
        if(key=='e'){if(edit_selected()<0){notice="Only /tmp or /etc files can be edited";draw_dir();}continue;}
        if(key=='\r'||key=='\n'||key==1003){
            if(count==0)continue;
            if(join(current,entries[selected].name,path)<0){notice="Path too long";draw_dir();continue;}
            if(entries[selected].dir){
                if(!strcmp(entries[selected].name,"..")){parent_dir();list_offset=0;filter[0]=0;refresh_dir();draw_dir();continue;}
                char previous[PATH_CAP];strcpy(previous,current);
                strcpy(current,path);list_offset=0;filter[0]=0;refresh_dir();
                if(*notice){strcpy(current,previous);list_offset=0;refresh_dir();notice="Cannot open directory";}
                draw_dir();
            } else if(safe_view(path)){
                mode_view=1;view_top=0;text_find[0]=0;text_searching=0;found_line=-1;
                reset_view_index();old_len=-1;old_error=-1;refresh_view();
            } else {notice="Preview: /tmp, /etc, /proc, /man only";draw_dir();}
        }
    }
    screen_leave();
    fcntl(0,F_SETFL,original_flags);
    ioctl(0,OS_TTY_SET_ECHO,original_echo);
    ioctl(0,OS_TTY_SET_CANON,original_canon);
    return 0;
}
