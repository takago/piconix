/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "piconix.h"
#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "hardware/sync.h"
#include <stdint.h>
#include <stddef.h>

static void idle_task(void) {
    for (;;) {
        os_console_poll();
        __wfi();
    }
}
extern const unsigned char piconix_shell_image_start[], piconix_shell_image_end[];
extern const unsigned char piconix_blink_image_start[], piconix_blink_image_end[];
extern const unsigned char piconix_omikuji_image_start[], piconix_omikuji_image_end[];
extern const unsigned char piconix_python_image_start[], piconix_python_image_end[];

static void register_image(int id, uintptr_t entry, const char *name,
                           const unsigned char *start, const unsigned char *end) {
    size_t size=(size_t)(end-start);
    os_register_program(id,(os_entry_t)(entry|1u),name);
    if(os_register_ram_image(id,start,size,size,0)<0)
        panic("PicoNIX: RAM image registration failed");
}
int main(void) {
    os_init();
    // Direct UART boot marker: visible even if the scheduler never starts.
#if PICONIX_CONSOLE_USB
    // USB enumeration is asynchronous: boot diagnostics may be lost if no
    // terminal is connected yet. Use UART0 build for early boot debugging.
#else
    uart_puts(uart0, "\r\n[PicoNIX] UART initialized\r\n");
#endif
    register_image(2,0x20010000u,"shell",piconix_shell_image_start,piconix_shell_image_end);
    register_image(1,0x20020000u,"blink",piconix_blink_image_start,piconix_blink_image_end);
    register_image(3,0x20030000u,"omikuji",piconix_omikuji_image_start,piconix_omikuji_image_end);
    register_image(4,0x20030000u,"python",piconix_python_image_start,piconix_python_image_end);
    os_init_idle(idle_task);
    if(os_start_program(2)<0) panic("PicoNIX: shell start failed");
    // Do not start Blink here: verify the `blink` shell command path.
#if !PICONIX_CONSOLE_USB
    uart_puts(uart0, "[PicoNIX] shell loaded; starting scheduler\r\n");
#endif
    os_start();
}
