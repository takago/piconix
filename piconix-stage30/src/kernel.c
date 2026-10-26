/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "piconix.h"
#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "hardware/structs/scb.h"
#include "hardware/structs/systick.h"
#include "hardware/clocks.h"
#include "hardware/sync.h"
#include "hardware/irq.h"
#include <string.h>
#include <stdio.h>
#if PICONIX_CONSOLE_USB
#include "pico/stdio_usb.h"
#include "tusb.h"
#endif

enum { UNUSED, READY, SLEEPING, DEAD, BLOCKED };
#define IDLE_SLOT OS_MAX_TASKS
#define TOTAL_SLOTS (OS_MAX_TASKS + 1)
enum { SYS_GETPID=1, SYS_KILL, SYS_EXIT, SYS_SLEEP, SYS_YIELD,
       SYS_OPEN, SYS_CLOSE, SYS_READ, SYS_WRITE, SYS_IOCTL, SYS_RESERVED_TASK_INFO, SYS_SPAWN, SYS_WAITPID, SYS_SET_FG, SYS_GETARG, SYS_CONSOLE_READY, SYS_UNLINK, SYS_DUP2 };
typedef struct { uint32_t *sp; uint32_t wake; uint8_t state; os_entry_t entry; const char *name; int fd[8]; uint16_t fd_pos[8]; uint8_t fd_flags[8]; int pid; int parent_pid; int startup_seconds; char script_path[32]; int wait_dev; void *wait_buf; size_t wait_size; uint8_t wait_frame_ready; } task_t;
// Fixed physical SRAM slots, not a 192-KiB .bss reservation.
// The linker must restrict the kernel to SRAM bank 0 (0x20000000..0x2000ffff).
_Static_assert(OS_MAX_TASKS == 3 && OS_TASK_BYTES == 65536u,
               "fixed RP2040 SRAM slot geometry changed");
__asm__(".global task_ram\n.set task_ram, 0x20010000");
extern uint8_t task_ram[OS_MAX_TASKS][OS_TASK_BYTES];
// Slots are intentionally NOLOAD/unallocated: no linker section occupies them.
static task_t tasks[TOTAL_SLOTS];
static uint8_t idle_stack[1024] __attribute__((aligned(8)));
static volatile int current = -1;

static volatile uint32_t ticks;
/* Educational, non-cryptographic PRNG for /dev/rand. */
static uint32_t rand_state = 0x9e3779b9u;
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
static volatile int foreground_pid;
/* The shell owns the console whenever no foreground child is selected. */
static int tty_owner_pid(void) { return foreground_pid ? foreground_pid : tasks[0].pid; }
/* Retain exit status independently of the reusable RAM slot. */
static struct { int pid, parent_pid, status, valid; } exited[OS_MAX_TASKS];
static void finish_task(int slot, int status) {
    task_t *t=&tasks[slot];
    if(t->state==UNUSED || t->state==DEAD) return;
    for(int i=0;i<OS_MAX_TASKS;i++) if(!exited[i].valid) {
        exited[i].pid=t->pid; exited[i].parent_pid=t->parent_pid;
        exited[i].status=status; exited[i].valid=1; break;
    }
    t->state=DEAD;
    if(foreground_pid==t->pid) foreground_pid=0;
}

static const uint8_t program_slot[5] = {0, 1, 0, 2, 2}; /* slot 3 shared by omikuji/python */
static int program_for_slot[OS_MAX_TASKS];
static struct { os_entry_t entry; const char *name; int slot; } programs[5];
// Optional prelinked RAM image. A nonzero image size means the caller must
// provide the exact slot-linked binary and entry address.
static struct { const uint8_t *image; size_t size; size_t zero_start; size_t zero_size; } images[5];
int os_register_ram_image(int id, const void *image, size_t size,
                          size_t zero_start, size_t zero_size) {
    if (started || id < 1 || id >= 5 || !image || !size) return OS_EINVAL;
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
/* Keep completed lines separate from the line currently being edited.
 * Stop polling USB when the FIFO fills, allowing USB backpressure. */
#define TTY_LINES 8u
static char tty_line[128];
static unsigned tty_len;
static struct { char text[128]; uint8_t len; } tty_lines[TTY_LINES];
static unsigned tty_head, tty_tail, tty_used;
static void tty_enqueue(void) {
    if (tty_used == TTY_LINES) return;
    memcpy(tty_lines[tty_head].text, tty_line, tty_len);
    tty_lines[tty_head].len = (uint8_t)tty_len;
    tty_head = (tty_head + 1u) % TTY_LINES;
    tty_used++;
    tty_len = 0;
}
static int tty_dequeue(void *p, size_t n) {
    if (!tty_used) return -1;
    unsigned count = tty_lines[tty_tail].len;
    if (count > n) count = (unsigned)n;
    if (count) memcpy(p, tty_lines[tty_tail].text, count);
    tty_tail = (tty_tail + 1u) % TTY_LINES;
    tty_used--;
    return (int)count;
}
#if !PICONIX_CONSOLE_USB
static void uart0_irq_handler(void);
#endif
static void uart1_irq_handler(void);
static void pend(void);
static void wake_readers(int dev);
static int uart_pop(int dev, uint8_t *ch) {
    if(rx[dev].tail==rx[dev].head) return 0;
    *ch=rx[dev].buf[rx[dev].tail];
    rx[dev].tail=(rx[dev].tail+1)%RX_CAP;
    return 1;
}
static void uart_push(int dev, uint8_t ch) {
    unsigned next=(rx[dev].head+1)%RX_CAP;
    if(next!=rx[dev].tail) {rx[dev].buf[rx[dev].head]=ch;rx[dev].head=next;}
}
/* USB console TX is queued in SVC, never waited on inside an exception.
 * The idle task drains it while USB IRQs can run. One slot is reserved. */
#if PICONIX_CONSOLE_USB
#define CONSOLE_TX_CAP 512u
static uint8_t console_tx[CONSOLE_TX_CAP];
static volatile unsigned console_tx_head, console_tx_tail;
static size_t console_queue(const uint8_t *p, size_t n) {
    size_t done=0;
    while(done<n) {
        unsigned next=(console_tx_head+1u)%CONSOLE_TX_CAP;
        if(next==console_tx_tail) break;
        console_tx[console_tx_head]=p[done++];
        console_tx_head=next;
    }
    return done;
}
static void console_tx_poll(void) {
    if(!stdio_usb_connected()) {
        /* Do not indefinitely retain output from a disconnected terminal. */
        console_tx_tail=console_tx_head;
        return;
    }
    /* Never ask stdio_usb to write when TinyUSB's TX FIFO is full.
     * Only the idle task drains; USB background IRQs remain enabled. */
    unsigned budget=64;
    while(budget-- && console_tx_tail!=console_tx_head &&
          tud_cdc_write_available()>0) {
        putchar_raw((int)console_tx[console_tx_tail]);
        console_tx_tail=(console_tx_tail+1u)%CONSOLE_TX_CAP;
    }
}
#endif
static void tty_put(uint8_t c) {
#if PICONIX_CONSOLE_USB
    (void)console_queue(&c,1);
#else
    uart_putc_raw(uart0,c);
#endif
}
static void console_input_char(uint8_t c) {
    if(c==3) {
        tty_len=0; tty_head=tty_tail=tty_used=0; 
        if(foreground_pid) {
            for(int i=0;i<OS_MAX_TASKS;i++) if(tasks[i].pid==foreground_pid) {
                finish_task(i,130); break;
            }
        }
        tty_put('^');tty_put('C');tty_put('\r');tty_put('\n');
    }
    else if(c==8 || c==127) {if(tty_len) {tty_len--;tty_put(8);tty_put(' ');tty_put(8);}}
    else if(c==4) { tty_enqueue(); }
    else if(c=='\r'||c=='\n') {
        if(tty_len<sizeof(tty_line)) tty_line[tty_len++]='\n';
        tty_enqueue();
        tty_put('\r');tty_put('\n');
    }
    else if(c>=32 && c<=126 && tty_len<sizeof(tty_line)-2) {tty_line[tty_len++]=(char)c;tty_put(c);}
}
// pico_stdio_usb owns the TinyUSB CDC interface and its callbacks.
// Read through the SDK stdio interface rather than overriding tud_cdc_rx_cb.
// The idle task polls this function while the shell waits for input.
void os_console_poll(void) {
#if PICONIX_CONSOLE_USB
    console_tx_poll();
    int ch;
    int received = 0;
    while (tty_used < TTY_LINES && (ch = getchar_timeout_us(0)) >= 0) {
        console_input_char((uint8_t)ch);
        received = 1;
    }
    if (received) wake_readers(0);
#endif
}
static void wake_readers(int dev) {
    for(int i=0;i<OS_MAX_TASKS;i++) {
        task_t *t=&tasks[i];
        if(t->state!=BLOCKED || t->wait_dev!=dev || !t->wait_frame_ready) continue;
        int n=0;
        if(dev==0) {
            if(!tty_used || t->pid!=tty_owner_pid()) continue;
            n=tty_dequeue(t->wait_buf, t->wait_size);
        } else {
            uint8_t *b=t->wait_buf;
            while((size_t)n<t->wait_size && uart_pop(1,&b[n])) n++;
            if(!n) continue;
        }
        // Never touch the saved r0 until PendSV has saved the PSP frame.
        t->sp[8]=(uint32_t)n;
        t->state=READY;
        t->wait_frame_ready=0;
        pend();
        if(dev==0) break;
    }
}
#if !PICONIX_CONSOLE_USB
static void uart0_irq_handler(void) {
    while(uart_is_readable(uart0)) console_input_char(uart_getc(uart0));
    wake_readers(0);
}
#endif
static void uart1_irq_handler(void) {
    while(uart_is_readable(uart1)) uart_push(1,uart_getc(uart1));
    wake_readers(1);
}

static void pend(void) { scb_hw->icsr = (1u << 28); __asm volatile("dsb\n isb" ::: "memory"); }
static void task_return(void) { os_exit(); }

void os_init(void) {
    memset(tasks, 0, sizeof(tasks));
    current=-1; ticks=0; svc_count=0; pendsv_count=0; context_switch_count=0; started=0; next_pid=1; foreground_pid=0; memset(exited,0,sizeof exited); tty_len=0; tty_head=tty_tail=tty_used=0; 
    memset(rx, 0, sizeof(rx));
#if PICONIX_CONSOLE_USB
    console_tx_head=console_tx_tail=0;
#endif
#if PICONIX_CONSOLE_USB
    if (!stdio_usb_init()) panic("PicoNIX: USB CDC initialization failed");
#else
    uart_init(uart0, 115200);
    gpio_set_function(0, GPIO_FUNC_UART);
    gpio_set_function(1, GPIO_FUNC_UART);
#endif
    uart_init(uart1,115200);
    gpio_set_function(4,GPIO_FUNC_UART);
    gpio_set_function(5,GPIO_FUNC_UART);
#if !PICONIX_CONSOLE_USB
    irq_set_exclusive_handler(UART0_IRQ,uart0_irq_handler);
#endif
    irq_set_exclusive_handler(UART1_IRQ,uart1_irq_handler);
#if !PICONIX_CONSOLE_USB
    irq_set_enabled(UART0_IRQ,true);
#endif
    irq_set_enabled(UART1_IRQ,true);
#if !PICONIX_CONSOLE_USB
    uart_set_irq_enables(uart0,true,false);
#endif
    uart_set_irq_enables(uart1,true,false);
    // Lowest priority for PendSV, SysTick immediately above it.
    // SHPR3: PendSV priority bits 23:22; SysTick bits 31:30.
    *(volatile uint32_t *)0xE000ED20u = 0xC0C00000u;
}
// A fixed slot belongs to one program.  Never migrate a program to another
// slot: this is the foundation for future slot-local .data/.bss/.text linking.
static int spawn_in_slot(int slot, int program_id, os_entry_t entry, const char *name) {
    if (!entry || slot<0 || slot>=OS_MAX_TASKS) return OS_EINVAL;
    uint32_t irq_state = save_and_disable_interrupts();
    task_t *t=&tasks[slot];
    if (t->state!=UNUSED && t->state!=DEAD) { restore_interrupts(irq_state); return OS_EAGAIN; }
    // Reset the fixed slot on every spawn before restoring the flash image.
    memset(task_ram[slot], 0, OS_TASK_BYTES);
    if (program_id && images[program_id].image) {
        // Slot-linked image: the entry must be inside the loaded code image.
        uintptr_t pc=(uintptr_t)entry & ~(uintptr_t)1u;
        uintptr_t base=(uintptr_t)task_ram[slot];
        if (pc < base || pc >= base + images[program_id].size) {
            restore_interrupts(irq_state); return OS_EINVAL;
        }
        // The image must already be linked to this exact RAM slot.
        memcpy(task_ram[slot], images[program_id].image, images[program_id].size);
        memset(task_ram[slot] + images[program_id].zero_start, 0,
               images[program_id].zero_size);
    }
    uint32_t *s=(uint32_t *)(task_ram[slot]+OS_TASK_BYTES);
    *(--s)=0x01000000u;           // xPSR (Thumb)
    *(--s)=(uint32_t)entry;       // PC
    *(--s)=(uint32_t)task_return;// LR
    for(int k=0;k<5;k++) *(--s)=0; // r12,r3,r2,r1,r0
    for(int k=0;k<8;k++) *(--s)=0; // r4-r11 saved by PendSV
    t->sp=s; t->entry=entry; t->name=name?name:"task";
    t->pid=next_pid++; t->startup_seconds=0; t->script_path[0]=0; t->parent_pid=(started && current>=0)?tasks[current].pid:0; t->wake=0; t->wait_dev=-1; t->wait_frame_ready=0;
    t->wait_buf=NULL; t->wait_size=0;
    for(int j=0;j<8;j++) { t->fd[j]=-1; t->fd_pos[j]=0; t->fd_flags[j]=0; }
    t->fd[0]=0; t->fd[1]=0; t->fd[2]=0;
    /* A spawned process inherits the parent's standard streams. */
    if(started && current>=0 && current<OS_MAX_TASKS) {
        for(int j=0;j<3;j++) {
            t->fd[j]=tasks[current].fd[j];
            t->fd_pos[j]=tasks[current].fd_pos[j];
            t->fd_flags[j]=tasks[current].fd_flags[j];
        }
    }
    t->state=READY;
    int pid=t->pid;
    restore_interrupts(irq_state);
    return pid;
}
void os_init_idle(os_entry_t entry) {
    if(started || !entry) return;
    task_t *t = &tasks[IDLE_SLOT];
    uint32_t *s = (uint32_t *)(idle_stack + sizeof(idle_stack));
    *(--s)=0x01000000u;
    *(--s)=(uint32_t)entry;
    *(--s)=(uint32_t)entry;
    for(int k=0;k<5;k++) *(--s)=0;
    for(int k=0;k<8;k++) *(--s)=0;
    t->sp=s; t->entry=entry; t->name="idle";
    t->pid=0; t->state=READY;
}
int os_spawn_named(os_entry_t entry, const char *name) {
    if (started) return OS_EINVAL;
    // Three application slots; the idle task has a separate kernel-owned stack.
    for(int i=0;i<OS_MAX_TASKS;i++)
        if(tasks[i].state==UNUSED && !program_for_slot[i]) return spawn_in_slot(i,0,entry,name);
    return OS_EAGAIN;
}
void os_register_program(int id,os_entry_t entry,const char *name) {
    if(started || id<1 || id>=5) return;
    int slot=program_slot[id];
    /* Multiple images may share one physical slot; only one runs at a time. */
    programs[id].entry=entry;programs[id].name=name;programs[id].slot=slot;
    program_for_slot[slot] = id;
}
int os_start_program(int id) {
    if (started || id<1 || id>=5 || !programs[id].entry) return OS_EINVAL;
    return spawn_in_slot(programs[id].slot, id, programs[id].entry, programs[id].name);
}
int os_spawn(os_entry_t entry) { return os_spawn_named(entry, "task"); }
void isr_systick(void) {
    ticks++;
    for(int i=0;i<OS_MAX_TASKS;i++)
        if(tasks[i].state==SLEEPING && (int32_t)(ticks-tasks[i].wake)>=0)
            tasks[i].state=READY;
    pend();
}
uint32_t *os_schedule(uint32_t *oldsp) {
    // The saved frame is only valid *after* PendSV has stored r4-r11.
    // Keep UART interrupts masked while publishing it and satisfying reads.
    uint32_t irq_state=save_and_disable_interrupts();
    if(current>=0 && oldsp) {
        tasks[current].sp=oldsp;
        if(tasks[current].state==BLOCKED)
            tasks[current].wait_frame_ready=1;
    }
    // An RX IRQ may have arrived after read() blocked but before PendSV saved
    // the frame. Retry the pending reads now that the frame is valid.
    wake_readers(0);
    wake_readers(1);
    pendsv_count++;
    int previous=current;
    int next=-1;
    for(int k=1;k<=OS_MAX_TASKS;k++) {
        int i=(current+k+TOTAL_SLOTS)%TOTAL_SLOTS;
        if(i==IDLE_SLOT) continue;
        if(tasks[i].state==READY) { next=i; break; }
    }
    if(next<0) {
        next=IDLE_SLOT;
    }
    if(previous>=0 && previous!=next) context_switch_count++;
    current=next;
    uint32_t *sp=tasks[next].sp;
    restore_interrupts(irq_state);
    return sp;
}
void os_start(void) {
    if(started || tasks[IDLE_SLOT].state!=READY) panic("PicoNIX: missing idle task");
    started=1;
    // 1 ms SysTick; scheduling decisions on every tick.
    systick_hw->rvr = (clock_get_hz(clk_sys)/1000u)-1u;
    systick_hw->cvr = 0;
    systick_hw->csr = 7;
    pend();
    while(1) __wfi();
}
static int slot_from_pid(int pid) {
    for(int i=0;i<TOTAL_SLOTS;i++)
        if(tasks[i].pid==pid && tasks[i].state!=UNUSED && tasks[i].state!=DEAD) return i;
    return -1;
}
static int in_task_range(const void *p, size_t n) {
    if(current<0 || current>=OS_MAX_TASKS || !p) return 0;
    uintptr_t lo=(uintptr_t)task_ram[current], hi=lo+OS_TASK_BYTES;
    uintptr_t a=(uintptr_t)p;
    return a>=lo && a<=hi && n<=hi-a;
}
static int path_match(const char *p, const char *s) {
    // Validate NUL-terminated paths in the kernel RAM, caller RAM slot,
    // or XIP flash. The shell itself executes from its own RAM slot.
    if(!p) return 0;
    uintptr_t a=(uintptr_t)p;
    if(!((a>=0x10000000u && a<0x11000000u) ||
         (a>=0x20000000u && a<0x20010000u) || in_task_range(p,1))) return 0;
    for(size_t i=0;i<32;i++) {
        uintptr_t q=a+i;
        if(!((q>=0x10000000u && q<0x11000000u) ||
             (q>=0x20000000u && q<0x20010000u) || in_task_range((void*)q,1))) return 0;
        if(p[i]!=s[i]) return 0;
        if(!s[i]) return 1;
    }
    return 0;
}
/* Stable internal device IDs; preserve the existing FD/device mapping. */
enum {
    DEV_CONSOLE = 0,
    DEV_GPIO_VALUE_BASE = 1,
    DEV_GPIO_VALUE_END = 29,
    DEV_UART1 = 30,
    DEV_PROC_UPTIME = 31,
    DEV_PROC_TICKS = 32,
    DEV_PROC_INTERRUPTS = 33,
    DEV_PROC_SWITCHES = 34,
    DEV_PROC_MEMORY = 35,
    DEV_GPIO_DIR_BASE = 36,
    DEV_GPIO_DIR_END = 64,
    DEV_GPIO_PULL_BASE = 65,
    DEV_GPIO_PULL_END = 93,
    DEV_RAND = 94,
    DEV_PROC_DEVICES = 95,
    DEV_PROC_TASKS = 96,
    DEV_TMP_LIST = 97,
    DEV_TMP_FIRST = 100,
    DEV_GPIO_MAX_PIN = 28
};
/* Small volatile regular-file store; IDs are disjoint from devices. */
#define TMP_FILES 4
/* Keep the RAM filesystem within the 64-KiB kernel SRAM slot. */
#define TMP_BYTES 512
static struct { char name[16]; uint16_t size; uint8_t used; uint8_t data[TMP_BYTES]; } tmp_files[TMP_FILES];
/* Check a user path without reading outside the caller's address range. */
static int tmp_name(const char *p, char name[16]) {
    static const char dir[]="/tmp/";
    for(unsigned i=0;i<sizeof(dir)-1;i++) {
        if(!in_task_range(p+i,1) || p[i]!=dir[i]) return 0;
    }
    unsigned len=0;
    for(;len<16;len++) {
        if(!in_task_range(p+5+len,1)) return 0;
        char c=p[5+len];
        if(!c) { if(!len) return 0; name[len]=0; return 1; }
        if(c=='/') return 0;
        if(len==15) return 0;
        name[len]=c;
    }
    return 0;
}
static int tmp_lookup(const char *name) {
    for(int i=0;i<TMP_FILES;i++) if(tmp_files[i].used && !strcmp(name,tmp_files[i].name)) return i;
    return -1;
}
static int tmp_open(const char *p,int flags) {
    char name[16];
    if(!tmp_name(p,name)) return OS_ENOENT;
    int index=tmp_lookup(name);
    if(index<0) {
        if(!(flags & OS_O_CREAT)) return OS_ENOENT;
        for(int i=0;i<TMP_FILES;i++) if(!tmp_files[i].used) { index=i; break; }
        if(index<0) return OS_ENOSPC;
    }
    int fd=3;
    while(fd<8 && tasks[current].fd[fd]>=0) fd++;
    if(fd==8) return OS_EMFILE;
    if(!tmp_files[index].used) {
        memset(&tmp_files[index],0,sizeof(tmp_files[index]));
        strcpy(tmp_files[index].name,name);
        tmp_files[index].used=1;
    }
    if((flags & OS_O_TRUNC) && (flags & 3)!=OS_O_RDONLY) tmp_files[index].size=0;
    tasks[current].fd[fd]=DEV_TMP_FIRST+index;
    tasks[current].fd_pos[fd]=(flags & OS_O_APPEND)?tmp_files[index].size:0;
    tasks[current].fd_flags[fd]=(uint8_t)flags;
    return fd;
}
static int tmp_unlink(const char *p) {
    char name[16];
    if(!tmp_name(p,name)) return OS_ENOENT;
    int index=tmp_lookup(name);
    if(index<0) return OS_ENOENT;
    /* Do not recycle a slot while an open descriptor still refers to it. */
    for(int t=0;t<OS_MAX_TASKS;t++) if(tasks[t].state!=UNUSED && tasks[t].state!=DEAD)
        for(int fd=0;fd<8;fd++) if(tasks[t].fd[fd]==DEV_TMP_FIRST+index) return OS_EBUSY;
    tmp_files[index].used=0;
    return 0;
}
static int tmp_rw(int fd,void *p,size_t n,int writing) {
    int id=tasks[current].fd[fd]-DEV_TMP_FIRST;
    if(id<0 || id>=TMP_FILES || !tmp_files[id].used) return OS_EBADF;
    int mode=tasks[current].fd_flags[fd] & 3;
    if((writing && mode==OS_O_RDONLY) || (!writing && mode==OS_O_WRONLY)) return OS_EBADF;
    size_t pos=tasks[current].fd_pos[fd];
    if(writing && (tasks[current].fd_flags[fd] & OS_O_APPEND)) pos=tmp_files[id].size;
    size_t end=writing?TMP_BYTES:tmp_files[id].size;
    size_t count=pos<end?end-pos:0;
    if(count>n) count=n;
    if(writing) {
        if(!count && n) return OS_ENOSPC;
        memcpy(tmp_files[id].data+pos,p,count);
        if(pos+count>tmp_files[id].size) tmp_files[id].size=(uint16_t)(pos+count);
    } else if(count) memcpy(p,tmp_files[id].data+pos,count);
    tasks[current].fd_pos[fd]=(uint16_t)(pos+count);
    return (int)count;
}
static int tmp_list_read(int fd,void *p,size_t n) {
    size_t offset=0,pos=tasks[current].fd_pos[fd],done=0;
    for(int i=0;i<TMP_FILES;i++) if(tmp_files[i].used) {
        size_t len=strlen(tmp_files[i].name)+1;
        if(pos<offset+len && done<n) {
            size_t start=pos>offset?pos-offset:0;
            size_t count=len-start;
            if(count>n-done) count=n-done;
            for(size_t j=0;j<count;j++) ((char*)p)[done+j]=start+j==len-1?'\n':tmp_files[i].name[start+j];
            done+=count;pos+=count;
        }
        offset+=len;
        if(done==n) break;
    }
    tasks[current].fd_pos[fd]=(uint16_t)pos;
    return (int)done;
}
/* Fixed device registry shared by open() and /sys/proc/devices listing. */
static const struct { const char *path; int id; } dev_registry[] = {
    {"/dev/console",DEV_CONSOLE}, {"/dev/uart1",DEV_UART1}, {"/dev/rand",DEV_RAND}
};
#define DEV_REGISTRY_COUNT (sizeof(dev_registry)/sizeof(dev_registry[0]))
static int device_open(const char *p, int flags) {
    if(flags<0 || (flags & ~(OS_O_WRONLY | OS_O_RDWR | OS_O_NONBLOCK | OS_O_CREAT | OS_O_TRUNC | OS_O_APPEND)) != 0 || (flags & 3)==3) return OS_EINVAL;
    if(path_match(p,"/tmp")) {
        if(flags!=OS_O_RDONLY) return OS_EINVAL;
        for(int fd=3;fd<8;fd++) if(tasks[current].fd[fd]<0) {
            tasks[current].fd[fd]=DEV_TMP_LIST;tasks[current].fd_pos[fd]=0;tasks[current].fd_flags[fd]=0;return fd;
        }
        return OS_EMFILE;
    }
    if(path_match(p,"/tmp/") || (in_task_range(p,5) && p[0]=='/' && p[1]=='t' && p[2]=='m' && p[3]=='p' && p[4]=='/'))
        return tmp_open(p,flags);
    if(flags & (OS_O_CREAT | OS_O_TRUNC | OS_O_APPEND)) return OS_EINVAL;
    int dev=-1;
    for(unsigned i=0;i<DEV_REGISTRY_COUNT;i++)
        if(path_match(p,dev_registry[i].path)) { dev=dev_registry[i].id; break; }
    if(dev<0 && path_match(p,"/sys/proc/uptime")) dev=DEV_PROC_UPTIME;
    else if(dev<0 && path_match(p,"/sys/proc/ticks")) dev=DEV_PROC_TICKS;
    else if(dev<0 && path_match(p,"/sys/proc/interrupts")) dev=DEV_PROC_INTERRUPTS;
    else if(dev<0 && path_match(p,"/sys/proc/context_switches")) dev=DEV_PROC_SWITCHES;
    else if(dev<0 && path_match(p,"/sys/proc/memory")) dev=DEV_PROC_MEMORY;
    else if(dev<0 && path_match(p,"/sys/proc/devices")) dev=DEV_PROC_DEVICES;
    else if(dev<0 && path_match(p,"/sys/proc/tasks")) dev=DEV_PROC_TASKS;
    if(dev<0) {
        // Avoid unbounded parsing of an invalid pointer.
        for(int pin=0;pin<=DEV_GPIO_MAX_PIN;pin++) {
            char name[20]; snprintf(name,sizeof(name),"/dev/gpio/%d",pin);
            if(path_match(p,name)) { dev=pin+DEV_GPIO_VALUE_BASE; break; }
            snprintf(name,sizeof(name),"/dev/gpio/%d.dir",pin);
            if(path_match(p,name)) { dev=DEV_GPIO_DIR_BASE+pin; break; }
            snprintf(name,sizeof(name),"/dev/gpio/%d.pull",pin);
            if(path_match(p,name)) { dev=DEV_GPIO_PULL_BASE+pin; break; }
        }
    }
    if(dev<0 || (((dev>=DEV_PROC_UPTIME && dev<=DEV_PROC_MEMORY) || dev==DEV_RAND || dev==DEV_PROC_DEVICES || dev==DEV_PROC_TASKS) && (flags & 3)!=OS_O_RDONLY)) return OS_EINVAL;
    if((flags & OS_O_NONBLOCK) && dev!=DEV_UART1) return OS_EINVAL;
    for(int fd=3;fd<8;fd++) if(tasks[current].fd[fd]<0) {
        tasks[current].fd[fd]=dev; tasks[current].fd_pos[fd]=0; tasks[current].fd_flags[fd]=(uint8_t)flags; return fd;
    }
    return OS_EMFILE;
}
static int device_rw_rand(void *p, size_t n, int writing) {
    if(writing) return OS_EBADF;
    if(n==0) return 0;
    uint32_t irq_state=save_and_disable_interrupts();
    /* Mix the caller's timing into the initial PRNG state. */
    static int seeded;
    if(!seeded) {
        rand_state ^= time_us_32() ^ ticks;
        if(!rand_state) rand_state=0x9e3779b9u;
        seeded=1;
    }
    uint32_t value=rand32();
    restore_interrupts(irq_state);
    size_t count=n<sizeof(value) ? n : sizeof(value);
    memcpy(p,&value,count);
    return (int)count;
}
/* Render one line at a time; proc/tasks can exceed the normal proc buffer.
 * The per-open cursor supports arbitrarily small read() requests. */
static int proc_tasks_read(int fd, void *p, size_t n) {
    static const char header[] = "PID  NAME              STATE    SLOT  MEMORY             SP\n";
    size_t pos=tasks[current].fd_pos[fd], offset=0, done=0;
    for(int row=-1;row<OS_MAX_TASKS;row++) {
        char line[112];
        const char *src;
        size_t len;
        if(row<0) { src=header; len=sizeof(header)-1; }
        else {
            task_t *t=&tasks[row];
            if(t->state==UNUSED || t->state==DEAD) continue;
            const char *state=t->state==READY?"READY":
                t->state==SLEEPING?"SLEEP":t->state==BLOCKED?"BLOCKED":"?";
            uintptr_t base=(uintptr_t)task_ram[row];
            int count=snprintf(line,sizeof line,"%3d  %-18.18s%-9s%d     %08lX-%08lX  %08lX\n",
                t->pid,t->name?t->name:"",state,row+1,
                (unsigned long)base,(unsigned long)(base+OS_TASK_BYTES-1u),
                (unsigned long)(uintptr_t)t->sp);
            if(count<0 || (size_t)count>=sizeof line) return OS_EINVAL;
            src=line; len=(size_t)count;
        }
        if(pos<offset+len && done<n) {
            size_t start=pos>offset?pos-offset:0;
            size_t count=len-start;
            if(count>n-done) count=n-done;
            memcpy((char *)p+done,src+start,count);
            done+=count; pos+=count;
        }
        offset+=len;
        if(done==n) break;
    }
    tasks[current].fd_pos[fd]=(uint16_t)pos;
    return (int)done;
}
static int device_rw_proc(int fd, int dev, void *p, size_t n, int writing) {
    if(writing) return OS_EBADF;
    if(dev==DEV_PROC_TASKS) return proc_tasks_read(fd,p,n);
    // Snapshot the counters before formatting; no extra interrupts or tasks.
    uint32_t irq_state=save_and_disable_interrupts();
    uint32_t t=ticks, svc=svc_count, psv=pendsv_count, switches=context_switch_count;
    restore_interrupts(irq_state);
    char text[160]; int len=0;
    if(dev==DEV_PROC_DEVICES) {
        /* Enumerate the same registry used by device_open(). */
        for(unsigned i=0;i<DEV_REGISTRY_COUNT;i++) {
            const char *name=dev_registry[i].path+5; /* skip /dev/ */
            int written=snprintf(text+len,sizeof(text)-(size_t)len,"%s\n",name);
            if(written<0 || (size_t)written>=sizeof(text)-(size_t)len) return OS_EINVAL;
            len+=written;
        }
        /* GPIO is a dynamically generated subtree, not a fixed device. */
        int written=snprintf(text+len,sizeof(text)-(size_t)len,"gpio/\n");
        if(written<0 || (size_t)written>=sizeof(text)-(size_t)len) return OS_EINVAL;
        len+=written;
    }
    switch(dev) {
        case DEV_PROC_UPTIME: len=snprintf(text,sizeof text,"%lu.%03lu s\n",(unsigned long)(t/1000u),(unsigned long)(t%1000u)); break;
        case DEV_PROC_TICKS: len=snprintf(text,sizeof text,"%lu\n",(unsigned long)t); break;
        case DEV_PROC_INTERRUPTS: len=snprintf(text,sizeof text,"SysTick %lu\nPendSV %lu\nSVC %lu\n",(unsigned long)t,(unsigned long)psv,(unsigned long)svc); break;
        case DEV_PROC_SWITCHES: len=snprintf(text,sizeof text,"%lu\n",(unsigned long)switches); break;
        case DEV_PROC_MEMORY: len=snprintf(text,sizeof text,"kernel 20000000-2000FFFF\nshell  20010000-2001FFFF (slot#1)\n       20020000-2002FFFF (slot#2)\n       20030000-2003FFFF (slot#3)\n"); break;
    }
    if(len<0) return OS_EINVAL;
    if((size_t)len>=sizeof text) len=sizeof text-1;
    // Each open proc file has a read cursor, so long reports can be
    // transferred in small chunks without truncation or repeated text.
    size_t pos=tasks[current].fd_pos[fd];
    if(pos>=(size_t)len) return 0;
    size_t count=(size_t)len-pos;
    if(count>n) count=n;
    if(count) memcpy(p,text+pos,count);
    tasks[current].fd_pos[fd]=(uint16_t)(pos+count);
    return (int)count;
}
static int device_rw_tty(int fd, int dev, void *p, size_t n, int writing) {
    int uartdev=(dev==DEV_CONSOLE)?0:1;
#if !PICONIX_CONSOLE_USB
    uart_inst_t *u=(dev==DEV_CONSOLE)?uart0:uart1;
#endif
    uint8_t *b=p;
    if(writing) {
        if (dev==DEV_CONSOLE) {
#if PICONIX_CONSOLE_USB
            if(!stdio_usb_connected()) return (int)n;
            return (int)console_queue(b,n);
#else
            for(size_t i=0;i<n;i++) tty_put(b[i]);
#endif
        } else {
#if PICONIX_CONSOLE_USB
            for(size_t i=0;i<n;i++) uart_putc_raw(uart1,b[i]);
#else
            for(size_t i=0;i<n;i++) uart_putc_raw(u,b[i]);
#endif
        }
        return (int)n;
    }
    if(n==0) return 0;
    if(uartdev==DEV_CONSOLE && tasks[current].pid!=tty_owner_pid()) return OS_EIO;
    uint32_t irq_state=save_and_disable_interrupts();
    int count=0;
    if(uartdev==DEV_CONSOLE && tty_used) {
        count=tty_dequeue(b,n);
        restore_interrupts(irq_state);
        return count; /* Zero is EOF from Ctrl+D on an empty line. */
    } else if(uartdev==1) {
        while((size_t)count<n && uart_pop(1,&b[count])) count++;
    }
    if(count) { restore_interrupts(irq_state); return count; }
    if(uartdev==1 && (tasks[current].fd_flags[fd] & OS_O_NONBLOCK)) {
        restore_interrupts(irq_state); return 0;
    }
    task_t *t=&tasks[current];
    t->wait_dev=uartdev;t->wait_buf=p;t->wait_size=n;
    t->wait_frame_ready=0;
    t->state=BLOCKED;
    pend();
    restore_interrupts(irq_state);
    return 0; // completed later by IRQ; SVC frame initially holds 0
}
static int device_rw_gpio(int dev, void *p, size_t n, int writing) {
    if(n==0) return 0;
    if(dev>=DEV_GPIO_DIR_BASE && dev<=DEV_GPIO_PULL_END) {
        int pin=(dev<=DEV_GPIO_DIR_END)?dev-DEV_GPIO_DIR_BASE:dev-DEV_GPIO_PULL_BASE;
        // UART pins must not be silently reconfigured by GPIO settings.
        if(writing && (pin==0 || pin==1 || pin==4 || pin==5)) return OS_EINVAL;
        if(dev<=DEV_GPIO_DIR_END) {
            if(writing) {
                const uint8_t *b=p;
                if(n==2 && b[0]=='i' && b[1]=='n') {gpio_init(pin);gpio_set_dir(pin,GPIO_IN);return (int)n;}
                if(n==3 && b[0]=='o' && b[1]=='u' && b[2]=='t') {gpio_init(pin);gpio_set_dir(pin,GPIO_OUT);return (int)n;}
                return OS_EINVAL;
            }
            const char *v=gpio_get_dir(pin)==GPIO_OUT?"out":"in";
            size_t len=(v[0]=='o')?3:2;
            if(n<len) len=n;
            memcpy(p,v,len); return (int)len;
        }
        if(writing) {
            const uint8_t *b=p;
            if(n==2 && b[0]=='u' && b[1]=='p') {gpio_pull_up(pin);return (int)n;}
            if(n==4 && !memcmp(b,"down",4)) {gpio_pull_down(pin);return (int)n;}
            if(n==4 && !memcmp(b,"none",4)) {gpio_disable_pulls(pin);return (int)n;}
            return OS_EINVAL;
        }
        const char *v=gpio_is_pulled_up(pin)?"up":gpio_is_pulled_down(pin)?"down":"none";
        size_t len=strlen(v);
        if(n<len) len=n;
        memcpy(p,v,len); return (int)len;
    }
    int pin=dev-DEV_GPIO_VALUE_BASE;
    if(writing) {
        const uint8_t *b=p;
        if(n>=2 && b[0]=='i' && b[1]=='n') {if(pin==0||pin==1||pin==4||pin==5) return OS_EINVAL; gpio_init(pin);gpio_set_dir(pin,GPIO_IN);return (int)n;}
        if(n>=3 && b[0]=='o' && b[1]=='u' && b[2]=='t') {if(pin==0||pin==1||pin==4||pin==5) return OS_EINVAL; gpio_init(pin);gpio_set_dir(pin,GPIO_OUT);return (int)n;}
        if(b[0]!='0' && b[0]!='1') return OS_EINVAL;
        gpio_put(pin,b[0]=='1');
        return (int)n;
    }
    ((uint8_t*)p)[0]=gpio_get(pin)?'1':'0';
    return 1;
}
static int device_rw(int fd, void *p, size_t n, int writing) {
    if(fd<0 || fd>=8 || tasks[current].fd[fd]<0) return OS_EBADF;
    if(n>0 && !in_task_range(p,n)) return OS_EINVAL;
    int dev=tasks[current].fd[fd];
    if(dev>=DEV_TMP_FIRST && dev<DEV_TMP_FIRST+TMP_FILES) return tmp_rw(fd,p,n,writing);
    if(dev==DEV_TMP_LIST) return writing?OS_EBADF:tmp_list_read(fd,p,n);
    if(dev==DEV_RAND) return device_rw_rand(p,n,writing);
    if((dev>=DEV_PROC_UPTIME && dev<=DEV_PROC_MEMORY) || dev==DEV_PROC_DEVICES || dev==DEV_PROC_TASKS)
        return device_rw_proc(fd,dev,p,n,writing);
    if(dev==DEV_CONSOLE || dev==DEV_UART1) return device_rw_tty(fd,dev,p,n,writing);
    return device_rw_gpio(dev,p,n,writing);
}
static int device_ioctl(int fd,int cmd,int arg) {
    if(fd<0 || fd>=8 || tasks[current].fd[fd]<DEV_GPIO_VALUE_BASE || tasks[current].fd[fd]>DEV_GPIO_VALUE_END) return OS_EBADF;
    int pin=tasks[current].fd[fd]-DEV_GPIO_VALUE_BASE;
    (void)arg;
    switch(cmd) {
        case OS_GPIO_INPUT: gpio_init(pin); gpio_set_dir(pin,GPIO_IN); break;
        case OS_GPIO_OUTPUT: gpio_init(pin); gpio_set_dir(pin,GPIO_OUT); break;
        case OS_GPIO_PULL_UP: gpio_pull_up(pin); break;
        case OS_GPIO_PULL_DOWN: gpio_pull_down(pin); break;
        case OS_GPIO_NO_PULL: gpio_disable_pulls(pin); break;
        default: return OS_EINVAL;
    }
    return 0;
}
// SVC receives a pointer to the hardware-stacked r0-r3,r12,lr,pc,xPSR.
void os_svc_dispatch(uint32_t *f) {
    int num=(int)f[0], a=(int)f[1], b=(int)f[2], c=(int)f[3];
    int result=OS_ENOSYS;
    svc_count++;
    if(current<0) { f[0]=OS_EINVAL; return; }
    switch(num) {
        case SYS_GETPID: result=tasks[current].pid; break;
        case SYS_KILL:
            {int slot=slot_from_pid(a);
            if(slot<0 || slot==current || slot==IDLE_SLOT) result=OS_ESRCH;
            else {finish_task(slot,128+15);result=0;}}
            break;
        case SYS_EXIT: if(current!=IDLE_SLOT) { finish_task(current,a); pend(); } result=0; break;
        case SYS_SLEEP: if(current!=IDLE_SLOT && a>0) { tasks[current].wake=ticks+(uint32_t)a; tasks[current].state=SLEEPING; pend(); } result=0; break;
        case SYS_YIELD: pend(); result=0; break;
        case SYS_WAITPID: {
            /* a=child pid, b=pointer to exit status; nonblocking primitive.
               libc-free wrapper below supplies blocking wait semantics. */
            int *status=(int *)(uintptr_t)b;
            if(!in_task_range(status,sizeof(*status))) { result=OS_EINVAL; break; }
            result=OS_ESRCH;
            for(int i=0;i<OS_MAX_TASKS;i++) if(exited[i].valid && (a==-1 || exited[i].pid==a) &&
                exited[i].parent_pid==tasks[current].pid) {
                *status=exited[i].status; result=exited[i].pid; exited[i].valid=0; break;
            }
            if(result==OS_ESRCH) for(int i=0;i<OS_MAX_TASKS;i++)
                if(tasks[i].pid==a && tasks[i].parent_pid==tasks[current].pid &&
                   tasks[i].state!=UNUSED && tasks[i].state!=DEAD) {result=0;break;}
            break;
        }
        case SYS_SET_FG: {
            if(a==0) { foreground_pid=0; result=0; break; }
            result=OS_ESRCH;
            for(int i=0;i<OS_MAX_TASKS;i++) if(tasks[i].pid==a &&
                tasks[i].parent_pid==tasks[current].pid && tasks[i].state!=DEAD) {
                foreground_pid=a; result=0; break;
            }
            break;
        }
        case SYS_SPAWN:
            if(a<1 || a>=5 || !programs[a].entry) result=OS_ENOENT;
            else if(b<0 || b>31 || (a!=3 && b!=0)) result=OS_EINVAL;
            else if(a==4 && c) {
                const char *path=(const char *)(uintptr_t)c;
                unsigned i=0;
                while(i<31 && in_task_range(path+i,1) && path[i]) i++;
                if(i==31 || !in_task_range(path+i,1)) result=OS_EINVAL;
                else result=0;
                if(result<0) break;
                result=spawn_in_slot(programs[a].slot,a,programs[a].entry,programs[a].name);
                if(result>0) {
                    task_t *child=&tasks[programs[a].slot];
                    for(unsigned j=0;j<=i;j++) child->script_path[j]=path[j];
                }
                break;
            }
            else {
                result=spawn_in_slot(programs[a].slot,a,programs[a].entry,programs[a].name);
                if(result>0) {
                    task_t *child=&tasks[programs[a].slot];
                    child->startup_seconds=b;
                    if(a==4 && c) {
                        const char *path=(const char *)(uintptr_t)c;
                        unsigned i=0;
                        while(i<sizeof(child->script_path)-1 && path[i]) {
                            child->script_path[i]=path[i]; i++;
                        }
                        child->script_path[i]=0;
                    }
                }
            }
            break;
        case SYS_GETARG:
            if(a && b && tasks[current].script_path[0]) {
                char *dest=(char *)(uintptr_t)a;
                unsigned n=0;
                while(tasks[current].script_path[n]) n++;
                if((unsigned)b<=n || !in_task_range(dest,n+1)) result=OS_EINVAL;
                else { memcpy(dest,tasks[current].script_path,n+1); result=(int)n; }
            } else result=tasks[current].startup_seconds;
            break;
        case SYS_CONSOLE_READY:
#if PICONIX_CONSOLE_USB
            result=stdio_usb_connected() ? 1 : 0;
#else
            result=1;
#endif
            break;
        case SYS_UNLINK: result=tmp_unlink((const char *)(uintptr_t)a); break;
        case SYS_DUP2:
            if(a<0 || a>=8 || b<0 || b>=8 || tasks[current].fd[a]<0) result=OS_EBADF;
            else {
                tasks[current].fd[b]=tasks[current].fd[a];
                tasks[current].fd_pos[b]=tasks[current].fd_pos[a];
                tasks[current].fd_flags[b]=tasks[current].fd_flags[a];
                result=b;
            }
            break;
        case SYS_OPEN: result=device_open((const char *)(uintptr_t)a,b); break;
        case SYS_CLOSE: if(a<3 || a>=8 || tasks[current].fd[a]<0) result=OS_EBADF;
            else {tasks[current].fd[a]=-1; tasks[current].fd_pos[a]=0; tasks[current].fd_flags[a]=0; result=0;} break;
        case SYS_READ: result=device_rw(a,(void *)(uintptr_t)b,(size_t)c,0); break;
        case SYS_WRITE: result=device_rw(a,(void *)(uintptr_t)b,(size_t)c,1); break;
        case SYS_IOCTL: result=device_ioctl(a,b,c); break;
    }
    f[0]=(uint32_t)result;
}
// Use explicit registers to pass arguments to the SVC dispatcher.
static inline int call(int n, uintptr_t a, uintptr_t b, uintptr_t c) {
    register uintptr_t r0 __asm("r0")=(uintptr_t)n;
    register uintptr_t r1 __asm("r1")=a;
    register uintptr_t r2 __asm("r2")=b;
    register uintptr_t r3 __asm("r3")=c;
    __asm volatile("svc #0" : "+r"(r0) : "r"(r1),"r"(r2),"r"(r3) : "memory","cc");
    return (int)r0;
}
int os_waitpid(int pid, int *status, int options) {
    if(options & ~1) return OS_EINVAL;
    for(;;) {
        int rc=call(SYS_WAITPID,pid,(uintptr_t)status,0);
        if(rc!=0 || (options & 1)) return rc;
        os_sleep(10); /* scheduler remains active; no busy wait */
    }
}
int os_set_foreground(int pid) { return call(SYS_SET_FG,pid,0,0); }
int os_spawn_program(int program_id) { return call(SYS_SPAWN,program_id,0,0); }
int os_getpid(void) { return call(SYS_GETPID,0,0,0); }
int os_kill(int pid) { return call(SYS_KILL,pid,0,0); }
void os_exit(void) { call(SYS_EXIT,0,0,0); for(;;) __wfi(); }
void os_sleep(uint32_t ms) { call(SYS_SLEEP,ms,0,0); }
void os_yield(void) { call(SYS_YIELD,0,0,0); }
int os_open(const char *p,int f) { return call(SYS_OPEN,(uintptr_t)p,f,0); }
int os_close(int fd) { return call(SYS_CLOSE,fd,0,0); }
int os_read(int fd,void *p,size_t n) { return call(SYS_READ,fd,(uintptr_t)p,n); }
int os_write(int fd,const void *p,size_t n) { return call(SYS_WRITE,fd,(uintptr_t)p,n); }
int os_ioctl(int fd,int cmd,int v) { return call(SYS_IOCTL,fd,cmd,v); }

int os_dup2(int oldfd,int newfd) { return call(SYS_DUP2,oldfd,newfd,0); }
int os_unlink(const char *p) { return call(SYS_UNLINK,(uintptr_t)p,0,0); }
