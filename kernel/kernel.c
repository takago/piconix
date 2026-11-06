/* SPDX-License-Identifier: GPL-3.0-or-later */
/* PicoNIXのカーネル本体．固定長のタスク・FD・パイプを管理し，
 * SVCによるシステムコール，PendSVによる切替，VFSとTTYを結び付ける．
 * ユーザポインタは現在の24 KiBタスク領域内かを検査してから参照する． */

#include "../include/gpio_status.h"
#include "../include/device_names.h"
#include "piconix.h"
#include "devices.h"
#include "pins.h"
#include "pico/stdlib.h"
#include "pico/unique_id.h"
#include "hardware/uart.h"
#include "hardware/clocks.h"
#include "hardware/structs/scb.h"
#include "hardware/structs/systick.h"
#include "hardware/sync.h"
#include "hardware/irq.h"
#include "net.h"
#include "net_socket.h"
#include <string.h>
#include <stdio.h>
#include "../include/piconix_trace.h"
#include "../include/piconix_dirent.h"
#include "../include/piconix_syscall.h"
#include "../include/piconix_select.h"
#include "pico/stdio_usb.h"
#include "tusb.h"

/* 関数トレース対象以外にはコンパイラの計測フックを挿入しない． */
#define NO_TRACE __attribute__((no_instrument_function))

/* Flash固有IDの下位32ビットを組み立てる．APのSSIDなどに使う識別値であり，暗号用途ではない． */
NO_TRACE uint32_t piconix_flash_id(void) {
    pico_unique_board_id_t id;
    pico_get_unique_board_id(&id);
    return ((uint32_t)id.id[PICO_UNIQUE_BOARD_ID_SIZE_BYTES-4] << 24) |
           ((uint32_t)id.id[PICO_UNIQUE_BOARD_ID_SIZE_BYTES-3] << 16) |
           ((uint32_t)id.id[PICO_UNIQUE_BOARD_ID_SIZE_BYTES-2] << 8) |
           (uint32_t)id.id[PICO_UNIQUE_BOARD_ID_SIZE_BYTES-1];
}

/* VFSのパス・ディレクトリ内部ID．別ファイルへ分割せず管理する． */
enum {
    DEV_SOCKET_UDP = 400,
    DEV_SOCKET_TCP = 401,
    DEV_SOCKET_TCP_ACCEPTED = 402,
    DEV_GPIO_VALUE_BASE = 1,
    DEV_GPIO_VALUE_END = 29,
    DEV_PROC_UPTIME = 31,
    DEV_PROC_TICKS = 32,
    DEV_PROC_INTERRUPTS = 33,
    DEV_PROC_SWITCHES = 34,
    DEV_PROC_MEMORY = 35,
    DEV_PROC_RAMFILES = 136,
    DEV_PROC_WIFI = 137,
    DEV_PROC_AP = 141,
    DEV_PROC_ARP = 142,
    DEV_PROC_NET_MEMORY = 143,
    DEV_GPIO_DIR_BASE = 36,
    DEV_GPIO_DIR_END = 64,
    DEV_GPIO_PULL_BASE = 65,
    DEV_GPIO_PULL_END = 93,
    DEV_PROC_DEVICES = 95,
    DEV_PROC_TASKS = 96,
    DEV_ADC_BASE = 121,
    DEV_ADC_END = 124,
    DEV_PWM_FREQ = 125,
    DEV_PWM_18 = 126,
    DEV_PWM_19 = 127,
    DEV_SYMS_KERNEL = 135,
    DEV_SYMS_USER_BASE = 200, /* 200〜263：登録済みユーザプログラム */
    DEV_PROC_FB = 129,
    DEV_PROC_TASK_BASE = 130, /* 130〜135：6個のタスクスロット用ID */
    DEV_TMP_FIRST = 100,
    DEV_GPIO_MAX_PIN = 28
};
enum { VDIR_ROOT=300, VDIR_BIN, VDIR_DEV, VDIR_GPIO, VDIR_ADC,
       VDIR_PWM, VDIR_PWM1, VDIR_PROC, VDIR_TASK, VDIR_SYM,
       VDIR_USERLAND, VDIR_NET, VDIR_TMP, VDIR_ETC, VDIR_DEV_NET, VDIR_I2C };

static int path_match(const char *p, const char *s);

/* 固定デバイス表はopen()と/proc/devicesの列挙で共有する． */
#define DEVICE_ENTRY(path, id) {path, id},
static const struct { const char *path; int id; } dev_registry[] = {
    PICONIX_FIXED_DEVICES(DEVICE_ENTRY)
#if PICONIX_WIFI
    PICONIX_WIFI_DEVICES(DEVICE_ENTRY)
#endif
};
#undef DEVICE_ENTRY
#define DEV_REGISTRY_COUNT (sizeof(dev_registry)/sizeof(dev_registry[0]))
/* 固定仮想ファイルのパスからIDへの対応をopenとFD表示で共有する． */
static const struct { const char *path; int id; } vfs_registry[] = {
    {"/proc/uptime",DEV_PROC_UPTIME}, {"/proc/ticks",DEV_PROC_TICKS},
    {"/proc/interrupts",DEV_PROC_INTERRUPTS},
    {"/proc/context_switches",DEV_PROC_SWITCHES},
    {"/proc/memory",DEV_PROC_MEMORY}, {"/proc/devices",DEV_PROC_DEVICES},
    {"/proc/ramfiles",DEV_PROC_RAMFILES},
    {"/proc/net/wifi",DEV_PROC_WIFI},
    {"/proc/net/ap",DEV_PROC_AP}, {"/proc/net/arp",DEV_PROC_ARP},
    {"/proc/net/memory",DEV_PROC_NET_MEMORY},
    {"/proc/tasks",DEV_PROC_TASKS}, {"/proc/fb",DEV_PROC_FB},
    {"/proc/sym/kernel",DEV_SYMS_KERNEL},
    {"/dev/pwm/1/freq",DEV_PWM_FREQ}, {"/dev/pwm/1/a",DEV_PWM_18},
    {"/dev/pwm/1/b",DEV_PWM_19}, {"/dev/adc/temp",DEV_ADC_END}
};
#define VFS_REGISTRY_COUNT (sizeof(vfs_registry)/sizeof(vfs_registry[0]))
/* 固定デバイス／仮想ファイルの絶対パスを内部IDに変換する．未登録なら-1．openと表示側で同じ表を共有する． */
static int vfs_fixed_path_id(const char *path) {
    for(unsigned i=0;i<DEV_REGISTRY_COUNT;i++)
        if(path_match(path,dev_registry[i].path))
            return dev_registry[i].id;
    for(unsigned i=0;i<VFS_REGISTRY_COUNT;i++)
        if(path_match(path,vfs_registry[i].path))
            return vfs_registry[i].id;
    return -1;
}

static const char *const vdir_paths[] = {
    "/", "/bin", "/dev", "/dev/gpio", "/dev/adc",
    "/dev/pwm", "/dev/pwm/1", "/proc", "/proc/task",
    "/proc/sym", "/proc/sym/userland", "/proc/net", "/tmp", "/etc", "/dev/net", "/dev/i2c"
};
/* 固定ディレクトリ名をIDへ変換する．一致しなければ-1． */
static int vfs_directory_id(const char *path) {
    for(unsigned i=0;i<sizeof(vdir_paths)/sizeof(vdir_paths[0]);i++)
        if(!strcmp(path,vdir_paths[i]))
            return VDIR_ROOT+(int)i;
    return -1;
}

/* 固定ディレクトリIDから絶対パスを求める．該当IDがなければNULLを返す． */
NO_TRACE static const char *vfs_directory_path(int id) {
    return id>=VDIR_ROOT && id<=VDIR_I2C ? vdir_paths[id-VDIR_ROOT] : 0;
}

/* 登録済みのデバイス・仮想ファイルIDをパスに戻す．見つからなければNULL． */
NO_TRACE static const char *vfs_fixed_id_path(int id) {
    for(unsigned i=0;i<DEV_REGISTRY_COUNT;i++)
        if(dev_registry[i].id==id)
            return dev_registry[i].path;
    for(unsigned i=0;i<VFS_REGISTRY_COUNT;i++)
        if(vfs_registry[i].id==id)
            return vfs_registry[i].path;
    return 0;
}

/* 固定デバイス登録表の項目数を返す．列挙とopenで同じ表を使う． */
NO_TRACE static unsigned vfs_device_count(void) { return DEV_REGISTRY_COUNT; }

/* 固定デバイス表の指定番号に対応するパスを返す．範囲外ならNULL． */
NO_TRACE static const char *vfs_device_path(unsigned index) {
    return index<DEV_REGISTRY_COUNT ? dev_registry[index].path : 0;
}

/* GPIOのファイル名を内部IDへ変換する．00などの別名は拒否し，列挙結果と一致する表記だけを認める． */
NO_TRACE static int vfs_gpio_id(const char *name) {
    if(*name<'0' || *name>'9')
        return -1;
    if(*name=='0' && name[1]>='0' && name[1]<='9')
        return -1;
    unsigned pin=0;
    do {
        pin=pin*10u+(unsigned)(*name++-'0');
        if(pin>DEV_GPIO_MAX_PIN)
            return -1;
    } while(*name>='0' && *name<='9');
    if(!pin_gpio(pin))
        return -1;
    if(!*name)
        return DEV_GPIO_VALUE_BASE+(int)pin;
    if(!strcmp(name,".dir"))
        return DEV_GPIO_DIR_BASE+(int)pin;
    if(!strcmp(name,".pull"))
        return DEV_GPIO_PULL_BASE+(int)pin;
    return -1;
}

enum { UNUSED, READY, SLEEPING, DEAD, BLOCKED };
#define IDLE_SLOT OS_MAX_TASKS
#define TOTAL_SLOTS (OS_MAX_TASKS + 1)
/* タスクごとのFD表．GPIO数とは無関係に定義する． */
#define OS_FD_MAX 16
/* 負の待機IDはデバイスID・waitpid(-2)と衝突させない． */
#define WAIT_TCP_ACCEPT (-3)
#define WAIT_TCP_READ   (-4)
#define WAIT_UDP_READ   (-5)
#define WAIT_SELECT     (-6)
/* 無名パイプの端点IDとデータ領域は固定のSRAM予算で管理する． */
#define PIPE_COUNT 2
#define PIPE_BYTES 256
#define PIPE_DEV_BASE 268 /* ユーザシンボルIDの範囲と重複させない． */
#define PIPE_READ_DEV(i) (PIPE_DEV_BASE + (i)*2)
#define PIPE_WRITE_DEV(i) (PIPE_DEV_BASE + (i)*2 + 1)
typedef struct { uint8_t data[PIPE_BYTES]; uint16_t head, used; uint8_t readers, writers; } pipe_t;
static pipe_t pipes[PIPE_COUNT];
/* デバイスIDがパイプの読取端・書込端に属するか判定する． */
NO_TRACE static int pipe_end(int dev) { return dev>=PIPE_DEV_BASE && dev<PIPE_DEV_BASE+PIPE_COUNT*2; }

/* パイプ端点のデバイスIDから，対応する固定長パイプ管理領域を求める． */
NO_TRACE static pipe_t *pipe_for_dev(int dev) { return &pipes[(dev-PIPE_DEV_BASE)/2]; }

/* パイプ端点のIDから書込側かどうかを判定する． */
NO_TRACE static int pipe_is_writer(int dev) { return (dev-PIPE_DEV_BASE)&1; }

/* パイプ端点の参照数をdeltaだけ増減する．両端の参照がなくなったときにリングを再利用可能な状態へ戻す． */
NO_TRACE static void pipe_ref(int dev, int delta) {
    if(!pipe_end(dev))
        return;
    pipe_t *p=pipe_for_dev(dev);
    uint8_t *count=pipe_is_writer(dev) ? &p->writers : &p->readers;
    *count=(uint8_t)((int)*count+delta);
    if(!p->readers && !p->writers) { p->head=0; p->used=0; }
}

/* 固定256バイトのリングで部分読み書きを行う．要求長を満たせないときも転送済みバイト数を返し，待機判断は呼出側に任せる． */
static int pipe_transfer(pipe_t *p, void *buffer, size_t size, int writing) {
    uint8_t *buf=(uint8_t *)buffer;
    unsigned count=0;
    while(count<size && count<PIPE_BYTES) {
        if(writing) {
            if(p->used==PIPE_BYTES)
                break;
            p->data[(p->head+p->used)&(PIPE_BYTES-1)]=buf[count];
            p->used++;
        } else {
            if(!p->used)
                break;
            buf[count]=p->data[p->head];
            p->head=(p->head+1)&(PIPE_BYTES-1);
            p->used--;
        }
        count++;
    }
    return (int)count;
}
typedef struct { uint32_t *sp; uint32_t wake; uint8_t state; os_entry_t entry; const char *name; int fd[OS_FD_MAX]; uint32_t fd_pos[OS_FD_MAX]; int fd_task_pid[OS_FD_MAX]; uint8_t fd_flags[OS_FD_MAX]; int pid; int parent_pid; int startup_seconds; char script_path[32]; int wait_dev; int wait_fd; void *wait_buf; size_t wait_size; uint8_t wait_frame_ready; int wait_pid; int *wait_status; uint8_t tty; uint8_t program_id; char cwd[128]; uint16_t adc_values[8]; } task_t;
// 24 KiBのRAMスロット6個を物理アドレスで予約する．.bssには含めない．
// リンカはカーネルの配置先を0x20000000〜0x2000DFFFに限定する必要がある．
_Static_assert(OS_MAX_TASKS == 6 && OS_TASK_BYTES == 24576u,
               "fixed RP2040 SRAM slot geometry changed");
__asm__(".global task_ram\n.set task_ram, 0x2000E000");
extern uint8_t task_ram[OS_MAX_TASKS][OS_TASK_BYTES];
// スロットはリンカのセクションを持たない固定アドレス領域である．
static task_t tasks[TOTAL_SLOTS];
/* Pico WではCYW43/lwIPの処理に使うアイドルスタックをネットワーク予約領域の末尾4 KiBに置く．通常のPicoではカーネル側の.bssに置き，両者ともELF配置検査の対象とする． */
#define IDLE_STACK_BYTES 4096u
#if PICONIX_WIFI
__asm__(".global idle_stack\n.set idle_stack, 0x2003F000");
extern uint8_t idle_stack[IDLE_STACK_BYTES];
#else
static uint8_t idle_stack[IDLE_STACK_BYTES] __attribute__((aligned(8)));
#endif
static volatile int current = -1;

static volatile uint32_t ticks;
/* /dev/rand用の教育向け擬似乱数．暗号学的な安全性はない． */
static uint32_t rand_state = 0x9e3779b9u;
/* xorshiftによる32ビット擬似乱数を更新して返す．暗号用ではない． */
static uint32_t rand32(void) {
    uint32_t x = rand_state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    rand_state = x;
    return x;
}
static volatile uint32_t svc_count, pendsv_count, context_switch_count;
static int started;
static int next_pid=1;
#if PICONIX_WIFI
#define OS_TTY_COUNT 3
#else
#define OS_TTY_COUNT 2
#endif
static volatile int foreground_pid[OS_TTY_COUNT];
/* TTYごとの前景グループ．最大3段のパイプを管理する． */
static int foreground_group[OS_TTY_COUNT][3];
static int tty_shell_pid[OS_TTY_COUNT];
/* 端末モードは前景プログラムが一時的に使用し，終了時に復元する． */
#if PICONIX_WIFI
#define TTY_DEFAULTS {1,1,1}
#else
#define TTY_DEFAULTS {1,1}
#endif
static uint8_t tty_canonical[OS_TTY_COUNT]=TTY_DEFAULTS, tty_echo[OS_TTY_COUNT]=TTY_DEFAULTS;
static uint8_t tty_onlcr[OS_TTY_COUNT]=TTY_DEFAULTS, tty_last_cr[OS_TTY_COUNT];
#undef TTY_DEFAULTS
static int tty_mode_owner[OS_TTY_COUNT];
/* 前景タスクの終了時には，親の端末モードを復元する． */
static int tty_restore_pid[OS_TTY_COUNT];
static uint8_t tty_saved_canon[OS_TTY_COUNT], tty_saved_echo[OS_TTY_COUNT], tty_saved_onlcr[OS_TTY_COUNT];
static int tty_saved_owner[OS_TTY_COUNT];
/* 使用されていない端末の一時的な排他利用． */
static int tty_claim_pid[OS_TTY_COUNT];
static void tty_mode_reset(int dev);
/* タスク終了処理から参照するため，TTYバッファを先に宣言する． */
#define TTY_LINES 8u
typedef struct {
    char line[128]; unsigned len;
    struct {char text[128];uint8_t len;} lines[TTY_LINES];
    unsigned head,tail,used,offset;
} tty_buffer_t;
static tty_buffer_t tty_buf[OS_TTY_COUNT];
/* RAMファイルの後ろに16 KiBのトレースリングを固定配置する． */
#define TRACE_CAPACITY 512u
_Static_assert(sizeof(trace_event_t) == 32, "trace record must be 32 bytes");
_Static_assert(TRACE_CAPACITY * sizeof(trace_event_t) == 0x4000u,
               "trace ring must exactly fit the 16-KiB SRAM gap");
/* task_ramと同様にトレース領域も.bssを使用しない．0x20038000〜0x2003BFFFを固定アドレスで予約し，記録のない領域を起動時に消去しない． */
__asm__(".global trace_ring\n.set trace_ring, 0x20038000");
extern trace_event_t trace_ring[TRACE_CAPACITY];
static uint32_t trace_head;
static uint32_t trace_count;
static uint32_t trace_serial;
static uint8_t trace_enabled;
#if PICONIX_KERNEL_FUNCTION_TRACE
static uint8_t trace_all_contexts;
#endif
static uint16_t tick_sample;
static uint8_t slice_ticks;
/* 32バイトのトレースイベントを16 KiBリングへ記録する．割込み・SVC・スケジューラが競合しないよう更新全体を割込み禁止区間に置く． */
static void __attribute__((no_instrument_function)) trace_record(uint8_t kind, uint8_t pid, uint32_t value, uint32_t detail,
                         uint8_t program_id, int8_t file_id, const void *data, unsigned len) {
    if(!trace_enabled || (program_id & 0x7fu)==7u)
        return;
    /* trace自身のタスク切替は記録せず，表示内容の自己増殖を防ぐ． */
    if(kind==TRACE_SWITCH) {
        for(int i=0;i<OS_MAX_TASKS;i++)
            if((tasks[i].pid==pid || tasks[i].pid==(int)value) &&
               tasks[i].state!=UNUSED && tasks[i].program_id==7u) return;
    }
    /* SysTick・UART・SVCから同時に記録し得るので，レコードとリング管理値の更新を割込み禁止区間に置く． */
    uint32_t irq_state=save_and_disable_interrupts();
    trace_event_t *e = &trace_ring[trace_head];
    e->tick = ticks;
    e->value = value;
    e->detail = detail;
    e->kind = kind;
    e->pid = pid;
    /* kindの上位2ビットでTTY（0=CDC，1=UART1，2=Telnet）を表す．program_idを保ち，32バイトのイベント形式を変更しない． */
    for(int i=0;i<OS_MAX_TASKS;i++)
        if((kind==TRACE_SWITCH ? value : pid) &&
           tasks[i].pid==(int)(kind==TRACE_SWITCH ? value : pid) &&
           tasks[i].state!=UNUSED) {
            e->kind |= (uint8_t)((tasks[i].tty & 3u) << 6);
            break;
        }
    e->program_id = program_id;
    e->file_id = file_id;
    memset(e->data, 0, sizeof e->data);
    if(data) {
        if(len > sizeof e->data)
            len = sizeof e->data;
        memcpy(e->data, data, len);
    }
    trace_head = (trace_head + 1u) & (TRACE_CAPACITY - 1u);
    if(trace_count < TRACE_CAPACITY)
        trace_count++;
    trace_serial++;
    restore_interrupts(irq_state);
}

/* 指定PIDの子を現在のTTYの前景へ移す．子が走る前に設定することで，spawn後の別システムコールとの競合を避ける． */
NO_TRACE static int foreground_child(int pid) {
    int tty=tasks[current].tty;
    for(int i=0;i<OS_MAX_TASKS;i++) if(tasks[i].pid==pid &&
        tasks[i].parent_pid==tasks[current].pid && tasks[i].state!=DEAD) {
        if(foreground_pid[tty]!=pid && !tty_restore_pid[tty]) {
            tty_saved_canon[tty]=tty_canonical[tty];
            tty_saved_echo[tty]=tty_echo[tty];
            tty_saved_onlcr[tty]=tty_onlcr[tty];
            tty_saved_owner[tty]=tty_mode_owner[tty];
            tty_restore_pid[tty]=pid;
        }
        if(foreground_pid[tty]==tasks[current].pid)
            for(int g=0;g<3;g++)
                foreground_group[tty][g]=0;
        foreground_pid[tty]=pid;
        int present=0;
        for(int g=0;g<3;g++)
            if(foreground_group[tty][g]==pid)
                present=1;
        if(!present) for(int g=0;g<3;g++) if(!foreground_group[tty][g]) {
            foreground_group[tty][g]=pid; break;
        }
        return 0;
    }
    return OS_ESRCH;
}

/* TTYを占有するプロセスを求める．明示的占有，前景，シェルの順で優先する． */
NO_TRACE static int tty_owner_pid(int tty) { return tty_claim_pid[tty] ? tty_claim_pid[tty] :
    (foreground_pid[tty] ? foreground_pid[tty] : tty_shell_pid[tty]); }
/* スロットの再利用とは独立した有限個の終了通知． */
static struct { int pid, parent_pid; } exited[OS_MAX_TASKS];
/* 親PIDに紐づく終了通知を一件取り出す．通知はPIDのみ保持し，スロット再利用とwaitpidを両立する．見つからなければ0． */
NO_TRACE static int take_exit(int parent, int requested) {
    for(int i=0;i<OS_MAX_TASKS;i++)
        if(exited[i].pid && exited[i].parent_pid==parent &&
           (requested==-1 || exited[i].pid==requested)) {
            int pid=exited[i].pid;
            exited[i].pid=0;
            return pid;
        }
    return 0;
}

/* 一つのFDに関連する内部ID，位置，フラグ，対象PIDをまとめて設定する．各操作でフィールドの不整合を作らないための共通入口． */
static void fd_set(task_t *t, int fd, int dev, uint32_t pos, int flags, int target_pid) {
    t->fd[fd]=dev;
    t->fd_pos[fd]=pos;
    t->fd_task_pid[fd]=target_pid;
    t->fd_flags[fd]=(uint8_t)flags;
}
/* 一つのFDを解放する．ソケットはタスクが所有し，パイプは端点ごとの参照数を持つ． */
static int udp_owner_pid;
#define TCP_USER_SLOTS NET_SOCKET_TCP_SLOTS
static int tcp_owner_pid[TCP_USER_SLOTS];
/* タスク内のTCP用FDを検証し，対応するネットワーク管理スロットを返す． */
NO_TRACE static int tcp_fd_slot(task_t *t,int fd) {
    if(fd<0 || fd>=OS_FD_MAX ||
       (t->fd[fd]!=DEV_SOCKET_TCP && t->fd[fd]!=DEV_SOCKET_TCP_ACCEPTED)) return -1;
    unsigned id=t->fd_pos[fd];
    return id<TCP_USER_SLOTS && tcp_owner_pid[id]==t->pid ? (int)id : -1;
}

/* 同一TCP管理スロットを参照する別のFDを探し，重複参照の後始末に使う． */
NO_TRACE static int tcp_other_fd(task_t *t,int id,int except) {
    for(int j=3;j<OS_FD_MAX;j++)
        if(j!=except && (t->fd[j]==DEV_SOCKET_TCP || t->fd[j]==DEV_SOCKET_TCP_ACCEPTED) &&
           t->fd_pos[j]==(unsigned)id) return 1;
    return 0;
}

/* ソケットFDは一つのlwIP資源を所有するため，パイプと異なりdup()の対象外とする． */
NO_TRACE static int fd_is_socket(int dev) {
    return dev==DEV_SOCKET_UDP || dev==DEV_SOCKET_TCP ||
           dev==DEV_SOCKET_TCP_ACCEPTED;
}
static unsigned icmp_fd_refs;
/* FDが保持する資源を解放する．パイプは端点参照数，ネットワークソケットは所有者とlwIP資源も更新する． */
static void fd_release(task_t *t,int fd) {
    int dev=t->fd[fd];
    if(dev<0)
        return;
    if(dev==DEV_SOCKET_UDP) {
        net_socket_udp_close(); udp_owner_pid=0;
    } else if(dev==DEV_SOCKET_TCP || dev==DEV_SOCKET_TCP_ACCEPTED) {
        int id=tcp_fd_slot(t,fd);
        if(id>=0) {
            if(dev==DEV_SOCKET_TCP)
                net_socket_tcp_close_listener(id);
            else net_socket_tcp_close_client(id);
            if(!tcp_other_fd(t,id,fd)) {
                net_socket_tcp_close(id);
                tcp_owner_pid[id]=0;
            }
        }
    }
    if(dev==DEV_NET_PACKET)
        net_packet_reader(-1);
    if(dev==DEV_NET_ICMP && --icmp_fd_refs==0)
        net_echo_close();
    pipe_ref(dev,-1);
    fd_set(t,fd,-1,0,0,0);
}

/* FDを子／別番号へ複製する．パイプやパケット監視用デバイスの参照数を増やし，最後のcloseまで実体を保つ． */
NO_TRACE static void fd_inherit(task_t *dst, int to, const task_t *src, int from) {
    pipe_ref(src->fd[from],1);
    if(src->fd[from]==DEV_NET_PACKET)
        net_packet_reader(1);
    if(src->fd[from]==DEV_NET_ICMP)
        icmp_fd_refs++;
    fd_set(dst,to,src->fd[from],src->fd_pos[from],src->fd_flags[from],src->fd_task_pid[from]);
}

/* 同一タスク内でFDの参照先と状態を複製する．端点の参照数も継承側で管理する． */
static void fd_copy(task_t *t, int dst, int src) {
    fd_inherit(t,dst,t,src);
}

/* 終了済みのタスクスロットは直ちに再利用できる． */
NO_TRACE static int task_active(const task_t *t) {
    return t->state!=UNUSED && t->state!=DEAD;
}

/* パイプ段のうち現在生存する前景プロセスを探す．先行段だけが終了しても残った段へTTY入力を渡す． */
NO_TRACE static int tty_foreground_survivor(int tty) {
    for(int g=2;g>=0;g--) {
        int pid=foreground_group[tty][g];
        if(!pid)
            continue;
        for(int i=0;i<OS_MAX_TASKS;i++)
            if(tasks[i].pid==pid && tasks[i].tty==tty && task_active(&tasks[i]))
                return pid;
    }
    return 0;
}

/* タスク終了時に通知，FD，通信資源，TTYの前景・モードを後始末する．スロットは再利用可能だがwaitpid用のPID通知は別に保持する． */
NO_TRACE static void finish_task(int slot, int status) {
    (void)status; /* PicoNIXが終了後に保持するのはPIDであり，終了ステータスではない． */
    task_t *t=&tasks[slot];
    if(!task_active(t))
        return;
    int parent_alive=0;
    for(int i=0;i<OS_MAX_TASKS;i++)
        if(i!=slot && tasks[i].pid==t->parent_pid &&
           task_active(&tasks[i])) parent_alive=1;
    if(parent_alive) {
        /* 終了通知領域が満杯なら最も古い位置の通知を上書きする． */
        int pos=-1;
        for(int i=0;i<OS_MAX_TASKS;i++)
            if(!exited[i].pid) { pos=i; break; }
        if(pos<0) pos=0;
        exited[pos].pid=t->pid;
        exited[pos].parent_pid=t->parent_pid;
    }
    /* 終了済みの親は通知を受け取れないため破棄する． */
    for(int i=0;i<OS_MAX_TASKS;i++)
        if(exited[i].parent_pid==t->pid)
            exited[i].pid=0;
    for(int fd=0;fd<OS_FD_MAX;fd++) fd_release(t,fd);
    /* FDが失われた場合にも，タスクに紐づいたソケットを後始末する． */
    if(udp_owner_pid==t->pid) { net_socket_udp_close(); udp_owner_pid=0; }
    for(int id=0;id<TCP_USER_SLOTS;id++) if(tcp_owner_pid[id]==t->pid) { net_socket_tcp_close(id); tcp_owner_pid[id]=0; }
    t->state=DEAD;
    /* 終了したパイプ段を外してから生存者を決める．シェルは別の段の終了待ちを継続する場合がある． */
    for(int g=0;g<3;g++)
        if(foreground_group[t->tty][g]==t->pid)
            foreground_group[t->tty][g]=0;
    int survivor=tty_foreground_survivor(t->tty);
    if(foreground_pid[t->tty]==t->pid)
        foreground_pid[t->tty]=survivor;
    if(tty_shell_pid[t->tty]==t->pid)
        tty_shell_pid[t->tty]=0;
    for(int dev=0;dev<OS_TTY_COUNT;dev++) {
        if(tty_claim_pid[dev]==t->pid) {
            tty_claim_pid[dev]=0;
            tty_mode_reset(dev);
        } else if(tty_restore_pid[dev]==t->pid) {
            if(dev==t->tty && survivor) {
                /* 前景パイプの最初の段が先に終わっても，全段の終了まで端末モードを復元しない． */
                tty_restore_pid[dev]=survivor;
                if(tty_mode_owner[dev]==t->pid)
                    tty_mode_owner[dev]=survivor;
            } else {
                tty_canonical[dev]=tty_saved_canon[dev];
                tty_echo[dev]=tty_saved_echo[dev];
                tty_onlcr[dev]=tty_saved_onlcr[dev];
                tty_last_cr[dev]=0;
                tty_mode_owner[dev]=tty_saved_owner[dev];
                tty_restore_pid[dev]=0;
                /* 前景コマンドの終了時に未消費の入力キューを保持する． */
            }
        } else if(tty_mode_owner[dev]==t->pid) tty_mode_reset(dev);
    }
}

static int tty_rx_ready(int tty);

/* 中断中のSVCの保存フレームへ戻り値を書き込み，待機情報を消してREADYにする．通常の関数returnでは再開できない点に注意． */
static void task_unblock(task_t *t, int result) {
    t->sp[8]=(uint32_t)result;
    t->wait_dev=-1;
    t->wait_fd=-1;
    t->wait_buf=NULL;
    t->wait_size=0;
    t->wait_pid=0;
    t->wait_status=NULL;
    t->wait_frame_ready=0;
    t->state=READY;
}

/* ブロック中のパイプread/writeを再試行する．保存フレーム完成後だけ復帰させ，書き手消滅時のEOF・読み手消滅時のエラーを扱う． */
NO_TRACE static void wake_pipes(void) {
    for(int i=0;i<OS_MAX_TASKS;i++) {
        task_t *t=&tasks[i];
        if(t->state!=BLOCKED || !t->wait_frame_ready ||
           !pipe_end(t->wait_dev)) continue;
        int dev=t->wait_dev, writing=(dev-PIPE_DEV_BASE)&1;
        pipe_t *p=pipe_for_dev(dev);
        int result;
        if(writing && !p->readers)
            result=OS_EIO;
        else if(!writing && !p->used && !p->writers) result=0;
        else if((writing && p->used==PIPE_BYTES) || (!writing && !p->used)) continue;
        else {
            result=pipe_transfer(p,t->wait_buf,t->wait_size,writing);
        }
        task_unblock(t,result);
    }
}

/* lwIPの受信状態をスケジューラから確認してソケット待機を解除する．
 * コールバックはIDLEタスク側で実行するため，その途中ではPCBに触れない． */
NO_TRACE static void wake_sockets(void) {
    if(net_poll_in_progress()) return;
    for(int i=0;i<OS_MAX_TASKS;i++) {
        task_t *t=&tasks[i];
        if(t->state!=BLOCKED || !t->wait_frame_ready ||
           t->wait_dev>WAIT_TCP_ACCEPT || t->wait_dev<WAIT_UDP_READ) continue;
        int fd=t->wait_fd, result=OS_EBADF;
        if(fd>=0 && fd<OS_FD_MAX) {
            if(t->wait_dev==WAIT_TCP_ACCEPT && t->fd[fd]==DEV_SOCKET_TCP) {
                int id=(int)t->fd_pos[fd];
                if(id<TCP_USER_SLOTS && tcp_owner_pid[id]==t->pid && net_socket_tcp_listening(id)) {
                    int newfd=-1;
                    for(int j=3;j<OS_FD_MAX;j++) if(t->fd[j]<0) { newfd=j; break; }
                    if(newfd<0)
                        result=OS_EMFILE;
                    else if(!net_socket_tcp_accept_ready(id)) continue;
                    else {
                        fd_set(t,newfd,DEV_SOCKET_TCP_ACCEPTED,(uint32_t)id,OS_O_RDWR,0);
                        result=newfd;
                    }
                }
            } else if(t->wait_dev==WAIT_TCP_READ &&
                      (t->fd[fd]==DEV_SOCKET_TCP || t->fd[fd]==DEV_SOCKET_TCP_ACCEPTED)) {
                int id=(int)t->fd_pos[fd];
                if(id<TCP_USER_SLOTS && tcp_owner_pid[id]==t->pid) {
                    int status=net_socket_tcp_status(id);
                    if(status==-2)
                        result=0;
                    else if(status<0) result=OS_EIO;
                    else if(status==0) continue;
                    else {
                        int n=net_socket_tcp_read(id,t->wait_buf,(unsigned)t->wait_size);
                        if(n>0)
                            result=n;
                        else if(n==-2) result=0;
                        else if(n<0) result=OS_EIO;
                        else continue;
                    }
                }
            } else if(t->wait_dev==WAIT_UDP_READ && t->fd[fd]==DEV_SOCKET_UDP &&
                      udp_owner_pid==t->pid) {
                piconix_udp_packet_t *pkt=(piconix_udp_packet_t *)t->wait_buf;
                int n=net_socket_udp_recv((void *)pkt->data,pkt->length,&pkt->peer_ip,&pkt->peer_port);
                if(n==-3)
                    continue;
                result=n==-2 ? OS_EINVAL : n<0 ? OS_EIO : n;
            }
        }
        trace_record(TRACE_SVC_EXIT,(uint8_t)t->pid,
                     t->wait_dev==WAIT_TCP_ACCEPT ? SYS_ACCEPT_TCP :
                     t->wait_dev==WAIT_UDP_READ ? SYS_RECVFROM : SYS_READ,
                     (uint32_t)result,t->program_id,-1,NULL,0);
        task_unblock(t,result);
    }
}

/* 選択したFDの読書き可能性を調べる．要求マスクを変更せず，返却用マスクだけ更新する．
 * 今回はTCP/UDP，TTY，パイプを対象とし，その他のFDはEINVALとする． */
NO_TRACE static int select_ready(task_t *t,int nfds,piconix_select_request_t *r) {
    uint16_t reads=0,writes=0;
    unsigned wanted=(unsigned)(r->read_mask | r->write_mask);
    if(nfds<0 || nfds>OS_FD_MAX || (wanted >> nfds))
        return OS_EINVAL;
    for(int fd=0;fd<nfds;fd++) {
        unsigned bit=1u<<fd;
        if(!(wanted&bit))
            continue;
        int dev=t->fd[fd], rd=0, wr=0;
        if(dev<0)
            return OS_EBADF;
        if(dev==DEV_SOCKET_UDP) {
            if(udp_owner_pid!=t->pid)
                return OS_EBADF;
            rd=net_socket_udp_read_ready(); wr=1;
        } else if(dev==DEV_SOCKET_TCP || dev==DEV_SOCKET_TCP_ACCEPTED) {
            int id=(int)t->fd_pos[fd];
            if(id<0 || id>=TCP_USER_SLOTS || tcp_owner_pid[id]!=t->pid)
                return OS_EBADF;
            int listening=(dev==DEV_SOCKET_TCP && net_socket_tcp_listening(id));
            rd=listening ? net_socket_tcp_accept_peek(id) : net_socket_tcp_recv_ready(id);
            wr=listening ? 0 : net_socket_tcp_write_ready(id);
        } else if(dev==DEV_CONSOLE || dev==DEV_UART1 || dev==DEV_WIFI_TTY) {
            int tty=dev==DEV_CONSOLE?0:dev==DEV_UART1?1:2;
            rd=tty_rx_ready(tty); wr=1;
        } else if(pipe_end(dev)) {
            pipe_t *pipe=pipe_for_dev(dev);
            if(pipe_is_writer(dev))
                wr=pipe->used<PIPE_BYTES || !pipe->readers;
            else rd=pipe->used>0 || !pipe->writers;
        } else return OS_EINVAL;
        if(rd && (r->read_mask&bit))
            reads|=(uint16_t)bit;
        if(wr && (r->write_mask&bit))
            writes|=(uint16_t)bit;
    }
    r->ready_read=reads; r->ready_write=writes;
    unsigned bits=(unsigned)(reads|writes), count=0;
    while(bits) { count+=bits&1u; bits>>=1; }
    return (int)count;
}

/* SVCで停止したselectをスケジューラから再検査する．時間切れは0を返す． */
NO_TRACE static void wake_selects(void) {
    if(net_poll_in_progress()) return;
    for(int i=0;i<OS_MAX_TASKS;i++) {
        task_t *t=&tasks[i];
        if(t->state!=BLOCKED || t->wait_dev!=WAIT_SELECT || !t->wait_frame_ready)
            continue;
        piconix_select_request_t *r=(piconix_select_request_t *)t->wait_buf;
        int n=select_ready(t,(int)t->wait_size,r);
        if(n==0 && (r->timeout_ms<0 || (int32_t)(ticks-t->wake)<0))
            continue;
        trace_record(TRACE_SVC_EXIT,(uint8_t)t->pid,SYS_SELECT,(uint32_t)n,t->program_id,-1,NULL,0);
        task_unblock(t,n);
    }
}

/* 子の終了を待っていた親を起こす．終了通知のPIDをSVC戻り値へ渡し，タスクスロットを占有し続けない． */
NO_TRACE static void wake_waiters(void) {
    for(int i=0;i<OS_MAX_TASKS;i++) {
        task_t *t=&tasks[i];
        if(t->state!=BLOCKED || t->wait_dev!=-2 || !t->wait_frame_ready)
            continue;
        int pid=take_exit(t->pid,t->wait_pid);
        if(!pid)
            continue;
        if(t->wait_status)
            *t->wait_status=0;
        trace_record(TRACE_SVC_EXIT,(uint8_t)t->pid,SYS_WAITPID,
                     (uint32_t)pid,t->program_id,-1,NULL,0);
        task_unblock(t,pid);
    }
}

/* プログラムは位置独立であり，実行時に空きスロットのRAMアドレスを与える． */
#define PROGRAM_CAPACITY 65  /* プログラムIDは1〜64を使用し，0を予約する． */
static struct { os_entry_t entry; const char *name; } programs[PROGRAM_CAPACITY];
/* 未使用または終了済みの24 KiBユーザスロットを探す．空きがなければ-1． */
NO_TRACE static int free_program_slot(void) {
    for (int i=0;i<OS_MAX_TASKS;i++)
        if (tasks[i].state==UNUSED || tasks[i].state==DEAD)
            return i;
    return -1;
}
// 事前にリンクされたRAMイメージを任意登録する．サイズが非ゼロの場合，スロット配置用の正しいバイナリと入口が必要である．
// 使用するイメージは固定スロット向けに生成する．
static struct { const uint8_t *image; size_t size; size_t zero_start; size_t zero_size; } images[PROGRAM_CAPACITY];
/* Flash上のPICイメージとゼロ初期化範囲を登録する．登録時にはRAMへコピーせず，spawn時に空きスロットへ展開する． */
NO_TRACE int os_register_ram_image(int id, const void *image, size_t size,
                          size_t zero_start, size_t zero_size) {
    if (started || id < 1 || id >= PROGRAM_CAPACITY || !image || !size)
        return OS_EINVAL;
    if (size > OS_TASK_BYTES - 4096u || zero_start > OS_TASK_BYTES - 4096u ||
        zero_size > OS_TASK_BYTES - 4096u - zero_start ||
        (zero_size && zero_start < size)) return OS_EINVAL;
    images[id].image = image;
    images[id].size = size;
    images[id].zero_start = zero_start;
    images[id].zero_size = zero_size;
    return 0;
}
#define RX_CAP 256
static struct { uint8_t buf[RX_CAP]; volatile uint16_t head, tail; } rx[2];
/* TTYを標準の行入力・エコー・改行変換へ戻し，未完了入力も初期化する． */
NO_TRACE static void tty_mode_reset(int dev) {
    tty_canonical[dev]=tty_echo[dev]=tty_onlcr[dev]=1;
    tty_last_cr[dev]=0;
    tty_mode_owner[dev]=0;
    memset(&tty_buf[dev],0,sizeof tty_buf[dev]);
}

/* 非カノニカル入力を小さな固定キューへ積む．1バイトごとに行スロットを消費しないよう，末尾へまとめる． */
NO_TRACE static void tty_raw_enqueue(int dev,uint8_t c) {
    tty_buffer_t *t=&tty_buf[dev];
    if(t->used) {
        unsigned last=(t->head+TTY_LINES-1u)%TTY_LINES;
        /* 読み出し途中の末尾項目には追記しない． */
        if((last!=t->tail || t->offset==0) &&
           t->lines[last].len<sizeof(t->lines[last].text)) {
            t->lines[last].text[t->lines[last].len++]=(char)c;
            return;
        }
    }
    if(t->used==TTY_LINES)return;
    t->lines[t->head].text[0]=(char)c;
    t->lines[t->head].len=1;
    t->head=(t->head+1u)%TTY_LINES;
    t->used++;
}

/* 編集済み行を受信キューへ移す．キュー満杯なら追加せず，上位の入力制御による流量制限を使う． */
NO_TRACE static void tty_enqueue(int dev) {
    tty_buffer_t *t=&tty_buf[dev];
    if(t->used==TTY_LINES)return;
    memcpy(t->lines[t->head].text,t->line,t->len);
    t->lines[t->head].len=(uint8_t)t->len;
    t->head=(t->head+1u)%TTY_LINES;t->used++;t->len=0;
}

/* TTYキューから最大nバイト返す．一行の一部を読み出した場合は位置を保持し，データがなければ-1． */
static int tty_dequeue(int dev,void *p,size_t n) {
    tty_buffer_t *t=&tty_buf[dev];
    if(!t->used)return -1;
    unsigned count=t->lines[t->tail].len-t->offset;
    if(count>n)count=(unsigned)n;
    if(count)memcpy(p,t->lines[t->tail].text+t->offset,count);
    t->offset+=count;
    if(t->offset==t->lines[t->tail].len){t->tail=(t->tail+1u)%TTY_LINES;t->used--;t->offset=0;}
    return (int)count;
}
static volatile uint32_t uart1_irq_count;
static void uart1_irq_handler(void);
static void pend(void);
static void wake_readers(int dev);
/* 指定TTYの受信リングから1バイト取り出す．空なら0を返す． */
NO_TRACE static int uart_pop(int dev, uint8_t *ch) {
    if(rx[dev].tail==rx[dev].head)
        return 0;
    *ch=rx[dev].buf[rx[dev].tail];
    rx[dev].tail=(rx[dev].tail+1)%RX_CAP;
    return 1;
}

/* UART1をTTY所有状態に応じて読み分ける．所有中は整形済み入力，未所有ならUART FIFOの生バイトを返す． */
NO_TRACE static int uart1_read(uint8_t *buf,size_t size) {
    int count=0;
    if(tty_owner_pid(1)) {
        if(tty_buf[1].used)return tty_dequeue(1,buf,size);
        return -1;
    }
    while((size_t)count<size && uart_pop(1,&buf[count]))count++;
    return count;
}

/* 受信リングへ1バイト追加する．満杯なら書き込まず，メモリを越えない． */
NO_TRACE static void uart_push(int dev, uint8_t ch) {
    unsigned next=(rx[dev].head+1)%RX_CAP;
    if(next!=rx[dev].tail) {rx[dev].buf[rx[dev].head]=ch;rx[dev].head=next;}
}
/* USBコンソールの送信はSVC内で待たずにキューへ積む．アイドル側で割込みを許可しながら送信し，空き一枠を維持する． */
#define CONSOLE_TX_CAP 512u
static uint8_t console_tx[CONSOLE_TX_CAP];
static volatile unsigned console_tx_head, console_tx_tail;
/* USB CDCへの送信データを固定長リングへ積む．例外処理中にUSB送信待ちをしないため，積めたバイト数を返す． */
NO_TRACE static size_t console_queue(const uint8_t *p, size_t n) {
    size_t done=0;
    while(done<n) {
        unsigned next=(console_tx_head+1u)%CONSOLE_TX_CAP;
        if(next==console_tx_tail)
            break;
        console_tx[console_tx_head]=p[done++];
        console_tx_head=next;
    }
    return done;
}

/* アイドルタスク側でUSB送信リングを少量ずつ空にする．USB切断時は古い出力を溜め続けない． */
NO_TRACE static void console_tx_poll(void) {
    if(!stdio_usb_connected()) {
        /* 切断済み端末の出力を無制限に蓄積しない． */
        console_tx_tail=console_tx_head;
        return;
    }
    /* TinyUSBのTX FIFOが満杯なら書かない．実際の送信はアイドル側だけで行い，USB割込みを動作可能に保つ． */
    unsigned budget=64;
    while(budget-- && console_tx_tail!=console_tx_head &&
          tud_cdc_write_available()>0) {
        putchar_raw((int)console_tx[console_tx_tail]);
        console_tx_tail=(console_tx_tail+1u)%CONSOLE_TX_CAP;
    }
}

/* USB CDCコンソールの送信キューへ1文字を渡す． */
NO_TRACE static void tty_put(uint8_t c) {
    (void)console_queue(&c,1);
}

/* TTY番号に応じてUSB CDC，UART1，Wi-Fi端末へ1文字出力する． */
NO_TRACE static void tty_put_dev(int dev,uint8_t c) {
    if(dev==0)tty_put(c);
    else if(dev==1)uart_putc_raw(uart1,c);
#if PICONIX_WIFI
    else if(dev==2)(void)net_terminal_write(&c,1);
#endif
}

/* TTY固有の出力先へ1文字を書き込む．必要ならLFの前にCRを付け，両バイトを受け付けたときだけ成功を返す． */
NO_TRACE static int tty_tx_char(int dev, uint8_t c, int insert_cr) {
    if(dev==0) {
        unsigned used=(console_tx_head+CONSOLE_TX_CAP-console_tx_tail)%CONSOLE_TX_CAP;
        if(CONSOLE_TX_CAP-1u-used < (unsigned)(1+insert_cr)) return 0;
        if(insert_cr) tty_put('\r');
        tty_put(c);
        return 1;
    }
#if PICONIX_WIFI
    if(dev==2) {
        uint8_t bytes[2]={'\r',c};
        return net_terminal_write(bytes+(!insert_cr),1+insert_cr)==1+insert_cr;
    }
#endif
    if(insert_cr) tty_put_dev(dev,'\r');
    tty_put_dev(dev,c);
    return 1;
}

/* 正規モードでTABは0x09のまま保持し，エコー表示だけ4桁タブ位置へ展開する． */
NO_TRACE static unsigned tty_line_columns(const tty_buffer_t *t) {
    unsigned col=0;
    for(unsigned i=0;i<t->len;i++)
        col+=t->line[i]=='\t' ? 4-(col&3u) : 1;
    return col;
}
/* Ctrl+CはTCPソケットの破棄を伴うため，UART割込みやlwIPコールバックの実行中には終了処理を行わない． */
static volatile unsigned char tty1_ctrl_c_pending;
/* Ctrl+Cで同じTTYの前景グループを終了させる．UART割込み内からネットワーク資源を破棄しないよう遅延実行する． */
NO_TRACE static void tty_foreground_interrupt(int dev) {
    uint32_t irq_state=save_and_disable_interrupts();
    tty_buffer_t *t=&tty_buf[dev];
    memset(t,0,sizeof *t);
    for(int g=0;g<3;g++) {
        int pid=foreground_group[dev][g];
        for(int i=0;i<OS_MAX_TASKS;i++)
            if(pid && tasks[i].pid==pid) { finish_task(i,130); break; }
        foreground_group[dev][g]=0;
    }
    restore_interrupts(irq_state);
    tty_put_dev(dev,'^');tty_put_dev(dev,'C');tty_put_dev(dev,'\r');tty_put_dev(dev,'\n');
}

/* TTYの入力モードに従い1文字を処理する．行編集，エコー，Ctrl+C，Ctrl+Dと非カノニカル入力を分ける． */
static void tty_input_char(int dev,uint8_t c) {
    tty_buffer_t *t=&tty_buf[dev];
    if(!tty_canonical[dev] && (c!=3 || tty_claim_pid[dev])){
        tty_raw_enqueue(dev,c);
        if(tty_echo[dev])tty_put_dev(dev,c);
        return;
    }
    if(c==3){
        if(dev==1) {
            /* UART割込み内では終了を保留し，アイドル側でタスクとソケットを解放する． */
            tty1_ctrl_c_pending=1;
            return;
        }
        tty_foreground_interrupt(dev);
    }else if(c==8||c==127){
        if(t->len){
            unsigned before=tty_line_columns(t);
            t->len--;
            unsigned width=before-tty_line_columns(t);
            if(tty_echo[dev])for(unsigned i=0;i<width;i++){
                tty_put_dev(dev,8);tty_put_dev(dev,' ');tty_put_dev(dev,8);
            }
        }
    }else if(c==4){tty_enqueue(dev);}
    else if(c=='\r'||c=='\n'){
        if(t->len<sizeof(t->line))t->line[t->len++]='\n';
        tty_enqueue(dev);
        if(tty_echo[dev]){tty_put_dev(dev,'\r');tty_put_dev(dev,'\n');}
    }else if((c=='\t'||(c>=32&&c<=126))&&t->len<sizeof(t->line)-2){
        unsigned col=tty_line_columns(t);
        t->line[t->len++]=(char)c;
        if(tty_echo[dev]){
            if(c=='\t')for(unsigned i=4-(col&3u);i;i--)tty_put_dev(dev,' ');
            else tty_put_dev(dev,c);
        }
    }
}
// TinyUSB CDCの所有者はpico_stdio_usbであるため，コールバックを横取りしない．
// SDKの標準入出力経由で読み出す．
// シェルが入力待ちでも，アイドル側からこの処理を呼ぶ．
/* ネットワーク端末の接続と終了はlwIPコールバックではなくアイドル側で管理し，各接続にTTY2専用シェルを割り当てる． */
#if PICONIX_WIFI
static int spawn_in_slot(int slot, int program_id, os_entry_t entry, const char *name,
                         int argc, const char *const *args, const uint8_t *raw, unsigned raw_size);
static int wifi_session_active;
static unsigned wifi_session_epoch;
/* 切断されたTelnetセッションのタスクとTTY状態を破棄する．USB/UARTの独立したセッションは触らない． */
NO_TRACE static void wifi_session_stop(void) {
    uint32_t irq_state=save_and_disable_interrupts();
    for(int i=0;i<OS_MAX_TASKS;i++) {
        if(tasks[i].tty==2 && tasks[i].state!=UNUSED && tasks[i].state!=DEAD)
            finish_task(i,130);
    }
    foreground_pid[2]=0;
    tty_shell_pid[2]=0;
    tty_claim_pid[2]=0;
    tty_restore_pid[2]=0;
    tty_mode_reset(2);
    memset(&tty_buf[2],0,sizeof tty_buf[2]);
    restore_interrupts(irq_state);
}

/* Telnetの接続状態・世代番号を監視して専用シェルを作る．高速再接続時にも旧セッションの入力や前景状態を継承させない． */
NO_TRACE static void wifi_session_poll(void) {
    int connected=net_terminal_connected();
    unsigned epoch=net_terminal_epoch();
    /* すばやい再接続でも前の接続のシェルを使い回さない． */
    if(wifi_session_active && epoch!=wifi_session_epoch) {
        wifi_session_stop();
        wifi_session_active=0;
    }
    if(!connected) {
        if(wifi_session_active)
            wifi_session_stop();
        wifi_session_active=0;
        return;
    }
    if(wifi_session_active)
        return;
    /* TCPの接続ごとに専用シェルを用意し，USBシェルとは分離する． */
    uint32_t irq_state=save_and_disable_interrupts();
    int slot=free_program_slot();
    int pid=slot<0 ? OS_EAGAIN :
        spawn_in_slot(slot,2,programs[2].entry,programs[2].name,0,NULL,NULL,0);
    if(pid>0) {
        task_t *child=&tasks[slot];
        /* spawn_in_slotは最初に標準TTY0を継承するのでTTY2へ付け替える． */
        if(tty_shell_pid[0]==pid)
            tty_shell_pid[0]=0;
        child->tty=2;
        child->parent_pid=0;
        for(int j=0;j<3;j++)
            fd_set(child,j,DEV_WIFI_TTY,0,0,0);
        tty_shell_pid[2]=pid;
        wifi_session_active=1;
        wifi_session_epoch=epoch;
    }
    restore_interrupts(irq_state);
}
#endif
/* アイドル側からUSB／Telnet入力，送信キュー，保留中のUART Ctrl+Cを処理する．割込み処理を短く保つための入口． */
NO_TRACE void os_console_poll(void) {
    console_tx_poll();
    if(tty1_ctrl_c_pending) {
        tty1_ctrl_c_pending=0;
        tty_foreground_interrupt(1);
    }
    int ch;
    int received = 0;
    while (tty_buf[0].used < TTY_LINES && (ch = getchar_timeout_us(0)) >= 0) {
        tty_input_char(0,(uint8_t)ch);
        received = 1;
    }
    if (received)
        wake_readers(0);
#if PICONIX_WIFI
    /* net_pollの後，アイドル側でTCP受信バイトをTTY2へ渡す． */
    wifi_session_poll();
    if(wifi_session_active) {
        uint8_t b[1]; int n;
        while(tty_buf[2].used<TTY_LINES && (n=net_terminal_read(b,sizeof b))>0) {
            for(int i=0;i<n;i++)tty_input_char(2,b[i]);
            wake_readers(2);
        }
    }
#endif
}

/* 共有TTY受信経路．所有者がいないUART1はrawバイト列として扱う． */
NO_TRACE static int tty_rx_ready(int tty) {
    if(tty<0 || tty>=OS_TTY_COUNT)
        return 0;
    if(tty==1 && !tty_owner_pid(1))
        return rx[1].head!=rx[1].tail;
    return tty_buf[tty].used!=0;
}

/* 対象TTYの受信キューから読み取る．UART1とその他の端末で読取元を切り替える． */
static int tty_rx_read(int tty,void *dst,size_t size) {
    if(tty==1)
        return uart1_read(dst,size);
    return tty_dequeue(tty,dst,size);
}

/* TTY入力待ちのタスクを，受信データと前景所有条件が揃った場合に再開させる．SVCフレームの保存完了を待つ． */
NO_TRACE static void wake_readers(int dev) {
    for(int i=0;i<OS_MAX_TASKS;i++) {
        task_t *t=&tasks[i];
        if(t->state!=BLOCKED || t->wait_dev!=dev || !t->wait_frame_ready)
            continue;
        if(dev<0 || dev>=OS_TTY_COUNT)
            continue;
        if(tty_owner_pid(dev) && t->pid!=tty_owner_pid(dev))
            continue;
        if(!tty_rx_ready(dev))
            continue;
        int n=tty_rx_read(dev,t->wait_buf,t->wait_size);
        // PendSVがPSPフレームを保存するまで，保存r0に触れてはならない．
        /* ブロック中の端末readは，最初のSVCではなくここで完了する． */
        trace_record(TRACE_SVC_EXIT, (uint8_t)t->pid, SYS_READ,
                     (uint32_t)n, t->program_id,
                     (int8_t)t->fd[t->wait_fd], n>0 ? t->wait_buf : NULL,
                     n>0 ? (unsigned)n : 0u);
        task_unblock(t,n);
        pend();
        if(dev==0)
            break;
    }
}

/* UART1の受信割込みを処理し，入力文字をTTYバッファへ蓄える． */
NO_TRACE static void uart1_irq_handler(void) {
    uart1_irq_count++;
    while(uart_is_readable(uart1)){
        uint8_t c=uart_getc(uart1);
        if(tty_owner_pid(1))tty_input_char(1,c);
        else uart_push(1,c);
    }
    wake_readers(1);
}

/* PendSVを保留状態にする．実際のタスク切替は例外入口で実行する． */
NO_TRACE static void pend(void) { scb_hw->icsr = (1u << 28); __asm volatile("dsb\n isb" ::: "memory"); }

/* タスクの入口関数がreturnした場合の終了処理へ誘導する． */
NO_TRACE static void task_return(void) { os_exit(); }

/* タスク表，FD，TTY，固定メモリ管理と割込みを初期化する．スケジューラ始動前に一度だけ呼ぶ． */
NO_TRACE void os_init(void) {
    memset((void *)(uintptr_t)0x20032000u,0,2048u); /* 起動時に共有RAMを初期化する． */
    memset(tasks, 0, sizeof(tasks));
    current=-1; ticks=0; uart1_irq_count=0;
    gpio_driver_init();
    fb_init();
    adc_driver_init();
    i2c_driver_init();
    pwm_driver_init();
    svc_count=0; pendsv_count=0; context_switch_count=0; started=0; next_pid=1; memset((void *)foreground_pid,0,sizeof foreground_pid); memset(foreground_group,0,sizeof foreground_group); memset(tty_shell_pid,0,sizeof tty_shell_pid); memset(exited,0,sizeof exited); memset(tty_buf,0,sizeof tty_buf); for(int ti=0;ti<OS_TTY_COUNT;ti++) tty_canonical[ti]=1; for(int ti=0;ti<OS_TTY_COUNT;ti++) tty_echo[ti]=1; for(int ti=0;ti<OS_TTY_COUNT;ti++) tty_onlcr[ti]=1; memset(tty_last_cr,0,sizeof tty_last_cr); memset(tty_mode_owner,0,sizeof tty_mode_owner); memset(tty_claim_pid,0,sizeof tty_claim_pid); memset(tty_restore_pid,0,sizeof tty_restore_pid); memset(tty_saved_owner,0,sizeof tty_saved_owner);
    memset(rx, 0, sizeof(rx));
    console_tx_head=console_tx_tail=0;
    if (!stdio_usb_init())
        panic("PicoNIX: USB CDC initialization failed");
    uart_init(uart1,115200);
    gpio_set_function(20,GPIO_FUNC_UART);
    gpio_set_function(21,GPIO_FUNC_UART);
    irq_set_exclusive_handler(UART1_IRQ,uart1_irq_handler);
    irq_set_enabled(UART1_IRQ,true);
    uart_set_irq_enables(uart1,true,false);
    // PendSVを最下位の優先度にし，SysTickをその一段上に置く．
    // SHPR3のPendSV優先度は23〜22ビット，SysTickは31〜30ビットに置く．
    *(volatile uint32_t *)0xE000ED20u = 0xC0C00000u;
}

// 一つのPICイメージはどの空きRAMスロットでも実行できる．
/* 指定スロットにPICイメージを複写し，argc/argvと初期レジスタフレームを配置する．入口のThumbビットと固定末尾スタックを保ち，PIDを返す． */
NO_TRACE static int spawn_in_slot(int slot, int program_id, os_entry_t entry, const char *name,
                         int argc, const char *const *args, const uint8_t *raw, unsigned raw_size) {
    if (!entry || slot<0 || slot>=OS_MAX_TASKS)
        return OS_EINVAL;
    uint32_t irq_state = save_and_disable_interrupts();
    task_t *t=&tasks[slot];
    if (t->state!=UNUSED && t->state!=DEAD) { restore_interrupts(irq_state); return OS_EAGAIN; }
    // 位置独立イメージをコピーする前に，対象スロットを初期化する．
    memset(task_ram[slot], 0, OS_TASK_BYTES);
    if (program_id && images[program_id].image) {
        // app_entryはslot.ldで先頭に固定し，ビルド時にも検査する．
        entry=(os_entry_t)((uintptr_t)task_ram[slot] | 1u);
        memcpy(task_ram[slot], images[program_id].image, images[program_id].size);
        memset(task_ram[slot] + images[program_id].zero_start, 0,
               images[program_id].zero_size);
    }
    if(raw && raw_size)
        memcpy(task_ram[slot],raw,raw_size);
    /* 子スロットの上端へargv配列と文字列（合計128バイトまで）を置く．初期レジスタフレームはその下に置く． */
    uint8_t *top=task_ram[slot]+OS_TASK_BYTES;
    char *strings=(char *)(top-192);
    char **argv=(char **)(top-64);
    unsigned used=0;
    if (argc==0) { /* 起動時のTTY/シェルでは，argv[0]に登録名を渡す． */
        args=&name;
        argc=1;
    }
    for(int i=0;i<argc;i++) {
        const char *src=args[i];
        unsigned len=0;
        while(src[len])
            len++;
        argv[i]=strings+used;
        memcpy(strings+used,src,len+1);
        used+=len+1;
    }
    argv[argc]=NULL;
    uint32_t *s=(uint32_t *)(top-256);
    *(--s)=0x01000000u;           // Thumb実行状態のxPSR
    *(--s)=(uint32_t)entry;       // PC
    *(--s)=(uint32_t)task_return;// LR
    *(--s)=0;                    // r12
    *(--s)=0;                    // r3
    *(--s)=0;                    // r2
    *(--s)=(uint32_t)argv;       // r1へargvを設定する．
    *(--s)=argc;                 // r0へargcを設定する．
    for(int k=0;k<8;k++)
        *(--s)=0; // r4〜r11はPendSVが保存・復元する．
    t->sp=s; t->entry=entry; t->name=name?name:"task"; t->program_id=(uint8_t)program_id;
    t->pid=next_pid++; t->startup_seconds=0; t->script_path[0]=0; t->parent_pid=(started && current>=0)?tasks[current].pid:0; t->wake=0; t->wait_dev=-1; t->wait_frame_ready=0; t->wait_pid=0; t->wait_status=NULL;
    t->wait_buf=NULL; t->wait_size=0; strcpy(t->cwd,(started && current>=0)?tasks[current].cwd:"/"); t->tty=(started && current>=0)?tasks[current].tty:0;
    for(int j=0;j<OS_FD_MAX;j++)
        fd_set(t,j,-1,0,0,0);
    for(int j=0;j<3;j++)
        fd_set(t,j,0,0,0,0);
    /* 子タスクは親の標準入出力FDを継承する． */
    if(started && current>=0 && current<OS_MAX_TASKS) {
        for(int j=0;j<3;j++) fd_inherit(t,j,&tasks[current],j);
    }
    t->state=READY;
    if(program_id==2 && !tty_shell_pid[t->tty])
        tty_shell_pid[t->tty]=t->pid;
    int pid=t->pid;
    restore_interrupts(irq_state);
    return pid;
}

/* アイドルタスク専用スタックと初期例外フレームを用意する．Pico Wではネットワーク領域の末尾4 KiBを使用する． */
NO_TRACE void os_init_idle(os_entry_t entry) {
    if(started || !entry)
        return;
    /* 初期例外フレームを作る前にアイドルスタックを0xA5で埋める．起動後に高水位を診断できる． */
    memset(idle_stack, 0xa5, sizeof(idle_stack));
    task_t *t = &tasks[IDLE_SLOT];
    uint32_t *s = (uint32_t *)(idle_stack + sizeof(idle_stack));
    *(--s)=0x01000000u;
    *(--s)=(uint32_t)entry;
    *(--s)=(uint32_t)entry;
    for(int k=0;k<5;k++)
        *(--s)=0;
    for(int k=0;k<8;k++)
        *(--s)=0;
    t->sp=s; t->entry=entry; t->name="idle";
    t->pid=0; t->state=READY;
}

/* スタックに埋めた0xA5の残量から使用量を推定する．最大使用量の目安であり，安全性の証明ではない． */
NO_TRACE unsigned os_idle_stack_used_bytes(void) {
    unsigned i=0;
    while (i<sizeof(idle_stack) && idle_stack[i]==0xa5)
        ++i;
    return (unsigned)sizeof(idle_stack)-i;
}

/* アイドルタスク専用スタックの確保サイズをバイト単位で返す． */
NO_TRACE unsigned os_idle_stack_capacity_bytes(void) { return (unsigned)sizeof(idle_stack); }

/* スケジューラ開始前に指定の入口関数をタスクとして登録する．空きスロットがなければOS_EAGAIN． */
NO_TRACE int os_spawn_named(os_entry_t entry, const char *name) {
    if (started)
        return OS_EINVAL;
    // ユーザスロットは6個．アイドルタスクは別のカーネル管理スタックを使用する．
    for(int i=0;i<OS_MAX_TASKS;i++)
        if(tasks[i].state==UNUSED)
            return spawn_in_slot(i,0,entry,name,0,NULL,NULL,0);
    return OS_EAGAIN;
}

/* プログラムID・名前・入口を起動前の表へ登録する．実行用RAMの割当ては行わない． */
NO_TRACE void os_register_program(int id,os_entry_t entry,const char *name) {
    if(started || id<1 || id>=PROGRAM_CAPACITY)
        return;
    programs[id].entry=entry; programs[id].name=name;
}

/* 登録済みプログラムを起動前の空きスロットへ配置してPIDを返す． */
NO_TRACE int os_start_program(int id) {
    if (started || id<1 || id>=PROGRAM_CAPACITY || !programs[id].entry)
        return OS_EINVAL;
    int slot=free_program_slot();
    return slot<0 ? OS_EAGAIN : spawn_in_slot(slot,id,programs[id].entry,programs[id].name,0,NULL,NULL,0);
}

/* 起動前に無名タスクを登録する簡易入口．既定名「task」を付ける． */
NO_TRACE int os_spawn(os_entry_t entry) { return os_spawn_named(entry, "task"); }

/* 1 ms周期の時刻更新と睡眠解除を行う．通常は10 tickごとにPendSVを要求し，重い処理は割込み内で行わない． */
NO_TRACE void isr_systick(void) {
    fb_tick();
    ticks += 1u;
    if(++tick_sample == 1000u) {
        tick_sample=0;
        if(current >= 0)
        trace_record(TRACE_TICK1K,
                     (uint8_t)(current == IDLE_SLOT ? 0 : tasks[current].pid),
                     100u, 0, (uint8_t)(current == IDLE_SLOT ? 0 : tasks[current].program_id),
                     -1, NULL, 0);
    }
    int woke=0;
    for(int i=0;i<OS_MAX_TASKS;i++)
        if(tasks[i].state==SLEEPING && (int32_t)(ticks-tasks[i].wake)>=0) {
            tasks[i].state=READY;
            woke=1;
        }
    if(++slice_ticks == 10u) { slice_ticks=0; pend(); }
    else if(woke) pend();
}

/* PendSVが保存した旧PSPを受け，次のタスクのPSPを返す．ブロック中のSVCを再開可能にしてからREADYタスクを選ぶ． */
NO_TRACE uint32_t *os_schedule(uint32_t *oldsp) {
    // PendSVがr4〜r11を保存し終えるまで，タスクの保存フレームは不完全である．
    // PSPの公開と待機readの再開処理中はUART割込みを抑止する．
    uint32_t irq_state=save_and_disable_interrupts();
    if(current>=0 && oldsp) {
        tasks[current].sp=oldsp;
        if(tasks[current].state==BLOCKED)
            tasks[current].wait_frame_ready=1;
    }
    // readがブロックしてからPendSV保存までにRX割込みが到着する場合がある．
    // フレームが完成した後で保留中のreadを再検査する．
    wake_readers(0);
    wake_readers(1);
    wake_waiters();
    wake_pipes();
    wake_sockets();
    wake_selects();
    pendsv_count++;
    int previous=current;
    int next=-1;
    for(int k=1;k<=OS_MAX_TASKS;k++) {
        int i=(current+k+TOTAL_SLOTS)%TOTAL_SLOTS;
        if(i==IDLE_SLOT)
            continue;
        if(tasks[i].state==READY) { next=i; break; }
    }
#if PICONIX_WIFI
    /* すべてのユーザタスクがREADYでも，50 msごとにアイドル側へ実行機会を作る．lwIP処理は例外内ではなくスレッドモードで行う．割込み禁止区間などにより厳密な周期は保証されない． */
    static uint32_t last_net_slice;
    if (next >= 0 && previous != IDLE_SLOT &&
        (uint32_t)(ticks - last_net_slice) >= 50u) {
        next = IDLE_SLOT;
        last_net_slice = ticks;
    } else if (previous == IDLE_SLOT) {
        last_net_slice = ticks;
    }
#endif
#if PICONIX_WIFI
    /* NO_SYS構成のlwIPコールバック中に，他タスクから同じPCBのraw APIを並行実行させない． */
    if(previous==IDLE_SLOT && net_poll_in_progress())
        next=IDLE_SLOT;
#endif
    if(next<0) {
        next=IDLE_SLOT;
    }
    if(previous>=0 && previous!=next) {
        context_switch_count++;
        trace_record(TRACE_SWITCH, (uint8_t)(previous == IDLE_SLOT ? 0 : tasks[previous].pid),
                     (uint32_t)(next == IDLE_SLOT ? 0 : tasks[next].pid), 0,
                     (uint8_t)(next == IDLE_SLOT ? 0 : tasks[next].program_id), -1, NULL, 0);
    }
    current=next;
    uint32_t *sp=tasks[next].sp;
    restore_interrupts(irq_state);
    return sp;
}

/* SysTickを1 msに設定し，最初のPendSVでタスク実行を開始する．以後は割込み待機から復帰するだけで戻らない． */
NO_TRACE void os_start(void) {
    if(started || tasks[IDLE_SLOT].state!=READY)
        panic("PicoNIX: missing idle task");
    started=1;
    // SysTickは1 ms周期，通常のタイムスライスは10 tickである．
    systick_hw->rvr = (clock_get_hz(clk_sys)/1000u)-1u;
    systick_hw->cvr = 0;
    systick_hw->csr = 7;
    pend();
    while(1)
        __wfi();
}

/* 有効なPIDを持つタスクのスロット番号を探す．存在しなければ-1． */
NO_TRACE static int slot_from_pid(int pid) {
    for(int i=0;i<TOTAL_SLOTS;i++)
        if(tasks[i].pid==pid && task_active(&tasks[i]))
            return i;
    return -1;
}

/* ユーザポインタpからnバイトが現在のスロット内に収まるか検査する．終端を越えた読み書きを避けるため，和のオーバーフローも防ぐ． */
static int in_task_range(const void *p, size_t n) {
    if(current<0 || current>=OS_MAX_TASKS || !p)
        return 0;
    uintptr_t lo=(uintptr_t)task_ram[current], hi=lo+OS_TASK_BYTES;
    uintptr_t a=(uintptr_t)p;
    return a>=lo && a<=hi && n<=hi-a;
}

/* この関数へ渡すパスはカーネル側で正規化済みであり，ユーザポインタの検査は呼出側で終えている． */
NO_TRACE static int path_match(const char *p, const char *s) {
    return strcmp(p,s)==0;
}
/* RAMファイルの管理情報はカーネル内，実データは固定11個×2 KiBの別領域に置く．物理領域は.bssに含めず配置検査する． */
#define TMP_FILES 11
#define TMP_BYTES 2048
#define TMP_NAME_CAP 32  /* 先頭スラッシュを除いた内部パスをNUL終端込みで保持する． */
#define SHM_BASE 0x20032000u
#define SHM_BYTES 2048u
#define RAMFILE_BASE 0x20032800u
__asm__(".global shared_ram\n.set shared_ram, 0x20032000");
__asm__(".global ramfile_data\n.set ramfile_data, 0x20032800");
static struct { char name[TMP_NAME_CAP]; uint16_t size; uint8_t used; } tmp_files[TMP_FILES];
/* RAMファイル番号から固定サイズのデータ領域先頭アドレスを計算する． */
NO_TRACE static uint8_t *tmp_payload(int id) {
    return (uint8_t *)(uintptr_t)(RAMFILE_BASE+(uint32_t)id*TMP_BYTES);
}

/* ユーザパスの参照範囲外を読み込まないようにする． */
/* /tmpまたは/etc直下の名前を検証して内部形式へ変換する．両者は11個の固定RAMファイル枠を共有する．深い階層や長すぎる名前は拒否する． */
static int tmp_name(const char *p, char name[TMP_NAME_CAP]) {
    const char *prefix;
    if(!strncmp(p,"/tmp/",5))
        prefix="tmp/";
    else if(!strncmp(p,"/etc/",5)) prefix="etc/";
    else return 0;
    p+=5;
    unsigned len=0;
    while(p[len] && p[len]!='/' && len<TMP_NAME_CAP-4)
        len++;
    if(!len || p[len])
        return 0;
    memcpy(name,prefix,4);
    memcpy(name+4,p,len);
    name[4+len]=0;
    return 1;
}
/* 初期ファイル内容はuserland/fsから生成し，Flashに格納する． */
#include "piconix_initial_fs.h"
/* Flashに埋め込まれた初期ファイルを起動後のRAMファイル領域へ一度だけ展開する． */
static void tmp_init(void) {
    static int initialized;
    if(initialized)
        return;
    initialized=1;
    for(unsigned i=0;i<piconix_initial_file_count;i++) {
        const struct piconix_initial_file *file=&piconix_initial_files[i];
        if(i>=TMP_FILES || file->size>TMP_BYTES)
            break;
        strcpy(tmp_files[i].name,file->name);
        tmp_files[i].used=1;
        tmp_files[i].size=(uint16_t)file->size;
        memcpy(tmp_payload((int)i),file->data,file->size);
    }
}

/* /tmpにあるRAMファイルを名前で検索し，該当番号または-1を返す． */
static int tmp_lookup(const char *name) {
    tmp_init();
    for(int i=0;i<TMP_FILES;i++)
        if(tmp_files[i].used && !strcmp(name,tmp_files[i].name))
            return i;
    return -1;
}

/* ユーザがopenするFDは3以降．0〜2は標準入出力用である． */
static int free_fd(void) {
    for(int fd=3;fd<OS_FD_MAX;fd++)
        if(tasks[current].fd[fd]<0)
            return fd;
    return -1;
}

/* デバイスIDが/procの仮想ファイルを表すか判定する． */
static int proc_device(int dev) {
    return (dev>=DEV_PROC_UPTIME && dev<=DEV_PROC_MEMORY) ||
           dev==DEV_PROC_DEVICES || dev==DEV_PROC_TASKS || dev==DEV_PROC_FB ||
           dev==DEV_PROC_RAMFILES || dev==DEV_PROC_WIFI ||
           dev==DEV_PROC_AP || dev==DEV_PROC_ARP ||
           dev==DEV_PROC_NET_MEMORY ||
           (dev>=DEV_PROC_TASK_BASE && dev<DEV_PROC_TASK_BASE+OS_MAX_TASKS);
}

/* 固定11個のRAMファイルスロットから対象を探し，必要なら作成する．FDやファイル領域が不足すればエラーを返す． */
static int tmp_open(const char *p,int flags) {
    tmp_init();
    char name[TMP_NAME_CAP];
    if(!tmp_name(p,name))
        return OS_ENOENT;
    int index=tmp_lookup(name);
    if(index<0) {
        if(!(flags & OS_O_CREAT))
            return OS_ENOENT;
        for(int i=0;i<TMP_FILES;i++) if(!tmp_files[i].used) { index=i; break; }
        if(index<0)
            return OS_ENOSPC;
    }
    int fd=free_fd();
    if(fd<0)
        return OS_EMFILE;
    if(!tmp_files[index].used) {
        memset(&tmp_files[index],0,sizeof(tmp_files[index]));
        memset(tmp_payload(index),0,TMP_BYTES);
        strcpy(tmp_files[index].name,name);
        tmp_files[index].used=1;
    }
    if((flags & OS_O_TRUNC) && (flags & 3)!=OS_O_RDONLY)
        tmp_files[index].size=0;
    fd_set(&tasks[current],fd,DEV_TMP_FIRST+index,
           (flags & OS_O_APPEND)?tmp_files[index].size:0,flags,0);
    return fd;
}

/* RAMファイルを削除する．他タスクがFDで参照中なら再利用による混乱を避けるためOS_EBUSYとする． */
static int tmp_unlink(const char *p) {
    tmp_init();
    char name[TMP_NAME_CAP];
    if(!tmp_name(p,name))
        return OS_ENOENT;
    int index=tmp_lookup(name);
    if(index<0)
        return OS_ENOENT;
    /* FDがファイル枠を参照している間は削除後の枠再利用を許可しない． */
    for(int t=0;t<OS_MAX_TASKS;t++) if(tasks[t].state!=UNUSED && tasks[t].state!=DEAD)
        for(int fd=0;fd<OS_FD_MAX;fd++)
            if(tasks[t].fd[fd]==DEV_TMP_FIRST+index)
                return OS_EBUSY;
    tmp_files[index].used=0;
    return 0;
}

/* ファイルの現在位置を使って読み書きする．各ファイルは最大2 KiBで，部分転送とappend・truncateを扱う． */
static int tmp_rw(int fd,void *p,size_t n,int writing) {
    tmp_init();
    int id=tasks[current].fd[fd]-DEV_TMP_FIRST;
    if(id<0 || id>=TMP_FILES || !tmp_files[id].used)
        return OS_EBADF;
    int mode=tasks[current].fd_flags[fd] & 3;
    if((writing && mode==OS_O_RDONLY) || (!writing && mode==OS_O_WRONLY))
        return OS_EBADF;
    size_t pos=tasks[current].fd_pos[fd];
    if(writing && (tasks[current].fd_flags[fd] & OS_O_APPEND))
        pos=tmp_files[id].size;
    size_t end=writing?TMP_BYTES:tmp_files[id].size;
    size_t count=pos<end?end-pos:0;
    if(count>n)
        count=n;
    if(writing) {
        if(!count && n)
            return OS_ENOSPC;
        if(pos>tmp_files[id].size)
            memset(tmp_payload(id)+tmp_files[id].size,0,pos-tmp_files[id].size);
        memcpy(tmp_payload(id)+pos,p,count);
        if(pos+count>tmp_files[id].size)
            tmp_files[id].size=(uint16_t)(pos+count);
    } else if(count) memcpy(p,tmp_payload(id)+pos,count);
    tasks[current].fd_pos[fd]=(uint32_t)(pos+count);
    return (int)count;
}

/* タスクごとのcwdを使って相対パスを正規化する．ユーザ文字列は終端NULまで各バイトの範囲を確認する． */
NO_TRACE static int absolute_path(const char *input, char out[128]) {
    char combined[256]; unsigned n=0, o=1;
    if(!in_task_range(input,1) || !*input)
        return -1;
    if(*input!='/') {
        const char *cwd=tasks[current].cwd;
        while(*cwd) { if(n>=sizeof(combined)-2) return -1; combined[n++]=*cwd++; }
        if(n==0 || combined[n-1]!='/')
            combined[n++]='/';
    }
    /* NULを含む各文字の参照前にユーザアドレスを検査する．スロット末尾で文字列が途切れても安全に失敗する． */
    for(;;) {
        if(!in_task_range(input,1))
            return -1;
        char ch=*input;
        if(!ch)
            break;
        if(n>=sizeof(combined)-1)
            return -1;
        combined[n++]=ch;
        input++;
    }
    combined[n]=0; out[0]='/'; out[1]=0;
    for(unsigned i=0;i<n;) {
        while(combined[i]=='/')
            i++;
        if(i==n)
            break;
        unsigned start=i;
        while(i<n && combined[i]!='/')
            i++;
        unsigned len=i-start;
        if(len==1 && combined[start]=='.')
            continue;
        if(len==2 && combined[start]=='.' && combined[start+1]=='.') {
            if(o>1) { while(o>1 && out[o-1]!='/') o--; if(o>1) o--; }
            out[o]=0; continue;
        }
        if(o>1) { if(o>=127) return -1; out[o++]='/'; }
        if(o+len>=128)
            return -1;
        memcpy(out+o,combined+start,len); o+=len; out[o]=0;
    }
    return 0;
}

/* VFSが公開する固定ディレクトリか調べる．使用不可のPWMディレクトリは除外する． */
NO_TRACE static int valid_directory(const char *path) {
    int id=vfs_directory_id(path);
    return id>=0 && (id!=VDIR_PWM1 || pin_any_pwm());
}

/* 仮想ディレクトリのnth番目の項目を計算する．一覧をメモリに展開せず，getdents用にその場で一件を返す． */
NO_TRACE static int vdir_nth(int dir, unsigned nth, piconix_dirent_t *ent) {
    unsigned count=0;
#define EMIT(n,d) do { if(count++==nth) { snprintf(ent->name,sizeof(ent->name),"%s",(n)); ent->is_dir=(d); return 1; } } while(0)
    char name[32];
    /* UNIXのgetdentsと同じく，先頭に「.」「..」を返す． */
    EMIT(".",1); EMIT("..",1);
    switch(dir) {
    case VDIR_ROOT:
        EMIT("bin",1); EMIT("dev",1); EMIT("proc",1); EMIT("tmp",1); EMIT("etc",1); break;
    case VDIR_BIN: case VDIR_USERLAND:
        for(int i=1;i<PROGRAM_CAPACITY;i++)
            if(programs[i].entry && programs[i].name) { EMIT(programs[i].name,0); }
        break;
    case VDIR_TMP: case VDIR_ETC:
        tmp_init();
    for(int i=0;i<TMP_FILES;i++) if(tmp_files[i].used) {
            const char *prefix=dir==VDIR_TMP?"tmp/":"etc/";
            if(!strncmp(tmp_files[i].name,prefix,4))
                EMIT(tmp_files[i].name+4,0);
        }
        break;
    case VDIR_DEV:
        for(unsigned i=0;i<vfs_device_count();i++) {
            const char *path=vfs_device_path(i);
            if(!strncmp(path,"/dev/",5) && !strchr(path+5,'/'))
                EMIT(path+5,0);
        }
        EMIT("gpio",1); EMIT("adc",1); EMIT("pwm",1); EMIT("i2c",1);
#if PICONIX_WIFI
        EMIT("net",1);
#endif
        break;
    case VDIR_DEV_NET:
#if PICONIX_WIFI
        EMIT("wifi",0); EMIT("packet",0); EMIT("icmp",0);
#endif
        break;
    case VDIR_GPIO:
        EMIT("fb",0);
        for(unsigned i=0;i<=28;i++) if(pin_gpio(i)) {
            snprintf(name,sizeof name,"%u",i); EMIT(name,0);
            snprintf(name,sizeof name,"%u.dir",i); EMIT(name,0);
            snprintf(name,sizeof name,"%u.pull",i); EMIT(name,0);
        }
        break;
    case VDIR_I2C:
        EMIT("1",0); break;
    case VDIR_ADC:
        for(unsigned i=0;i<3;i++) if(pin_adc(i)) {
            snprintf(name,sizeof name,"%u",i); EMIT(name,0);
        }
        EMIT("temp",0); break;
    case VDIR_PWM:
        if(pin_any_pwm()) { EMIT("1",1); } break;
    case VDIR_PWM1:
        if(pin_any_pwm()) { EMIT("freq",0); }
        if(pin_pwm(18)) { EMIT("a",0); }
        if(pin_pwm(19)) { EMIT("b",0); } break;
    case VDIR_PROC:
        EMIT("uptime",0); EMIT("ticks",0); EMIT("interrupts",0);
        EMIT("context_switches",0); EMIT("memory",0);
        EMIT("ramfiles",0); EMIT("devices",0); EMIT("tasks",0);
        EMIT("fb",0); EMIT("task",1); EMIT("sym",1); EMIT("net",1); break;
    case VDIR_NET:
        EMIT("wifi",0); EMIT("ap",0); EMIT("arp",0); break;
    case VDIR_TASK:
        for(int i=0;i<OS_MAX_TASKS;i++)
            if(tasks[i].state!=UNUSED && tasks[i].state!=DEAD) {
                snprintf(name,sizeof name,"%d",tasks[i].pid); EMIT(name,0);
            }
        break;
    case VDIR_SYM:
        EMIT("kernel",0); EMIT("userland",1); break;
    default: break;
    }
#undef EMIT
    return 0;
}

/* ディレクトリエントリの表示IDを，プログラム／デバイス／RAMファイルの識別子に対応させる． */
NO_TRACE static uint16_t vdir_entry_device_id(int dir, const char *name, int is_dir) {
    if(!is_dir && (dir==VDIR_BIN || dir==VDIR_USERLAND)) {
        for(int i=1;i<PROGRAM_CAPACITY;i++)
            if(programs[i].entry && programs[i].name && !strcmp(programs[i].name,name))
                return (uint16_t)i;
    }
    if(!is_dir && (dir==VDIR_TMP || dir==VDIR_ETC)) {
        const char *prefix=dir==VDIR_TMP?"tmp/":"etc/";
        for(int i=0;i<TMP_FILES;i++)
            if(tmp_files[i].used && !strncmp(tmp_files[i].name,prefix,4) &&
               !strcmp(tmp_files[i].name+4,name)) return (uint16_t)i;
    }
    if(is_dir || (dir!=VDIR_DEV && dir!=VDIR_DEV_NET && dir!=VDIR_GPIO &&
                  dir!=VDIR_I2C && dir!=VDIR_ADC && dir!=VDIR_PWM1 &&
                  dir!=VDIR_PROC && dir!=VDIR_NET && dir!=VDIR_TASK &&
                  dir!=VDIR_SYM && dir!=VDIR_USERLAND)) return 0xffff;
    const char *parent=vfs_directory_path(dir);
    if(!parent)
        return 0xffff;
    char path[96];
    if(snprintf(path,sizeof path,"%s/%s",parent,name)>=(int)sizeof path)
        return 0xffff;
    int id=vfs_fixed_path_id(path);
    if(id<0 && dir==VDIR_GPIO) id=vfs_gpio_id(name);
    if(id<0 && dir==VDIR_ADC && name[0]>='0' && name[0]<='2' && !name[1])
        id=DEV_ADC_BASE+name[0]-'0';
    if(id<0 && dir==VDIR_TASK) {
        for(int i=0;i<OS_MAX_TASKS;i++) {
            char pid[16];snprintf(pid,sizeof pid,"%d",tasks[i].pid);
            if(tasks[i].state!=UNUSED && tasks[i].state!=DEAD && !strcmp(pid,name))
                {id=DEV_PROC_TASK_BASE+i;break;}
        }
    }
    return id>=0 && id<65535 ? (uint16_t)id : 0xffff;
}

/* ディレクトリFDの現在位置から固定長エントリを返す．複数回のreadで続きが得られるようFD位置を更新する． */
static int vfs_getdents(int fd, piconix_dirent_t *out, size_t n) {
    if(fd<0 || fd>=OS_FD_MAX)
        return OS_EBADF;
    int dir=tasks[current].fd[fd];
    if(dir<VDIR_ROOT || dir>VDIR_I2C)
        return OS_EBADF;
    if(n<sizeof(*out) || !in_task_range(out,sizeof(*out)))
        return OS_EINVAL;
    piconix_dirent_t entry={0};
    int found=vdir_nth(dir,tasks[current].fd_pos[fd],&entry);
    if(found) { entry.device_id=vdir_entry_device_id(dir,entry.name,entry.is_dir); memcpy(out,&entry,sizeof entry); tasks[current].fd_pos[fd]++; }
    return found;
}

/* 正規化済み絶対パスをデバイス／ディレクトリ／RAMファイルへ振り分け，FDを割り当てる． */
static int vfs_open(const char *p, int flags) {
    if(flags<0 || (flags & ~(OS_O_WRONLY | OS_O_RDWR | OS_O_NONBLOCK | OS_O_CREAT | OS_O_TRUNC | OS_O_APPEND)) != 0 || (flags & 3)==3)
        return OS_EINVAL;
    int dir=vfs_directory_id(p);
    if(dir>=0) {
        if(flags!=OS_O_RDONLY || (dir==VDIR_PWM1 && !pin_any_pwm()))
            return OS_EINVAL;
        int fd=free_fd(); if(fd<0) return OS_EMFILE;
        fd_set(&tasks[current],fd,dir,0,0,0); return fd;
    }
    if(!strncmp(p,"/tmp/",5) || !strncmp(p,"/etc/",5))
        return tmp_open(p,flags);
    if(flags & (OS_O_CREAT | OS_O_TRUNC | OS_O_APPEND))
        return OS_EINVAL;
    int dev=vfs_fixed_path_id(p);
    if(dev<0 && !strncmp(p,"/proc/task/",11)) {
        const char *q=p+11;
        unsigned pid=0;
        if(*q) {
            while(*q>='0' && *q<='9' && pid<1000000u)
                pid=pid*10u+(unsigned)(*q++-'0');
            if(!*q && pid<1000000u)
                for(int i=0;i<OS_MAX_TASKS;i++)
                    if(tasks[i].state!=UNUSED && tasks[i].state!=DEAD &&
                       tasks[i].pid==(int)pid) {dev=DEV_PROC_TASK_BASE+i;break;}
        }
    }
    if(dev<0 && !strncmp(p,"/proc/sym/userland/",19)) {
        const char *name=p+19;
        if(*name && !strchr(name,'/'))
            for(int id=1;id<PROGRAM_CAPACITY;id++)
                if(programs[id].entry && programs[id].name &&
                   !strcmp(name,programs[id].name)) {
                    dev=DEV_SYMS_USER_BASE+id;
                    break;
                }
    }
    if(dev<0 && !strncmp(p,"/dev/gpio/",10))
        dev=vfs_gpio_id(p+10);
    if(dev<0 && !strncmp(p,"/dev/adc/",9) && p[9]>='0' && p[9]<='2' && p[10]==0 && pin_adc((unsigned)(p[9]-'0')))
        dev=DEV_ADC_BASE+(p[9]-'0');
    /* 固定PWMのパスは，対応ピンが利用可能な場合だけ有効にする． */
    if((dev==DEV_PWM_FREQ && !pin_any_pwm()) ||
       (dev==DEV_PWM_18 && !pin_pwm(18)) ||
       (dev==DEV_PWM_19 && !pin_pwm(19))) dev=-1;
    if(dev<0 || (((dev>=DEV_ADC_BASE && dev<=DEV_ADC_END) ||
                   proc_device(dev) || dev==DEV_RAND || dev==DEV_SYMS_KERNEL ||
                   (dev>DEV_SYMS_USER_BASE && dev<DEV_SYMS_USER_BASE+PROGRAM_CAPACITY) || dev==DEV_TRACE) &&
                  (flags & 3)!=OS_O_RDONLY)) return OS_EINVAL;
    if(dev==DEV_WIFI_CTRL && (flags & 3)!=OS_O_WRONLY)
        return OS_EINVAL;
    if(dev==DEV_NET_PACKET && (flags & 3)!=OS_O_RDONLY)
        return OS_EINVAL;
    if(dev==DEV_NET_ICMP && (flags & 3)!=OS_O_RDWR)
        return OS_EINVAL;
    if(dev==DEV_NET_ICMP) {
        for(int i=0;i<OS_MAX_TASKS;i++) if(tasks[i].state!=UNUSED && tasks[i].state!=DEAD)
            for(int j=0;j<OS_FD_MAX;j++)
                if(tasks[i].fd[j]==DEV_NET_ICMP)
                    return OS_EBUSY;
    }
    if((flags & OS_O_NONBLOCK) && dev!=DEV_UART1 && dev!=DEV_CONSOLE)
        return OS_EINVAL;
    int fd=free_fd();
    if(fd<0)
        return OS_EMFILE;
    fd_set(&tasks[current],fd,dev,
           dev==DEV_TRACE ? trace_serial-trace_count :
           dev==DEV_NET_PACKET ? net_packet_serial() : 0,flags,
           (dev>=DEV_PROC_TASK_BASE && dev<DEV_PROC_TASK_BASE+OS_MAX_TASKS)
           ? tasks[dev-DEV_PROC_TASK_BASE].pid : 0);
    if(dev==DEV_NET_PACKET)
        net_packet_reader(1);
    if(dev==DEV_NET_ICMP)
        icmp_fd_refs++;
    return fd;
}

/* /dev/randomの読出し．初回に時刻とtickで状態を混ぜ，割込みからの更新と競合させない．最大4バイトを返し，書込みは拒否する． */
static int device_rw_rand(void *p, size_t n, int writing) {
    if(writing)
        return OS_EBADF;
    if(n==0) return 0;
    uint32_t irq_state=save_and_disable_interrupts();
    /* 初回の擬似乱数状態に呼出し時刻を混ぜる． */
    static int seeded;
    if(!seeded) {
        rand_state ^= time_us_32() ^ ticks;
        if(!rand_state)
            rand_state=0x9e3779b9u;
        seeded=1;
    }
    uint32_t value=rand32();
    restore_interrupts(irq_state);
    size_t count=n<sizeof(value) ? n : sizeof(value);
    memcpy(p,&value,count);
    return (int)count;
}

/* /proc出力の位置と要求バイト数を考慮して，文字列の一部分だけをコピーする． */
NO_TRACE static int proc_emit(int fd, void *dst, size_t limit, size_t *offset,
                     size_t *done, const char *line, size_t len) {
    size_t pos=tasks[current].fd_pos[fd];
    if(pos < *offset+len && *done<limit) {
        size_t start=pos>*offset?pos-*offset:0;
        size_t take=len-start;
        if(take>limit-*done)
            take=limit-*done;
        memcpy((char *)dst+*done,line+start,take);
        *done+=take;
        tasks[current].fd_pos[fd]+=(uint32_t)take;
    }
    *offset+=len;
    return *done==limit;
}

/* 生存タスクの概要を/proc/tasks形式へ逐次生成する．スナップショット用の大きな配列を確保しない． */
NO_TRACE static int proc_tasks_read(int fd, void *p, size_t n) {
    static const char header[] = "PID  NAME              STATE    TTY      SLOT  MEMORY             SP\n";
    size_t offset=0, done=0;
    if(proc_emit(fd,p,n,&offset,&done,header,sizeof(header)-1))
        return (int)done;
    for(int row=0;row<OS_MAX_TASKS;row++) {
        task_t *t=&tasks[row];
        if(t->state==UNUSED || t->state==DEAD)
            continue;
        const char *state=t->state==READY?"READY":
            t->state==SLEEPING?"SLEEP":t->state==BLOCKED?"BLOCKED":"?";
        uintptr_t base=(uintptr_t)task_ram[row];
        char line[128];
        int len=snprintf(line,sizeof line,"%3d  %-18.18s%-9s%-9s%d     %08lX-%08lX  %08lX\n",
            t->pid,t->name?t->name:"",state,t->tty==0?"console":t->tty==1?"uart1":t->tty==2?"wifi":"-",row+1,
            (unsigned long)base,(unsigned long)(base+OS_TASK_BYTES-1u),
            (unsigned long)(uintptr_t)t->sp);
        if(len<0 || (size_t)len>=sizeof line)
            return OS_EINVAL;
        if(proc_emit(fd,p,n,&offset,&done,line,(size_t)len))
            break;
    }
    return (int)done;
}

/* /procの8×8表示状況をテキストで返す．読出し位置はFDごとに維持し，画面全体を複製する大きな領域を使わない． */
NO_TRACE static int proc_fb_read(int fd, void *dst, size_t n) {
    uint8_t pixels[8];
    int enabled=fb_active();
    /* フレームバッファの実データを読み，表示用の複製状態を持たない． */
    if(fb_rw(pixels,sizeof pixels,0)<0)
        return OS_EINVAL;
    char line[80]; size_t offset=0,done=0;
    int len=snprintf(line,sizeof line,"Framebuffer: 8x8\nEnabled: %s\n",
                     enabled?"yes":"no");
    if(len<0 || (size_t)len>=sizeof line)
        return OS_EINVAL;
    if(proc_emit(fd,dst,n,&offset,&done,line,(size_t)len))
        return (int)done;
    for(int y=0;y<8;y++) {
        for(int x=0;x<8;x++)
            line[x]=(pixels[y] & (1u<<(7-x)))?'#':'.';
        line[8]='\n';
        if(proc_emit(fd,dst,n,&offset,&done,line,9))
            break;
    }
    return (int)done;
}

/* FDの表示名はカーネルが管理するデバイスIDから求める． */
NO_TRACE static const char *fd_path(int dev, int task_pid, int tcp_id, char *buf, size_t size) {
    if(dev>=VDIR_ROOT && dev<=VDIR_I2C)
        return vfs_directory_path(dev);
    const char *fixed=vfs_fixed_id_path(dev);
    if(fixed)
        return fixed;
    /* ソケットFDはVFSのパスではないため，/procへ識別用の名称を表示する． */
    if(dev==DEV_SOCKET_UDP || dev==DEV_SOCKET_TCP || dev==DEV_SOCKET_TCP_ACCEPTED) {
        int kind=dev==DEV_SOCKET_UDP ? 0 : dev==DEV_SOCKET_TCP ? 1 : 2;
        const char *type=kind==0 ? "udp" : kind==1 ? "tcp" : "tcp-peer";
        uint16_t port=net_socket_local_port(kind,tcp_id);
        if(port)
            snprintf(buf,size,"socket:[%s]:%u",type,(unsigned)port);
        else snprintf(buf,size,"socket:[%s]",type);
        return buf;
    }
    if(dev>=DEV_TMP_FIRST && dev<DEV_TMP_FIRST+TMP_FILES) {
        int index=dev-DEV_TMP_FIRST;
        if(tmp_files[index].used) {
            snprintf(buf,size,"/%s",tmp_files[index].name);
            return buf;
        }
    }
    if(dev>=DEV_PROC_TASK_BASE && dev<DEV_PROC_TASK_BASE+OS_MAX_TASKS) {
        snprintf(buf,size,"/proc/task/%d",task_pid);
        return buf;
    }

    if(dev>DEV_SYMS_USER_BASE && dev<DEV_SYMS_USER_BASE+PROGRAM_CAPACITY &&
       programs[dev-DEV_SYMS_USER_BASE].name) {
        snprintf(buf,size,"/proc/sym/userland/%s",programs[dev-DEV_SYMS_USER_BASE].name);
        return buf;
    }
    if(dev>=DEV_GPIO_VALUE_BASE && dev<=DEV_GPIO_VALUE_END)
        snprintf(buf,size,"/dev/gpio/%d",dev-DEV_GPIO_VALUE_BASE);
    else if(dev>=DEV_GPIO_DIR_BASE && dev<=DEV_GPIO_DIR_END)
        snprintf(buf,size,"/dev/gpio/%d.dir",dev-DEV_GPIO_DIR_BASE);
    else if(dev>=DEV_GPIO_PULL_BASE && dev<=DEV_GPIO_PULL_END)
        snprintf(buf,size,"/dev/gpio/%d.pull",dev-DEV_GPIO_PULL_BASE);
    else if(dev>=DEV_ADC_BASE && dev<DEV_ADC_END)
        snprintf(buf,size,"/dev/adc/%d",dev-DEV_ADC_BASE);
    else snprintf(buf,size,"device:%d",dev);
    return buf;
}

/* 指定スロットのタスク詳細とFDを表示用テキストへ整形する． */
NO_TRACE static int proc_task_read(int fd, int slot, void *dst, size_t n) {
    task_t *t=&tasks[slot];
    if(t->state==UNUSED || t->state==DEAD ||
       t->pid!=tasks[current].fd_task_pid[fd]) return 0;
    char line[160]; size_t offset=0,done=0;
    const char *state=t->state==READY?"READY":t->state==SLEEPING?"SLEEPING":
                      t->state==BLOCKED?"BLOCKED":"UNKNOWN";
#define PROC_LINE(...) do { \
    int len=snprintf(line,sizeof line,__VA_ARGS__); \
    if(len<0 || (size_t)len>=sizeof line) return OS_EINVAL; \
    if(proc_emit(fd,dst,n,&offset,&done,line,(size_t)len)) return (int)done; \
} while(0)
    PROC_LINE("PID: %d\nName: %s\nState: %s\nParent: %d\nSlot: %d\n",
              t->pid,t->name?t->name:"",state,t->parent_pid,slot+1);
    PROC_LINE("Memory: %08lX-%08lX\nSaved SP: %08lX\n",
              (unsigned long)(uintptr_t)task_ram[slot],
              (unsigned long)((uintptr_t)task_ram[slot]+OS_TASK_BYTES-1u),
              (unsigned long)(uintptr_t)t->sp);
    /* 現在実行中のタスクについては，保存済みフレームに最新レジスタが入っていない． */
    if(slot==current) {
        PROC_LINE("Registers: running (saved context is stale)\n");
    } else {
        uintptr_t sp=(uintptr_t)t->sp;
        uintptr_t low=(uintptr_t)task_ram[slot];
        uintptr_t high=low+OS_TASK_BYTES;
        if(sp>=low && sp<=high && high-sp>=16u*sizeof(uint32_t) && !(sp&3u)) {
            const uint32_t *r=t->sp;
            PROC_LINE("Saved registers (PendSV frame):\n");
            for(int i=0;i<8;i++) {
                int reg=i+4; /* r4..r11 */
                PROC_LINE("  r%d: %08lX\n",reg,(unsigned long)r[i]);
            }
            PROC_LINE("  r0: %08lX  r1: %08lX\n",(unsigned long)r[8],(unsigned long)r[9]);
            PROC_LINE("  r2: %08lX  r3: %08lX\n",(unsigned long)r[10],(unsigned long)r[11]);
            PROC_LINE("  r12: %08lX  lr: %08lX\n",(unsigned long)r[12],(unsigned long)r[13]);
            PROC_LINE("  pc: %08lX  xpsr: %08lX\n",(unsigned long)r[14],(unsigned long)r[15]);
        } else PROC_LINE("Registers: saved frame unavailable\n");
    }
    PROC_LINE("Open files:\n");
    for(int i=0;i<OS_FD_MAX;i++) if(t->fd[i]>=0) {
        char path[48];
        const char *name=fd_path(t->fd[i],t->fd_task_pid[i],(int)t->fd_pos[i],path,sizeof path);
        PROC_LINE("  fd %d: device=%d flags=%u pos=%lu name=%s\n",i,t->fd[i],
                  (unsigned)t->fd_flags[i],(unsigned long)t->fd_pos[i],name);
    }
#undef PROC_LINE
    return (int)done;
}

/* 使用中RAMファイルの実データアドレスと名前を列挙する．/dev/mem閲覧との対応を示すメタ情報である． */
NO_TRACE static int proc_ramfiles_read(int fd, void *p, size_t n) {
    size_t pos=tasks[current].fd_pos[fd], done=0, skip=0;
    tmp_init();
    for(int i=0;i<TMP_FILES;i++) if(tmp_files[i].used) {
        char line[80];
        int len=snprintf(line,sizeof line,"%08lX %s\n",
                         (unsigned long)(uintptr_t)tmp_payload(i),
                         tmp_files[i].name);
        if(len<=0 || (size_t)len>=sizeof line)
            return OS_EINVAL;
        if(pos>=skip+(size_t)len) {skip+=(size_t)len;continue;}
        size_t off=pos>skip?pos-skip:0;
        size_t count=(size_t)len-off;
        if(count>n-done)count=n-done;
        memcpy((char*)p+done,line+off,count);
        done+=count;pos+=count;skip+=(size_t)len;
        if(done==n)break;
    }
    tasks[current].fd_pos[fd]+=(uint32_t)done;
    return (int)done;
}

/* /procの読み取りを担当する．小さな結果バッファを使い，FD位置からの部分読出しに対応する．書込みは拒否する． */
static int device_rw_proc(int fd, int dev, void *p, size_t n, int writing) {
    if(writing)
        return OS_EBADF;
    if(dev==DEV_PROC_TASKS)
        return proc_tasks_read(fd,p,n);
    if(dev==DEV_PROC_RAMFILES)
        return proc_ramfiles_read(fd,p,n);
    if(dev==DEV_PROC_FB)
        return proc_fb_read(fd,p,n);
    if(dev>=DEV_PROC_TASK_BASE && dev<DEV_PROC_TASK_BASE+OS_MAX_TASKS)
        return proc_task_read(fd,dev-DEV_PROC_TASK_BASE,p,n);
    // カウンタは割込み禁止区間で一括取得し，表示用処理中に不整合が生じないようにする．
    uint32_t irq_state=save_and_disable_interrupts();
    uint32_t t=ticks, svc=svc_count, psv=pendsv_count, switches=context_switch_count;
    uint32_t uart1_irqs=uart1_irq_count;
    restore_interrupts(irq_state);
    char text[448]; int len=0;
    if(dev==DEV_PROC_WIFI)
        len=net_wifi_info(text,sizeof(text));
    if(dev==DEV_PROC_NET_MEMORY)
        len=net_memory_info(text,sizeof(text));
    if(dev==DEV_PROC_AP || dev==DEV_PROC_ARP)
        len=net_diag_info(text,sizeof(text),dev==DEV_PROC_ARP);
    if(dev==DEV_PROC_DEVICES) {
        /* vfs_openと同じ登録表を列挙する． */
        for(unsigned i=0;i<vfs_device_count();i++) {
            const char *name=vfs_device_path(i)+5; /* /dev/の接頭辞を除く． */
            int written=snprintf(text+len,sizeof(text)-(size_t)len,"%s\n",name);
            if(written<0 || (size_t)written>=sizeof(text)-(size_t)len)
                return OS_EINVAL;
            len+=written;
        }
        /* GPIOは固定デバイスではなく動的に生成される下位ディレクトリである． */
        int written=snprintf(text+len,sizeof(text)-(size_t)len,"gpio/\nadc/\npwm/\ni2c/\n");
        if(written<0 || (size_t)written>=sizeof(text)-(size_t)len)
            return OS_EINVAL;
        len+=written;
    }
    switch(dev) {
        case DEV_PROC_UPTIME: len=snprintf(text,sizeof text,"%lu.%03lu s\n",(unsigned long)(t/1000u),(unsigned long)(t%1000u)); break;
        case DEV_PROC_TICKS: len=snprintf(text,sizeof text,"%lu\n",(unsigned long)t); break;
        case DEV_PROC_INTERRUPTS:
            len=snprintf(text,sizeof text,"SysTick %lu\nPendSV %lu\nSVC %lu\nuart1 %lu\n",
                         (unsigned long)t,(unsigned long)psv,(unsigned long)svc,(unsigned long)uart1_irqs);
            break;
        case DEV_PROC_SWITCHES: len=snprintf(text,sizeof text,"%lu\n",(unsigned long)switches); break;
        case DEV_PROC_MEMORY: len=snprintf(text,sizeof text,"kernel 20000000-2000DFFF\nslots  2000E000-%08lX (%d x 24 KiB)\nshm    20032000-200327FF (2 KiB)\nfiles  20032800-20037FFF (11 x 2 KiB)\ntrace  20038000-2003BFFF (16 KiB)\n%s", (unsigned long)(0x2000E000u + OS_MAX_TASKS * OS_TASK_BYTES - 1u), OS_MAX_TASKS, "net    2003C000-2003FFFF (16 KiB reserved)\n"); break;
    }
    if(len<0)
        return OS_EINVAL;
    if((size_t)len>=sizeof text)
        len=sizeof text-1;
    // 開かれた/procファイルはFDごとに読み出し位置を保持する．
    // 長い結果も分割readで重複なく取得できる．
    size_t pos=tasks[current].fd_pos[fd];
    if(pos>=(size_t)len)
        return 0;
    size_t count=(size_t)len-pos;
    if(count>n)
        count=n;
    if(count)
        memcpy(p,text+pos,count);
    tasks[current].fd_pos[fd]=(uint32_t)(pos+count);
    return (int)count;
}

/* TTYに対するread/writeを実装する．空のブロッキングreadはタスクをBLOCKEDにし，入力後にSVC戻り値を書き換える． */
static int device_rw_tty(int fd, int dev, void *p, size_t n, int writing) {
    int uartdev=(dev==DEV_CONSOLE)?0:(dev==DEV_WIFI_TTY?2:1);
    uint8_t *b=p;
    if(writing) {
        if(tty_claim_pid[uartdev] && tty_claim_pid[uartdev]!=tasks[current].pid)
            return OS_EIO;
        if(dev==DEV_CONSOLE && !stdio_usb_connected())
            return (int)n;
        size_t i=0;
        for(;i<n;i++) {
            uint8_t c=b[i];
            int insert_cr=tty_onlcr[uartdev] && c=='\n' && !tty_last_cr[uartdev];
            if(!tty_tx_char(uartdev,c,insert_cr)) break;
            tty_last_cr[uartdev]=(c=='\r');
        }
        /* ネットワーク送信キュー満杯時にwriteが0で成功するとシェル出力を失う．再試行可能エラーを返し，部分書込みは転送長を返す．write自体のブロッキング待機は未対応である． */
#if PICONIX_WIFI
        if(uartdev==2 && n!=0 && i==0)
            return net_terminal_connected() ? OS_EAGAIN : OS_EIO;
#endif
        return (int)i;
    }
    if(n==0) return 0;
    if(tty_owner_pid(uartdev) && tasks[current].pid!=tty_owner_pid(uartdev))
        return OS_EIO;
    uint32_t irq_state=save_and_disable_interrupts();
    if(tty_rx_ready(uartdev)) {
        int count=tty_rx_read(uartdev,b,n);
        restore_interrupts(irq_state);
        return count; /* 0バイトの入力はCtrl+DによるEOFの可能性がある． */
    }
    if(tasks[current].fd_flags[fd] & OS_O_NONBLOCK) {
        restore_interrupts(irq_state); return 0;
    }
    task_t *t=&tasks[current];
    t->wait_dev=uartdev;t->wait_fd=fd;t->wait_buf=p;t->wait_size=n;
    t->wait_frame_ready=0;
    t->state=BLOCKED;
    pend();
    restore_interrupts(irq_state);
    return 0; // SVC時点の戻り値は仮の0で，受信割込み後に保存フレームへ結果を書き込む．
}

/* /dev/pwmの文字列読書き．値はドライバへ渡し，読出しはFD位置を使って部分転送を許す．失敗時は負のエラー値を返す． */
NO_TRACE static int device_rw_pwm(int fd, int dev, void *p, size_t n, int writing) {
    if(!n)
        return 0;
    unsigned which=(dev==DEV_PWM_FREQ) ? 0u : (unsigned)(dev-DEV_PWM_18+1);
    if(writing) {
        unsigned x;
        if(pwm_parse_value(which,p,n,&x)<0)
            return OS_EINVAL;
        if(pwm_driver_set(which,x)<0)
            return OS_EINVAL;
        return (int)n;
    }
    char b[20];
    unsigned x=pwm_driver_get(which);
    int len=dev==DEV_PWM_FREQ ? snprintf(b,sizeof b,"%u\n",x)
                              : snprintf(b,sizeof b,"%u.%u\n",x/10u,x%10u);
    size_t pos=tasks[current].fd_pos[fd];
    if(pos>=(size_t)len)
        return 0;
    size_t count=(size_t)len-pos;
    if(count>n)
        count=n;
    memcpy(p,b+pos,count);
    tasks[current].fd_pos[fd]+=(uint32_t)count;
    return (int)count;
}

/* ADCを数値文字列として読む．分割readでは同じ測定値を返し，最後まで読み終えてから次の測定を始める． */
NO_TRACE static int device_rw_adc(int fd, int dev, void *p, size_t n, int writing) {
    if(writing || (tasks[current].fd_flags[fd] & 3)!=OS_O_RDONLY)
        return OS_EBADF;
    if(!n)
        return 0;
    if(tasks[current].fd_pos[fd]==0)
        tasks[current].adc_values[fd]=(uint16_t)adc_driver_read(dev==DEV_ADC_END ? 4u : (unsigned)(dev-DEV_ADC_BASE));
    unsigned raw=tasks[current].adc_values[fd];
    char text[20];
    int len=adc_driver_format(raw,dev==DEV_ADC_END,text,sizeof text);
    size_t pos=tasks[current].fd_pos[fd];
    if(pos>=(size_t)len)
        pos=0;
    size_t count=(size_t)len-pos;
    if(count>n)
        count=n;
    memcpy(p,text+pos,count);
    pos+=count;
    /* 測定値の終端まで読んだ時点で次回用の読み出し位置を先頭へ戻す． */
    tasks[current].fd_pos[fd]=(uint32_t)(pos==(size_t)len ? 0 : pos);
    return (int)count;
}
extern const unsigned char piconix_symbols_start[], piconix_symbols_end[];
extern const unsigned char piconix_kernel_syms_start[], piconix_kernel_syms_end[];
/* read/writeの共通入口．FD・ユーザバッファを検査し，内部デバイスIDに応じて個別の入出力処理へ振り分ける． */
static int vfs_rw(int fd, void *p, size_t n, int writing) {
    if(fd<0 || fd>=OS_FD_MAX || tasks[current].fd[fd]<0)
        return OS_EBADF;
    if(n>0 && !in_task_range(p,n))
        return OS_EINVAL;
    int dev=tasks[current].fd[fd];
    if(dev==DEV_SOCKET_TCP || dev==DEV_SOCKET_TCP_ACCEPTED) {
        int id=tcp_fd_slot(&tasks[current],fd);
        if(id<0)
            return OS_EBADF;
        if(dev==DEV_SOCKET_TCP && net_socket_tcp_listening(id))
            return OS_EBADF;
        if(n==0) return 0;
        if(!net_socket_tcp_connected(id)) {
            int status=net_socket_tcp_status(id);
            if(status==-2)
                return writing ? OS_EIO : 0;
            return status<0 ? OS_EIO : OS_EAGAIN;
        }
        int r=writing ? net_socket_tcp_write(id,p,(unsigned)n) : net_socket_tcp_read(id,p,(unsigned)n);
        return r==-2 ? (writing ? OS_EIO : 0) : r<0 ? OS_EIO : r==0 ? OS_EAGAIN : r;
    }
    if(dev>=VDIR_ROOT && dev<=VDIR_I2C)
        return OS_EBADF;
    if(dev>=DEV_TMP_FIRST && dev<DEV_TMP_FIRST+TMP_FILES)
        return tmp_rw(fd,p,n,writing);
    if(dev==DEV_NET_PACKET) {
        if(writing)
            return OS_EBADF;
        if(n<sizeof(piconix_packet_t))
            return OS_EINVAL;
        int ret=net_packet_read(&tasks[current].fd_pos[fd],(piconix_packet_t *)p);
        return ret==0 ? OS_EAGAIN : ret;
    }
    if(dev==DEV_NET_ICMP) {
        if(n!=sizeof(piconix_echo_t))
            return OS_EINVAL;
        int ret=writing ? net_echo_write((const piconix_echo_t *)p)
                       : net_echo_read((piconix_echo_t *)p);
        return ret<0 ? OS_EIO : ret==0 ? OS_EAGAIN : ret;
    }
    if(dev==DEV_TRACE) {
        if(writing)
            return OS_EBADF;
        if(n < sizeof(trace_event_t))
            return n ? OS_EINVAL : 0;
        uint32_t irq=save_and_disable_interrupts();
        uint32_t pos=tasks[current].fd_pos[fd];
        uint32_t oldest_serial=trace_serial-trace_count;
        if((int32_t)(pos-oldest_serial)<0)
            pos=oldest_serial;
        uint32_t available=trace_serial-pos;
        uint32_t count=(uint32_t)(n/sizeof(trace_event_t));
        if(count>available)
            count=available;
        uint32_t oldest=(trace_head+TRACE_CAPACITY-trace_count)&(TRACE_CAPACITY-1u);
        for(uint32_t i=0;i<count;i++)
            memcpy((uint8_t *)p+i*sizeof(trace_event_t),
                   &trace_ring[(oldest+(pos-oldest_serial)+i)&(TRACE_CAPACITY-1u)],sizeof(trace_event_t));
        tasks[current].fd_pos[fd]=pos+count;
        restore_interrupts(irq);
        return (int)(count*sizeof(trace_event_t));
    }
    if(dev==DEV_SYMS_KERNEL ||
       (dev>DEV_SYMS_USER_BASE && dev<DEV_SYMS_USER_BASE+PROGRAM_CAPACITY)) {
        if(writing || (tasks[current].fd_flags[fd] & 3)!=OS_O_RDONLY)
            return OS_EBADF;
        uint32_t pos=tasks[current].fd_pos[fd];
        const unsigned char *start, *end;
        if(dev==DEV_SYMS_KERNEL) {
            size_t reserved=(size_t)(piconix_kernel_syms_end-piconix_kernel_syms_start);
            uint32_t length=0;
            if(reserved>=4u)
                memcpy(&length,piconix_kernel_syms_start,4u);
            if(length>reserved-4u)
                length=0;
            start=piconix_kernel_syms_start+4u;
            end=start+length;
            if(pos>=(size_t)(end-start))
                return 0;
            if(n>(size_t)(end-start)-pos)
                n=(size_t)(end-start)-pos;
            memcpy(p,start+pos,n);
            tasks[current].fd_pos[fd]=pos+(uint32_t)n;
            return (int)n;
        }
        /* 要求されたユーザプログラムのシンボル記録だけを読み，RAMへ全件複写しない． */
        const char *name=programs[dev-DEV_SYMS_USER_BASE].name;
        if(!name)
            return OS_ENOENT;
        size_t name_len=strlen(name), skip=pos, done=0;
        start=piconix_symbols_start; end=piconix_symbols_end;
        while(start<end && done<n) {
            const unsigned char *line=start;
            while(start<end && *start!='\n')
                start++;
            if(start<end)
                start++;
            size_t len=(size_t)(start-line);
            if(len>name_len && !memcmp(line,name,name_len) && line[name_len]=='\t') {
                if(skip>=len)
                    skip-=len;
                else {
                    size_t count=len-skip;
                    if(count>n-done)
                        count=n-done;
                    memcpy((unsigned char *)p+done,line+skip,count);
                    done+=count;
                    skip=0;
                }
            }
        }
        tasks[current].fd_pos[fd]=pos+(uint32_t)done;
        return (int)done;
    }
    if(dev==DEV_SHM) {
        int mode=tasks[current].fd_flags[fd] & 3;
        if((writing && mode==OS_O_RDONLY) || (!writing && mode==OS_O_WRONLY))
            return OS_EBADF;
        uint32_t pos=tasks[current].fd_pos[fd];
        if(pos>=SHM_BYTES)
            return 0;
        size_t count=n;
        if(count>SHM_BYTES-pos)
            count=SHM_BYTES-pos;
        if(writing)
            memmove((void *)(uintptr_t)(SHM_BASE+pos),p,count);
        else memmove(p,(const void *)(uintptr_t)(SHM_BASE+pos),count);
        tasks[current].fd_pos[fd]=pos+(uint32_t)count;
        return (int)count;
    }
    if(dev==DEV_MEM) {
        if(writing ? (tasks[current].fd_flags[fd] & 3)==OS_O_RDONLY
                   : (tasks[current].fd_flags[fd] & 3)==OS_O_WRONLY) return OS_EBADF;
        if(!n)
            return 0;
        uint32_t addr=tasks[current].fd_pos[fd];
        uint32_t end=0;
        /* 読める範囲は2 MiBのXIP Flashと256 KiBの主SRAMに限定する．周辺レジスタと特殊領域には触れない． */
        if(addr>=0x10000000u && addr<0x10200000u)
            end=0x10200000u;
        if(addr>=0x20000000u && addr<0x20040000u)
            end=0x20040000u;
        if(!end)
            return OS_EINVAL;
        /* Flashは読出し専用．カーネル，トレース，Wi-Fi領域の書込みは保護し，タスクスロットの明示的な書込みのみ認める． */
        if(writing) {
            /* RAMファイルの実データは書込み可能だが，名前や長さ等のVFS管理情報は保護する． */
            int file_payload=0;
            for(int i=0;i<TMP_FILES;i++) {
                uint32_t base=(uint32_t)(uintptr_t)tmp_payload(i);
                if(addr>=base && addr<base+TMP_BYTES) {
                    end=base+TMP_BYTES; file_payload=1; break;
                }
            }
            if(!file_payload) {
                if(addr>=0x2000E000u && addr<SHM_BASE)
                    end=SHM_BASE;
                else if(addr>=SHM_BASE && addr<SHM_BASE+SHM_BYTES) end=SHM_BASE+SHM_BYTES;
                else if(addr>=RAMFILE_BASE && addr<0x20038000u) end=0x20038000u;
                else return OS_EINVAL;
            }
        }
        size_t count=n;
        if(count>end-addr)
            count=end-addr;
        if(writing)
            memmove((void *)(uintptr_t)addr,p,count);
        else memmove(p,(const void *)(uintptr_t)addr,count);
        tasks[current].fd_pos[fd]=addr+(uint32_t)count;
        return (int)count;
    }
    if(dev==DEV_WIFI_CTRL) {
        if(!writing)
            return OS_EBADF;
        /* echoの1バイト書込みにも対応できるよう，openしたFDの位置で「restart」コマンドを解析する．要求全体を検査してから再接続を予約する． */
        static const char cmd[] = "restart\n";
        uint32_t pos = tasks[current].fd_pos[fd];
        if(pos > sizeof(cmd)-1u || n > sizeof(cmd)-1u-pos)
            return OS_EINVAL;
        for(size_t i=0;i<n;i++)
            if(((const char *)p)[i] != cmd[pos+i]) {
                /* 末尾の改行だけは省略できるが，ほかの余分な文字は許可しない． */
                return OS_EINVAL;
            }
        if(pos < 7u && pos+n >= 7u && net_wifi_request_restart()!=0)
            return OS_EIO;
        tasks[current].fd_pos[fd] = pos + (uint32_t)n;
        return (int)n;
    }
    if(dev==DEV_FB)
        return fb_rw(p,n,writing);
    if(dev>=DEV_ADC_BASE && dev<=DEV_ADC_END)
        return device_rw_adc(fd,dev,p,n,writing);
    if(dev>=DEV_PWM_FREQ && dev<=DEV_PWM_19)
        return device_rw_pwm(fd,dev,p,n,writing);
    if(dev==DEV_RAND)
        return device_rw_rand(p,n,writing);
    if(proc_device(dev))
        return device_rw_proc(fd,dev,p,n,writing);
    if(dev==DEV_CONSOLE || dev==DEV_UART1 || (PICONIX_WIFI && dev==DEV_WIFI_TTY))
        return device_rw_tty(fd,dev,p,n,writing);
    if(dev>=DEV_GPIO_DIR_BASE && dev<=DEV_GPIO_DIR_END)
        return gpio_driver_rw(1,(unsigned)(dev-DEV_GPIO_DIR_BASE),p,n,writing);
    if(dev>=DEV_GPIO_PULL_BASE && dev<=DEV_GPIO_PULL_END)
        return gpio_driver_rw(2,(unsigned)(dev-DEV_GPIO_PULL_BASE),p,n,writing);
    if(dev>=DEV_GPIO_VALUE_BASE && dev<=DEV_GPIO_VALUE_END)
        return gpio_driver_rw(0,(unsigned)(dev-DEV_GPIO_VALUE_BASE),p,n,writing);
    /* I2Cの操作はread/writeではなくioctlに限定する．不明IDをGPIOに渡さない． */
    return OS_EBADF;
}

/* 端末の所有権と行入力・エコー・改行変換を管理する．前景／一時占有者以外からの設定変更を防ぐ． */
static int device_ioctl_tty(int dev,int cmd,int arg) {
    int tty=dev==DEV_CONSOLE ? 0 :
            dev==DEV_UART1 ? 1 :
            (PICONIX_WIFI && dev==DEV_WIFI_TTY) ? 2 : -1;
    if(tty<0)
        return OS_EINVAL;
    if(cmd==OS_TTY_CLAIM) {
        if(tty_owner_pid(tty))
            return OS_EBUSY;
        /* 別タスクがFDを開いている場合も端末は占有中と判断する． */
        int id=tty==2 ? DEV_WIFI_TTY : (tty ? DEV_UART1 : DEV_CONSOLE);
        for(int i=0;i<OS_MAX_TASKS;i++) {
            if(i==current || tasks[i].state==UNUSED || tasks[i].state==DEAD)
                continue;
            for(int f=0;f<OS_FD_MAX;f++)
                if(tasks[i].fd[f]==id)
                    return OS_EBUSY;
        }
        tty_claim_pid[tty]=tasks[current].pid;
        tty_mode_reset(tty);
        /* 端末を占有する前に受信した入力を破棄する． */
        if(tty<2)rx[tty].tail=rx[tty].head;
        return 0;
    }
    if(cmd==OS_TTY_RELEASE) {
        if(tty_claim_pid[tty]!=tasks[current].pid)return OS_EIO;
        tty_claim_pid[tty]=0;
        tty_mode_reset(tty);
        return 0;
    }
    if(tty_owner_pid(tty)!=tasks[current].pid)return OS_EIO;
    if(cmd==OS_TTY_PERSIST){
        tty_mode_owner[tty]=0;
        /* stty/resetによる設定はコマンドの終了後も残す意図である． */
        if(tty_restore_pid[tty]==tasks[current].pid) tty_restore_pid[tty]=0;
        return 0;
    }
    if(cmd==OS_TTY_GET_CANON)return tty_canonical[tty];
    if(cmd==OS_TTY_GET_ECHO)return tty_echo[tty];
    if(cmd==OS_TTY_GET_ONLCR)return tty_onlcr[tty];
    if(arg!=0 && arg!=1)return OS_EINVAL;
    if(cmd==OS_TTY_SET_CANON && tty_canonical[tty]!=arg){
        /* 入力モードを切り替えても，既に受け取ったバイトは保持する． */
        tty_canonical[tty]=(uint8_t)arg;
    }
    if(cmd==OS_TTY_SET_ECHO)tty_echo[tty]=(uint8_t)arg;
    if(cmd==OS_TTY_SET_ONLCR)tty_onlcr[tty]=(uint8_t)arg;
    tty_mode_owner[tty]=tasks[current].pid;
    return 0;
}

/* GPIO値のFDから，各ピンの状態を問い合わせられる． */
NO_TRACE static int device_ioctl_gpio_status(int fd,int cmd) {
    int dev=tasks[current].fd[fd];
    if(dev<DEV_GPIO_VALUE_BASE || dev>DEV_GPIO_VALUE_END)
        return OS_EBADF;
    unsigned pin=(unsigned)(cmd-OS_GPIO_STATUS_BASE);
    unsigned mode=GPIO_STATUS_OTHER;
    unsigned level=0, pull=0;
    if(pin<16 && fb_active())
        mode=GPIO_STATUS_FB;
    else {
        gpio_function_t fn=gpio_get_function(pin);
        if(fn==GPIO_FUNC_PWM)
            mode=GPIO_STATUS_PWM;
        else if(fn==GPIO_FUNC_I2C) mode=GPIO_STATUS_I2C;
        else if(fn==GPIO_FUNC_UART) mode=(pin==20 || pin==0) ? GPIO_STATUS_UART_TX : GPIO_STATUS_UART_RX;
        else if(fn==GPIO_FUNC_NULL) mode=(pin>=26 && pin<=28 && pin_adc(pin-26)) ? GPIO_STATUS_ADC : GPIO_STATUS_FREE;
        else if(fn==GPIO_FUNC_SIO) {
            mode=gpio_get_dir(pin)==GPIO_OUT ? GPIO_STATUS_OUT : GPIO_STATUS_IN;
            level=gpio_get(pin)!=0;
            pull=gpio_is_pulled_up(pin)?1u:(gpio_is_pulled_down(pin)?2u:0u);
        } else if(pin>=26 && pin<=28 && pin_adc(pin-26)) mode=GPIO_STATUS_ADC;
    }
    return (int)(mode | (level<<GPIO_STATUS_LEVEL_SHIFT) | (pull<<GPIO_STATUS_PULL_SHIFT));
}

/* ioctlの共通入口．TTY，I2C，フレームバッファ，トレース，GPIOへ振り分ける． */
static int device_ioctl(int fd,int cmd,int arg) {
    if(fd<0 || fd>=OS_FD_MAX || tasks[current].fd[fd]<0)
        return OS_EBADF;
    int dev=tasks[current].fd[fd];
    if(cmd>=OS_TTY_GET_CANON && cmd<=OS_TTY_RELEASE)
        return device_ioctl_tty(dev,cmd,arg);
    if(dev==DEV_I2C1) {
        if(cmd!=OS_I2C_TRANSFER)
            return OS_EINVAL;
        piconix_i2c_transfer_t *t=(piconix_i2c_transfer_t *)(uintptr_t)(unsigned)arg;
        if(!in_task_range(t,sizeof(*t)))
            return OS_EINVAL;
        return i2c_driver_transfer(t);
    }
    if(tasks[current].fd[fd]==DEV_FB)
        return fb_ioctl(cmd);
    if(tasks[current].fd[fd]==DEV_TRACE) {
        uint32_t irq=save_and_disable_interrupts();
        if(cmd==TRACE_ON) {
            trace_enabled=0;
#if PICONIX_KERNEL_FUNCTION_TRACE
            trace_all_contexts=(arg!=0);
#endif
            trace_head=trace_count=trace_serial=0;
            trace_enabled=1;
        } else if(cmd==TRACE_OFF) {
            trace_enabled=0;
        }
        else if(cmd==TRACE_CONTEXT_SET) {
#if PICONIX_KERNEL_FUNCTION_TRACE
            trace_all_contexts=(arg!=0);
#endif
        } else if(cmd==TRACE_CONTEXT_GET) {
#if PICONIX_KERNEL_FUNCTION_TRACE
            int enabled=trace_all_contexts;
#else
            int enabled=0;
#endif
            restore_interrupts(irq); return enabled;
        }
        else if(cmd==TRACE_STATUS) { int enabled=trace_enabled; restore_interrupts(irq); return enabled; }
        else if(cmd==TRACE_LOST) {
            uint32_t oldest=trace_serial-trace_count, pos=tasks[current].fd_pos[fd];
            int lost=(int32_t)(pos-oldest)<0 ? (int)(oldest-pos) : 0;
            restore_interrupts(irq); return lost;
        }
        else { restore_interrupts(irq); return OS_EINVAL; }
        restore_interrupts(irq);
        return 0;
    }
    /* 既存のGPIO FDを通じてGP0〜28を照会する．FBが使用中でも状態を確認でき，モード・レベル・プルを小さなビット列にまとめる． */
    if(cmd>=OS_GPIO_STATUS_BASE && cmd<OS_GPIO_STATUS_BASE+29)
        return device_ioctl_gpio_status(fd,cmd);
    if(tasks[current].fd[fd]<DEV_GPIO_VALUE_BASE || tasks[current].fd[fd]>DEV_GPIO_VALUE_END)
        return OS_EBADF;
    int pin=tasks[current].fd[fd]-DEV_GPIO_VALUE_BASE;
    if(!pin_gpio(pin))
        return OS_EINVAL;
    (void)arg;
    return gpio_driver_ioctl((unsigned)pin,cmd);
}

// SVCはハードウェアが積んだr0〜r3・r12・lr・pc・xPSRへのポインタを受け取る．
/* SVC例外で積まれたr0〜r3を取り出し，番号ごとの処理結果をr0へ返す．
 * ポインタを含む引数はユーザスロット内か確認する．待機操作は保存フレームの更新で後から完了する． */
void os_svc_dispatch(uint32_t *f) {
    int num=(int)f[0], a=(int)f[1], b=(int)f[2], c=(int)f[3];
    int result=OS_ENOSYS;
    svc_count++;
    if(current<0) { f[0]=OS_EINVAL; return; }
    /* closeがFDを解放する前に，トレース用のファイルIDを取得する． */
    int8_t file_id = -1;
    if((num==SYS_READ || num==SYS_WRITE || num==SYS_CLOSE || num==SYS_IOCTL || num==SYS_LSEEK || num==SYS_GETDENTS) &&
       a>=0 && a<OS_FD_MAX && tasks[current].fd[a]>=0 && tasks[current].fd[a]<=127)
        file_id = (int8_t)tasks[current].fd[a];
    /* openの入口でも実パスを記録する．無効なユーザポインタは参照しない．
     * 出口と同じく先頭16バイトまでで，トレース無効時には検査を省く． */
    const void *enter_preview=NULL;
    unsigned enter_preview_len=0;
    if(trace_enabled && num==SYS_OPEN) {
        const char *path=(const char *)(uintptr_t)a;
        while(enter_preview_len<16 && in_task_range(path+enter_preview_len,1) &&
              path[enter_preview_len]) enter_preview_len++;
        if(enter_preview_len && in_task_range(path,enter_preview_len))
            enter_preview=path;
    }
    trace_record(TRACE_SVC_ENTER, (uint8_t)tasks[current].pid, (uint32_t)num,
                 (uint32_t)a, tasks[current].program_id, file_id,
                 enter_preview, enter_preview_len);
    switch(num) {
        case SYS_SELECT: {
            piconix_select_request_t *r=(piconix_select_request_t *)(uintptr_t)b;
            if(a<0 || a>OS_FD_MAX || !r || !in_task_range(r,sizeof(*r)) || r->timeout_ms < -1) {
                result=OS_EINVAL; break;
            }
            /* lwIPの処理中に触れないという既存のSVC/IDLE排他条件を利用する． */
            result=select_ready(&tasks[current],a,r);
            if(result || r->timeout_ms==0)
                break;
            task_t *t=&tasks[current];
            t->wait_dev=WAIT_SELECT; t->wait_buf=r; t->wait_size=(size_t)a;
            t->wake=ticks+(uint32_t)(r->timeout_ms<0?0:r->timeout_ms);
            t->wait_frame_ready=0; t->state=BLOCKED;
            pend(); result=0; break;
        }
        case SYS_SOCKET: {
            if(a!=2 || (b!=1 && b!=2) || c!=0) { result=OS_EINVAL; break; }
            if(b==2 && udp_owner_pid) { result=OS_EBUSY; break; }
            int fd=free_fd();
            if(fd<0) { result=OS_EMFILE; break; }
            int id=0;
            if(b==1) {
                id=net_socket_tcp_alloc();
                if(id<0) { result=OS_EBUSY; break; }
                tcp_owner_pid[id]=tasks[current].pid;
            } else udp_owner_pid=tasks[current].pid;
            fd_set(&tasks[current],fd,b==1 ? DEV_SOCKET_TCP : DEV_SOCKET_UDP,(uint32_t)id,OS_O_RDWR,0);
            result=fd;
            break;
        }
        case SYS_LISTEN_TCP: {
            int id=tcp_fd_slot(&tasks[current],a);
            if(id<0 || tasks[current].fd[a]!=DEV_SOCKET_TCP)
                result=OS_EBADF;
            else if(b<=0 || b>65535) result=OS_EINVAL;
            else result=net_socket_tcp_listen(id,(uint16_t)b)==0 ? 0 : OS_EBUSY;
            break;
        }
        case SYS_ACCEPT_TCP: {
            int id=tcp_fd_slot(&tasks[current],a);
            if(id<0 || tasks[current].fd[a]!=DEV_SOCKET_TCP || !net_socket_tcp_listening(id)) {
                result=OS_EBADF; break;
            }
            int fd=free_fd();
            if(fd<0) { result=OS_EMFILE; break; }
            if(!net_socket_tcp_accept_ready(id)) {
                task_t *t=&tasks[current];
                if(t->fd_flags[a] & OS_O_NONBLOCK) { result=OS_EAGAIN; break; }
                t->wait_dev=WAIT_TCP_ACCEPT; t->wait_fd=a;
                t->wait_frame_ready=0; t->state=BLOCKED;
                pend(); result=0; break;
            }
            fd_set(&tasks[current],fd,DEV_SOCKET_TCP_ACCEPTED,(uint32_t)id,OS_O_RDWR,0);
            result=fd;
            break;
        }
        case SYS_CONNECT_TCP: {
            int id=tcp_fd_slot(&tasks[current],a);
            if(id<0 || tasks[current].fd[a]!=DEV_SOCKET_TCP)
                result=OS_EBADF;
            else {
                const piconix_tcp_endpoint_t *ep=(const piconix_tcp_endpoint_t *)(uintptr_t)b;
                if(!ep || !in_task_range(ep,sizeof(*ep)) || !ep->peer_port)
                    result=OS_EINVAL;
                else result=net_socket_tcp_connect(id,ep->peer_ip,ep->peer_port)==0 ? 0 : OS_EIO;
            }
            break;
        }
        case SYS_SHUTDOWN_TCP: {
            int id=tcp_fd_slot(&tasks[current],a);
            if(id<0 || (tasks[current].fd[a]!=DEV_SOCKET_TCP &&
                        tasks[current].fd[a]!=DEV_SOCKET_TCP_ACCEPTED)) result=OS_EBADF;
            else result=net_socket_tcp_shutdown_write(id)==0 ? 0 : OS_EIO;
            break;
        }
        case SYS_TCP_STATUS: {
            int id=tcp_fd_slot(&tasks[current],a);
            if(id<0 || tasks[current].fd[a]!=DEV_SOCKET_TCP)
                result=OS_EBADF;
            else result=net_socket_tcp_status(id);
            break;
        }
        case SYS_BIND:
            if(a<0 || a>=OS_FD_MAX || tasks[current].fd[a]!=DEV_SOCKET_UDP || udp_owner_pid!=tasks[current].pid)
                result=OS_EBADF;
            else if(b<=0 || b>65535) result=OS_EINVAL;
            else result=net_socket_udp_bind((uint16_t)b)==0 ? 0 : OS_EINVAL;
            break;
        case SYS_SENDTO: {
            if(a<0 || a>=OS_FD_MAX || tasks[current].fd[a]!=DEV_SOCKET_UDP || udp_owner_pid!=tasks[current].pid) { result=OS_EBADF; break; }
            const piconix_udp_packet_t *pkt=(const piconix_udp_packet_t *)(uintptr_t)b;
            if(!pkt || !in_task_range(pkt,sizeof(*pkt)) || pkt->length>NET_SOCKET_UDP_MAX ||
               (pkt->length && (!pkt->data || !in_task_range(pkt->data,pkt->length)))) { result=OS_EINVAL; break; }
            int n=net_socket_udp_send(pkt->data,pkt->length,pkt->peer_ip,pkt->peer_port);
            result=n<0 ? OS_EIO : n;
            break;
        }
        case SYS_RECVFROM: {
            if(a<0 || a>=OS_FD_MAX || tasks[current].fd[a]!=DEV_SOCKET_UDP || udp_owner_pid!=tasks[current].pid) { result=OS_EBADF; break; }
            piconix_udp_packet_t *pkt=(piconix_udp_packet_t *)(uintptr_t)b;
            if(!pkt || !in_task_range(pkt,sizeof(*pkt)) || !pkt->data ||
               pkt->length>NET_SOCKET_UDP_MAX || !in_task_range(pkt->data,pkt->length)) { result=OS_EINVAL; break; }
            int n=net_socket_udp_recv((void *)pkt->data,pkt->length,&pkt->peer_ip,&pkt->peer_port);
            if(n==-3 && !(tasks[current].fd_flags[a] & OS_O_NONBLOCK)) {
                task_t *t=&tasks[current];
                t->wait_dev=WAIT_UDP_READ; t->wait_fd=a; t->wait_buf=pkt;
                t->wait_frame_ready=0; t->state=BLOCKED;
                pend(); result=0; break;
            }
            result=n==-2 ? OS_EINVAL : n==-3 ? OS_EAGAIN : n<0 ? OS_EIO : n;
            break;
        }
        case SYS_GETPID: result=tasks[current].pid; break;
        case SYS_FLASH_ID: result=(int)piconix_flash_id(); break;
        case SYS_GET_TTY: result=tasks[current].tty; break;
        case SYS_KILL:
            {int slot=slot_from_pid(a);
            if(slot<0 || slot==current || slot==IDLE_SLOT)
                result=OS_ESRCH;
            else {finish_task(slot,128+15);result=0;}}
            break;
        case SYS_EXIT: if(current!=IDLE_SLOT) { finish_task(current,a); pend(); } result=0; break;
        case SYS_SLEEP: if(current!=IDLE_SLOT && a>0) { tasks[current].wake=ticks+(uint32_t)a; tasks[current].state=SLEEPING; pend(); } result=0; break;
        case SYS_YIELD: pend(); result=0; break;
        case SYS_WAITPID: {
            /* 第3引数cのWNOHANGビットを扱う．子スロットは終了直後に解放し，PID通知だけでwaitpidを完了させる． */
            int *status=(int *)(uintptr_t)b;
            if((c & ~1) || (a<1 && a!=-1) ||
               (status && !in_task_range(status,sizeof(*status)))) {
                result=OS_EINVAL; break;
            }
            int pid=take_exit(tasks[current].pid,a);
            if(pid) {
                if(status)
                    *status=0;
                result=pid;
                break;
            }
            int running=0;
            for(int i=0;i<OS_MAX_TASKS;i++)
                if(i!=current && tasks[i].parent_pid==tasks[current].pid &&
                   tasks[i].state!=UNUSED && tasks[i].state!=DEAD &&
                   (a==-1 || tasks[i].pid==a)) { running=1; break; }
            if(!running) { result=OS_ESRCH; break; }
            if(c & 1) { result=0; break; }
            task_t *t=&tasks[current];
            t->wait_dev=-2;
            t->wait_pid=a;
            t->wait_status=status;
            t->wait_frame_ready=0;
            t->state=BLOCKED;
            pend();
            result=0; /* 本来の戻り値は子の終了後に保存フレームへ書き込む． */
            break;
        }
        case SYS_SET_FG: {
            int tty=tasks[current].tty;
            if(a==0) {
                /* waitpidの後にシェルの前景所有を復元する．入れ子のシェルが親の前景所有を消さないようにする． */
                int owner=tasks[current].pid;
                int parent=tasks[current].parent_pid;
                foreground_pid[tty]=parent ? owner : 0;
                for(int g=0;g<3;g++)
                    foreground_group[tty][g]=0;
                if(parent)
                    foreground_group[tty][0]=owner;
                result=0; break;
            }
            result=foreground_child(a);
            break;
        }
        case SYS_TTY_SPAWN: {
            /* gettyが別のシリアル端末へ新しいタスクを割り当てる． */
            if(a!=1 && a!=2) { result=OS_EINVAL; break; }
            int tty=(int)a-1;
            if(tty_owner_pid(tty)) { result=OS_EBUSY; break; }
            if(b!=2 && b!=4) { result=OS_EINVAL; break; }
            uint32_t irq_state=save_and_disable_interrupts();
            int slot=free_program_slot();
            result=slot<0 ? OS_EAGAIN : spawn_in_slot(slot,b,programs[b].entry,programs[b].name,0,NULL,NULL,0);
            if(result>0) {
                task_t *child=&tasks[slot];
                if(tty_shell_pid[0]==child->pid)
                    tty_shell_pid[0]=0;
                child->tty=(uint8_t)tty;
                /* 継承した標準FDにはパイプ端点が含まれ得るので，先に解放する． */
                for(int j=0;j<3;j++) {
                    fd_release(child,j);
                    fd_set(child,j,tty ? DEV_UART1 : DEV_CONSOLE,0,0,0);
                }
                if(b==2)
                    tty_shell_pid[tty]=child->pid;
                else foreground_pid[tty]=child->pid;
            }
            restore_interrupts(irq_state);
            break;
        }
        case SYS_SPAWN_TMP: {
            if(b!=0 && b!=1) { result=OS_EINVAL; break; }
            /* /tmp上の最小ThumbイメージはPNX1，16ビット長，予約16ビットのヘッダを持つ． */
            const char *path=(const char *)(uintptr_t)a;
            char filename[TMP_NAME_CAP];
            char resolved[128];
            if(absolute_path(path,resolved)<0 || !tmp_name(resolved,filename)) { result=OS_EINVAL; break; }
            int f=tmp_lookup(filename);
            if(f<0) { result=OS_ENOENT; break; }
            const uint8_t *image=tmp_payload(f);
            unsigned size=tmp_files[f].size;
            if(size<10 || image[0]!='P' || image[1]!='N' || image[2]!='X' || image[3]!='1' ||
               image[6] || image[7]) { result=OS_EINVAL; break; }
            unsigned length=(unsigned)image[4] | ((unsigned)image[5]<<8);
            if(!length || length>size-8 || length>OS_TASK_BYTES-4096u) {
                result=OS_EINVAL; break;
            }
            int slot=free_program_slot();
            result=slot<0 ? OS_EAGAIN : spawn_in_slot(slot,0,
                (os_entry_t)((uintptr_t)task_ram[slot]|1u),"a.out",0,NULL,image+8,length);
            if(result>0 && b==1) foreground_child(result);
            break;
        }
        case SYS_PROGRAM_ID: {
            /* /binのプログラム名を登録表の実際のIDへ解決する． */
            const char *name=(const char *)(uintptr_t)a;
            char key[33];
            unsigned n=0;
            result=OS_EINVAL;
            if(!in_task_range(name,1))
                break;
            while(n<sizeof(key)-1 && in_task_range(name+n,1) && name[n]) {
                key[n]=name[n]; n++;
            }
            if(n==sizeof(key)-1 || !in_task_range(name+n,1))
                break;
            key[n]=0;
            const char *lookup=key;
            if(!strncmp(lookup,"/bin/",5))
                lookup+=5;
            result=OS_ENOENT;
            if(!*lookup)
                break;
            for(int id=1;id<PROGRAM_CAPACITY;id++) {
                if(programs[id].entry && programs[id].name &&
                   !strcmp(lookup,programs[id].name)) { result=id; break; }
            }
            break;
        }
        case SYS_SPAWN: {
            if(a<1 || a>=PROGRAM_CAPACITY || !programs[a].entry) { result=OS_ENOENT; break; }
            int foreground=((unsigned)b & 0x100u)!=0;
            int argc=(int)((unsigned)b & ~0x100u);
            const char *const *argv=(const char *const *)(uintptr_t)c;
            if(argc<1 || argc>8 || !in_task_range(argv,(size_t)argc*sizeof(char *))) {
                result=OS_EINVAL; break;
            }
            unsigned total=0;
            result=0;
            for(int i=0;i<argc;i++) {
                const char *arg=argv[i];
                unsigned len=0;
                if(!in_task_range(arg,1)) { result=OS_EINVAL; break; }
                while(len<128-total && in_task_range(arg+len,1) && arg[len])
                    len++;
                if(len==128-total || !in_task_range(arg+len,1)) {
                    result=OS_EINVAL; break;
                }
                total+=len+1;
            }
            if(result<0)
                break;
            int slot=free_program_slot();
            result=slot<0 ? OS_EAGAIN : spawn_in_slot(slot,a,programs[a].entry,programs[a].name,argc,argv,NULL,0);
            if(result>0 && foreground) foreground_child(result);
            break;
        }
        case SYS_CONSOLE_READY:
            result=tasks[current].tty ? 1 : (stdio_usb_connected() ? 1 : 0);
            break;
        case SYS_CHDIR: {
            char path[128];
            if(absolute_path((const char *)(uintptr_t)a,path)<0 || !valid_directory(path))
                result=OS_ENOENT;
            else {strcpy(tasks[current].cwd,path); result=0;}
            break;
        }
        case SYS_GETCWD: {
            const size_t len=strlen(tasks[current].cwd)+1;
            if(!a || b<(int)len || !in_task_range((void *)(uintptr_t)a,len))
                result=OS_EINVAL;
            else {memcpy((void *)(uintptr_t)a,tasks[current].cwd,len); result=0;}
            break;
        }
        case SYS_UNLINK: {
            char path[128];
            result=absolute_path((const char *)(uintptr_t)a,path)<0 ? OS_EINVAL : tmp_unlink(path);
            break;
        }
        case SYS_PIPE: {
            int *pair=(int *)(uintptr_t)a;
            if(!in_task_range(pair,2*sizeof(int))) { result=OS_EINVAL; break; }
            task_t *t=&tasks[current];
            int fds[2], found=0;
            for(int j=3;j<OS_FD_MAX && found<2;j++)
                if(t->fd[j]<0)
                    fds[found++]=j;
            if(found!=2) { result=OS_EMFILE; break; }
            int index=-1;
            for(int j=0;j<PIPE_COUNT;j++)
                if(!pipes[j].readers && !pipes[j].writers) { index=j; break; }
            if(index<0) { result=OS_EAGAIN; break; }
            pipe_t *p=&pipes[index];
            p->head=0; p->used=0; p->readers=1; p->writers=1;
            fd_set(t,fds[0],PIPE_READ_DEV(index),0,OS_O_RDONLY,0);
            fd_set(t,fds[1],PIPE_WRITE_DEV(index),0,OS_O_WRONLY,0);
            pair[0]=fds[0]; pair[1]=fds[1]; result=0;
            break;
        }
        case SYS_DUP: {
            task_t *t=&tasks[current];
            if(a<0 || a>=OS_FD_MAX || t->fd[a]<0) { result=OS_EBADF; break; }
            /* ソケットは参照数で所有管理していないため，dupできない． */
            if(fd_is_socket(t->fd[a])) { result=OS_EINVAL; break; }
            int target=free_fd();
            if(target<0) { result=OS_EMFILE; break; }
            fd_copy(t,target,a);
            result=target;
            break;
        }
        case SYS_DUP2:
            if((a>=0 && a<OS_FD_MAX && fd_is_socket(tasks[current].fd[a])) ||
               (b>=0 && b<OS_FD_MAX && fd_is_socket(tasks[current].fd[b]))) {
                result=OS_EINVAL; break;
            }
            if(a<0 || a>=OS_FD_MAX || b<0 || b>=OS_FD_MAX || tasks[current].fd[a]<0)
                result=OS_EBADF;
            else {
                if(a!=b) {
                    fd_release(&tasks[current],b);
                    fd_copy(&tasks[current],b,a);
                }
                result=b;
            }
            break;
        case SYS_OPEN: {
            char path[128];
            result=absolute_path((const char *)(uintptr_t)a,path)<0 ? OS_EINVAL : vfs_open(path,b);
            break;
        }
        case SYS_CLOSE:
            if(a<0 || a>=OS_FD_MAX || tasks[current].fd[a]<0)
                result=OS_EBADF;
            else { fd_release(&tasks[current],a); result=0; }
            break;
        case SYS_GETDENTS: result=vfs_getdents(a,(piconix_dirent_t *)(uintptr_t)b,(size_t)c); break;
        case SYS_READ:
        case SYS_WRITE: {
            int writing=(num==SYS_WRITE);
            task_t *t=&tasks[current];
            if(a<0 || a>=OS_FD_MAX || t->fd[a]<0 || !pipe_end(t->fd[a])) {
                result=vfs_rw(a,(void *)(uintptr_t)b,(size_t)c,writing);
                if(!writing && result==OS_EAGAIN && a>=0 && a<OS_FD_MAX &&
                   (t->fd[a]==DEV_SOCKET_TCP || t->fd[a]==DEV_SOCKET_TCP_ACCEPTED) &&
                   !(t->fd_flags[a] & OS_O_NONBLOCK)) {
                    t->wait_dev=WAIT_TCP_READ; t->wait_fd=a;
                    t->wait_buf=(void *)(uintptr_t)b; t->wait_size=(size_t)c;
                    t->wait_frame_ready=0; t->state=BLOCKED;
                    pend(); result=0;
                }
                break;
            }
            int dev=t->fd[a];
            if(((dev-PIPE_DEV_BASE)&1)!=writing) { result=OS_EBADF; break; }
            if(c && !in_task_range((void *)(uintptr_t)b,(size_t)c)) { result=OS_EINVAL; break; }
            pipe_t *p=pipe_for_dev(dev);
            if(!c) { result=0; break; }
            if(writing && !p->readers) { result=OS_EIO; break; }
            if(!writing && !p->used && !p->writers) { result=0; break; }
            if((writing && p->used==PIPE_BYTES) || (!writing && !p->used)) {
                if(t->fd_flags[a] & OS_O_NONBLOCK) { result=OS_EAGAIN; break; }
                t->wait_dev=dev; t->wait_fd=a;
                t->wait_buf=(void *)(uintptr_t)b; t->wait_size=(size_t)c;
                t->wait_frame_ready=0; t->state=BLOCKED;
                pend(); result=0; break;
            }
            result=pipe_transfer(p,(void *)(uintptr_t)b,(size_t)c,writing);
            break;
        }
        case SYS_IOCTL: result=device_ioctl(a,b,c); break;
        case SYS_FCNTL:
            if(a<0 || a>=OS_FD_MAX || tasks[current].fd[a]<0)
                result=OS_EBADF;
            else if(b==1) result=tasks[current].fd_flags[a]; /* F_GETFLは現在のFDフラグを取得する． */
            else if(b==2) { /* F_SETFLで変更可能なのはO_NONBLOCKだけである． */
                if(c & ~(OS_O_NONBLOCK | OS_O_RDWR | OS_O_WRONLY | OS_O_CREAT | OS_O_TRUNC | OS_O_APPEND))
                    result=OS_EINVAL;
                else if(!fd_is_socket(tasks[current].fd[a]) &&
                        tasks[current].fd[a]!=DEV_CONSOLE &&
                        tasks[current].fd[a]!=DEV_UART1 &&
                        !(PICONIX_WIFI && tasks[current].fd[a]==DEV_WIFI_TTY)) result=OS_EINVAL;
                else {
                    tasks[current].fd_flags[a]=(tasks[current].fd_flags[a] & ~OS_O_NONBLOCK) | (c & OS_O_NONBLOCK);
                    result=0;
                }
            } else result=OS_EINVAL;
            break;
        case SYS_LSEEK: {
            if(a<0 || a>=OS_FD_MAX || tasks[current].fd[a]<0) { result=OS_EBADF; break; }
            int dev=tasks[current].fd[a];
            if(dev==DEV_MEM) {
                /* /dev/memのseekはファイル先頭からではなく物理アドレスそのものを指定する． */
                if(c!=0) { result=OS_EINVAL; break; }
                uint32_t pos=(uint32_t)b;
                if((pos>=0x10000000u && pos<0x10200000u) ||
                   (pos>=0x20000000u && pos<0x20040000u)) {
                    tasks[current].fd_pos[a]=pos;
                    result=b;
                } else result=OS_EINVAL;
                break;
            }
            int32_t end;
            if(dev==DEV_SHM)
                end=SHM_BYTES;
            else if(dev==DEV_SYMS_KERNEL) {
                /* カーネルの埋込みシンボル表はXIP Flash内に長さ接頭辞を付けて格納する． */
                size_t reserved=(size_t)(piconix_kernel_syms_end-piconix_kernel_syms_start);
                uint32_t length=0;
                if(reserved>=4u)
                    memcpy(&length,piconix_kernel_syms_start,4u);
                if(reserved<4u || length>reserved-4u)
                    length=0;
                end=(int32_t)length;
            }
            else if(dev>=DEV_TMP_FIRST && dev<DEV_TMP_FIRST+TMP_FILES &&
                    tmp_files[dev-DEV_TMP_FIRST].used)
                end=tmp_files[dev-DEV_TMP_FIRST].size;
            else { result=OS_EINVAL; break; }
            int32_t base;
            if(c==0) base=0;                 /* 絶対位置SEEK_SET */
            else if(c==1) base=(int32_t)tasks[current].fd_pos[a]; /* 現在位置基準SEEK_CUR */
            else if(c==2) base=end;          /* 末尾基準SEEK_END */
            else { result=OS_EINVAL; break; }
            int64_t pos=(int64_t)base+(int32_t)b;
            int32_t limit=(dev==DEV_SHM)?SHM_BYTES:
                          (dev==DEV_SYMS_KERNEL)?end:TMP_BYTES;
            if(pos<0 || pos>limit) { result=OS_EINVAL; break; }
            tasks[current].fd_pos[a]=(uint32_t)pos;
            result=(int)pos;
            break;
        }
    }
    if(num==SYS_OPEN && result>=0 && result<OS_FD_MAX && tasks[current].fd[result]<=127)
        file_id=(int8_t)tasks[current].fd[result];
    /* 成功した操作だけをトレースへ記録し，利用者データの断片を付与する．/dev/traceは自己記録を避けるため対象外である． */
    const void *preview=NULL;
    unsigned preview_len=0;
    if(num==SYS_OPEN && result>=0) {
        const char *path=(const char *)(uintptr_t)a;
        static const unsigned max_name=16;
        while(preview_len<max_name && in_task_range(path+preview_len,1) &&
              path[preview_len]) preview_len++;
        if(in_task_range(path,preview_len))
            preview=path;
    } else if((num==SYS_READ || num==SYS_WRITE) && result>0 && file_id!=99 &&
              in_task_range((const void *)(uintptr_t)b,(unsigned)result)) {
        preview=(const void *)(uintptr_t)b;
        preview_len=(unsigned)result;
    }
    /* ブロッキング端末readの結果はwake_readersで非同期に確定する． */
    if(tasks[current].state!=BLOCKED)
        trace_record(TRACE_SVC_EXIT, (uint8_t)tasks[current].pid, (uint32_t)num,
                     (uint32_t)result, tasks[current].program_id, file_id, preview, preview_len);
    f[0]=(uint32_t)result;
}

// SVCディスパッチャへ渡す引数は明示的にレジスタへ割り当てる．
NO_TRACE static inline int call(int n, uintptr_t a, uintptr_t b, uintptr_t c) {
    register uintptr_t r0 __asm("r0")=(uintptr_t)n;
    register uintptr_t r1 __asm("r1")=a;
    register uintptr_t r2 __asm("r2")=b;
    register uintptr_t r3 __asm("r3")=c;
    __asm volatile("svc #0" : "+r"(r0) : "r"(r1),"r"(r2),"r"(r3) : "memory","cc");
    return (int)r0;
}

/* SVCでwaitpidを実行するカーネル側ラッパー．PID・終了状態・オプションを渡す． */
NO_TRACE int os_waitpid(int pid, int *status, int options) {
    return call(SYS_WAITPID,pid,(uintptr_t)status,(uintptr_t)options);
}

/* SVCで指定PIDをTTYの前景プロセスへ設定する． */
NO_TRACE int os_set_foreground(int pid) { return call(SYS_SET_FG,pid,0,0); }

/* SVCで登録済みのプログラムIDを起動する．結果はPIDまたはエラー値． */
NO_TRACE int os_spawn_program(int program_id) { return call(SYS_SPAWN,program_id,0,0); }

/* SVCを呼び出し，現在実行中のPIDを取得する． */
NO_TRACE int os_getpid(void) { return call(SYS_GETPID,0,0,0); }

/* SVCを呼び出し，現在のタスクが所属するTTY番号を取得する． */
NO_TRACE int os_get_tty(void) { return call(SYS_GET_TTY,0,0,0); }

/* SVCで指定PIDのタスクの終了を要求する． */
NO_TRACE int os_kill(int pid) { return call(SYS_KILL,pid,0,0); }

/* SVCで現在のタスクを終了する．終了後に処理へ復帰しない． */
NO_TRACE void os_exit(void) { call(SYS_EXIT,0,0,0); for(;;) __wfi(); }

/* SVCで指定ミリ秒だけタスクを睡眠状態へ移す． */
NO_TRACE void os_msleep(uint32_t ms) { call(SYS_SLEEP,ms,0,0); }

/* SVCで自発的にCPU実行権を譲る． */
NO_TRACE void os_yield(void) { call(SYS_YIELD,0,0,0); }

/* SVCでVFSのパスを開き，FDまたはエラー値を返す． */
NO_TRACE int os_open(const char *p,int f) { return call(SYS_OPEN,(uintptr_t)p,f,0); }

/* SVCで指定FDを閉じ，必要な端点参照を解放する． */
NO_TRACE int os_close(int fd) { return call(SYS_CLOSE,fd,0,0); }

/* SVCでFDから最大nバイトを読み取る．待機・非ブロック動作はFD設定に従う． */
NO_TRACE int os_read(int fd,void *p,size_t n) { return call(SYS_READ,fd,(uintptr_t)p,n); }

/* SVCでFDに最大nバイトを書き込む．出力先はVFSが振り分ける． */
NO_TRACE int os_write(int fd,const void *p,size_t n) { return call(SYS_WRITE,fd,(uintptr_t)p,n); }

/* SVCでデバイス固有操作を要求する．cmdと値をそのまま渡す． */
NO_TRACE int os_ioctl(int fd,int cmd,int v) { return call(SYS_IOCTL,fd,cmd,v); }

/* SVCでFDフラグ等を取得・変更する． */
NO_TRACE int os_fcntl(int fd,int cmd,int value) { return call(SYS_FCNTL,fd,cmd,value); }

/* SVCでパイプを作成し，読取端と書込端のFDを配列へ返す． */
NO_TRACE int os_pipe(int fds[2]) { return call(SYS_PIPE,(uintptr_t)fds,0,0); }

/* SVCでFDを複製し，共有する端点への参照を増やす． */
NO_TRACE int os_dup(int oldfd) { return call(SYS_DUP,oldfd,0,0); }

/* SVCでFDを指定番号へ複製する．リダイレクトに利用する． */
NO_TRACE int os_dup2(int oldfd,int newfd) { return call(SYS_DUP2,oldfd,newfd,0); }

/* SVCでRAMファイルの指定パスを削除する． */
NO_TRACE int os_unlink(const char *p) { return call(SYS_UNLINK,(uintptr_t)p,0,0); }

/* TCPクライアント・サーバとUDP通信を提供するソケット関連システムコール． */
NO_TRACE int os_socket(int domain,int type,int protocol) { return call(SYS_SOCKET,domain,type,protocol); }

/* SVCで指定FDをTCPの待受に設定する． */
NO_TRACE int os_tcp_listen(int fd,uint16_t port) { return call(SYS_LISTEN_TCP,fd,port,0); }

/* SVCで指定FDのTCP接続を開始する．接続先はエンドポイント構造体で渡す． */
NO_TRACE int os_tcp_connect(int fd,const piconix_tcp_endpoint_t *ep) { return call(SYS_CONNECT_TCP,fd,(uintptr_t)ep,0); }

/* SVCでTCP接続の進行・確立・異常を問い合わせる． */
NO_TRACE int os_tcp_status(int fd) { return call(SYS_TCP_STATUS,fd,0,0); }

/* SVCで待受FDから確立済みの接続を受け取り，新しいFDを得る． */
NO_TRACE int os_tcp_accept(int fd) { return call(SYS_ACCEPT_TCP,fd,0,0); }

#if PICONIX_KERNEL_FUNCTION_TRACE
/* コンパイラの関数入口・出口フックを受け，教材用の選抜関数だけ記録する．
 * -finstrument-functions は kernel.c 以外の指定ドライバにも適用される．
 * 他モジュールの公開関数は devices.h / net_socket.h / net.h 等の宣言を参照する．
 * TRACE_MATCH は関数のアドレスを比べる記録対象フィルタであり，
 * ELFから生成する /proc/sym/kernel のシンボル表（mem -s kernel用）とは独立する．
 * 通常はSVC内だけ，trace on -a / trace run -a はSVC外も対象とする．
 * このフック自身は再帰計測しない． */
static void __attribute__((no_instrument_function))
trace_kernel_func(void *func, uint8_t kind) {
    if(!trace_enabled)
        return;
    uint32_t ipsr;
    __asm volatile("mrs %0, ipsr" : "=r"(ipsr));
    if(!trace_all_contexts && ipsr!=11u)
        return;
    /* SVCのみ呼出元PIDを持つ．IRQ・通常カーネル処理はPIDなし． */
    int svc=ipsr==11u && current>=0 && current<OS_MAX_TASKS;
    const char *name=NULL;
    /* 一覧は意図的にここに置く．関数名を表示するだけでなく，
     * 比較の順番を含めた既存の動作を保つため，表への置換はしない．
     * 対象を増減したときは test_kernel_trace_targets.py で所在と計測設定を確認する．
     * NO_TRACEは計測フック自体を抑制する．呼出先へは継承されない． */
#define TRACE_MATCH(fn) if(func==(void *)(uintptr_t)(fn)) name=#fn
    /* SVC境界と待機解除を観察する．SVCイベントとの重複は意図的．
     * task_unblock はSVC外でも呼ばれるため，観察には -a が必要な場合がある． */
    TRACE_MATCH(os_svc_dispatch);
    else TRACE_MATCH(task_unblock);
    else TRACE_MATCH(vfs_open);
    else TRACE_MATCH(vfs_rw);
    else TRACE_MATCH(tmp_open);
    else TRACE_MATCH(tmp_rw);
    else TRACE_MATCH(tmp_unlink);
    else TRACE_MATCH(vfs_getdents);
    else TRACE_MATCH(device_rw_tty);
    else TRACE_MATCH(tty_rx_read);
    else TRACE_MATCH(device_ioctl);
    else TRACE_MATCH(fd_release);
    else TRACE_MATCH(fd_copy);
    else TRACE_MATCH(pipe_transfer);
    /* パス解決とFD管理． */
    else TRACE_MATCH(vfs_fixed_path_id);
    else TRACE_MATCH(vfs_directory_id);
    else TRACE_MATCH(free_fd);
    else TRACE_MATCH(fd_set);
    else TRACE_MATCH(in_task_range);
    /* RAMファイルの名前解決・管理． */
    else TRACE_MATCH(tmp_init);
    else TRACE_MATCH(tmp_name);
    else TRACE_MATCH(tmp_lookup);
    /* /dev/randの読出しから擬似乱数生成まで． */
    else TRACE_MATCH(device_rw_rand);
    else TRACE_MATCH(rand32);
    /* TTY文字入力とキュー操作．頻繁なtty_rx_readyの状態検査は計測しない． */
    else TRACE_MATCH(tty_input_char);
    else TRACE_MATCH(tty_dequeue);
    /* /procとioctlの内部振り分け． */
    else TRACE_MATCH(proc_device);
    else TRACE_MATCH(device_rw_proc);
    else TRACE_MATCH(device_ioctl_tty);
    /* SVCから呼ばれる各デバイスドライバの公開入口． */
    else TRACE_MATCH(adc_driver_read);
    else TRACE_MATCH(adc_driver_format);
    else TRACE_MATCH(i2c_driver_transfer);
    else TRACE_MATCH(pwm_driver_get);
    else TRACE_MATCH(pwm_driver_set);
    else TRACE_MATCH(pwm_parse_value);
    else TRACE_MATCH(gpio_driver_rw);
    else TRACE_MATCH(gpio_driver_ioctl);
    else TRACE_MATCH(fb_ioctl);
    else TRACE_MATCH(fb_rw);
    /* ソケットの実処理を観察し，周期的な接続・状態確認は計測しない． */
    else TRACE_MATCH(net_socket_udp_bind);
    else TRACE_MATCH(net_socket_udp_recv);
    else TRACE_MATCH(net_socket_udp_send);
    else TRACE_MATCH(net_socket_tcp_alloc);
    else TRACE_MATCH(net_socket_tcp_listen);
    else TRACE_MATCH(net_socket_tcp_connect);
    else TRACE_MATCH(net_socket_tcp_read);
    else TRACE_MATCH(net_socket_tcp_write);
    else TRACE_MATCH(net_socket_tcp_close);
    /* pingのICMPデバイス経路．pollと受信コールバックは除外． */
    else TRACE_MATCH(net_echo_write);
    else TRACE_MATCH(net_echo_read);
    else TRACE_MATCH(net_echo_close);
#undef TRACE_MATCH
    if(!name)
        return;
    task_t *t=svc ? &tasks[current] : NULL;
    if(t && (t->state==UNUSED || t->program_id==7u))
        return;
    unsigned len=0;
    while(name[len] && len<16u)
        len++;
    trace_record(kind,t?(uint8_t)t->pid:0,(uint32_t)(uintptr_t)func,0,
                 t?t->program_id:0,-1,name,len);
}

/* -finstrument-functions が生成する入口通知．呼出元情報は保存しない． */
void __attribute__((no_instrument_function))
__cyg_profile_func_enter(void *func, void *caller) {
    (void)caller;
    trace_kernel_func(func,TRACE_FUNC_ENTER);
}

/* -finstrument-functions が生成する出口通知．引数・戻り値は採取しない． */
void __attribute__((no_instrument_function))
__cyg_profile_func_exit(void *func, void *caller) {
    (void)caller;
    trace_kernel_func(func,TRACE_FUNC_EXIT);
}
#endif
