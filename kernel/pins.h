/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef PICONIX_PINS_H
#define PICONIX_PINS_H
/* Edit pin modes here; GPIO, PWM and ADC listings follow these settings. */
#define PIN_GPIO 0
#define PIN_ADC  1
#define PIN_PWM  2
#define PIN_UART 3
#define GP0_MODE PIN_GPIO
#define GP1_MODE PIN_GPIO
#define GP2_MODE PIN_GPIO
#define GP3_MODE PIN_GPIO
#define GP4_MODE PIN_GPIO
#define GP5_MODE PIN_GPIO
#define GP6_MODE PIN_GPIO
#define GP7_MODE PIN_GPIO
#define GP8_MODE PIN_GPIO
#define GP9_MODE PIN_GPIO
#define GP10_MODE PIN_GPIO
#define GP11_MODE PIN_GPIO
#define GP12_MODE PIN_GPIO
#define GP13_MODE PIN_GPIO
#define GP14_MODE PIN_GPIO
#define GP15_MODE PIN_GPIO
#define GP16_MODE PIN_GPIO
#define GP17_MODE PIN_GPIO
#define GP18_MODE PIN_PWM
#define GP19_MODE PIN_PWM
#define GP20_MODE PIN_UART
#define GP21_MODE PIN_UART
#define GP22_MODE PIN_GPIO
#define GP23_MODE PIN_GPIO
#define GP24_MODE PIN_GPIO
#define GP25_MODE PIN_GPIO
#define GP26_MODE PIN_ADC
#define GP27_MODE PIN_ADC
#define GP28_MODE PIN_ADC

#define PWM_DEFAULT_FREQ 1000u
#define PWM_DEFAULT_DUTY 0u
/* USB CDC is the default console: GP0/1 are free GPIO pins.
 * UART0 console builds must reserve GP0/1 for UART instead. */
#if defined(PICONIX_CONSOLE_USB) && !PICONIX_CONSOLE_USB
#error "LED framebuffer requires USB CDC console (GP0/1 reserved for LED matrix)"
#if GP0_MODE != PIN_UART || GP1_MODE != PIN_UART
#error "UART0 console requires GP0/1 to be PIN_UART"
#endif
#endif
/* UART1 uses GP20/21. */
#if GP20_MODE != PIN_UART || GP21_MODE != PIN_UART
#error "UART1 requires GP20/21 to be PIN_UART"
#endif
#if (GP0_MODE != PIN_GPIO && GP0_MODE != PIN_UART) || (GP1_MODE != PIN_GPIO && GP1_MODE != PIN_UART)
#error "GP0/1 support GPIO or UART only"
#endif
/* RP2040: ADC on GP26..28, PWM slice 1 A/B on GP18/19. */
#if (GP18_MODE != PIN_GPIO && GP18_MODE != PIN_PWM) || (GP19_MODE != PIN_GPIO && GP19_MODE != PIN_PWM)
#error "GP18/19 support GPIO or PWM only"
#endif
#if (GP26_MODE != PIN_GPIO && GP26_MODE != PIN_ADC) || (GP27_MODE != PIN_GPIO && GP27_MODE != PIN_ADC) || (GP28_MODE != PIN_GPIO && GP28_MODE != PIN_ADC)
#error "GP26..28 support GPIO or ADC only"
#endif
#if GP2_MODE != PIN_GPIO
#error "GP2 supports GPIO only"
#endif
#if GP3_MODE != PIN_GPIO
#error "GP3 supports GPIO only"
#endif
#if GP6_MODE != PIN_GPIO
#error "GP6 supports GPIO only"
#endif
#if GP7_MODE != PIN_GPIO
#error "GP7 supports GPIO only"
#endif
#if GP8_MODE != PIN_GPIO
#error "GP8 supports GPIO only"
#endif
#if GP9_MODE != PIN_GPIO
#error "GP9 supports GPIO only"
#endif
#if GP10_MODE != PIN_GPIO
#error "GP10 supports GPIO only"
#endif
#if GP11_MODE != PIN_GPIO
#error "GP11 supports GPIO only"
#endif
#if GP12_MODE != PIN_GPIO
#error "GP12 supports GPIO only"
#endif
#if GP13_MODE != PIN_GPIO
#error "GP13 supports GPIO only"
#endif
#if GP14_MODE != PIN_GPIO
#error "GP14 supports GPIO only"
#endif
#if GP15_MODE != PIN_GPIO
#error "GP15 supports GPIO only"
#endif
#if GP16_MODE != PIN_GPIO
#error "GP16 supports GPIO only"
#endif
#if GP17_MODE != PIN_GPIO
#error "GP17 supports GPIO only"
#endif
#if GP22_MODE != PIN_GPIO
#error "GP22 supports GPIO only"
#endif
#if GP23_MODE != PIN_GPIO
#error "GP23 supports GPIO only"
#endif
#if GP24_MODE != PIN_GPIO
#error "GP24 supports GPIO only"
#endif
#if GP25_MODE != PIN_GPIO
#error "GP25 supports GPIO only"
#endif

#define PIN_MODE(n) pin_mode(n)
static inline int pin_mode(unsigned pin) {
    static const unsigned char modes[29] = {
GP0_MODE, GP1_MODE, GP2_MODE, GP3_MODE, GP4_MODE, GP5_MODE, GP6_MODE, GP7_MODE, GP8_MODE, GP9_MODE, GP10_MODE, GP11_MODE, GP12_MODE, GP13_MODE, GP14_MODE, GP15_MODE, GP16_MODE, GP17_MODE, GP18_MODE, GP19_MODE, GP20_MODE, GP21_MODE, GP22_MODE, GP23_MODE, GP24_MODE, GP25_MODE, GP26_MODE, GP27_MODE, GP28_MODE
    };
    return pin<29 ? modes[pin] : -1;
}
static inline int pin_gpio(unsigned pin) { return pin_mode(pin)==PIN_GPIO; }
static inline int pin_adc(unsigned ch) { return ch<3 && pin_mode(26u+ch)==PIN_ADC; }
static inline int pin_pwm(unsigned pin) { return (pin==18 || pin==19) && pin_mode(pin)==PIN_PWM; }
static inline int pin_any_pwm(void) { return pin_pwm(18) || pin_pwm(19); }
#endif
