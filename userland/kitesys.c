/* 0..9 counter on 8x8 FB; buttons shift glyph; GP18 beeps on each tick. */
#include <stdint.h>
#include "unistd.h"
#include "fcntl.h"
#include "stdio.h"
#include "../include/piconix.h"

/* Digits 0..9 from the supplied 8x8 LED patterns.
 * Row bits follow the framebuffer horizontal wiring order. */
static const uint8_t font[10][7] = {
    {0x00, 0x06, 0x09, 0x09, 0x09, 0x09, 0x06},
    {0x00, 0x02, 0x06, 0x02, 0x02, 0x02, 0x07},
    {0x00, 0x06, 0x09, 0x01, 0x02, 0x04, 0x0f},
    {0x00, 0x06, 0x09, 0x01, 0x0e, 0x01, 0x0e},
    {0x00, 0x0a, 0x0a, 0x0a, 0x0f, 0x02, 0x02},
    {0x00, 0x0f, 0x08, 0x0e, 0x01, 0x09, 0x06},
    {0x00, 0x06, 0x08, 0x0e, 0x09, 0x09, 0x06},
    {0x00, 0x0f, 0x09, 0x09, 0x02, 0x02, 0x04},
    {0x00, 0x06, 0x09, 0x06, 0x09, 0x09, 0x06},
    {0x00, 0x06, 0x09, 0x07, 0x01, 0x02, 0x04}
};

static int display(int fd, int digit, int shift) {
    uint8_t image[8] = {0};
    for (int y = 0; y < 7; ++y)
        image[y] = (uint8_t)(font[digit][y] << shift);
    return write(fd, image, sizeof(image)) == (int)sizeof(image) ? 0 : -1;
}

/* Returns 1 exactly once per stable press (active-low, 30 ms debounce). */
static int pressed(int fd, int *stable, int *candidate, int *samples) {
    char value;
    if (read(fd, &value, 1) != 1) return -1;
    int now = value == '0' ? 0 : 1;
    if (now != *candidate) {
        *candidate = now;
        *samples = 1;
    } else if (*samples < 3) {
        ++*samples;
    }
    if (*samples == 3 && now != *stable) {
        *stable = now;
        return now == 0;
    }
    return 0;
}

static int pwm_value(int fd, const char *value) {
    int n = 0;
    while (value[n]) ++n;
    return write(fd, value, (size_t)n) == n ? 0 : -1;
}

int main(void) {
    int fb = open("/dev/gpio/fb", O_RDWR);
    int left = open("/dev/gpio/16", O_RDONLY);
    int right = open("/dev/gpio/17", O_RDONLY);
    int freq = open("/dev/pwm/1/freq", O_WRONLY);
    int buzzer = open("/dev/pwm/1/a", O_WRONLY);
    if (fb < 0 || left < 0 || right < 0 || freq < 0 || buzzer < 0) return 1;
    if (pwm_value(buzzer, "0\n") < 0 ||
        pwm_value(freq, "2000\n") < 0) return 1;

    /* Configure both switches as inputs with internal pull-ups. */
    if (ioctl(left, OS_GPIO_INPUT, 0) < 0 || ioctl(left, OS_GPIO_PULL_UP, 0) < 0 ||
        ioctl(right, OS_GPIO_INPUT, 0) < 0 || ioctl(right, OS_GPIO_PULL_UP, 0) < 0 ||
        ioctl(fb, OS_FB_ENABLE, 0) < 0) return 1;

    puts("KiteSys: framebuffer enabled");
    puts("KiteSys: Use 'fbctl off' to release GPIO0-15 after exit.");

    int digit = 0, shift = 1;
    int stable[2] = {1, 1}, candidate[2] = {1, 1}, samples[2] = {0, 0};
    if (display(fb, digit, shift) < 0) return 1;

    int elapsed = 0, beep_remaining = 0;
    for (;;) {
        int a = pressed(left, &stable[0], &candidate[0], &samples[0]);
        int b = pressed(right, &stable[1], &candidate[1], &samples[1]);
        if (a < 0 || b < 0) return 1;
        int old_shift = shift;
        if (a == 1 && shift > 0) --shift; /* GP16: shift toward the opposite side */
        if (b == 1 && shift < 3) ++shift; /* GP17: shift toward the opposite side */
        if (shift != old_shift && display(fb, digit, shift) < 0) return 1;

        msleep(10);
        if (beep_remaining > 0) {
            beep_remaining -= 10;
            if (beep_remaining == 0 && pwm_value(buzzer, "0\n") < 0) return 1;
        }
        elapsed += 10;
        if (elapsed >= 1000) {
            elapsed -= 1000;
            digit = (digit + 1) % 10;
            if (display(fb, digit, shift) < 0 ||
                pwm_value(buzzer, "50\n") < 0) return 1;
            beep_remaining = 50;
        }
    }
}
