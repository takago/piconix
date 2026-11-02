/* Small HTTP/1.0 server and local-network device demo for Pico W. */
#include "unistd.h"
#include "fcntl.h"
#include "stdio.h"
#include "string.h"
#include <stdint.h>

static int sendall(int fd,const char *p,unsigned n){
    unsigned idle=0;
    while(n){int r=send(fd,p,n,0);
        if(r==OS_EAGAIN||r==0){if(++idle>1500)return -1;msleep(2);continue;}
        if(r<0)return -1;p+=r;n-=(unsigned)r;idle=0;
    }return 0;
}
static int same(const char *a,const char *b){return strcmp(a,b)==0;}
static void response(int fd,int status,const char *body,unsigned size,const char *type){
    const char *state=status==200?"200 OK":status==400?"400 Bad Request":status==405?"405 Method Not Allowed":status==413?"413 Content Too Large":status==501?"501 Not Implemented":"404 Not Found";
    char head[176];
    int n=snprintf(head,sizeof head,"HTTP/1.0 %s\r\nContent-Type: %s\r\nContent-Length: %u\r\nCache-Control: no-store\r\nConnection: close\r\n\r\n",state,type,size);
    if(n<=0||(unsigned)n>=sizeof head)return;
    if(sendall(fd,head,(unsigned)n)==0&&size)sendall(fd,body,size);
}
static void text_reply(int fd,int status,const char *message){response(fd,status,message,(unsigned)strlen(message),"text/plain; charset=utf-8");}
static int hexval(char c){if(c>='0'&&c<='9')return c-'0';if(c>='a'&&c<='f')return c-'a'+10;if(c>='A'&&c<='F')return c-'A'+10;return -1;}
static const char *pwm_path(const char *path){
    if(same(path,"/api/pwm/freq"))return "/dev/pwm/1/freq";
    if(same(path,"/api/pwm/a"))return "/dev/pwm/1/a";
    if(same(path,"/api/pwm/b"))return "/dev/pwm/1/b";
    return 0;
}
static void api(int fd,int post,const char *path,const char *body,unsigned count){
    if(same(path,"/api/fb")){
        unsigned char pixels[8];char out[17];
        int file=open("/dev/gpio/fb",post?O_RDWR:O_RDONLY);
        if(file<0){text_reply(fd,404,"Device unavailable\n");return;}
        if(post){
            if(count!=16){close(file);text_reply(fd,400,"Expected 16 hex digits\n");return;}
            for(unsigned i=0;i<8;i++){
                int hi=hexval(body[i*2]),lo=hexval(body[i*2+1]);
                if(hi<0||lo<0){close(file);text_reply(fd,400,"Invalid hex digit\n");return;}
                pixels[i]=(unsigned char)((hi<<4)|lo);
            }
            if(ioctl(file,FB_ENABLE,0)<0||write(file,pixels,8)!=8){close(file);text_reply(fd,400,"Framebuffer error\n");return;}
        }else if(read(file,pixels,8)!=8){close(file);text_reply(fd,400,"Framebuffer error\n");return;}
        close(file);
        const char hex[]="0123456789ABCDEF";
        for(unsigned i=0;i<8;i++){out[i*2]=hex[pixels[i]>>4];out[i*2+1]=hex[pixels[i]&15];}
        out[16]='\n'; /* report current pattern */
        response(fd,200,out,17,"text/plain; charset=utf-8");return;
    }
    const char *device=pwm_path(path);
    if(!device){text_reply(fd,404,"Not found\n");return;}
    int file=open(device,post?O_WRONLY:O_RDONLY);
    if(file<0){text_reply(fd,404,"Device unavailable\n");return;}
    if(post){
        if(count==0||count>10){close(file);text_reply(fd,400,"Invalid value\n");return;}
        for(unsigned i=0;i<count;i++)if((body[i]<'0'||body[i]>'9')&&body[i]!='.'){
            close(file);text_reply(fd,400,"Invalid value\n");return;
        }
        if(write(file,body,count)!=(int)count){close(file);text_reply(fd,400,"Invalid PWM value\n");return;}
        close(file);file=open(device,O_RDONLY);
        if(file<0){text_reply(fd,400,"PWM error\n");return;}
    }
    char value[24];int n=read(file,value,sizeof value);
    close(file);
    if(n<1){text_reply(fd,400,"PWM error\n");return;}
    response(fd,200,value,(unsigned)n,"text/plain; charset=utf-8");
}
/* A bounded request: only GET and small POST bodies, no chunked encoding.
 * A 2 KiB static request buffer handles normal browser headers without
 * consuming the task's 4 KiB stack. Only one connection is handled at once. */
static void handle(int fd){
    static char req[2048]; /* Browsers send larger headers than curl; keep off the 4 KiB stack. */
    unsigned used=0,header=0,elapsed=0;
    while(!header&&elapsed<3000){
        int n=recv(fd,req+used,sizeof req-1-used,0);
        if(n==OS_EAGAIN||n==0){msleep(5);elapsed+=5;continue;}
        if(n<0)return;used+=(unsigned)n;req[used]=0;
        for(unsigned i=3;i<used;i++)if(req[i-3]=='\r'&&req[i-2]=='\n'&&req[i-1]=='\r'&&req[i]=='\n') {header=i+1;break;}
        if(!header&&used==sizeof req-1){text_reply(fd,413,"Headers too large\n");return;}
    }
    if(!header){text_reply(fd,400,"Incomplete headers\n");return;}
    char method[8],path[48];
    unsigned k=0,j=0;
    while(k<header&&req[k]!=' '&&j<sizeof method-1)method[j++]=req[k++];
    method[j]=0;
    if(j==0||k>=header||req[k++]!=' '){text_reply(fd,400,"Bad request\n");return;}
    j=0;
    while(k<header&&req[k]!=' '&&j<sizeof path-1)path[j++]=req[k++];
    path[j]=0;
    if(!j||k>=header||req[k]!=' '){text_reply(fd,400,"Bad request\n");return;}
    int post=same(method,"POST");
    if(!post&&!same(method,"GET")){text_reply(fd,405,"GET or POST required\n");return;}
    unsigned length=0;int got_length=0;
    /* Content-Length is required for POST; inspect complete header lines only. */
    for(unsigned i=0;i+15<header;i++){
        if(i!=0&&req[i-1]!='\n')continue;
        if(strncmp(req+i,"Content-Length:",15)!=0&&strncmp(req+i,"content-length:",15)!=0)continue;
        unsigned j=i+15;
        while(j<header&&(req[j]==' '||req[j]=='\t'))j++;
        got_length=1;
        if(j>=header||req[j]<'0'||req[j]>'9'){text_reply(fd,400,"Invalid length\n");return;}
        while(j<header&&req[j]>='0'&&req[j]<='9'){
            length=length*10u+(unsigned)(req[j++]-'0');
            if(length>32){text_reply(fd,413,"Body too large\n");return;}
        }
        break;
    }
    if(post&&!got_length){text_reply(fd,400,"Content-Length required\n");return;}
    if(post){
        char body[33];unsigned count=used-header;
        if(count>length){count=length;} /* only the stated body length belongs to request */
        if(count)memcpy(body,req+header,count);
        elapsed=0;
        while(count<length&&elapsed<3000){
            int n=recv(fd,body+count,length-count,0);
            if(n==OS_EAGAIN||n==0){msleep(5);elapsed+=5;continue;}
            if(n<0)return;count+=(unsigned)n;
        }
        if(count!=length){text_reply(fd,400,"Incomplete body\n");return;}
        api(fd,1,path,body,length);return;
    }
    if(!strncmp(path,"/api/",5)){api(fd,0,path,0,0);return;}
    const char *page=same(path,"/")||same(path,"/index.html")?"/etc/index.html":
        same(path,"/fbcheck.html")?"/etc/fbcheck.html":
        same(path,"/pwm.html")?"/etc/pwm.html":0;
    if(!page){text_reply(fd,404,"Not found\n");return;}
    int file=open(page,O_RDONLY);
    if(file<0){text_reply(fd,404,"Missing HTML page\n");return;}
    /* Static file is at most 2 KiB in PicoNIX. */
    char content[128];
    const char h[]="HTTP/1.0 200 OK\r\nContent-Type: text/html; charset=utf-8\r\nCache-Control: no-store\r\nConnection: close\r\n\r\n";
    if(sendall(fd,h,sizeof h-1)==0){int n;
        while((n=read(file,content,sizeof content))>0)if(sendall(fd,content,(unsigned)n)<0)break;
    }close(file);
}
int main(void){
    int server=socket(AF_INET,SOCK_STREAM,0);
    if(server<0){puts("httpd: socket failed");return 1;}
    struct sockaddr_in addr={0};addr.sin_family=AF_INET;addr.sin_port=htons(80);
    if(bind(server,(const struct sockaddr *)&addr,sizeof addr)<0||listen(server,1)<0){
        puts("httpd: cannot listen on port 80");close(server);return 1;
    }
    puts("httpd: listening on TCP 80");
    for(;;){int client=accept(server,0,0);
        if(client==OS_EAGAIN){msleep(10);continue;}
        if(client<0)break;handle(client);close(client);
    }close(server);return 0;
}
