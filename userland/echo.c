/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "unistd.h"
#include "string.h"
int main(int argc,char *argv[]) {
    for(int i=1;i<argc;i++) {
        if(i>1 && write(1," ",1)<0) return 1;
        const char *s=argv[i]; size_t n=strlen(s),done=0;
        while(done<n) {int w=write(1,s+done,n-done);if(w<0)return 1;if(!w){msleep(1);continue;}done+=(unsigned)w;}
    }
    return write(1,"\n",1)<0;
}
