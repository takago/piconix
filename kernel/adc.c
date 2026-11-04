/* SPDX-License-Identifier: GPL-3.0-or-later */
/* ADC値および内部温度センサの読み取りと文字列への変換を担当する． */

#include "devices.h"
#include "pins.h"
#include "hardware/adc.h"
#include <stdio.h>
#include <stdint.h>
/* ADCと温度センサを初期化し，利用可能な入力ピンをADC機能に切り替える． */
void adc_driver_init(void) {
    adc_init();
    for(unsigned ch=0;ch<3;ch++) if(pin_adc(ch)) adc_gpio_init(26u+ch);
    adc_set_temp_sensor_enabled(true);
}

/* 指定ADCチャネルを選択して生の12ビット測定値を返す． */
unsigned adc_driver_read(unsigned channel) {
    adc_select_input(channel);
    return adc_read();
}

/* ADC値をテキストへ変換する．temperature指定時はRP2040の換算式で温度を表示する． */
int adc_driver_format(unsigned raw, int temperature, char *text, size_t size) {
    if(!temperature) return snprintf(text,size,"%u\n",raw);
    /* RP2040データシートの温度換算式を整数の0.1度単位で計算する． */
    int32_t v_01mv=(int32_t)(raw*33000u/4096u);
    int32_t temp_01c=270-(v_01mv-7060)*1000/1721;
    unsigned mag=(unsigned)(temp_01c<0 ? -temp_01c : temp_01c);
    return snprintf(text,size,"%s%u.%u\n",temp_01c<0?"-":"",mag/10u,mag%10u);
}
