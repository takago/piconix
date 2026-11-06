/* 8×8 LED表示用のフレームバッファと表示更新を担当する． */
/* 8×8 LED表示．GP0〜7はカソード（Lowで点灯），GP8〜15はアノード（Highで点灯）． */
#include "../include/piconix.h"
#include "devices.h"
#include "pins.h"
#include "pico/stdlib.h"
#include "hardware/sync.h"
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
