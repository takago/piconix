/* Pico Wで動く簡易HTTPサーバ．
 * 要求行・ヘッダ・本文を固定長の領域で検査し，メモリ使用量を抑える． */
/* Pico W向けの簡易HTTP/1.0サーバ兼ローカルデバイス操作例． */
#include "unistd.h"
#include "fcntl.h"
#include "stdio.h"
#include "string.h"
#include <stdint.h>

/* TCP接続への送信を完了するまで繰り返す．EAGAIN時は短く睡眠し，待機回数に上限を設ける． */
static int sendall(int fd,const char *p,unsigned n){
    unsigned idle=0;
    while(n){int r=send(fd,p,n,0);
        if(r==OS_EAGAIN||r==0){if(++idle>1500)return -1;msleep(2);continue;}
        if(r<0)return -1;
        p+=r;n-=(unsigned)r;idle=0;
    }return 0;
}

/* HTTPのパスや操作名を文字列一致で判定する． */
static int same(const char *a,const char *b){return strcmp(a,b)==0;}

/* HTTP/1.0の状態行・ヘッダ・本文を送る．Content-Lengthを明示し接続を閉じる． */
static void response(int fd,int status,const char *body,unsigned size,const char *type){
    const char *state=status==200?"200 OK":status==400?"400 Bad Request":status==405?"405 Method Not Allowed":status==413?"413 Content Too Large":status==501?"501 Not Implemented":"404 Not Found";
    char head[176];
    int n=snprintf(head,sizeof head,"HTTP/1.0 %s\r\nContent-Type: %s\r\nContent-Length: %u\r\nCache-Control: no-store\r\nConnection: close\r\n\r\n",state,type,size);
    if(n<=0||(unsigned)n>=sizeof head)return;
    if(sendall(fd,head,(unsigned)n)==0&&size)sendall(fd,body,size);
}

/* UTF-8プレーンテキストのHTTP応答を生成する． */
static void text_reply(int fd,int status,const char *message){response(fd,status,message,(unsigned)strlen(message),"text/plain; charset=utf-8");}

/* 16進数1桁を0〜15へ変換する．不正な文字なら-1． */
static int hexval(char c){if(c>='0'&&c<='9')return c-'0';if(c>='a'&&c<='f')return c-'a'+10;if(c>='A'&&c<='F')return c-'A'+10;return -1;}

/* Web APIのPWMパスをVFSの/dev/pwmパスへ変換する．不明パスならNULL． */
static const char *pwm_path(const char *path){
    if(same(path,"/api/pwm/freq"))return "/dev/pwm/1/freq";
    if(same(path,"/api/pwm/a"))return "/dev/pwm/1/a";
    if(same(path,"/api/pwm/b"))return "/dev/pwm/1/b";
    return 0;
}

/* フレームバッファとPWM用HTTP APIを解析してデバイス操作へ振り分ける． */
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
        out[16]='\n'; /* 現在の表示パターンを報告する． */
        response(fd,200,out,17,"text/plain; charset=utf-8");return;
    }
    /* 操作一回につき要求を一つ送る．キーごとにTCP接続を三つ作らない． */
    if(same(path,"/api/tone")){
        if(!post||count<1||count>4){text_reply(fd,400,"Expected frequency\n");return;}
        unsigned freq=0;
        for(unsigned i=0;i<count;i++){
            if(body[i]<'0'||body[i]>'9'){text_reply(fd,400,"Invalid frequency\n");return;}
            freq=freq*10u+(unsigned)(body[i]-'0');
        }
        if(freq && (freq<100||freq>5000)){text_reply(fd,400,"Frequency out of range\n");return;}
        int a=open("/dev/pwm/1/a",O_WRONLY);
        if(a<0){text_reply(fd,404,"PWM unavailable\n");return;}
        if(write(a,"0",1)!=1){close(a);text_reply(fd,400,"PWM error\n");return;}
        close(a);
        if(freq){
            char digits[5];unsigned n=0,v=freq;
            do {digits[n++]=(char)('0'+v%10);v/=10;}while(v);
            for(unsigned i=0;i<n/2;i++){char t=digits[i];digits[i]=digits[n-1-i];digits[n-1-i]=t;}
            int f=open("/dev/pwm/1/freq",O_WRONLY);
            if(f<0){text_reply(fd,400,"PWM error\n");return;}
            int ok=write(f,digits,n)==(int)n;close(f);
            if(!ok){text_reply(fd,400,"PWM error\n");return;}
            a=open("/dev/pwm/1/a",O_WRONLY);
            if(a<0){text_reply(fd,400,"PWM error\n");return;}
            ok=write(a,"50",2)==2;close(a);
            if(!ok){text_reply(fd,400,"PWM error\n");return;}
        }
        text_reply(fd,200,"OK\n");return;
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

/* ヘッダを一行ずつ検査し，曖昧・不正な長さを拒否する．この教材には汎用HTTPパーサも動的確保も不要である． */
static int content_length(const char *req,unsigned header,unsigned *length,int *found){
    *length=0;*found=0;
    for(unsigned i=0;i+1<header;){
        unsigned start=i;
        while(i+1<header&&(req[i]!='\r'||req[i+1]!='\n'))i++;
        if(i+1>=header)return -1;
        unsigned end=i;i+=2;
        if(end-start<15 ||
           (strncmp(req+start,"Content-Length:",15)!=0&&
            strncmp(req+start,"content-length:",15)!=0))continue;
        if(*found)return -1; /* 重複した長さ指定は曖昧なので拒否する． */
        unsigned j=start+15;
        while(j<end&&(req[j]==' '||req[j]=='\t'))j++;
        if(j==end||req[j]<'0'||req[j]>'9')return -1;
        unsigned n=0;
        while(j<end&&req[j]>='0'&&req[j]<='9'){
            n=n*10u+(unsigned)(req[j++]-'0');
            if(n>32)return -2; /* 整数のオーバーフロー前に値を検査する． */
        }
        while(j<end&&(req[j]==' '||req[j]=='\t'))j++;
        if(j!=end)return -1;
        *length=n;*found=1;
    }
    return 0;
}

/* ノンブロッキング通信ではOS_EAGAINは一時的な状態，0はTCP EOFである． */
static int recv_timed(int fd,char *buf,unsigned size,unsigned *elapsed){
    fd_set rd=0;
    FD_SET(fd,&rd);
    struct timeval timeout={0,5000};
    int ready=select(fd+1,&rd,0,0,&timeout);
    if(ready<0)return ready;
    if(!ready){*elapsed+=5;return OS_EAGAIN;}
    return recv(fd,buf,size,0);
}

/* 固定長要求バッファでGETと小さなPOSTだけを扱い，chunked転送は実装しない．2 KiBの静的領域により4 KiBスタックの圧迫を避け，接続は一度に一つ処理する． */
static void handle(int fd){
    static char req[2048]; /* ブラウザのヘッダはcurlより長くなるため，4 KiBスタックの外へバッファを置く． */
    unsigned used=0,header=0,elapsed=0;
    while(!header&&elapsed<3000){
        int n=recv_timed(fd,req+used,sizeof req-1-used,&elapsed);
        if(n==OS_EAGAIN)continue;
        if(n==0){text_reply(fd,400,"Incomplete headers\n");return;}
        if(n<0)return;
        used+=(unsigned)n;req[used]=0;
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
    unsigned length;int got_length;
    int length_status=content_length(req,header,&length,&got_length);
    if(length_status<0){
        text_reply(fd,length_status==-2?413:400,
                   length_status==-2?"Body too large\n":"Invalid length\n");
        return;
    }
    if(post&&!got_length){text_reply(fd,400,"Content-Length required\n");return;}
    if(post){
        char body[33];unsigned count=used-header;
        if(count>length){count=length;} /* 本文はContent-Lengthで宣言された長さだけを扱う． */
        if(count)memcpy(body,req+header,count);
        elapsed=0;
        while(count<length&&elapsed<3000){
            int n=recv_timed(fd,body+count,length-count,&elapsed);
            if(n==OS_EAGAIN)continue;
            if(n==0){text_reply(fd,400,"Incomplete body\n");return;}
            if(n<0)return;
            count+=(unsigned)n;
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
    /* 静的HTMLファイルもPicoNIXの2 KiBファイル上限に従う． */
    char content[128];
    const char h[]="HTTP/1.0 200 OK\r\nContent-Type: text/html; charset=utf-8\r\nCache-Control: no-store\r\nConnection: close\r\n\r\n";
    if(sendall(fd,h,sizeof h-1)==0){int n;
        while((n=read(file,content,sizeof content))>0)if(sendall(fd,content,(unsigned)n)<0)break;
    }close(file);
}

/* TCP待受と要求処理を繰り返す．単一接続・固定長バッファに制限し，資源が小さい環境でのHTTPを示す． */
int main(int argc, char **argv){
    (void)argv;
    if(argc!=1) {puts("usage: httpd");return 1;}
    int server=socket(AF_INET,SOCK_STREAM,0);
    if(server<0){puts("httpd: socket failed");return 1;}
    struct sockaddr_in addr={0};addr.sin_family=AF_INET;addr.sin_port=htons(80);
    if(bind(server,(const struct sockaddr *)&addr,sizeof addr)<0||listen(server,1)<0){
        puts("httpd: cannot listen on port 80");close(server);return 1;
    }
    puts("httpd: listening on TCP 80");
    for(;;){int client=accept(server,0,0);
        if(client==OS_EAGAIN){msleep(10);continue;}
        if(client<0)break;
        /* 要求受信の3秒タイムアウトを維持するため，クライアントFDだけ非ブロッキング． */
        fcntl(client,F_SETFL,O_NONBLOCK);
        handle(client);close(client);
    }close(server);return 0;
}
