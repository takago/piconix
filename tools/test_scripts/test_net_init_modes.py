#!/usr/bin/env python3
"""Compile the actual net_init() source in both STA and AP configurations.

The normal Pico W build compiles only one Wi-Fi mode at a time.  This
regression check catches AP-only symbols referenced by the STA branch even
when the AP-only definitions are excluded by the preprocessor.
"""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[2]
net = (root / "kernel/net.c").read_text()
start_marker = "extern uint32_t piconix_flash_id(void);\nvoid net_init(void) {"
end_marker = "\n#if !PICONIX_WIFI_STA\n/* Called outside cyw43_arch_poll(), never from an lwIP callback. */"
assert net.count(start_marker) == 1
assert net.count(end_marker) == 1
fragment = net[net.index(start_marker):net.index(end_marker)]

stub = r'''
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#define CYW43_ITF_AP 1
#define CYW43_AUTH_WPA2_AES_PSK 1
#define ERR_OK 0
#define PICONIX_STA_SSID "test-ssid"
typedef struct { uint32_t address; } ip4_addr_t;
struct netif { int unused; };
struct { struct netif netif[2]; } cyw43_state;
#define IP4_ADDR(p,a,b,c,d) ((p)->address = ((uint32_t)(a) << 24) | \
    ((uint32_t)(b) << 16) | ((uint32_t)(c) << 8) | (uint32_t)(d))
static int wifi_ready;
static char wifi_ssid[33];
static struct netif *loop_iface;
struct netif *netif_find(const char *);
int cyw43_arch_init(void);
void cyw43_arch_enable_sta_mode(void);
void mdns_resp_init(void);
#if PICONIX_WIFI_STA
static char sta_mdns_name[18];
#else
static int ap_mdns_error, ap_mdns_registered, ap_dhcp_active;
void cyw43_arch_enable_ap_mode(const char *, const char *, int);
void netif_set_addr(struct netif *, const ip4_addr_t *,
                    const ip4_addr_t *, const ip4_addr_t *);
int mdns_resp_add_netif(struct netif *, const char *);
int net_service_telnet_init(void);
int net_service_remote_devctl_init(void);
int ap_dhcp_server_start(void);
#endif
'''

with tempfile.TemporaryDirectory(prefix="piconix_net_init_") as temp:
    source = Path(temp) / "net_init.c"
    source.write_text(stub + "\n" + fragment + "\n")
    for mode, name in [(0, "AP"), (1, "STA")]:
        subprocess.run(
            ["cc", "-std=c11", "-Wall", "-Wextra", "-Werror",
             "-fsyntax-only", f"-DPICONIX_WIFI_STA={mode}", str(source)],
            check=True,
        )
        print(f"PASS: net_init {name} mode compile")
