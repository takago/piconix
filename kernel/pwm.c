/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "devices.h"
#include "pins.h"
#include "hardware/pwm.h"
#include "hardware/clocks.h"
#include "pico/stdlib.h"
#include <stdint.h>
static unsigned pwm_freq_hz=PWM_DEFAULT_FREQ;
/* Duty cycles are stored in tenths of a percent (0..1000). */
static unsigned pwm_duty[2]={PWM_DEFAULT_DUTY*10u,PWM_DEFAULT_DUTY*10u};
static void apply_pwm(void) {
    if(!pin_any_pwm()) return;
    uint32_t clk=clock_get_hz(clk_sys);
    uint32_t div=(clk/pwm_freq_hz+65534u)/65535u;
    if(div<1) div=1;
    if(div>255) div=255;
    uint32_t period=clk/(pwm_freq_hz*div);
    if(period<1) period=1;
    if(period>65535) period=65535;
    unsigned slice=pwm_gpio_to_slice_num(18);
    pwm_set_enabled(slice,false);
    pwm_set_clkdiv_int_frac(slice,(uint8_t)div,0);
    pwm_set_wrap(slice,(uint16_t)(period-1));
    /* 100%% level must not wrap in 16-bit arithmetic. */
    pwm_set_chan_level(slice,PWM_CHAN_A,(uint16_t)(pwm_duty[0]==1000 ? period-1 : (uint32_t)((uint64_t)period*pwm_duty[0]/1000u)));
    pwm_set_chan_level(slice,PWM_CHAN_B,(uint16_t)(pwm_duty[1]==1000 ? period-1 : (uint32_t)((uint64_t)period*pwm_duty[1]/1000u)));
    pwm_set_enabled(slice,true);
}
void pwm_driver_init(void) {
    if(pin_pwm(18)) gpio_set_function(18,GPIO_FUNC_PWM);
    if(pin_pwm(19)) gpio_set_function(19,GPIO_FUNC_PWM);
    apply_pwm();
}
unsigned pwm_driver_get(unsigned which) { return which==0 ? pwm_freq_hz : pwm_duty[which-1]; }
int pwm_driver_set(unsigned which, unsigned value) {
    if(which>2) return -1;
    if(which==0) { if(value<8 || value>100000 || !pin_any_pwm()) return -1; pwm_freq_hz=value; }
    else { if(value>1000 || !pin_pwm(which==1 ? 18 : 19)) return -1; pwm_duty[which-1]=value; }
    apply_pwm(); return 0;
}

/* Shared by local VFS writes and the UDP device service. */
int pwm_parse_value(unsigned which, const void *data, size_t n, unsigned *value) {
        if(n>12) return -1;
        const char *b=data;
        unsigned x=0; size_t i=0;
        while(i<n && b[i]>='0' && b[i]<='9') {
            x=x*10u+(unsigned)(b[i]-'0');
            if(x>1000000u) return -1;
            i++;
        }
        if(i==0) return -1;
        if(which==0) {
            if(i<n && !(i+1==n && b[i]=='\n')) return -1;
            if(x<8 || x>100000) return -1;
        } else {
            /* Accept integer percentages or exactly one decimal place. */
            unsigned tenths=x*10u;
            if(i<n && b[i]=='.') {
                i++;
                if(i>=n || b[i]<'0' || b[i]>'9') return -1;
                tenths=x*10u+(unsigned)(b[i++]-'0');
            }
            if(i<n && !(i+1==n && b[i]=='\n')) return -1;
            if(tenths>1000) return -1;
            x=tenths;
        }
    *value=x;
    return 0;
}
