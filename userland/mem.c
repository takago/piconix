/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Live read-only memory viewer via /dev/mem. No kernel changes. */
#include "unistd.h"
#include "fcntl.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include <stdint.h>
#define ROWS 12
#define COUNT (ROWS*8)
static unsigned char previous[COUNT],current[COUNT];
static int valid(unsigned a){return (a>=0x20000000u&&a<=0x2003ffffu)||(a>=0x10000000u&&a<=0x101fffffu);}
static void hex(unsigned v,int digits){const char *h="0123456789ABCDEF";for(int i=(digits-1)*4;i>=0;i-=4)putchar(h[(v>>i)&15]);}
static int fetch(int fd,unsigned address){if(!valid(address)||!valid(address+COUNT-1))return -1;if(lseek(fd,(long)address,SEEK_SET)<0)return -1;return read(fd,current,COUNT)==COUNT?0:-1;}
static void draw(unsigned addr,int pause,int have,int changed){
 printf("\033[Hmem 0x");hex(addr,8);printf("  %s  changed bytes highlighted\033[K\r\n",pause?"PAUSED":"LIVE");
 for(int r=0;r<ROWS;r++){
  hex(addr+8*r,8);printf("  ");
  for(int i=0;i<8;i++){int j=r*8+i;if(have&&changed&&previous[j]!=current[j])printf("\033[7m");hex(current[j],2);if(have&&changed&&previous[j]!=current[j])printf("\033[0m");putchar(' ');}
  printf(" |");for(int i=0;i<8;i++){unsigned char c=current[r*8+i];putchar(c>=32&&c<127?c:'.');}printf("|\033[K\r\n");
 }
 printf("\033[KUp/Down: 8 bytes  PgUp/Dn: page  SPACE: pause  q: quit\033[K");
}
static int parse(const char *s,unsigned *out){char *end;long n=strtol(s,&end,0);if(!*s||*end||n<0||!valid((unsigned)n))return -1;*out=(unsigned)n&~7u;return 0;}
int main(int argc,char **argv){
 unsigned addr=0x20000000u;
 if(argc>2||(argc==2&&parse(argv[1],&addr)<0)){puts("usage: mem [address]  (SRAM 0x20000000..0x2003ffff)");return 1;}
 int fd=open("/dev/mem",O_RDONLY);if(fd<0){puts("mem: cannot open /dev/mem");return 1;}
 int canon=ioctl(0,OS_TTY_GET_CANON,0),echo=ioctl(0,OS_TTY_GET_ECHO,0),flags=fcntl(0,F_GETFL,0);
 if(canon<0||echo<0||flags<0){close(fd);return 1;}
 if(ioctl(0,OS_TTY_SET_CANON,0)<0||ioctl(0,OS_TTY_SET_ECHO,0)<0||fcntl(0,F_SETFL,flags|O_NONBLOCK)<0){
  ioctl(0,OS_TTY_SET_ECHO,echo);ioctl(0,OS_TTY_SET_CANON,canon);close(fd);return 1;
 }
 printf("\033[?1049h\033[?25l\033[2J");
 int have=0,pause=0,esc=0,run=1,dirty=1;
 while(run){
  char c;while(read(0,&c,1)==1){
   if(esc==1){esc=c=='['?2:0;continue;}
   if(esc==2){esc=0;int delta=0;
    if(c=='A')delta=-8;else if(c=='B')delta=8;
    else if(c=='5'){esc=5;continue;}else if(c=='6'){esc=6;continue;}
    if(delta){unsigned next=addr+delta;if(valid(next)&&valid(next+COUNT-1)){addr=next;have=0;dirty=1;}}continue;
   }
   if(esc==5||esc==6){int delta=esc==5?-(int)COUNT:COUNT;esc=0;if(c=='~'){unsigned next=addr+delta;if(valid(next)&&valid(next+COUNT-1)){addr=next;have=0;dirty=1;}}continue;}
   if(c==27){esc=1;continue;}
   if(c=='q'||c=='Q'||c==3){run=0;break;}
   if(c==' '){pause=!pause;dirty=1;}
  }
  if(!pause||dirty){if(fetch(fd,addr)==0){draw(addr,pause,have,!pause);memcpy(previous,current,COUNT);have=1;}else {printf("\033[Hmem: invalid/unreadable address\033[K");}dirty=0;}
  msleep(200);
 }
 printf("\033[0m\033[?1049l\033[?25h");fcntl(0,F_SETFL,flags);close(fd);return 0;
}
