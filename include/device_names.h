/* VFSの固定デバイス名とIDの対応を一元化する． */
/* 固定デバイスのIDと標準パスをVFSとtrace表示で共有する． */
#ifndef PICONIX_DEVICE_NAMES_H
#define PICONIX_DEVICE_NAMES_H
enum {
    DEV_CONSOLE = 0,
    DEV_FB = 128,
    DEV_FBCHAR = 146,
    DEV_UART1 = 30,
    DEV_WIFI_TTY = 97,
    DEV_WIFI_CTRL = 138,
    DEV_RAND = 94,
    DEV_MEM = 98,
    DEV_SHM = 140,
    DEV_TRACE = 99,
    DEV_I2C1 = 139,
    DEV_NET_PACKET = 144,
    DEV_NET_ICMP = 145,
    DEV_PTY_MASTER = 274,
    DEV_PTY_SLAVE = 275,
    DEV_PTY_SCREEN = 276,
    DEV_PTY_KEY = 277,
};
/* X(path, id)はビルド条件に依存しない固定エントリを展開する． */
#define PICONIX_FIXED_DEVICES(X) \
    X("/dev/console", DEV_CONSOLE) \
    X("/dev/fb", DEV_FB) \
    X("/dev/fbchar", DEV_FBCHAR) \
    X("/dev/ttyS1", DEV_UART1) \
    X("/dev/ptmx", DEV_PTY_MASTER) \
    X("/dev/pts/0", DEV_PTY_SLAVE) \
    X("/dev/pts/0.screen", DEV_PTY_SCREEN) \
    X("/dev/pts/0.key", DEV_PTY_KEY) \
    X("/dev/rand", DEV_RAND) \
    X("/dev/mem", DEV_MEM) \
    X("/dev/shm", DEV_SHM) \
    X("/dev/trace", DEV_TRACE) \
    X("/dev/i2c/1", DEV_I2C1)
#define PICONIX_WIFI_DEVICES(X) \
    X("/dev/ttyN0", DEV_WIFI_TTY) \
    X("/dev/net/wifi", DEV_WIFI_CTRL) \
    X("/dev/net/packet", DEV_NET_PACKET) \
    X("/dev/net/icmp", DEV_NET_ICMP)
#endif
