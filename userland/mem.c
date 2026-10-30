/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Live read-only memory viewer via /dev/mem. */
#include "unistd.h"
#include "fcntl.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include <stdint.h>
#define ROWS 16
#define BYTES_PER_ROW 16
#define COUNT (ROWS*BYTES_PER_ROW)
static unsigned char previous[COUNT],current[COUNT];
static int valid(unsigned a){return (a>=0x20000000u&&a<=0x2003ffffu)||(a>=0x10000000u&&a<=0x101fffffu);}
static void hex(unsigned v,int digits){const char *h="0123456789ABCDEF";for(int i=(digits-1)*4;i>=0;i-=4)putchar(h[(v>>i)&15]);}
static int fetch(int fd,unsigned address){if(!valid(address)||!valid(address+COUNT-1))return -1;if(lseek(fd,(long)address,SEEK_SET)<0)return -1;return read(fd,current,COUNT)==COUNT?0:-1;}
static const char *region(unsigned addr){
 if(addr>=0x10000000u&&addr<=0x101fffffu)return "XIP FLASH";
 if(addr<0x20018000u)return "KERNEL SRAM";
 if(addr<=0x2003ffffu)return "TASK SRAM";
 return "UNKNOWN";
}
static void draw(unsigned addr,int pause,int have,int changed){
 printf("\033[H\033[7mPicoNIX mem   %s   %s\033[K\033[0m\r\n",region(addr),pause?"PAUSED":"LIVE");
 printf("Address 0x");hex(addr,8);printf("  256 bytes  16 bytes/row  (reversed = changed)\033[K\r\n");
 printf("          00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F   ASCII\033[K\r\n");
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
  printf("|\033[K\r\n");
 }
 printf("\033[7mArrows: 16B  PgUp/Dn: 256B  SPACE Pause  q Quit\033[K\033[0m\r\n");
 printf("k Kernel  1..5 Task slots  f Flash   (refresh 1s)\033[K");
}
static int parse(const char *s,unsigned *out){char *end;long n=strtol(s,&end,0);if(!*s||*end||n<0||!valid((unsigned)n))return -1;*out=(unsigned)n&~15u;return 0;}
static int shifted(unsigned addr,int delta,unsigned *next){
 unsigned n=addr+(unsigned)delta;
 if(!valid(n)||!valid(n+COUNT-1))return 0;
 *next=n;return 1;
}
int main(int argc,char **argv){
 unsigned addr=0x20000000u;
 if(argc>2||(argc==2&&parse(argv[1],&addr)<0)){puts("usage: mem [address]  (SRAM or XIP flash)");return 1;}
 int fd=open("/dev/mem",O_RDONLY);if(fd<0){puts("mem: cannot open /dev/mem");return 1;}
 int canon=ioctl(0,OS_TTY_GET_CANON,0),echo=ioctl(0,OS_TTY_GET_ECHO,0),flags=fcntl(0,F_GETFL,0);
 if(canon<0||echo<0||flags<0){close(fd);return 1;}
 if(ioctl(0,OS_TTY_SET_CANON,0)<0||ioctl(0,OS_TTY_SET_ECHO,0)<0||fcntl(0,F_SETFL,flags|O_NONBLOCK)<0){
  ioctl(0,OS_TTY_SET_ECHO,echo);ioctl(0,OS_TTY_SET_CANON,canon);close(fd);return 1;
 }
 printf("\033[?1049h\033[?25l\033[2J");
 int have=0,pause=0,esc=0,run=1,dirty=1;
 int ticks=25,highlighted=0;
 while(run){
  char c;while(read(0,&c,1)==1){
   if(esc==1){esc=c=='['?2:0;continue;}
   if(esc==2){esc=0;int delta=0;
    if(c=='A')delta=-BYTES_PER_ROW;else if(c=='B')delta=BYTES_PER_ROW;
    else if(c=='5'){esc=5;continue;}else if(c=='6'){esc=6;continue;}
    if(delta){unsigned next;if(shifted(addr,delta,&next)){addr=next;have=0;dirty=1;}}continue;
   }
   if(esc==5||esc==6){int delta=esc==5?-COUNT:COUNT;esc=0;if(c=='~'){unsigned next;if(shifted(addr,delta,&next)){addr=next;have=0;dirty=1;}}continue;}
   if(c==27){esc=1;continue;}
   if(c=='q'||c=='Q'||c==3){run=0;break;}
   if(c==' '){pause=!pause;dirty=1;}
   if(c=='k'||c=='f'||(c>='1'&&c<='5')){
    addr=c=='k'?0x20000000u:c=='f'?0x10000000u:0x20018000u+(unsigned)(c-'1')*0x8000u;
    have=0;dirty=1;
   }
  }
  /* Poll keys frequently, but refresh the large hex view only once a second.
     Redraw only on navigation, content changes, or to clear change markers. */
  if(dirty||(!pause&&ticks>=25)){
   if(fetch(fd,addr)==0){
    int changed=have&&memcmp(previous,current,COUNT)!=0;
    if(dirty||changed||highlighted){
     draw(addr,pause,have,changed&&!pause);
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
