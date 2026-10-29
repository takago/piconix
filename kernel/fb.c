/* 8x8 LED framebuffer: GP0..7 cathodes (active low), GP8..15 anodes (active high). */
#include "../include/piconix.h"
#include "devices.h"
#include "pins.h"
#include "pico/stdlib.h"
#include "hardware/sync.h"
#include <string.h>

#define COL_MASK 0x00ffu
#define ROW_MASK 0xff00u
#define FB_MASK  0xffffu
static uint8_t pixels[8];
static volatile unsigned enabled;
static unsigned row;
void fb_init(void) { enabled=0; row=0; memset(pixels,0,sizeof pixels); }
int fb_active(void) { return enabled != 0; }

void fb_tick(void) {
    if(!enabled) return;
    /* Blank anodes before changing cathode data to avoid ghosting. */
    gpio_clr_mask(ROW_MASK);
    /* Only the eight cathode bits change; anodes remain blanked. */
    gpio_clr_mask(COL_MASK);
    gpio_set_mask((uint32_t)(~pixels[row]) & COL_MASK);
    gpio_set_mask(1u << (row+8u));
    row=(row+1u)&7u;
}

int fb_ioctl(int cmd) {
    if(cmd==OS_FB_STATUS) return (int)enabled;
    if(cmd!=OS_FB_ENABLE && cmd!=OS_FB_DISABLE) return OS_EINVAL;
    uint32_t irq=save_and_disable_interrupts();
    if(cmd==OS_FB_ENABLE && !enabled) {
        /* Configure pin mux/pulls individually, but update SIO pins in bulk. */
        for(unsigned i=0;i<16;i++) {
            gpio_init(i);
            gpio_disable_pulls(i);
        }
        gpio_clr_mask(ROW_MASK);
        gpio_set_mask(COL_MASK); /* cathodes HIGH, anodes LOW */
        gpio_set_dir_out_masked(FB_MASK);
        row=0;
        enabled=1;
    } else if(cmd==OS_FB_DISABLE && enabled) {
        enabled=0;
        gpio_clr_mask(ROW_MASK);
        gpio_set_mask(COL_MASK);
        gpio_set_dir_in_masked(FB_MASK);
        for(unsigned i=0;i<16;i++) gpio_disable_pulls(i);
    }
    restore_interrupts(irq);
    return 0;
}

int fb_rw(void *p,size_t n,int writing) {
    if(n!=8) return OS_EINVAL;
    uint32_t irq=save_and_disable_interrupts();
    if(writing) memcpy(pixels,p,8);
    else memcpy(p,pixels,8);
    restore_interrupts(irq);
    return 8;
}
