/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "piconix.h"
#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "hardware/sync.h"
#include <stdint.h>
#include <stddef.h>
#include "net.h"

static void idle_task(void) {
    for (;;) {
        net_poll();
        os_console_poll();
#if !PICONIX_WIFI
        __wfi();
#endif
    }
}
extern const unsigned char piconix_shell_image_start[], piconix_shell_image_end[];
extern const unsigned char piconix_blink_image_start[], piconix_blink_image_end[];
extern const unsigned char piconix_omikuji_image_start[], piconix_omikuji_image_end[];
extern const unsigned char piconix_python_image_start[], piconix_python_image_end[];
extern const unsigned char piconix_hello_image_start[], piconix_hello_image_end[];
extern const unsigned char piconix_hexdump_image_start[], piconix_hexdump_image_end[];
extern const unsigned char piconix_trace_image_start[], piconix_trace_image_end[];

extern const unsigned char piconix_ps_image_start[], piconix_ps_image_end[];
extern const unsigned char piconix_cat_image_start[], piconix_cat_image_end[];
extern const unsigned char piconix_watch_image_start[], piconix_watch_image_end[];
extern const unsigned char piconix_pwd_image_start[], piconix_pwd_image_end[];
extern const unsigned char piconix_ls_image_start[], piconix_ls_image_end[];
extern const unsigned char piconix_rm_image_start[], piconix_rm_image_end[];
extern const unsigned char piconix_echo_image_start[], piconix_echo_image_end[];
extern const unsigned char piconix_kill_image_start[], piconix_kill_image_end[];
extern const unsigned char piconix_tty_image_start[], piconix_tty_image_end[];
extern const unsigned char piconix_fbctl_image_start[], piconix_fbctl_image_end[];
extern const unsigned char piconix_kitesys_image_start[], piconix_kitesys_image_end[];
extern const unsigned char piconix_pins_image_start[], piconix_pins_image_end[];
extern const unsigned char piconix_pwmctl_image_start[], piconix_pwmctl_image_end[];
extern const unsigned char piconix_stty_image_start[], piconix_stty_image_end[];
extern const unsigned char piconix_mb_image_start[], piconix_mb_image_end[];
extern const unsigned char piconix_reset_image_start[], piconix_reset_image_end[];
extern const unsigned char piconix_lsof_image_start[], piconix_lsof_image_end[];
extern const unsigned char piconix_pong_image_start[], piconix_pong_image_end[];
extern const unsigned char piconix_gpio_image_start[], piconix_gpio_image_end[];
extern const unsigned char piconix_fbed_image_start[], piconix_fbed_image_end[];
extern const unsigned char piconix_asm_image_start[], piconix_asm_image_end[];
extern const unsigned char piconix_help_image_start[], piconix_help_image_end[];
extern const unsigned char piconix_edit_image_start[], piconix_edit_image_end[];
extern const unsigned char piconix_mem_image_start[], piconix_mem_image_end[];
extern const unsigned char piconix_tree_image_start[], piconix_tree_image_end[];
extern const unsigned char piconix_find_image_start[], piconix_find_image_end[];
extern const unsigned char piconix_nc_image_start[], piconix_nc_image_end[];
extern const unsigned char piconix_httpd_image_start[], piconix_httpd_image_end[];
extern const unsigned char piconix_tcp_echod_image_start[], piconix_tcp_echod_image_end[];
extern const unsigned char piconix_udp_echod_image_start[], piconix_udp_echod_image_end[];
#define RAM_APP_ORIGIN 0x20010000u

static void register_image(int id, const char *name,
                           const unsigned char *start, const unsigned char *end) {
    size_t size=(size_t)(end-start);
    os_register_program(id,(os_entry_t)(RAM_APP_ORIGIN|1u),name);
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
    register_image(2,"sh",piconix_shell_image_start,piconix_shell_image_end);
    register_image(1,"blink",piconix_blink_image_start,piconix_blink_image_end);
    register_image(3,"omikuji",piconix_omikuji_image_start,piconix_omikuji_image_end);
    register_image(4,"python",piconix_python_image_start,piconix_python_image_end);
    register_image(5,"hello",piconix_hello_image_start,piconix_hello_image_end);
    register_image(6,"hexdump",piconix_hexdump_image_start,piconix_hexdump_image_end);
    register_image(7,"trace",piconix_trace_image_start,piconix_trace_image_end);
    register_image(8,"ps",piconix_ps_image_start,piconix_ps_image_end);
    register_image(9,"cat",piconix_cat_image_start,piconix_cat_image_end);
    register_image(10,"watch",piconix_watch_image_start,piconix_watch_image_end);
    register_image(11,"pwd",piconix_pwd_image_start,piconix_pwd_image_end);
    register_image(12,"ls",piconix_ls_image_start,piconix_ls_image_end);
    register_image(13,"rm",piconix_rm_image_start,piconix_rm_image_end);
    register_image(14,"echo",piconix_echo_image_start,piconix_echo_image_end);
    register_image(15,"kill",piconix_kill_image_start,piconix_kill_image_end);
    register_image(16,"tty",piconix_tty_image_start,piconix_tty_image_end);
    register_image(18,"fbctl",piconix_fbctl_image_start,piconix_fbctl_image_end);
    register_image(17,"kitesys",piconix_kitesys_image_start,piconix_kitesys_image_end);
    register_image(19,"pins",piconix_pins_image_start,piconix_pins_image_end);
    register_image(20,"pwmctl",piconix_pwmctl_image_start,piconix_pwmctl_image_end);
    register_image(21,"stty",piconix_stty_image_start,piconix_stty_image_end);
    register_image(22,"mb",piconix_mb_image_start,piconix_mb_image_end);
    register_image(23,"reset",piconix_reset_image_start,piconix_reset_image_end);
    register_image(24,"lsof",piconix_lsof_image_start,piconix_lsof_image_end);
    register_image(25,"pong",piconix_pong_image_start,piconix_pong_image_end);
    register_image(26,"gpio",piconix_gpio_image_start,piconix_gpio_image_end);
    register_image(27,"fbed",piconix_fbed_image_start,piconix_fbed_image_end);
    register_image(28,"as",piconix_asm_image_start,piconix_asm_image_end);
    register_image(29,"help",piconix_help_image_start,piconix_help_image_end);
    register_image(30,"edit",piconix_edit_image_start,piconix_edit_image_end);
    register_image(31,"mem",piconix_mem_image_start,piconix_mem_image_end);
    register_image(32,"tree",piconix_tree_image_start,piconix_tree_image_end);
    register_image(33,"find",piconix_find_image_start,piconix_find_image_end);
    register_image(34,"nc",piconix_nc_image_start,piconix_nc_image_end);
    register_image(35,"httpd",piconix_httpd_image_start,piconix_httpd_image_end);
    register_image(36,"tcp_echod",piconix_tcp_echod_image_start,piconix_tcp_echod_image_end);
    register_image(37,"udp_echod",piconix_udp_echod_image_start,piconix_udp_echod_image_end);
    net_init();
    os_init_idle(idle_task);
    if(os_start_program(2)<0) panic("PicoNIX: shell start failed");
    // Do not start Blink here: verify the `blink` shell command path.
#if !PICONIX_CONSOLE_USB
    uart_puts(uart0, "[PicoNIX] shell loaded; starting scheduler\r\n");
#endif
    os_start();
}
