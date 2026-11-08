/* 8×8 LED表示用のフレームバッファと表示更新を担当する． */
/* 8×8 LED表示．GP0〜7はカソード（Lowで点灯），GP8〜15はアノード（Highで点灯）． */
#include "../include/piconix.h"
#include "devices.h"
#include "pins.h"
#include "pico/stdlib.h"
#include "hardware/sync.h"
#include "../include/font5x7.h"
#include <string.h>

/* 関数トレースの対象外では計測フックを生成しない． */
#define NO_TRACE __attribute__((no_instrument_function))

#define COL_MASK 0x00ffu
#define ROW_MASK 0xff00u
#define FB_MASK  0xffffu
static uint8_t pixels[8];
static volatile unsigned enabled;
static unsigned row;
/* 8×8 LEDマトリクスの表示状態と走査位置を初期化する． */
NO_TRACE void fb_init(void) { enabled=0; row=0; memset(pixels,0,sizeof pixels); }

/* フレームバッファ表示が有効なら真を返す．有効時は共有GPIOの直接操作を制限する． */
NO_TRACE int fb_active(void) { return enabled != 0; }

/* タイマ割込みごとに表示行を切り替える．行切替前の消灯で残像を抑える． */
NO_TRACE void fb_tick(void) {
    if(!enabled)
        return;
    /* カソードを切り替える前にアノードを消し，残像点灯を防ぐ． */
    gpio_clr_mask(ROW_MASK);
    /* カソード8ビットだけを書き換え，アノードは消灯状態に保つ． */
    gpio_clr_mask(COL_MASK);
    gpio_set_mask((uint32_t)(~pixels[row]) & COL_MASK);
    gpio_set_mask(1u << (row+8u));
    row=(row+1u)&7u;
}

/* 表示の有効化・無効化・状態取得を処理する．GPIOと競合しないよう割込みを抑止する． */
int fb_ioctl(int cmd) {
    if(cmd==OS_FB_STATUS)
        return (int)enabled;
    if(cmd!=OS_FB_ENABLE && cmd!=OS_FB_DISABLE)
        return OS_EINVAL;
    uint32_t irq=save_and_disable_interrupts();
    if(cmd==OS_FB_ENABLE && !enabled) {
        /* ピン機能とプルは個別設定するが，SIOピンの値はまとめて更新する． */
        for(unsigned i=0;i<16;i++) {
            gpio_init(i);
            gpio_disable_pulls(i);
        }
        gpio_clr_mask(ROW_MASK);
        gpio_set_mask(COL_MASK); /* カソードHigh，アノードLowの消灯状態． */
        gpio_set_dir_out_masked(FB_MASK);
        row=0;
        enabled=1;
    } else if(cmd==OS_FB_DISABLE && enabled) {
        enabled=0;
        gpio_clr_mask(ROW_MASK);
        gpio_set_mask(COL_MASK);
        gpio_set_dir_in_masked(FB_MASK);
        for(unsigned i=0;i<16;i++)
            gpio_disable_pulls(i);
    }
    restore_interrupts(irq);
    return 0;
}

/* 8バイトの表示ビット列を読み書きする．長さを固定して境界外アクセスを防ぐ． */
int fb_rw(void *p,size_t n,int writing) {
    if(n!=8)
        return OS_EINVAL;
    uint32_t irq=save_and_disable_interrupts();
    if(writing)
        memcpy(pixels,p,8);
    else memcpy(p,pixels,8);
    restore_interrupts(irq);
    return 8;
}

/* 文字指定型の表示デバイス．書込み後も表示内容は保持される．
 * framebufferの1バイトは1行，最上位ビットが左端． */
static const struct { const char *name; uint8_t rows[8]; } fb_icons[] = {
    {"heart", {0x66,0xff,0xff,0xff,0x7e,0x3c,0x18,0x00}},
    {"star",  {0x18,0x18,0xff,0x7e,0x3c,0x66,0xc3,0x00}},
    {"smile", {0x3c,0x42,0xa5,0x81,0xa5,0x99,0x42,0x3c}},
    {"sad",   {0x3c,0x42,0xa5,0x81,0x99,0xa5,0x42,0x3c}},
    {"up",    {0x18,0x3c,0x7e,0xdb,0x18,0x18,0x18,0x00}},
    {"down",  {0x18,0x18,0x18,0xdb,0x7e,0x3c,0x18,0x00}},
    {"left",  {0x10,0x30,0x7f,0xff,0x7f,0x30,0x10,0x00}},
    {"right", {0x08,0x0c,0xfe,0xff,0xfe,0x0c,0x08,0x00}},
    {"check", {0x00,0x03,0x06,0xcc,0x78,0x30,0x00,0x00}},
    {"cross", {0x00,0xc3,0x66,0x3c,0x3c,0x66,0xc3,0x00}},
};

/* 一覧を小さな断片に分け，fd位置から有限長だけ返す．
 * アイコン名は描画用のfb_iconsを参照し，二重定義を避ける． */
static void fb_char_copy(char *dst,size_t max,size_t *done,size_t *skip,
                         const char *text) {
    size_t len=strlen(text);
    if(*skip>=len) { *skip-=len; return; }
    size_t count=len-*skip;
    if(count>max-*done) count=max-*done;
    if(count) memcpy(dst+*done,text+*skip,count);
    *done+=count;
    *skip=0;
}

/* 文字表示デバイスの使用方法と利用可能な図柄を位置指定で返す． */
int fb_char_read(void *data,size_t n,uint32_t offset) {
    if(!n) return 0;
    size_t done=0,skip=offset;
    char *dst=(char *)data;
    fb_char_copy(dst,n,&done,&skip,"Characters: printable ASCII 32-126 (one character)\nPatterns: ");
    for(unsigned i=0;i<sizeof fb_icons/sizeof fb_icons[0];i++) {
        fb_char_copy(dst,n,&done,&skip,fb_icons[i].name);
        fb_char_copy(dst,n,&done,&skip," ");
    }
    fb_char_copy(dst,n,&done,&skip,"clear\nUsage: echo A > /dev/fbchar\n       echo heart > /dev/fbchar\nManual: /man/led-patterns\n");
    return (int)done;
}

/* ASCII 1文字か記号名を8行へ変換．不明な指定は表示を変更しない． */
int fb_char_write(const void *data,size_t n) {
    const unsigned char *src=(const unsigned char *)data;
    char name[16];
    uint8_t image[8]={0};
    if(!n) return 0;
    if(n==1 && (src[0]=='\n' || src[0]=='\r')) return (int)n;
    if(n>=sizeof name) return OS_EINVAL;
    size_t end=n;
    while(end && (src[end-1]=='\n' || src[end-1]=='\r')) end--;
    if(!end) return (int)n;
    for(size_t i=0;i<end;i++) name[i]=(char)src[i];
    name[end]=0;
    if(end==1 && (unsigned char)name[0]>=32 && (unsigned char)name[0]<=126) {
        const unsigned char *glyph=piconix_font5x7[(unsigned char)name[0]-32];
        for(int x=0;x<5;x++)
            for(int y=0;y<8;y++)
                if(glyph[x]&(1u<<y)) image[y] |= (uint8_t)(1u<<(6-x));
    } else if(!strcmp(name,"clear")) {
        /* clear -> all zeros */
    } else {
        unsigned i;
        for(i=0;i<sizeof fb_icons/sizeof fb_icons[0];i++)
            if(!strcmp(name,fb_icons[i].name)) { memcpy(image,fb_icons[i].rows,8); break; }
        if(i==sizeof fb_icons/sizeof fb_icons[0]) return OS_EINVAL;
    }
    if(fb_ioctl(OS_FB_ENABLE)<0) return OS_EIO;
    return fb_rw(image,sizeof image,1)<0 ? OS_EIO : (int)n;
}
