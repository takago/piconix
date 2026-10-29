/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "../include/piconix.h"
#include "devices.h"
#include "pins.h"
#include "pico/stdlib.h"
#include <string.h>
void gpio_driver_init(void) {
    /* Default board I/O: GP25 LED, GP16/17 active-low buttons. */
    gpio_init(25);
    gpio_put(25, 0);
    gpio_set_dir(25, GPIO_OUT);
    gpio_init(16);
    gpio_set_dir(16, GPIO_IN);
    gpio_pull_up(16);
    gpio_init(17);
    gpio_set_dir(17, GPIO_IN);
    gpio_pull_up(17);
}
int gpio_driver_rw(int kind, unsigned pin, void *p, size_t n, int writing) {
    if(pin<16 && fb_active()) return OS_EBUSY;
    if(n==0) return 0;
    if(kind==1 || kind==2) {
        if(!pin_gpio(pin)) return OS_EINVAL;
        if(kind==1) {
            if(writing) {
                const uint8_t *b=p;
                if(n==2 && b[0]=='i' && b[1]=='n') {gpio_init(pin);gpio_set_dir(pin,GPIO_IN);return (int)n;}
                if(n==3 && b[0]=='o' && b[1]=='u' && b[2]=='t') {gpio_init(pin);gpio_set_dir(pin,GPIO_OUT);return (int)n;}
                return OS_EINVAL;
            }
            const char *v=gpio_get_dir(pin)==GPIO_OUT?"out":"in";
            size_t len=(v[0]=='o')?3:2;
            if(n<len) len=n;
            memcpy(p,v,len); return (int)len;
        }
        if(writing) {
            const uint8_t *b=p;
            if(n==2 && b[0]=='u' && b[1]=='p') {gpio_pull_up(pin);return (int)n;}
            if(n==4 && !memcmp(b,"down",4)) {gpio_pull_down(pin);return (int)n;}
            if(n==4 && !memcmp(b,"none",4)) {gpio_disable_pulls(pin);return (int)n;}
            return OS_EINVAL;
        }
        const char *v=gpio_is_pulled_up(pin)?"up":gpio_is_pulled_down(pin)?"down":"none";
        size_t len=strlen(v);
        if(n<len) len=n;
        memcpy(p,v,len); return (int)len;
    }
    if(!pin_gpio(pin)) return OS_EINVAL;
    if(writing) {
        const uint8_t *b=p;
        if(n>=2 && b[0]=='i' && b[1]=='n') {gpio_init(pin);gpio_set_dir(pin,GPIO_IN);return (int)n;}
        if(n>=3 && b[0]=='o' && b[1]=='u' && b[2]=='t') {gpio_init(pin);gpio_set_dir(pin,GPIO_OUT);return (int)n;}
        if(b[0]!='0' && b[0]!='1') return OS_EINVAL;
        gpio_put(pin,b[0]=='1');
        return (int)n;
    }
    ((uint8_t*)p)[0]=gpio_get(pin)?'1':'0';
    return 1;
}

int gpio_driver_ioctl(unsigned pin, int cmd) {
    if(pin<16 && fb_active()) return OS_EBUSY;
    if(!pin_gpio(pin)) return OS_EINVAL;
    switch(cmd) {
        case OS_GPIO_INPUT: gpio_init(pin);gpio_set_dir(pin,GPIO_IN);break;
        case OS_GPIO_OUTPUT: gpio_init(pin);gpio_set_dir(pin,GPIO_OUT);break;
        case OS_GPIO_PULL_UP: gpio_pull_up(pin);break;
        case OS_GPIO_PULL_DOWN: gpio_pull_down(pin);break;
        case OS_GPIO_NO_PULL: gpio_disable_pulls(pin);break;
        default: return OS_EINVAL;
    }
    return 0;
}
