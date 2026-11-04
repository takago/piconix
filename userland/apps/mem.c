/* SPDX-License-Identifier: GPL-3.0-or-later */
/* 物理メモリとタスクスロットを閲覧する学習用モニタ．
 * カーネル／ユーザのシンボル情報を読み，アドレス表示と対応付ける． */

/* 実メモリを読み出すモニタ．Flashに埋め込まれたシンボル名も必要に応じて表示する． */
#include "unistd.h"
#include "fcntl.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include <stdint.h>
#include "../lib/screen.h"
/* 24 KiBのタスク枠内に収まるよう，表示データは画面程度に制限する．6行は見出し・操作へ割り当て，広い端末でも最大36データ行にする． */
#define MAX_ROWS 36
#define BYTES_PER_ROW 16
#define MAX_COUNT (MAX_ROWS*BYTES_PER_ROW)
static unsigned view_rows = 16;
static unsigned view_count = 16*BYTES_PER_ROW;
static unsigned goto_row = 21;
/* カーネル・タスク・共有RAM・ファイル・トレース・ネットワーク領域を表示用に区分する． */
static void mem_layout(int rows) {
 if(rows<8) rows=24;
 view_rows=(unsigned)(rows-6);
 if(view_rows>MAX_ROWS)view_rows=MAX_ROWS;
 view_count=view_rows*BYTES_PER_ROW;
 goto_row=view_rows+5;
}
#define SYMBOL_NAME_MAX 128
static unsigned char previous[MAX_COUNT],current[MAX_COUNT];
static char labels[MAX_ROWS][SYMBOL_NAME_MAX];
static char symbol_app[96];
static int symbols_available;
static char slot_names[6][20];
/* 読み出し対象アドレスが公開されたSRAMまたはXIP Flash範囲にあるか確認する． */
static int valid(unsigned a){return (a>=0x20000000u&&a<=0x2003ffffu)||(a>=0x10000000u&&a<=0x101fffffu);}

/* 指定桁数の16進数を標準出力へ表示する． */
static void hex(unsigned v,int digits){const char *h="0123456789ABCDEF";for(int i=(digits-1)*4;i>=0;i-=4)putchar(h[(v>>i)&15]);}

/* /dev/memにseekして表示対象領域を取得する．範囲外・短い読出しは失敗とする． */
static int fetch(int fd,unsigned address){if(!valid(address)||!valid(address+view_count-1))return -1;if(lseek(fd,(long)address,SEEK_SET)<0)return -1;return read(fd,current,view_count)==(int)view_count?0:-1;}

/* 物理アドレスをカーネル，タスク，RAMファイル等の表示領域名に対応付ける． */
static const char *region(unsigned addr){
 if(addr>=0x10000000u&&addr<=0x101fffffu)return "XIP FLASH";
 if(addr<0x2000e000u)return "KERNEL SRAM";
 if(addr<0x20032000u)return "TASK SRAM";
 if(addr<0x20032800u)return "SHARED RAM";
 if(addr<0x20038000u)return "RAM FILES";
 if(addr<0x2003c000u)return "TRACE SRAM";
 if(addr<=0x2003ffffu)return "NETWORK SRAM";
 return "UNKNOWN";
}

/* シンボル表の形式は「アプリ名 TAB 相対16進アドレス TAB 種別 TAB 名前 LF」である．各イメージは0x2000E000基準でリンクされ，オフセットを実スロット位置へ加える．/proc/symから読み，ELFを直接解析しない． */
static int symbol_record(char *line,char **app,unsigned *offset,char *kind,char **name){
 char *a=strchr(line,'\t');if(!a)return 0;*a++=0;
 char *b=strchr(a,'\t');if(!b)return 0;*b++=0;
 char *c=strchr(b,'\t');if(!c||b+1!=c)return 0;*c++=0;
 char *end;long v=strtol(a,&end,16);
 if(!*a||*end||v<0||!*c)return 0;
 if(!strcmp(line,"kernel")){
  if(!((unsigned)v>=0x10000000u&&(unsigned)v<0x10200000u) &&
     !((unsigned)v>=0x20000000u&&(unsigned)v<0x2000e000u))return 0;
 }else if((unsigned)v>=0x6000u)return 0;
 *app=line;*offset=(unsigned)v;*kind=*b;*name=c;return 1;
}

/* シンボル表からアドレスと名前を読み，現在の表示位置と関連付ける． */
static int scan_symbols(const char *selected,unsigned base,int list_mode,unsigned slot_hint){
 char path[128];
 if(selected) {
  if(!strcmp(selected,"kernel")) strcpy(path,"/proc/sym/kernel");
  else snprintf(path,sizeof path,"/proc/sym/userland/%s",selected);
 } else return -1;
 int fd=open(path,O_RDONLY);
 if(fd<0)return -1;
 char buf[128];unsigned used=0;int overflow=0,found=0,read_error=0;
 unsigned char chunk[128];int n;
 while((n=read(fd,chunk,sizeof chunk))>0){
  for(int i=0;i<n;i++){
   char c=(char)chunk[i];
   if(c!='\n'){
    if(used+1<sizeof buf&&!overflow)buf[used++]=c;
    else overflow=1;
    continue;
   }
   if(!overflow){
    buf[used]=0;char *app,*name,kind;unsigned offset;
    if(symbol_record(buf,&app,&offset,&kind,&name)){
     if(selected&&!strcmp(app,selected)){
      found++;
      if(list_mode==2){hex(offset,4);printf("  %c  %s\n",kind,name);}
      else if(!strcmp(app,"kernel")){
       unsigned actual=offset;
       if(actual>=base&&actual-base<view_count){
        unsigned row=(actual-base)/BYTES_PER_ROW;
        unsigned pos=(unsigned)strlen(labels[row]), len=(unsigned)strlen(name);
        unsigned sep=pos?2u:0u;
        if(pos+sep+len<SYMBOL_NAME_MAX){
         if(sep){labels[row][pos++]=',';labels[row][pos++]=' ';}
         memcpy(labels[row]+pos,name,len+1);
        }else if(pos+sep+3<SYMBOL_NAME_MAX){
         if(sep){labels[row][pos++]=',';labels[row][pos++]=' ';}
         memcpy(labels[row]+pos,"...",4);
        }
       }
      }
      else if(slot_hint>=0x2000e000u&&slot_hint<0x20032000u){
       /* タスクは6個の24 KiBスロットのいずれかで動作する． */
       unsigned slot_base=0x2000e000u+((slot_hint-0x2000e000u)/0x6000u)*0x6000u;
       unsigned actual=slot_base+offset;
       if(actual>=base&&actual<base+view_count){
        unsigned row=(actual-base)/BYTES_PER_ROW;
        /* 同じ16バイト行のシンボルをカタログ順でまとめる．長すぎるときは省略記号用の領域を残す． */
        unsigned pos=(unsigned)strlen(labels[row]);
        unsigned len=(unsigned)strlen(name);
        unsigned sep=pos?2u:0u;
        if(pos+sep+len<SYMBOL_NAME_MAX){
         if(sep){labels[row][pos++]=',';labels[row][pos++]=' ';}
         memcpy(labels[row]+pos,name,len+1);
        }else if(pos+sep+3<SYMBOL_NAME_MAX){
         if(sep){labels[row][pos++]=',';labels[row][pos++]=' ';}
         labels[row][pos++]='.';labels[row][pos++]='.';labels[row][pos++]='.';
         labels[row][pos]=0;
        }
       }
      }
     }
    }
   }
   used=0;overflow=0;
  }
 }
 if(n<0)read_error=1;
 close(fd);
 return read_error?-1:found;
}

/* /proc/tasksを現在の各RAMスロットの情報源とする．空白区切りで解析し，取得に失敗してもスロット内容を推測しない． */
static int task_program(unsigned addr,char *name,unsigned capacity){
 if(addr<0x2000e000u||addr>=0x20032000u)return 0;
 unsigned slot=(addr-0x2000e000u)/0x6000u+1;
 int fd=open("/proc/tasks",O_RDONLY);
 if(fd<0)return 0;
 char line[160];unsigned used=0;int found=0,overflow=0;
 char chunk[96];int n;
 while((n=read(fd,chunk,sizeof chunk))>0&&!found){
  for(int i=0;i<n;i++){
   char c=chunk[i];
   if(c!='\n'){
    if(!overflow&&used+1<sizeof line)line[used++]=c;
    else overflow=1;
    continue;
   }
   if(!overflow){
    line[used]=0;
    char *fields[5];int nf=0;
    char *q=line;
    while(*q&&nf<5){
     while(*q==' '||*q=='\t')q++;
     if(!*q)break;
     fields[nf++]=q;
     while(*q&&*q!=' '&&*q!='\t')q++;
     if(*q)*q++=0;
    }
    if(nf==5){
     char *end;long number=strtol(fields[4],&end,10);
     if(!*end&&number==(long)slot&&strcmp(fields[1],"NAME")){
      const char *app=fields[1];
      if(!strcmp(app,"shell"))app="sh";
      if(!strcmp(app,"asm"))app="as";
      strncpy(name,app,capacity-1);name[capacity-1]=0;
      found=1;break;
     }
    }
   }
   used=0;overflow=0;
  }
 }
 close(fd);return found;
}

/* /procから生存タスクのスロット情報を取得して表示へ反映する． */
static int refresh_slots(void){
 char next[6][20]={{0}};
 int fd=open("/proc/tasks",O_RDONLY);
 if(fd<0)return 0;
 char line[160];unsigned used=0;int overflow=0;
 char chunk[96];int n;
 while((n=read(fd,chunk,sizeof chunk))>0){
  for(int i=0;i<n;i++){
   char c=chunk[i];
   if(c!='\n'){
    if(!overflow&&used+1<sizeof line)line[used++]=c;
    else overflow=1;
    continue;
   }
   if(!overflow){
    line[used]=0;
    char *fields[5];int nf=0;char *q=line;
    while(*q&&nf<5){
     while(*q==' '||*q=='\t')q++;
     if(!*q)break;
     fields[nf++]=q;
     while(*q&&*q!=' '&&*q!='\t')q++;
     if(*q)*q++=0;
    }
    if(nf==5&&strcmp(fields[0],"PID")){
     char *end;long slot=strtol(fields[4],&end,10);
     if(!*end&&slot>=1&&slot<=6){
      const char *app=fields[1];
      if(!strcmp(app,"shell"))app="sh";
      if(!strcmp(app,"asm"))app="as";
      strncpy(next[slot-1],app,sizeof next[0]-1);
     }
    }
   }
   used=0;overflow=0;
  }
 }
 close(fd);
 if(n<0)return 0;
 int changed=memcmp(slot_names,next,sizeof next)!=0;
 memcpy(slot_names,next,sizeof next);
 return changed;
}

/* 利用中スロットのプログラム名から表示ラベルを更新する．終了・置換も検出し，呼出側が任意の名前を与えられないようにする． */
static void append_app(char *list,unsigned cap,const char *name){
 if(!*name)return;
 /* 隣接領域に同じ名前があっても二重に連結しない． */
 const char *p=list;
 while(*p){
  const char *e=strchr(p,',');unsigned len=e?(unsigned)(e-p):(unsigned)strlen(p);
  if(len==strlen(name)&&!strncmp(p,name,len))return;
  if(!e)break;
  p=e+2;
 }
 unsigned used=(unsigned)strlen(list), len=(unsigned)strlen(name);
 if(used+len+(used?2u:0u)>=cap)return;
 if(used){list[used++]=',';list[used++]=' ';}
 memcpy(list+used,name,len+1);
}

/* 現在存在するRAMファイルの場所と名前を表示へ反映する． */
static void refresh_ram_labels(unsigned addr) {
 int fd=open("/proc/ramfiles",O_RDONLY);
 if(fd<0)return;
 char line[80];unsigned used=0;char c;
 while(read(fd,&c,1)==1){
  if(c!='\n') {if(used+1<sizeof line)line[used++]=c;continue;}
  line[used]=0;used=0;
  if(strlen(line)<10 || line[8]!=' ')continue;
  unsigned a=0;int ok=1;
  for(int i=0;i<8;i++){
   char x=line[i];unsigned d=x>='0'&&x<='9'?(unsigned)(x-'0'):
      x>='A'&&x<='F'?(unsigned)(x-'A'+10):x>='a'&&x<='f'?(unsigned)(x-'a'+10):16u;
   if(d>15){ok=0;break;}a=(a<<4)|d;
  }
  if(!ok || a<addr || a-addr>=view_count)continue;
  unsigned row=(a-addr)/BYTES_PER_ROW;
  char *dst=labels[row];size_t n=strlen(dst);
  if(n+2<sizeof labels[row] && n){dst[n++]=',';dst[n++]=' ';dst[n]=0;}
  snprintf(dst+n,sizeof labels[row]-n,"RAM_FILE: /%s",line+9);
 }
 close(fd);
}

/* メモリアドレスと各領域の名称を再計算する． */
static int refresh_labels(unsigned addr){
 char old_app[sizeof symbol_app];
 /* 4 KiBを超える一時スタック配列を作らず，前回のラベルをハッシュ化する． */
 unsigned old_hash=2166136261u;
 for(unsigned r=0;r<view_rows;r++){
  const unsigned char *p=(const unsigned char *)labels[r];
  while(*p){old_hash=(old_hash^*p++)*16777619u;}
  old_hash=(old_hash^0xffu)*16777619u;
 }
 memcpy(old_app,symbol_app,sizeof old_app);
 int old_available=symbols_available;
 memset(labels,0,sizeof labels);
 symbol_app[0]=0;symbols_available=0;
 /* 表示窓はタスクスロットの境界をまたぐ場合がある． */
 for(unsigned off=0;off<view_count;off+=BYTES_PER_ROW){
  unsigned a=addr+off;
  char app[32]={0};
  if(a<0x2000e000u || (a>=0x10000000u&&a<0x10200000u)){
   strncpy(app,"kernel",sizeof app-1);
  }else task_program(a,app,sizeof app);
  if(!app[0])continue;
  /* 同じ表示窓にある同一アプリのシンボルを重複走査しない． */
  int already=0;
  const char *list=symbol_app;
  while(*list){
   const char *end=strchr(list,',');unsigned len=end?(unsigned)(end-list):(unsigned)strlen(list);
   if(len==strlen(app)&&!strncmp(list,app,len)){already=1;break;}
   if(!end)break;
   list=end+2;
  }
  if(already)continue;
  append_app(symbol_app,sizeof symbol_app,app);
  int n=scan_symbols(app,addr,0,a);
  if(n>0)symbols_available=1;
 }
 refresh_ram_labels(addr);
 unsigned new_hash=2166136261u;
 for(unsigned r=0;r<view_rows;r++){
  const unsigned char *s=(const unsigned char *)labels[r];
  while(*s){new_hash=(new_hash^*s++)*16777619u;}
  new_hash=(new_hash^0xffu)*16777619u;
 }
 return old_hash!=new_hash || strcmp(old_app,symbol_app)!=0 ||
        old_available!=symbols_available;
}

/* アドレス範囲と内容，タスク・シンボルを端末の大きさに合わせて描画する． */
static void draw(unsigned addr,int pause,int have,int changed){
 /* 描画中だけ自動折返しを無効にし，入力前に復元する． */
 printf("\033[?7l");
 screen_move(1,1);
 printf("\033[7mPicoNIX mem   %s   %s\033[K\033[0m",region(addr),pause?"PAUSED":"LIVE");
 screen_move(2,1);
 printf("Address 0x");hex(addr,8);
 printf("  %u bytes  16 bytes/row\033[K",view_count);
 screen_move(3,1);
 printf("          00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F   ASCII            SYMBOL: %s\033[K",symbol_app[0]?symbol_app:"-");
 for(unsigned r=0;r<view_rows;r++){
  screen_move((int)r+4,1);
  hex(addr+BYTES_PER_ROW*r,8);printf("  ");
  for(int i=0;i<BYTES_PER_ROW;i++){
   unsigned j=r*BYTES_PER_ROW+(unsigned)i;
   if(have&&changed&&previous[j]!=current[j])printf("\033[7m");
   hex(current[j],2);
   if(have&&changed&&previous[j]!=current[j])printf("\033[0m");
   putchar(' ');
  }
  putchar('|');
  for(int i=0;i<BYTES_PER_ROW;i++){
   unsigned char c=current[r*BYTES_PER_ROW+(unsigned)i];putchar(c>=32&&c<127?c:'.');
  }
  printf("| %s\033[K",labels[r]);
 }
 screen_move((int)view_rows+4,1);
 printf("\033[7mArrows: 16B  PgUp/Dn: %uB  SPACE Pause  [q] Quit\033[K\033[0m",view_count);
 screen_move((int)view_rows+5,1);
 printf("[k] Kernel RAM  [K] Kernel XIP  [1-6] Slots  [g] Goto  Symbols: %s\033[K",symbol_app[0]?(symbols_available?"loaded":"unavailable"):"none");
 screen_move((int)view_rows+6,1);
 printf("Slots:");
 for(int i=0;i<6;i++)printf(" %d:%s",i+1,slot_names[i][0]?slot_names[i]:"free");
 printf("\033[K\033[?7h");
}

/* 対話操作で入力されたアドレスや数値を解析する． */
static int parse(const char *s,unsigned *out){char *end;long n=strtol(s,&end,0);if(!*s||*end||n<0||!valid((unsigned)n))return -1;*out=(unsigned)n&~15u;return 0;}

/* 表示するメモリ先頭アドレスを指定量だけ移動する． */
static int shifted(unsigned addr,int delta,unsigned *next){
 unsigned n=addr+(unsigned)delta;
 if(!valid(n)||!valid(n+view_count-1))return 0;
 *next=n;return 1;
}

/* /proc/symの対象から選択可能なカーネル／アプリのシンボル表を列挙する． */
static int list_symbol_sources(void){
 puts("  kernel");
 int fd=open("/bin",O_RDONLY);
 if(fd<0)return -1;
 piconix_dirent_t ent;int count=1,n;
 while((n=getdents(fd,&ent,sizeof ent))>0){
  printf("  %s\n",ent.name);count++;
 }
 close(fd);
 return n<0?-1:count;
}

/* アドレス移動と表示切替を行う対話型メモリ閲覧を開始する． */
int main(int argc,char **argv){
 unsigned addr=0x20000000u;
 if(argc>=2&&!strcmp(argv[1],"-s")){
  if(argc>3){puts("usage: mem -s [app]");return 1;}
  int mode=argc==2?1:2;
  if(mode==2){
   if(!strcmp(argv[2],"kernel"))puts("Symbols: kernel (absolute addresses)");
   else printf("Symbols: /bin/%s (slot-relative offsets)\n",argv[2]);
  }
  int found=mode==1?list_symbol_sources():scan_symbols(argv[2],0,mode,0);
  if(found<=0){
   puts(found<0?"mem: symbol table unavailable":"mem: no symbols found");
   return 1;
  }
  return 0;
 }
 if(argc>2||(argc==2&&parse(argv[1],&addr)<0)){
  puts("usage: mem [address] | mem -s [app]");return 1;
 }
 int fd=open("/dev/mem",O_RDONLY);if(fd<0){puts("mem: cannot open /dev/mem");return 1;}
 if(!valid(addr+view_count-1)){puts("mem: invalid memory range");close(fd);return 1;}
 int canon=ioctl(0,OS_TTY_GET_CANON,0),echo=ioctl(0,OS_TTY_GET_ECHO,0),flags=fcntl(0,F_GETFL,0);
 if(canon<0||echo<0||flags<0){close(fd);return 1;}
 if(ioctl(0,OS_TTY_SET_CANON,0)<0||ioctl(0,OS_TTY_SET_ECHO,0)<0||fcntl(0,F_SETFL,flags|O_NONBLOCK)<0){
  ioctl(0,OS_TTY_SET_ECHO,echo);ioctl(0,OS_TTY_SET_CANON,canon);close(fd);return 1;
 }
 screen_enter();
 screen_size_t size={24,80};
 screen_measure(&size);
 mem_layout(size.rows);
 printf("\033[2J\033[H");
 if(!valid(addr+view_count-1)){screen_leave();close(fd);return 1;}
 int have=0,pause=0,esc=0,run=1,dirty=1;
 int goto_mode=0;char goto_buf[12];int goto_len=0;
 int ticks=25,highlighted=0;
 while(run){
  char c;while(read(0,&c,1)==1){
   if(goto_mode){
    if(c==27){goto_mode=0;dirty=1;continue;}
    if(c=='\r'||c=='\n'){
     if(goto_len){unsigned next;goto_buf[goto_len]=0;
      char input[14];snprintf(input,sizeof input,"0x%s",goto_buf);
      if(parse(input,&next)==0 && valid(next+view_count-1)){
       addr=next;have=0;dirty=1;
      }
     }
     goto_mode=0;goto_len=0;dirty=1;continue;
    }
    if(c==8||c==127){if(goto_len)goto_len--;}
    else if(((c>='0'&&c<='9')||(c>='a'&&c<='f')||(c>='A'&&c<='F')) && goto_len<8)
     goto_buf[goto_len++]=c;
    goto_buf[goto_len]=0;
    screen_move((int)goto_row,1);printf("Goto HEX: %s_  (Enter / Esc)\033[K",goto_buf);
    continue;
   }
   if(esc==1){esc=c=='['?2:0;continue;}
   if(esc==2){esc=0;int delta=0;
    if(c=='A')delta=-BYTES_PER_ROW;else if(c=='B')delta=BYTES_PER_ROW;
    else if(c=='5'){esc=5;continue;}else if(c=='6'){esc=6;continue;}
    if(delta){unsigned next;if(shifted(addr,delta,&next)){addr=next;have=0;dirty=1;}}continue;
   }
   if(esc==5||esc==6){int delta=esc==5?-(int)view_count:(int)view_count;esc=0;if(c=='~'){unsigned next;if(shifted(addr,delta,&next)){addr=next;have=0;dirty=1;}}continue;}
   if(c==27){esc=1;continue;}
   if(c=='q'||c=='Q'||c==3){run=0;break;}
   if(c=='g'||c=='G'){goto_mode=1;goto_len=0;goto_buf[0]=0;screen_move((int)goto_row,1);printf("Goto HEX: _  (Enter / Esc)\033[K");continue;}
   if(c==' '){pause=!pause;dirty=1;}
   if(c=='k'||c=='K'||c=='f'||(c>='1'&&c<='6')){
    addr=c=='k'?0x20000000u:(c=='K'||c=='f')?0x10000000u:0x2000e000u+(unsigned)(c-'1')*0x6000u;
    have=0;dirty=1;
   }
  }
  if(dirty||(!pause&&ticks>=25)){
   if(fetch(fd,addr)==0){
    int changed=have&&memcmp(previous,current,view_count)!=0;
    int labels_changed=refresh_labels(addr);
    int slots_changed=refresh_slots();
    if(dirty||changed||highlighted||labels_changed||slots_changed){
     draw(addr,pause,have,changed&&!pause);
     if(goto_mode){screen_move((int)goto_row,1);printf("Goto HEX: %s_  (Enter / Esc)\033[K",goto_buf);}
     highlighted=changed&&!pause;
    }
    memcpy(previous,current,view_count);have=1;
   }else if(dirty){printf("\033[Hmem: invalid/unreadable address\033[K");}
   dirty=0;ticks=0;
  }
  msleep(40);
  if(ticks<25)ticks++;
 }
 screen_leave();fcntl(0,F_SETFL,flags);close(fd);return 0;
}
