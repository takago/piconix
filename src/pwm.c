/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "devices.h"
#include "pins.h"
#include "hardware/pwm.h"
#include "hardware/clocks.h"
#include "pico/stdlib.h"
#include <stdint.h>
static unsigned pwm_freq_hz=PWM_DEFAULT_FREQ;
static unsigned pwm_duty[2]={PWM_DEFAULT_DUTY,PWM_DEFAULT_DUTY};
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
    pwm_set_chan_level(slice,PWM_CHAN_A,(uint16_t)(pwm_duty[0]==100 ? period-1 : period*pwm_duty[0]/100));
    pwm_set_chan_level(slice,PWM_CHAN_B,(uint16_t)(pwm_duty[1]==100 ? period-1 : period*pwm_duty[1]/100));
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
    else { if(value>100 || !pin_pwm(which==1 ? 18 : 19)) return -1; pwm_duty[which-1]=value; }
    apply_pwm(); return 0;
}
