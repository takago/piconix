/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Live read-only memory viewer, with optional Flash-resident symbol names. */
#include "unistd.h"
#include "fcntl.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include <stdint.h>
#define ROWS 16
#define BYTES_PER_ROW 16
#define COUNT (ROWS*BYTES_PER_ROW)
#define SYMBOL_NAME_MAX 128
static unsigned char previous[COUNT],current[COUNT];
static char labels[ROWS][SYMBOL_NAME_MAX];
static char symbol_app[96];
static int symbols_available;
static int valid(unsigned a){return (a>=0x20000000u&&a<=0x2003ffffu)||(a>=0x10000000u&&a<=0x101fffffu);}
static void hex(unsigned v,int digits){const char *h="0123456789ABCDEF";for(int i=(digits-1)*4;i>=0;i-=4)putchar(h[(v>>i)&15]);}
static int fetch(int fd,unsigned address){if(!valid(address)||!valid(address+COUNT-1))return -1;if(lseek(fd,(long)address,SEEK_SET)<0)return -1;return read(fd,current,COUNT)==COUNT?0:-1;}
static const char *region(unsigned addr){
 if(addr>=0x10000000u&&addr<=0x101fffffu)return "XIP FLASH";
 if(addr<0x20018000u)return "KERNEL SRAM";
 if(addr<=0x2003ffffu)return "TASK SRAM";
 return "UNKNOWN";
}
/* Catalog records: application<TAB>hex_offset<TAB>type<TAB>name<LF>.
   Each application is linked at 0x20018000; offsets are slot-relative.
   Read through /proc/sym, never dereference an ELF symbol table. */
static int symbol_record(char *line,char **app,unsigned *offset,char *kind,char **name){
 char *a=strchr(line,'\t');if(!a)return 0;*a++=0;
 char *b=strchr(a,'\t');if(!b)return 0;*b++=0;
 char *c=strchr(b,'\t');if(!c||b+1!=c)return 0;*c++=0;
 char *end;long v=strtol(a,&end,16);
 if(!*a||*end||v<0||!*c)return 0;
 if(!strcmp(line,"kernel")){
  if(!((unsigned)v>=0x10000000u&&(unsigned)v<0x10200000u) &&
     !((unsigned)v>=0x20000000u&&(unsigned)v<0x20018000u))return 0;
 }else if((unsigned)v>=0x8000u)return 0;
 *app=line;*offset=(unsigned)v;*kind=*b;*name=c;return 1;
}
/* Process a catalog as a stream, with fixed-size memory and no malloc.
   Lines larger than the buffer are skipped (including their remainder). */
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
       if(actual>=base&&actual-base<COUNT){
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
      else if(slot_hint>=0x20018000u&&slot_hint<0x20040000u){
       /* A task may occupy one of five 32-KiB slots. */
       unsigned slot_base=0x20018000u+((slot_hint-0x20018000u)/0x8000u)*0x8000u;
       unsigned actual=slot_base+offset;
       if(actual>=base&&actual<base+COUNT){
        unsigned row=(actual-base)/BYTES_PER_ROW;
        /* Keep all symbols on the same 16-byte row, in catalog order.
           Reserve space for an ellipsis if the combined names are too long. */
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
/* /proc/tasks is the authority for the current program in each RAM slot.
   Parse a line as whitespace-separated fields; a failed lookup simply means
   no labels, never an unsafe guess about a slot occupant. */
static int task_program(unsigned addr,char *name,unsigned capacity){
 if(addr<0x20018000u||addr>=0x20040000u)return 0;
 unsigned slot=(addr-0x20018000u)/0x8000u+1;
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
/* Refresh labels from the currently occupied slot, never a caller-supplied
   application name. Also detect when an app leaves or replaces a slot. */
static void append_app(char *list,unsigned cap,const char *name){
 if(!*name)return;
 /* Names must not be appended twice even when adjacent regions repeat. */
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
/* Label a live /tmp data buffer without assuming the kernel layout. */
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
  if(!ok || a<addr || a-addr>=COUNT)continue;
  unsigned row=(a-addr)/BYTES_PER_ROW;
  char *dst=labels[row];size_t n=strlen(dst);
  if(n+2<sizeof labels[row] && n){dst[n++]=',';dst[n++]=' ';dst[n]=0;}
  snprintf(dst+n,sizeof labels[row]-n,"RAM_FILE: /tmp/%s",line+9);
 }
 close(fd);
}
static int refresh_labels(unsigned addr){
 char old[ROWS][SYMBOL_NAME_MAX],old_app[sizeof symbol_app];
 memcpy(old,labels,sizeof old);
 memcpy(old_app,symbol_app,sizeof old_app);
 int old_available=symbols_available;
 memset(labels,0,sizeof labels);
 symbol_app[0]=0;symbols_available=0;
 /* A 256-byte window can straddle a task-slot boundary. */
 for(unsigned off=0;off<COUNT;off+=BYTES_PER_ROW){
  unsigned a=addr+off;
  char app[32]={0};
  if(a<0x20018000u || (a>=0x10000000u&&a<0x10200000u)){
   strncpy(app,"kernel",sizeof app-1);
  }else task_program(a,app,sizeof app);
  if(!app[0])continue;
  /* Scan only once per distinct application in the current window. */
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
 return memcmp(old,labels,sizeof old)!=0 ||
        strcmp(old_app,symbol_app)!=0 || old_available!=symbols_available;
}
static void draw(unsigned addr,int pause,int have,int changed){
 printf("\033[H\033[7mPicoNIX mem   %s   %s\033[K\033[0m\r\n",region(addr),pause?"PAUSED":"LIVE");
 printf("Address 0x");hex(addr,8);printf("  256 bytes  16 bytes/row\033[K\r\n");
 printf("          00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F   ASCII            SYMBOL: %s\033[K\r\n",symbol_app[0]?symbol_app:"-");
 for(int r=0;r<ROWS;r++){
  hex(addr+BYTES_PER_ROW*r,8);printf("  ");
  for(int i=0;i<BYTES_PER_ROW;i++){
   int j=r*BYTES_PER_ROW+i;
   if(have&&changed&&previous[j]!=current[j])printf("\033[7m");
   hex(current[j],2);
   if(have&&changed&&previous[j]!=current[j])printf("\033[0m");
   putchar(' ');
  }
  putchar('|');
  for(int i=0;i<BYTES_PER_ROW;i++){
   unsigned char c=current[r*BYTES_PER_ROW+i];putchar(c>=32&&c<127?c:'.');
  }
  printf("| %s\033[K\r\n",labels[r]);
 }
 printf("\033[7mArrows: 16B  PgUp/Dn: 256B  g Goto  SPACE Pause  q Quit\033[K\033[0m\r\n");
 printf("k Kernel  1..5 Task slots  f Flash   Symbols: %s\033[K",symbol_app[0]?(symbols_available?"loaded":"unavailable"):"none");
}
static int parse(const char *s,unsigned *out){char *end;long n=strtol(s,&end,0);if(!*s||*end||n<0||!valid((unsigned)n))return -1;*out=(unsigned)n&~15u;return 0;}
static int shifted(unsigned addr,int delta,unsigned *next){
 unsigned n=addr+(unsigned)delta;
 if(!valid(n)||!valid(n+COUNT-1))return 0;
 *next=n;return 1;
}
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
 if(!valid(addr+COUNT-1)){puts("mem: invalid memory range");close(fd);return 1;}
 int canon=ioctl(0,OS_TTY_GET_CANON,0),echo=ioctl(0,OS_TTY_GET_ECHO,0),flags=fcntl(0,F_GETFL,0);
 if(canon<0||echo<0||flags<0){close(fd);return 1;}
 if(ioctl(0,OS_TTY_SET_CANON,0)<0||ioctl(0,OS_TTY_SET_ECHO,0)<0||fcntl(0,F_SETFL,flags|O_NONBLOCK)<0){
  ioctl(0,OS_TTY_SET_ECHO,echo);ioctl(0,OS_TTY_SET_CANON,canon);close(fd);return 1;
 }
 printf("\033[?1049h\033[?25l\033[2J");
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
      if(parse(input,&next)==0 && valid(next+COUNT-1)){
       addr=next;have=0;dirty=1;
      }
     }
     goto_mode=0;goto_len=0;dirty=1;continue;
    }
    if(c==8||c==127){if(goto_len)goto_len--;}
    else if(((c>='0'&&c<='9')||(c>='a'&&c<='f')||(c>='A'&&c<='F')) && goto_len<8)
     goto_buf[goto_len++]=c;
    goto_buf[goto_len]=0;
    printf("\033[22;1HGoto HEX: %s_  (Enter / Esc)\033[K",goto_buf);
    continue;
   }
   if(esc==1){esc=c=='['?2:0;continue;}
   if(esc==2){esc=0;int delta=0;
    if(c=='A')delta=-BYTES_PER_ROW;else if(c=='B')delta=BYTES_PER_ROW;
    else if(c=='5'){esc=5;continue;}else if(c=='6'){esc=6;continue;}
    if(delta){unsigned next;if(shifted(addr,delta,&next)){addr=next;have=0;dirty=1;}}continue;
   }
   if(esc==5||esc==6){int delta=esc==5?-COUNT:COUNT;esc=0;if(c=='~'){unsigned next;if(shifted(addr,delta,&next)){addr=next;have=0;dirty=1;}}continue;}
   if(c==27){esc=1;continue;}
   if(c=='q'||c=='Q'||c==3){run=0;break;}
   if(c=='g'||c=='G'){goto_mode=1;goto_len=0;goto_buf[0]=0;printf("\033[22;1HGoto HEX: _  (Enter / Esc)\033[K");continue;}
   if(c==' '){pause=!pause;dirty=1;}
   if(c=='k'||c=='f'||(c>='1'&&c<='5')){
    addr=c=='k'?0x20000000u:c=='f'?0x10000000u:0x20018000u+(unsigned)(c-'1')*0x8000u;
    have=0;dirty=1;
   }
  }
  if(dirty||(!pause&&ticks>=25)){
   if(fetch(fd,addr)==0){
    int changed=have&&memcmp(previous,current,COUNT)!=0;
    int labels_changed=refresh_labels(addr);
    if(dirty||changed||highlighted||labels_changed){
     draw(addr,pause,have,changed&&!pause);
     if(goto_mode)printf("\033[22;1HGoto HEX: %s_  (Enter / Esc)\033[K",goto_buf);
     highlighted=changed&&!pause;
    }
    memcpy(previous,current,COUNT);have=1;
   }else if(dirty){printf("\033[Hmem: invalid/unreadable address\033[K");}
   dirty=0;ticks=0;
  }
  msleep(40);
  if(ticks<25)ticks++;
 }
 printf("\033[0m\033[?1049l\033[?25h");fcntl(0,F_SETFL,flags);close(fd);return 0;
}
