/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "../include/gpio_status.h"
#include "piconix.h"
#include "devices.h"
#include "pins.h"
#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "hardware/clocks.h"
#include "hardware/structs/scb.h"
#include "hardware/structs/systick.h"
#include "hardware/sync.h"
#include "hardware/irq.h"
#include <string.h>
#include <stdio.h>
#include "../userland/trace.h"
#include "../include/piconix_dirent.h"
#if PICONIX_CONSOLE_USB
#include "pico/stdio_usb.h"
#include "tusb.h"
#endif

enum { UNUSED, READY, SLEEPING, DEAD, BLOCKED };
#define IDLE_SLOT OS_MAX_TASKS
#define TOTAL_SLOTS (OS_MAX_TASKS + 1)
enum { SYS_GETPID=1, SYS_KILL, SYS_EXIT, SYS_SLEEP, SYS_YIELD,
       SYS_OPEN, SYS_CLOSE, SYS_READ, SYS_WRITE, SYS_IOCTL, SYS_RESERVED_TASK_INFO, SYS_SPAWN, SYS_WAITPID, SYS_SET_FG, SYS_RESERVED_GETARG, SYS_CONSOLE_READY, SYS_UNLINK, SYS_DUP2, SYS_TTY_SPAWN, SYS_GET_TTY, SYS_LSEEK, SYS_CHDIR, SYS_GETCWD, SYS_FCNTL, SYS_SPAWN_TMP, SYS_GETDENTS };
typedef struct { uint32_t *sp; uint32_t wake; uint8_t state; os_entry_t entry; const char *name; int fd[8]; uint32_t fd_pos[8]; int fd_task_pid[8]; uint8_t fd_flags[8]; int pid; int parent_pid; int startup_seconds; char script_path[32]; int wait_dev; int wait_fd; void *wait_buf; size_t wait_size; uint8_t wait_frame_ready; int wait_pid; int *wait_status; uint8_t tty; uint8_t program_id; char cwd[128]; uint16_t adc_values[8]; } task_t;
// Five fixed 32-KiB SRAM slots, not a .bss reservation.
// The linker must restrict the kernel to kernel SRAM (0x20000000..0x20017fff).
_Static_assert(OS_MAX_TASKS == 5 && OS_TASK_BYTES == 32768u,
               "fixed RP2040 SRAM slot geometry changed");
__asm__(".global task_ram\n.set task_ram, 0x20018000");
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
static volatile int foreground_pid[2];
static int tty_shell_pid[2];
/* Console mode is owned by the foreground application and reset on exit. */
static uint8_t tty_canonical[2]={1,1}, tty_echo[2]={1,1};
static uint8_t tty_onlcr[2]={1,1}, tty_last_cr[2];
static int tty_mode_owner[2];
/* Foreground task gets a temporary TTY mode; restore what its parent had. */
static int tty_restore_pid[2];
static uint8_t tty_saved_canon[2], tty_saved_echo[2], tty_saved_onlcr[2];
static int tty_saved_owner[2];
/* Temporary exclusive control of an otherwise idle terminal. */
static int tty_claim_pid[2];
static void tty_mode_reset(int dev);
/* TTY line buffers must be declared before finish_task() uses them. */
#define TTY_LINES 8u
typedef struct {
    char line[128]; unsigned len;
    struct {char text[128];uint8_t len;} lines[TTY_LINES];
    unsigned head,tail,used,offset;
} tty_buffer_t;
static tty_buffer_t tty_buf[2];
/* Fixed 32-KiB trace ring, 1024 records of 32 bytes. */
#define TRACE_CAPACITY 1024u
_Static_assert(sizeof(trace_event_t) == 32, "trace record must be 32 bytes");
static trace_event_t trace_ring[TRACE_CAPACITY];
static uint32_t trace_head;
static uint32_t trace_count;
static uint32_t trace_serial;
static uint8_t trace_enabled;
static uint16_t tick_sample;
static uint8_t slice_ticks;
static void trace_record(uint8_t kind, uint8_t pid, uint32_t value, uint32_t detail,
                         uint8_t program_id, int8_t file_id, const void *data, unsigned len) {
    if(!trace_enabled || (program_id & 0x7fu)==7u) return;
    /* Never record switches to/from the trace viewer either. */
    if(kind==TRACE_SWITCH) {
        for(int i=0;i<OS_MAX_TASKS;i++)
            if((tasks[i].pid==pid || tasks[i].pid==(int)value) &&
               tasks[i].state!=UNUSED && tasks[i].program_id==7u) return;
    }
    trace_event_t *e = &trace_ring[trace_head];
    e->tick = ticks;
    e->value = value;
    e->detail = detail;
    e->kind = kind;
    e->pid = pid;
    /* Upper bit stores the task TTY; low bits remain the program ID. */
    for(int i=0;i<OS_MAX_TASKS;i++)
        if(pid && tasks[i].pid==pid && tasks[i].state!=UNUSED) {
            if(tasks[i].tty) program_id |= 0x80u;
            break;
        }
    e->program_id = program_id;
    e->file_id = file_id;
    memset(e->data, 0, sizeof e->data);
    if(data) {
        if(len > sizeof e->data) len = sizeof e->data;
        memcpy(e->data, data, len);
    }
    trace_head = (trace_head + 1u) & (TRACE_CAPACITY - 1u);
    if(trace_count < TRACE_CAPACITY) trace_count++;
    trace_serial++;
}

static int tty_owner_pid(int tty) { return tty_claim_pid[tty] ? tty_claim_pid[tty] :
    (foreground_pid[tty] ? foreground_pid[tty] : tty_shell_pid[tty]); }
/* Five pending completion notices, independent of reusable RAM slots. */
static struct { int pid, parent_pid; } exited[OS_MAX_TASKS];
static int take_exit(int parent, int requested) {
    for(int i=0;i<OS_MAX_TASKS;i++)
        if(exited[i].pid && exited[i].parent_pid==parent &&
           (requested==-1 || exited[i].pid==requested)) {
            int pid=exited[i].pid;
            exited[i].pid=0;
            return pid;
        }
    return 0;
}
/* Keep all per-descriptor fields consistent on open, close and reset. */
static void fd_set(task_t *t, int fd, int dev, uint32_t pos, int flags, int target_pid) {
    t->fd[fd]=dev;
    t->fd_pos[fd]=pos;
    t->fd_task_pid[fd]=target_pid;
    t->fd_flags[fd]=(uint8_t)flags;
}
static void finish_task(int slot, int status) {
    (void)status; /* PicoNIX retains PIDs, not exit status. */
    task_t *t=&tasks[slot];
    if(t->state==UNUSED || t->state==DEAD) return;
    int parent_alive=0;
    for(int i=0;i<OS_MAX_TASKS;i++)
        if(i!=slot && tasks[i].pid==t->parent_pid &&
           tasks[i].state!=UNUSED && tasks[i].state!=DEAD) parent_alive=1;
    if(parent_alive) {
        /* Oldest notice may be discarded when all five entries are occupied. */
        int pos=-1;
        for(int i=0;i<OS_MAX_TASKS;i++)
            if(!exited[i].pid) { pos=i; break; }
        if(pos<0) pos=0;
        exited[pos].pid=t->pid;
        exited[pos].parent_pid=t->parent_pid;
    }
    /* A dead parent cannot collect notifications. */
    for(int i=0;i<OS_MAX_TASKS;i++)
        if(exited[i].parent_pid==t->pid) exited[i].pid=0;
    t->state=DEAD;
    for(int dev=0;dev<2;dev++) {
        if(tty_claim_pid[dev]==t->pid) {
            tty_claim_pid[dev]=0;
            tty_mode_reset(dev);
        } else if(tty_restore_pid[dev]==t->pid) {
            tty_canonical[dev]=tty_saved_canon[dev];
            tty_echo[dev]=tty_saved_echo[dev];
            tty_onlcr[dev]=tty_saved_onlcr[dev];
            tty_last_cr[dev]=0;
            tty_mode_owner[dev]=tty_saved_owner[dev];
            tty_restore_pid[dev]=0;
            memset(&tty_buf[dev],0,sizeof tty_buf[dev]);
        } else if(tty_mode_owner[dev]==t->pid) tty_mode_reset(dev);
    }
    if(foreground_pid[t->tty]==t->pid) foreground_pid[t->tty]=0;
    if(tty_shell_pid[t->tty]==t->pid) tty_shell_pid[t->tty]=0;
}
static void wake_waiters(void) {
    for(int i=0;i<OS_MAX_TASKS;i++) {
        task_t *t=&tasks[i];
        if(t->state!=BLOCKED || t->wait_dev!=-2 || !t->wait_frame_ready) continue;
        int pid=take_exit(t->pid,t->wait_pid);
        if(!pid) continue;
        if(t->wait_status) *t->wait_status=0;
        t->sp[8]=(uint32_t)pid;
        trace_record(TRACE_SVC_EXIT,(uint8_t)t->pid,SYS_WAITPID,
                     (uint32_t)pid,t->program_id,-1,NULL,0);
        t->state=READY;
        t->wait_frame_ready=0;
    }
}

/* Program images are position independent; RAM slots are assigned at spawn. */
#define PROGRAM_CAPACITY 65  /* IDs 1..64; slot 0 reserved */
static struct { os_entry_t entry; const char *name; } programs[PROGRAM_CAPACITY];
static int free_program_slot(void) {
    for (int i=0;i<OS_MAX_TASKS;i++)
        if (tasks[i].state==UNUSED || tasks[i].state==DEAD) return i;
    return -1;
}
// Optional prelinked RAM image. A nonzero image size means the caller must
// provide the exact slot-linked binary and entry address.
static struct { const uint8_t *image; size_t size; size_t zero_start; size_t zero_size; } images[PROGRAM_CAPACITY];
int os_register_ram_image(int id, const void *image, size_t size,
                          size_t zero_start, size_t zero_size) {
    if (started || id < 1 || id >= PROGRAM_CAPACITY || !image || !size) return OS_EINVAL;
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
static void tty_mode_reset(int dev) {
    tty_canonical[dev]=tty_echo[dev]=tty_onlcr[dev]=1;
    tty_last_cr[dev]=0;
    tty_mode_owner[dev]=0;
    memset(&tty_buf[dev],0,sizeof tty_buf[dev]);
}
static void tty_raw_enqueue(int dev,uint8_t c) {
    tty_buffer_t *t=&tty_buf[dev];
    if(t->used==TTY_LINES)return;
    t->lines[t->head].text[0]=(char)c;t->lines[t->head].len=1;
    t->head=(t->head+1u)%TTY_LINES;t->used++;
}
static void tty_enqueue(int dev) {
    tty_buffer_t *t=&tty_buf[dev];
    if(t->used==TTY_LINES)return;
    memcpy(t->lines[t->head].text,t->line,t->len);
    t->lines[t->head].len=(uint8_t)t->len;
    t->head=(t->head+1u)%TTY_LINES;t->used++;t->len=0;
}
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
#if !PICONIX_CONSOLE_USB
static volatile uint32_t console_irq_count;
static void uart0_irq_handler(void);
#endif
static volatile uint32_t uart1_irq_count;
static void uart1_irq_handler(void);
static void pend(void);
static void wake_readers(int dev);
static int uart_pop(int dev, uint8_t *ch) {
    if(rx[dev].tail==rx[dev].head) return 0;
    *ch=rx[dev].buf[rx[dev].tail];
    rx[dev].tail=(rx[dev].tail+1)%RX_CAP;
    return 1;
}
/* UART1 is a raw stream until a getty/shell owns the terminal. */
static int uart1_read(uint8_t *buf,size_t size) {
    int count=0;
    if(tty_owner_pid(1)) {
        if(tty_buf[1].used)return tty_dequeue(1,buf,size);
        return -1;
    }
    while((size_t)count<size && uart_pop(1,&buf[count]))count++;
    return count;
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
static void tty_put_dev(int dev,uint8_t c) {
    if(dev==0)tty_put(c);
    else uart_putc_raw(uart1,c);
}
/* Canonical input keeps TAB (0x09); only the echo uses four-column stops. */
static unsigned tty_line_columns(const tty_buffer_t *t) {
    unsigned col=0;
    for(unsigned i=0;i<t->len;i++)
        col+=t->line[i]=='\t' ? 4-(col&3u) : 1;
    return col;
}
static void tty_input_char(int dev,uint8_t c) {
    tty_buffer_t *t=&tty_buf[dev];
    if(!tty_canonical[dev] && (c!=3 || tty_claim_pid[dev])){
        tty_raw_enqueue(dev,c);
        if(tty_echo[dev])tty_put_dev(dev,c);
        return;
    }
    if(c==3){
        /* finish_task restores the foreground task's saved TTY mode. */
        memset(t,0,sizeof *t);
        int pid=foreground_pid[dev];
        for(int i=0;i<OS_MAX_TASKS;i++)if(pid && tasks[i].pid==pid){finish_task(i,130);break;}
        tty_put_dev(dev,'^');tty_put_dev(dev,'C');tty_put_dev(dev,'\r');tty_put_dev(dev,'\n');
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
// pico_stdio_usb owns the TinyUSB CDC interface and its callbacks.
// Read through the SDK stdio interface rather than overriding tud_cdc_rx_cb.
// The idle task polls this function while the shell waits for input.
void os_console_poll(void) {
#if PICONIX_CONSOLE_USB
    console_tx_poll();
    int ch;
    int received = 0;
    while (tty_buf[0].used < TTY_LINES && (ch = getchar_timeout_us(0)) >= 0) {
        tty_input_char(0,(uint8_t)ch);
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
            if(!tty_buf[0].used || t->pid!=tty_owner_pid(0)) continue;
            n=tty_dequeue(0,t->wait_buf, t->wait_size);
        } else {
            if(tty_owner_pid(1) && t->pid!=tty_owner_pid(1)) continue;
            uint8_t *b=t->wait_buf;
            if(tty_owner_pid(1) ? !tty_buf[1].used : rx[1].head==rx[1].tail) continue;
            n=uart1_read(b,t->wait_size); /* A zero-length read can be EOF. */
        }
        // Never touch the saved r0 until PendSV has saved the PSP frame.
        /* Terminal reads finish here, not at the initial blocking SVC. */
        trace_record(TRACE_SVC_EXIT, (uint8_t)t->pid, SYS_READ,
                     (uint32_t)n, t->program_id,
                     (int8_t)t->fd[t->wait_fd], n>0 ? t->wait_buf : NULL,
                     n>0 ? (unsigned)n : 0u);
        t->sp[8]=(uint32_t)n;
        t->state=READY;
        t->wait_frame_ready=0;
        pend();
        if(dev==0) break;
    }
}
#if !PICONIX_CONSOLE_USB
static void uart0_irq_handler(void) {
    console_irq_count++;
    while(uart_is_readable(uart0)) tty_input_char(0,uart_getc(uart0));
    wake_readers(0);
}
#endif
static void uart1_irq_handler(void) {
    uart1_irq_count++;
    while(uart_is_readable(uart1)){
        uint8_t c=uart_getc(uart1);
        if(tty_owner_pid(1))tty_input_char(1,c);
        else uart_push(1,c);
    }
    wake_readers(1);
}

static void pend(void) { scb_hw->icsr = (1u << 28); __asm volatile("dsb\n isb" ::: "memory"); }
static void task_return(void) { os_exit(); }


void os_init(void) {
    memset(tasks, 0, sizeof(tasks));
    current=-1; ticks=0; uart1_irq_count=0;
    gpio_driver_init();
    fb_init();
    adc_driver_init();
    pwm_driver_init();
#if !PICONIX_CONSOLE_USB
    console_irq_count=0;
#endif
    svc_count=0; pendsv_count=0; context_switch_count=0; started=0; next_pid=1; foreground_pid[0]=foreground_pid[1]=0; tty_shell_pid[0]=tty_shell_pid[1]=0; memset(exited,0,sizeof exited); memset(tty_buf,0,sizeof tty_buf); tty_canonical[0]=tty_canonical[1]=1; tty_echo[0]=tty_echo[1]=1; tty_onlcr[0]=tty_onlcr[1]=1; tty_last_cr[0]=tty_last_cr[1]=0; tty_mode_owner[0]=tty_mode_owner[1]=0; tty_claim_pid[0]=tty_claim_pid[1]=0; tty_restore_pid[0]=tty_restore_pid[1]=0; tty_saved_owner[0]=tty_saved_owner[1]=0; 
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
    gpio_set_function(20,GPIO_FUNC_UART);
    gpio_set_function(21,GPIO_FUNC_UART);
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
// One PIC program image may run in any free RAM slot.
static int spawn_in_slot(int slot, int program_id, os_entry_t entry, const char *name,
                         int argc, const char *const *args, const uint8_t *raw, unsigned raw_size) {
    if (!entry || slot<0 || slot>=OS_MAX_TASKS) return OS_EINVAL;
    uint32_t irq_state = save_and_disable_interrupts();
    task_t *t=&tasks[slot];
    if (t->state!=UNUSED && t->state!=DEAD) { restore_interrupts(irq_state); return OS_EAGAIN; }
    // Reset the selected slot before copying a position-independent image.
    memset(task_ram[slot], 0, OS_TASK_BYTES);
    if (program_id && images[program_id].image) {
        // app_entry is fixed at offset zero by slot.ld and checked at build time.
        entry=(os_entry_t)((uintptr_t)task_ram[slot] | 1u);
        memcpy(task_ram[slot], images[program_id].image, images[program_id].size);
        memset(task_ram[slot] + images[program_id].zero_start, 0,
               images[program_id].zero_size);
    }
    if(raw && raw_size) memcpy(task_ram[slot],raw,raw_size);
    /* Reserve argv[0..8] and up to 128 bytes of strings inside the child slot.
     * The initial context is kept below this area. */
    uint8_t *top=task_ram[slot]+OS_TASK_BYTES;
    char *strings=(char *)(top-192);
    char **argv=(char **)(top-64);
    unsigned used=0;
    if (argc==0) { /* Boot and tty spawn: argv[0] is the registered name. */
        args=&name;
        argc=1;
    }
    for(int i=0;i<argc;i++) {
        const char *src=args[i];
        unsigned len=0;
        while(src[len]) len++;
        argv[i]=strings+used;
        memcpy(strings+used,src,len+1);
        used+=len+1;
    }
    argv[argc]=NULL;
    uint32_t *s=(uint32_t *)(top-256);
    *(--s)=0x01000000u;           // xPSR (Thumb)
    *(--s)=(uint32_t)entry;       // PC
    *(--s)=(uint32_t)task_return;// LR
    *(--s)=0;                    // r12
    *(--s)=0;                    // r3
    *(--s)=0;                    // r2
    *(--s)=(uint32_t)argv;       // r1: argv
    *(--s)=argc;                 // r0: argc
    for(int k=0;k<8;k++) *(--s)=0; // r4-r11 saved by PendSV
    t->sp=s; t->entry=entry; t->name=name?name:"task"; t->program_id=(uint8_t)program_id;
    t->pid=next_pid++; t->startup_seconds=0; t->script_path[0]=0; t->parent_pid=(started && current>=0)?tasks[current].pid:0; t->wake=0; t->wait_dev=-1; t->wait_frame_ready=0; t->wait_pid=0; t->wait_status=NULL;
    t->wait_buf=NULL; t->wait_size=0; strcpy(t->cwd,(started && current>=0)?tasks[current].cwd:"/"); t->tty=(started && current>=0)?tasks[current].tty:0;
    for(int j=0;j<8;j++) fd_set(t,j,-1,0,0,0);
    for(int j=0;j<3;j++) fd_set(t,j,0,0,0,0);
    /* A spawned process inherits the parent's standard streams. */
    if(started && current>=0 && current<OS_MAX_TASKS) {
        for(int j=0;j<3;j++) {
            t->fd[j]=tasks[current].fd[j];
            t->fd_pos[j]=tasks[current].fd_pos[j];
            t->fd_task_pid[j]=tasks[current].fd_task_pid[j];
            t->fd_flags[j]=tasks[current].fd_flags[j];
        }
    }
    t->state=READY;
    if(program_id==2 && !tty_shell_pid[t->tty]) tty_shell_pid[t->tty]=t->pid;
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
    // Five application slots; the idle task has a separate kernel-owned stack.
    for(int i=0;i<OS_MAX_TASKS;i++)
        if(tasks[i].state==UNUSED) return spawn_in_slot(i,0,entry,name,0,NULL,NULL,0);
    return OS_EAGAIN;
}
void os_register_program(int id,os_entry_t entry,const char *name) {
    if(started || id<1 || id>=PROGRAM_CAPACITY) return;
    programs[id].entry=entry; programs[id].name=name;
}
int os_start_program(int id) {
    if (started || id<1 || id>=PROGRAM_CAPACITY || !programs[id].entry) return OS_EINVAL;
    int slot=free_program_slot();
    return slot<0 ? OS_EAGAIN : spawn_in_slot(slot,id,programs[id].entry,programs[id].name,0,NULL,NULL,0);
}
int os_spawn(os_entry_t entry) { return os_spawn_named(entry, "task"); }
void isr_systick(void) {
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
    wake_waiters();
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
void os_start(void) {
    if(started || tasks[IDLE_SLOT].state!=READY) panic("PicoNIX: missing idle task");
    started=1;
    // 1 ms SysTick; normal time slicing every 10 ticks.
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
/* This helper receives a kernel-normalized path, not a user pointer.
 * The caller validates the user pointer before constructing that path. */
static int path_match(const char *p, const char *s) {
    return strcmp(p,s)==0;
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
    DEV_PROC_RAMFILES = 136,
    DEV_GPIO_DIR_BASE = 36,
    DEV_GPIO_DIR_END = 64,
    DEV_GPIO_PULL_BASE = 65,
    DEV_GPIO_PULL_END = 93,
    DEV_RAND = 94,
    DEV_PROC_DEVICES = 95,
    DEV_PROC_TASKS = 96,
    DEV_ADC_BASE = 121,
    DEV_ADC_END = 124,
    DEV_PWM_FREQ = 125,
    DEV_PWM_18 = 126,
    DEV_PWM_19 = 127,
    DEV_MEM = 98,
    DEV_SYMS_KERNEL = 135,
    DEV_SYMS_USER_BASE = 200, /* 200..263: registered user programs */
    DEV_TRACE = 99,
    DEV_FB = 128,
    DEV_PROC_FB = 129,
    DEV_PROC_TASK_BASE = 130, /* 130..134: physical task slots */
    DEV_TMP_FIRST = 100,
    DEV_GPIO_MAX_PIN = 28
};
/* Small volatile regular-file store; IDs are disjoint from devices. */
#define TMP_FILES 4
/* Keep the RAM filesystem within the 96-KiB kernel SRAM region. */
#define TMP_BYTES 1024
static struct { char name[16]; uint16_t size; uint8_t used; uint8_t data[TMP_BYTES]; } tmp_files[TMP_FILES];
/* Check a user path without reading outside the caller's address range. */
static int tmp_name(const char *p, char name[16]) {
    static const char dir[]="/tmp/";
    for(unsigned i=0;i<sizeof(dir)-1;i++) {
        if(p[i]!=dir[i]) return 0;
    }
    unsigned len=0;
    for(;len<16;len++) {
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
/* User-openable descriptors start at 3; 0..2 are standard streams. */
static int free_fd(void) {
    for(int fd=3;fd<8;fd++) if(tasks[current].fd[fd]<0) return fd;
    return -1;
}
static int proc_device(int dev) {
    return (dev>=DEV_PROC_UPTIME && dev<=DEV_PROC_MEMORY) ||
           dev==DEV_PROC_DEVICES || dev==DEV_PROC_TASKS || dev==DEV_PROC_FB ||
           dev==DEV_PROC_RAMFILES ||
           (dev>=DEV_PROC_TASK_BASE && dev<DEV_PROC_TASK_BASE+OS_MAX_TASKS);
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
    int fd=free_fd();
    if(fd<0) return OS_EMFILE;
    if(!tmp_files[index].used) {
        memset(&tmp_files[index],0,sizeof(tmp_files[index]));
        strcpy(tmp_files[index].name,name);
        tmp_files[index].used=1;
    }
    if((flags & OS_O_TRUNC) && (flags & 3)!=OS_O_RDONLY) tmp_files[index].size=0;
    fd_set(&tasks[current],fd,DEV_TMP_FIRST+index,
           (flags & OS_O_APPEND)?tmp_files[index].size:0,flags,0);
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
    tasks[current].fd_pos[fd]=(uint32_t)(pos+count);
    return (int)count;
}
/* Fixed device registry shared by open() and /proc/devices listing. */
static const struct { const char *path; int id; } dev_registry[] = {
    {"/dev/console",DEV_CONSOLE}, {"/dev/gpio/fb",DEV_FB}, {"/dev/uart1",DEV_UART1}, {"/dev/rand",DEV_RAND}, {"/dev/mem",DEV_MEM}, {"/dev/trace",DEV_TRACE}
};
#define DEV_REGISTRY_COUNT (sizeof(dev_registry)/sizeof(dev_registry[0]))
/* Fixed virtual files share one path-to-ID mapping for open and FD display. */
static const struct { const char *path; int id; } vfs_registry[] = {
    {"/proc/uptime",DEV_PROC_UPTIME}, {"/proc/ticks",DEV_PROC_TICKS},
    {"/proc/interrupts",DEV_PROC_INTERRUPTS},
    {"/proc/context_switches",DEV_PROC_SWITCHES},
    {"/proc/memory",DEV_PROC_MEMORY}, {"/proc/devices",DEV_PROC_DEVICES},
    {"/proc/ramfiles",DEV_PROC_RAMFILES},
    {"/proc/tasks",DEV_PROC_TASKS}, {"/proc/fb",DEV_PROC_FB},
    {"/proc/sym/kernel",DEV_SYMS_KERNEL},
    {"/dev/pwm/1/freq",DEV_PWM_FREQ}, {"/dev/pwm/1/a",DEV_PWM_18},
    {"/dev/pwm/1/b",DEV_PWM_19}, {"/dev/adc/temp",DEV_ADC_END}
};
#define VFS_REGISTRY_COUNT (sizeof(vfs_registry)/sizeof(vfs_registry[0]))
static int fixed_path_id(const char *path) {
    for(unsigned i=0;i<DEV_REGISTRY_COUNT;i++)
        if(path_match(path,dev_registry[i].path)) return dev_registry[i].id;
    for(unsigned i=0;i<VFS_REGISTRY_COUNT;i++)
        if(path_match(path,vfs_registry[i].path)) return vfs_registry[i].id;
    return -1;
}

/* Resolve paths relative to each task, keeping device handlers absolute-only. */
static int absolute_path(const char *input, char out[128]) {
    char combined[256]; unsigned n=0, o=1;
    if(!in_task_range(input,1) || !*input) return -1;
    if(*input!='/') {
        const char *cwd=tasks[current].cwd;
        while(*cwd) { if(n>=sizeof(combined)-2) return -1; combined[n++]=*cwd++; }
        if(n==0 || combined[n-1]!='/') combined[n++]='/';
    }
    while(*input) { if(!in_task_range(input,1) || n>=sizeof(combined)-1) return -1; combined[n++]=*input++; }
    combined[n]=0; out[0]='/'; out[1]=0;
    for(unsigned i=0;i<n;) {
        while(combined[i]=='/') i++;
        if(i==n) break;
        unsigned start=i;
        while(i<n && combined[i]!='/') i++;
        unsigned len=i-start;
        if(len==1 && combined[start]=='.') continue;
        if(len==2 && combined[start]=='.' && combined[start+1]=='.') {
            if(o>1) { while(o>1 && out[o-1]!='/') o--; if(o>1) o--; }
            out[o]=0; continue;
        }
        if(o>1) { if(o>=127) return -1; out[o++]='/'; }
        if(o+len>=128) return -1;
        memcpy(out+o,combined+start,len); o+=len; out[o]=0;
    }
    return 0;
}
static int valid_directory(const char *path) {
    return !strcmp(path,"/") || !strcmp(path,"/bin") || !strcmp(path,"/tmp") ||
           !strcmp(path,"/dev") || !strcmp(path,"/dev/gpio") ||
           !strcmp(path,"/dev/adc") || !strcmp(path,"/dev/pwm") ||
           (pin_any_pwm() && !strcmp(path,"/dev/pwm/1")) || !strcmp(path,"/proc") || !strcmp(path,"/proc/task") ||
           !strcmp(path,"/proc/sym") || !strcmp(path,"/proc/sym/userland");
}
/* Directory descriptor IDs are separate from devices and regular files. */
enum { VDIR_ROOT=300, VDIR_BIN, VDIR_DEV, VDIR_GPIO, VDIR_ADC,
       VDIR_PWM, VDIR_PWM1, VDIR_PROC, VDIR_TASK, VDIR_SYM,
       VDIR_USERLAND, VDIR_TMP };
static const char *const vdir_paths[] = {
    "/", "/bin", "/dev", "/dev/gpio", "/dev/adc",
    "/dev/pwm", "/dev/pwm/1", "/proc", "/proc/task",
    "/proc/sym", "/proc/sym/userland", "/tmp"
};
static int vdir_id(const char *path) {
    for(unsigned i=0;i<sizeof(vdir_paths)/sizeof(vdir_paths[0]);i++)
        if(!strcmp(path,vdir_paths[i])) return VDIR_ROOT+(int)i;
    return -1;
}
/* Return one entry by ordinal, without allocating or keeping a directory cache. */
static int vdir_nth(int dir, unsigned nth, piconix_dirent_t *ent) {
    unsigned count=0;
#define EMIT(n,d) do { if(count++==nth) { snprintf(ent->name,sizeof(ent->name),"%s",(n)); ent->is_dir=(d); return 1; } } while(0)
    char name[32];
    switch(dir) {
    case VDIR_ROOT:
        EMIT("bin",1); EMIT("dev",1); EMIT("proc",1); EMIT("tmp",1); break;
    case VDIR_BIN: case VDIR_USERLAND:
        for(int i=1;i<PROGRAM_CAPACITY;i++)
            if(programs[i].entry && programs[i].name) { EMIT(programs[i].name,0); }
        break;
    case VDIR_TMP:
        for(int i=0;i<TMP_FILES;i++) if(tmp_files[i].used) { EMIT(tmp_files[i].name,0); }
        break;
    case VDIR_DEV:
        for(unsigned i=0;i<DEV_REGISTRY_COUNT;i++) {
            const char *path=dev_registry[i].path;
            if(!strncmp(path,"/dev/",5) && !strchr(path+5,'/')) EMIT(path+5,0);
        }
        EMIT("gpio",1); EMIT("adc",1); EMIT("pwm",1); break;
    case VDIR_GPIO:
        EMIT("fb",0);
        for(unsigned i=0;i<=28;i++) if(pin_gpio(i)) {
            snprintf(name,sizeof name,"%u",i); EMIT(name,0);
            snprintf(name,sizeof name,"%u.dir",i); EMIT(name,0);
            snprintf(name,sizeof name,"%u.pull",i); EMIT(name,0);
        }
        break;
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
        EMIT("fb",0); EMIT("task",1); EMIT("sym",1); break;
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
static int vfs_getdents(int fd, piconix_dirent_t *out, size_t n) {
    if(fd<0 || fd>=8) return OS_EBADF;
    int dir=tasks[current].fd[fd];
    if(dir<VDIR_ROOT || dir>VDIR_TMP) return OS_EBADF;
    if(n<sizeof(*out) || !in_task_range(out,sizeof(*out))) return OS_EINVAL;
    piconix_dirent_t entry={0};
    int found=vdir_nth(dir,tasks[current].fd_pos[fd],&entry);
    if(found) { memcpy(out,&entry,sizeof entry); tasks[current].fd_pos[fd]++; }
    return found;
}
/* Minimal VFS dispatch: keep the established FD IDs and backends. */
static int vfs_open(const char *p, int flags) {
    if(flags<0 || (flags & ~(OS_O_WRONLY | OS_O_RDWR | OS_O_NONBLOCK | OS_O_CREAT | OS_O_TRUNC | OS_O_APPEND)) != 0 || (flags & 3)==3) return OS_EINVAL;
    int dir=vdir_id(p);
    if(dir>=0) {
        if(flags!=OS_O_RDONLY || (dir==VDIR_PWM1 && !pin_any_pwm())) return OS_EINVAL;
        int fd=free_fd(); if(fd<0) return OS_EMFILE;
        fd_set(&tasks[current],fd,dir,0,0,0); return fd;
    }
    if(!strncmp(p,"/tmp/",5)) return tmp_open(p,flags);
    if(flags & (OS_O_CREAT | OS_O_TRUNC | OS_O_APPEND)) return OS_EINVAL;
    int dev=fixed_path_id(p);
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
    if(dev<0) {
        // Avoid unbounded parsing of an invalid pointer.
        for(int pin=0;pin<=DEV_GPIO_MAX_PIN;pin++) {
            if(!pin_gpio((unsigned)pin)) continue;
            char name[20]; snprintf(name,sizeof(name),"/dev/gpio/%d",pin);
            if(path_match(p,name)) { dev=pin+DEV_GPIO_VALUE_BASE; break; }
            snprintf(name,sizeof(name),"/dev/gpio/%d.dir",pin);
            if(path_match(p,name)) { dev=DEV_GPIO_DIR_BASE+pin; break; }
            snprintf(name,sizeof(name),"/dev/gpio/%d.pull",pin);
            if(path_match(p,name)) { dev=DEV_GPIO_PULL_BASE+pin; break; }
        }
    }
    if(dev<0 && !strncmp(p,"/dev/adc/",9) && p[9]>='0' && p[9]<='2' && p[10]==0 && pin_adc((unsigned)(p[9]-'0')))
        dev=DEV_ADC_BASE+(p[9]-'0');
    /* Fixed PWM paths remain conditional on their configured pins. */
    if((dev==DEV_PWM_FREQ && !pin_any_pwm()) ||
       (dev==DEV_PWM_18 && !pin_pwm(18)) ||
       (dev==DEV_PWM_19 && !pin_pwm(19))) dev=-1;
    if(dev<0 || (((dev>=DEV_ADC_BASE && dev<=DEV_ADC_END) ||
                   proc_device(dev) || dev==DEV_RAND || dev==DEV_MEM || dev==DEV_SYMS_KERNEL ||
                   (dev>DEV_SYMS_USER_BASE && dev<DEV_SYMS_USER_BASE+PROGRAM_CAPACITY) || dev==DEV_TRACE) &&
                  (flags & 3)!=OS_O_RDONLY)) return OS_EINVAL;
    if((flags & OS_O_NONBLOCK) && dev!=DEV_UART1 && dev!=DEV_CONSOLE) return OS_EINVAL;
    int fd=free_fd();
    if(fd<0) return OS_EMFILE;
    fd_set(&tasks[current],fd,dev,
           dev==DEV_TRACE ? trace_serial-trace_count : 0,flags,
           (dev>=DEV_PROC_TASK_BASE && dev<DEV_PROC_TASK_BASE+OS_MAX_TASKS)
           ? tasks[dev-DEV_PROC_TASK_BASE].pid : 0);
    if(dev>=DEV_ADC_BASE && dev<=DEV_ADC_END) {
        tasks[current].adc_values[fd]=(uint16_t)adc_driver_read(dev==DEV_ADC_END ? 4u : (unsigned)(dev-DEV_ADC_BASE));
    }
    return fd;
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
/* Copy one generated /proc line, respecting the per-open read cursor. */
static int proc_emit(int fd, void *dst, size_t limit, size_t *offset,
                     size_t *done, const char *line, size_t len) {
    size_t pos=tasks[current].fd_pos[fd];
    if(pos < *offset+len && *done<limit) {
        size_t start=pos>*offset?pos-*offset:0;
        size_t take=len-start;
        if(take>limit-*done) take=limit-*done;
        memcpy((char *)dst+*done,line+start,take);
        *done+=take;
        tasks[current].fd_pos[fd]+=(uint32_t)take;
    }
    *offset+=len;
    return *done==limit;
}
/* Generate one task line at a time; no whole-report kernel buffer. */
static int proc_tasks_read(int fd, void *p, size_t n) {
    static const char header[] = "PID  NAME              STATE    TTY      SLOT  MEMORY             SP\n";
    size_t offset=0, done=0;
    if(proc_emit(fd,p,n,&offset,&done,header,sizeof(header)-1)) return (int)done;
    for(int row=0;row<OS_MAX_TASKS;row++) {
        task_t *t=&tasks[row];
        if(t->state==UNUSED || t->state==DEAD) continue;
        const char *state=t->state==READY?"READY":
            t->state==SLEEPING?"SLEEP":t->state==BLOCKED?"BLOCKED":"?";
        uintptr_t base=(uintptr_t)task_ram[row];
        char line[128];
        int len=snprintf(line,sizeof line,"%3d  %-18.18s%-9s%-9s%d     %08lX-%08lX  %08lX\n",
            t->pid,t->name?t->name:"",state,t->tty==0?"console":t->tty==1?"uart1":"-",row+1,
            (unsigned long)base,(unsigned long)(base+OS_TASK_BYTES-1u),
            (unsigned long)(uintptr_t)t->sp);
        if(len<0 || (size_t)len>=sizeof line) return OS_EINVAL;
        if(proc_emit(fd,p,n,&offset,&done,line,(size_t)len)) break;
    }
    return (int)done;
}
static int proc_fb_read(int fd, void *dst, size_t n) {
    uint8_t pixels[8];
    int enabled=fb_active();
    /* Read the actual framebuffer; no duplicate state. */
    if(fb_rw(pixels,sizeof pixels,0)<0) return OS_EINVAL;
    char line[80]; size_t offset=0,done=0;
    int len=snprintf(line,sizeof line,"Framebuffer: 8x8\nEnabled: %s\n",
                     enabled?"yes":"no");
    if(len<0 || (size_t)len>=sizeof line) return OS_EINVAL;
    if(proc_emit(fd,dst,n,&offset,&done,line,(size_t)len)) return (int)done;
    for(int y=0;y<8;y++) {
        for(int x=0;x<8;x++) line[x]=(pixels[y] & (1u<<(7-x)))?'#':'.';
        line[8]='\n';
        if(proc_emit(fd,dst,n,&offset,&done,line,9)) break;
    }
    return (int)done;
}
/* Resolve an open descriptor from the kernel's authoritative device IDs. */
static const char *fd_path(int dev, int task_pid, char *buf, size_t size) {
    if(dev>=VDIR_ROOT && dev<=VDIR_TMP) return vdir_paths[dev-VDIR_ROOT];
    for(unsigned i=0;i<DEV_REGISTRY_COUNT;i++)
        if(dev_registry[i].id==dev) return dev_registry[i].path;
    if(dev>=DEV_TMP_FIRST && dev<DEV_TMP_FIRST+TMP_FILES) {
        int index=dev-DEV_TMP_FIRST;
        if(tmp_files[index].used) {
            snprintf(buf,size,"/tmp/%s",tmp_files[index].name);
            return buf;
        }
    }
    if(dev>=DEV_PROC_TASK_BASE && dev<DEV_PROC_TASK_BASE+OS_MAX_TASKS) {
        snprintf(buf,size,"/proc/task/%d",task_pid);
        return buf;
    }
    for(unsigned i=0;i<VFS_REGISTRY_COUNT;i++)
        if(vfs_registry[i].id==dev) return vfs_registry[i].path;
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
static int proc_task_read(int fd, int slot, void *dst, size_t n) {
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
    /* Running task's live registers are not captured in its saved frame. */
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
    for(int i=0;i<8;i++) if(t->fd[i]>=0) {
        char path[48];
        const char *name=fd_path(t->fd[i],t->fd_task_pid[i],path,sizeof path);
        PROC_LINE("  fd %d: device=%d flags=%u pos=%lu name=%s\n",i,t->fd[i],
                  (unsigned)t->fd_flags[i],(unsigned long)t->fd_pos[i],name);
    }
#undef PROC_LINE
    return (int)done;
}
/* Live /tmp data-region addresses, exposed for the mem viewer.  This is
 * metadata only; the contents remain accessible through /dev/mem. */
static int proc_ramfiles_read(int fd, void *p, size_t n) {
    size_t pos=tasks[current].fd_pos[fd], done=0, skip=0;
    for(int i=0;i<TMP_FILES;i++) if(tmp_files[i].used) {
        char line[80];
        int len=snprintf(line,sizeof line,"%08lX %s\n",
                         (unsigned long)(uintptr_t)tmp_files[i].data,
                         tmp_files[i].name);
        if(len<=0 || (size_t)len>=sizeof line) return OS_EINVAL;
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
static int device_rw_proc(int fd, int dev, void *p, size_t n, int writing) {
    if(writing) return OS_EBADF;
    if(dev==DEV_PROC_TASKS) return proc_tasks_read(fd,p,n);
    if(dev==DEV_PROC_RAMFILES) return proc_ramfiles_read(fd,p,n);
    if(dev==DEV_PROC_FB) return proc_fb_read(fd,p,n);
    if(dev>=DEV_PROC_TASK_BASE && dev<DEV_PROC_TASK_BASE+OS_MAX_TASKS)
        return proc_task_read(fd,dev-DEV_PROC_TASK_BASE,p,n);
    // Snapshot the counters before formatting; no extra interrupts or tasks.
    uint32_t irq_state=save_and_disable_interrupts();
    uint32_t t=ticks, svc=svc_count, psv=pendsv_count, switches=context_switch_count;
    uint32_t uart1_irqs=uart1_irq_count;
#if !PICONIX_CONSOLE_USB
    uint32_t console_irqs=console_irq_count;
#endif
    restore_interrupts(irq_state);
    char text[160]; int len=0;
    if(dev==DEV_PROC_DEVICES) {
        /* Enumerate the same registry used by vfs_open(). */
        for(unsigned i=0;i<DEV_REGISTRY_COUNT;i++) {
            const char *name=dev_registry[i].path+5; /* skip /dev/ */
            int written=snprintf(text+len,sizeof(text)-(size_t)len,"%s\n",name);
            if(written<0 || (size_t)written>=sizeof(text)-(size_t)len) return OS_EINVAL;
            len+=written;
        }
        /* GPIO is a dynamically generated subtree, not a fixed device. */
        int written=snprintf(text+len,sizeof(text)-(size_t)len,"gpio/\nadc/\npwm/\n");
        if(written<0 || (size_t)written>=sizeof(text)-(size_t)len) return OS_EINVAL;
        len+=written;
    }
    switch(dev) {
        case DEV_PROC_UPTIME: len=snprintf(text,sizeof text,"%lu.%03lu s\n",(unsigned long)(t/1000u),(unsigned long)(t%1000u)); break;
        case DEV_PROC_TICKS: len=snprintf(text,sizeof text,"%lu\n",(unsigned long)t); break;
        case DEV_PROC_INTERRUPTS:
#if PICONIX_CONSOLE_USB
            len=snprintf(text,sizeof text,"SysTick %lu\nPendSV %lu\nSVC %lu\nuart1 %lu\n",
                         (unsigned long)t,(unsigned long)psv,(unsigned long)svc,(unsigned long)uart1_irqs);
#else
            len=snprintf(text,sizeof text,"SysTick %lu\nPendSV %lu\nSVC %lu\nconsole %lu\nuart1 %lu\n",
                         (unsigned long)t,(unsigned long)psv,(unsigned long)svc,
                         (unsigned long)console_irqs,(unsigned long)uart1_irqs);
#endif
            break;
        case DEV_PROC_SWITCHES: len=snprintf(text,sizeof text,"%lu\n",(unsigned long)switches); break;
        case DEV_PROC_MEMORY: len=snprintf(text,sizeof text,"kernel 20000000-20017FFF\nslots  20018000-2003FFFF (5 x 32 KiB)\n"); break;
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
    tasks[current].fd_pos[fd]=(uint32_t)(pos+count);
    return (int)count;
}
static int device_rw_tty(int fd, int dev, void *p, size_t n, int writing) {
    int uartdev=(dev==DEV_CONSOLE)?0:1;
#if !PICONIX_CONSOLE_USB
    uart_inst_t *u=(dev==DEV_CONSOLE)?uart0:uart1;
#endif
    uint8_t *b=p;
    if(writing) {
        if(tty_claim_pid[uartdev] && tty_claim_pid[uartdev]!=tasks[current].pid) return OS_EIO;
#if PICONIX_CONSOLE_USB
        if(dev==DEV_CONSOLE && !stdio_usb_connected()) return (int)n;
#endif
        size_t i=0;
        for(;i<n;i++) {
            uint8_t c=b[i];
            int insert_cr=tty_onlcr[uartdev] && c=='\n' && !tty_last_cr[uartdev];
#if PICONIX_CONSOLE_USB
            if(dev==DEV_CONSOLE) {
                /* Reserve both bytes before accepting an LF. */
                unsigned used=(console_tx_head+CONSOLE_TX_CAP-console_tx_tail)%CONSOLE_TX_CAP;
                if(CONSOLE_TX_CAP-1u-used < (unsigned)(1+insert_cr)) break;
                if(insert_cr) tty_put('\r');
                tty_put(c);
            } else
#endif
            {
                if(insert_cr) tty_put_dev(uartdev,'\r');
                tty_put_dev(uartdev,c);
            }
            tty_last_cr[uartdev]=(c=='\r');
        }
        return (int)i;
    }
    if(n==0) return 0;
    if(tty_owner_pid(uartdev) && tasks[current].pid!=tty_owner_pid(uartdev)) return OS_EIO;
    uint32_t irq_state=save_and_disable_interrupts();
    int count=0;
    if(uartdev==DEV_CONSOLE && tty_buf[0].used) {
        count=tty_dequeue(0,b,n);
        restore_interrupts(irq_state);
        return count; /* Zero is EOF from Ctrl+D on an empty line. */
    } else if(uartdev==1 && (tty_owner_pid(1) ? tty_buf[1].used : rx[1].head!=rx[1].tail)) {
        count=uart1_read(b,n);
        restore_interrupts(irq_state);
        return count; /* Zero means terminal Ctrl+D, not no data. */
    }
    if(count) { restore_interrupts(irq_state); return count; }
    if(tasks[current].fd_flags[fd] & OS_O_NONBLOCK) {
        restore_interrupts(irq_state); return 0;
    }
    task_t *t=&tasks[current];
    t->wait_dev=uartdev;t->wait_fd=fd;t->wait_buf=p;t->wait_size=n;
    t->wait_frame_ready=0;
    t->state=BLOCKED;
    pend();
    restore_interrupts(irq_state);
    return 0; // completed later by IRQ; SVC frame initially holds 0
}
static int device_rw_pwm(int fd, int dev, void *p, size_t n, int writing) {
    if(!n) return 0;
    unsigned which=(dev==DEV_PWM_FREQ) ? 0u : (unsigned)(dev-DEV_PWM_18+1);
    if(writing) {
        if(n>12) return OS_EINVAL;
        unsigned x=0; size_t i=0;
        const char *b=p;
        while(i<n && b[i]>='0' && b[i]<='9') {
            x=x*10u+(unsigned)(b[i]-'0');
            if(x>1000000u) return OS_EINVAL;
            i++;
        }
        if(i==0 || (i<n && !(i+1==n && b[i]=='\n'))) return OS_EINVAL;
        /* Integer divider + 16-bit counter at RP2040 default clock. */
        if(dev==DEV_PWM_FREQ) {
            if(x<8 || x>100000) return OS_EINVAL;
        } else if(x>100) return OS_EINVAL;
        if(pwm_driver_set(which,x)<0) return OS_EINVAL;
        return (int)n;
    }
    char b[16]; int len=snprintf(b,sizeof b,"%u\n",pwm_driver_get(which));
    size_t pos=tasks[current].fd_pos[fd];
    if(pos>=(size_t)len) return 0;
    size_t count=(size_t)len-pos;
    if(count>n) count=n;
    memcpy(p,b+pos,count);
    tasks[current].fd_pos[fd]+=(uint32_t)count;
    return (int)count;
}
/* A fresh ADC sample is obtained on open/read; subsequent reads return EOF.
 * Use a regular read cursor so cat() terminates and watch can reopen. */
static int device_rw_adc(int fd, int dev, void *p, size_t n, int writing) {
    if(writing || (tasks[current].fd_flags[fd] & 3)!=OS_O_RDONLY) return OS_EBADF;
    if(!n) return 0;
    unsigned raw=tasks[current].adc_values[fd];
    char text[20];
    int len=adc_driver_format(raw,dev==DEV_ADC_END,text,sizeof text);
    size_t pos=tasks[current].fd_pos[fd];
    if(pos>=(size_t)len) return 0;
    size_t count=(size_t)len-pos;
    if(count>n) count=n;
    memcpy(p,text+pos,count);
    tasks[current].fd_pos[fd]=(uint32_t)(pos+count);
    return (int)count;
}
extern const unsigned char piconix_symbols_start[], piconix_symbols_end[];
extern const unsigned char piconix_kernel_syms_start[], piconix_kernel_syms_end[];
static int vfs_rw(int fd, void *p, size_t n, int writing) {
    if(fd<0 || fd>=8 || tasks[current].fd[fd]<0) return OS_EBADF;
    if(n>0 && !in_task_range(p,n)) return OS_EINVAL;
    int dev=tasks[current].fd[fd];
    if(dev>=VDIR_ROOT && dev<=VDIR_TMP) return OS_EBADF;
    if(dev>=DEV_TMP_FIRST && dev<DEV_TMP_FIRST+TMP_FILES) return tmp_rw(fd,p,n,writing);
    if(dev==DEV_TRACE) {
        if(writing) return OS_EBADF;
        if(n < sizeof(trace_event_t)) return n ? OS_EINVAL : 0;
        uint32_t irq=save_and_disable_interrupts();
        uint32_t pos=tasks[current].fd_pos[fd];
        uint32_t oldest_serial=trace_serial-trace_count;
        if((int32_t)(pos-oldest_serial)<0) pos=oldest_serial;
        uint32_t available=trace_serial-pos;
        uint32_t count=(uint32_t)(n/sizeof(trace_event_t));
        if(count>available) count=available;
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
        if(writing || (tasks[current].fd_flags[fd] & 3)!=OS_O_RDONLY) return OS_EBADF;
        uint32_t pos=tasks[current].fd_pos[fd];
        const unsigned char *start, *end;
        if(dev==DEV_SYMS_KERNEL) {
            size_t reserved=(size_t)(piconix_kernel_syms_end-piconix_kernel_syms_start);
            uint32_t length=0;
            if(reserved>=4u) memcpy(&length,piconix_kernel_syms_start,4u);
            if(length>reserved-4u) length=0;
            start=piconix_kernel_syms_start+4u;
            end=start+length;
            if(pos>=(size_t)(end-start)) return 0;
            if(n>(size_t)(end-start)-pos) n=(size_t)(end-start)-pos;
            memcpy(p,start+pos,n);
            tasks[current].fd_pos[fd]=pos+(uint32_t)n;
            return (int)n;
        }
        /* Stream only the requested program's catalog records, without RAM copies. */
        const char *name=programs[dev-DEV_SYMS_USER_BASE].name;
        if(!name) return OS_ENOENT;
        size_t name_len=strlen(name), skip=pos, done=0;
        start=piconix_symbols_start; end=piconix_symbols_end;
        while(start<end && done<n) {
            const unsigned char *line=start;
            while(start<end && *start!='\n') start++;
            if(start<end) start++;
            size_t len=(size_t)(start-line);
            if(len>name_len && !memcmp(line,name,name_len) && line[name_len]=='\t') {
                if(skip>=len) skip-=len;
                else {
                    size_t count=len-skip;
                    if(count>n-done) count=n-done;
                    memcpy((unsigned char *)p+done,line+skip,count);
                    done+=count;
                    skip=0;
                }
            }
        }
        tasks[current].fd_pos[fd]=pos+(uint32_t)done;
        return (int)done;
    }
    if(dev==DEV_MEM) {
        if(writing || (tasks[current].fd_flags[fd] & 3)!=OS_O_RDONLY) return OS_EBADF;
        if(!n) return 0;
        uint32_t addr=tasks[current].fd_pos[fd];
        uint32_t end=0;
        /* Read only Pico's 2-MiB XIP flash and 256-KiB main SRAM.
         * Avoid peripherals, XIP control registers and scratch banks. */
        if(addr>=0x10000000u && addr<0x10200000u) end=0x10200000u;
        if(addr>=0x20000000u && addr<0x20040000u) end=0x20040000u;
        if(!end) return OS_EINVAL;
        size_t count=n;
        if(count>end-addr) count=end-addr;
        memmove(p,(const void *)(uintptr_t)addr,count);
        tasks[current].fd_pos[fd]=addr+(uint32_t)count;
        return (int)count;
    }
    if(dev==DEV_FB) return fb_rw(p,n,writing);
    if(dev>=DEV_ADC_BASE && dev<=DEV_ADC_END) return device_rw_adc(fd,dev,p,n,writing);
    if(dev>=DEV_PWM_FREQ && dev<=DEV_PWM_19) return device_rw_pwm(fd,dev,p,n,writing);
    if(dev==DEV_RAND) return device_rw_rand(p,n,writing);
    if(proc_device(dev)) return device_rw_proc(fd,dev,p,n,writing);
    if(dev==DEV_CONSOLE || dev==DEV_UART1) return device_rw_tty(fd,dev,p,n,writing);
    if(dev>=DEV_GPIO_DIR_BASE && dev<=DEV_GPIO_DIR_END)
        return gpio_driver_rw(1,(unsigned)(dev-DEV_GPIO_DIR_BASE),p,n,writing);
    if(dev>=DEV_GPIO_PULL_BASE && dev<=DEV_GPIO_PULL_END)
        return gpio_driver_rw(2,(unsigned)(dev-DEV_GPIO_PULL_BASE),p,n,writing);
    return gpio_driver_rw(0,(unsigned)(dev-DEV_GPIO_VALUE_BASE),p,n,writing);
}
static int device_ioctl(int fd,int cmd,int arg) {
    if(fd<0 || fd>=8 || tasks[current].fd[fd]<0) return OS_EBADF;
    int dev=tasks[current].fd[fd];
    if(cmd>=OS_TTY_GET_CANON && cmd<=OS_TTY_RELEASE) {
        if(dev!=DEV_CONSOLE && dev!=DEV_UART1)return OS_EINVAL;
        int tty=(dev==DEV_CONSOLE)?0:1;
        if(cmd==OS_TTY_CLAIM) {
            if(tty_owner_pid(tty)) return OS_EBUSY;
            /* A background user with an open descriptor also makes it busy. */
            int id=tty ? DEV_UART1 : DEV_CONSOLE;
            for(int i=0;i<OS_MAX_TASKS;i++) {
                if(i==current || tasks[i].state==UNUSED || tasks[i].state==DEAD) continue;
                for(int f=0;f<8;f++) if(tasks[i].fd[f]==id) return OS_EBUSY;
            }
            tty_claim_pid[tty]=tasks[current].pid;
            tty_mode_reset(tty);
            /* Discard input received before the terminal was claimed. */
            rx[tty].tail=rx[tty].head;
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
            /* stty/reset intentionally changes the terminal beyond its lifetime. */
            if(tty_restore_pid[tty]==tasks[current].pid) tty_restore_pid[tty]=0;
            return 0;
        }
        if(cmd==OS_TTY_GET_CANON)return tty_canonical[tty];
        if(cmd==OS_TTY_GET_ECHO)return tty_echo[tty];
        if(cmd==OS_TTY_GET_ONLCR)return tty_onlcr[tty];
        if(arg!=0 && arg!=1)return OS_EINVAL;
        if(cmd==OS_TTY_SET_CANON && tty_canonical[tty]!=arg){
            memset(&tty_buf[tty],0,sizeof tty_buf[tty]);
            tty_canonical[tty]=(uint8_t)arg;
        }
        if(cmd==OS_TTY_SET_ECHO)tty_echo[tty]=(uint8_t)arg;
        if(cmd==OS_TTY_SET_ONLCR)tty_onlcr[tty]=(uint8_t)arg;
        tty_mode_owner[tty]=tasks[current].pid;
        return 0;
    }
    if(tasks[current].fd[fd]==DEV_FB) return fb_ioctl(cmd);
    if(tasks[current].fd[fd]==DEV_TRACE) {
        uint32_t irq=save_and_disable_interrupts();
        if(cmd==TRACE_ON) {
            trace_enabled=0;
            trace_head=trace_count=trace_serial=0;
            trace_enabled=1;
        } else if(cmd==TRACE_OFF) trace_enabled=0;
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
    /* Query any GP0..28 through an existing GPIO descriptor, even if fb owns it.
     * Return compact mode/level/pull bits; no additional device or state cache. */
    if(cmd>=OS_GPIO_STATUS_BASE && cmd<OS_GPIO_STATUS_BASE+29) {
        int dev=tasks[current].fd[fd];
        if(dev<DEV_GPIO_VALUE_BASE || dev>DEV_GPIO_VALUE_END) return OS_EBADF;
        unsigned pin=(unsigned)(cmd-OS_GPIO_STATUS_BASE);
        unsigned mode=GPIO_STATUS_OTHER;
        unsigned level=0, pull=0;
        if(pin<16 && fb_active()) mode=GPIO_STATUS_FB;
        else {
            gpio_function_t fn=gpio_get_function(pin);
            if(fn==GPIO_FUNC_PWM) mode=GPIO_STATUS_PWM;
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
    if(tasks[current].fd[fd]<DEV_GPIO_VALUE_BASE || tasks[current].fd[fd]>DEV_GPIO_VALUE_END) return OS_EBADF;
    int pin=tasks[current].fd[fd]-DEV_GPIO_VALUE_BASE;
    if(!pin_gpio(pin)) return OS_EINVAL;
    (void)arg;
    return gpio_driver_ioctl((unsigned)pin,cmd);
}
// SVC receives a pointer to the hardware-stacked r0-r3,r12,lr,pc,xPSR.
void os_svc_dispatch(uint32_t *f) {
    int num=(int)f[0], a=(int)f[1], b=(int)f[2], c=(int)f[3];
    int result=OS_ENOSYS;
    svc_count++;
    if(current<0) { f[0]=OS_EINVAL; return; }
    /* Snapshot the file ID before close() releases its descriptor. */
    int8_t file_id = -1;
    if((num==SYS_READ || num==SYS_WRITE || num==SYS_CLOSE || num==SYS_IOCTL || num==SYS_LSEEK || num==SYS_GETDENTS) &&
       a>=0 && a<8 && tasks[current].fd[a]>=0 && tasks[current].fd[a]<=127)
        file_id = (int8_t)tasks[current].fd[a];
    trace_record(TRACE_SVC_ENTER, (uint8_t)tasks[current].pid, (uint32_t)num,
                 (uint32_t)a, tasks[current].program_id, file_id, NULL, 0);
    switch(num) {
        case SYS_GETPID: result=tasks[current].pid; break;
        case SYS_GET_TTY: result=tasks[current].tty; break;
        case SYS_KILL:
            {int slot=slot_from_pid(a);
            if(slot<0 || slot==current || slot==IDLE_SLOT) result=OS_ESRCH;
            else {finish_task(slot,128+15);result=0;}}
            break;
        case SYS_EXIT: if(current!=IDLE_SLOT) { finish_task(current,a); pend(); } result=0; break;
        case SYS_SLEEP: if(current!=IDLE_SLOT && a>0) { tasks[current].wake=ticks+(uint32_t)a; tasks[current].state=SLEEPING; pend(); } result=0; break;
        case SYS_YIELD: pend(); result=0; break;
        case SYS_WAITPID: {
            /* c: WNOHANG (1). Wait without polling; the child slot is
               freed immediately and only a completion PID is retained. */
            int *status=(int *)(uintptr_t)b;
            if((c & ~1) || (a<1 && a!=-1) ||
               (status && !in_task_range(status,sizeof(*status)))) {
                result=OS_EINVAL; break;
            }
            int pid=take_exit(tasks[current].pid,a);
            if(pid) {
                if(status) *status=0;
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
            result=0; /* Real return value is filled after child exit. */
            break;
        }
        case SYS_SET_FG: {
            int tty=tasks[current].tty;
            if(a==0) { foreground_pid[tty]=0; result=0; break; }
            result=OS_ESRCH;
            for(int i=0;i<OS_MAX_TASKS;i++) if(tasks[i].pid==a &&
                tasks[i].parent_pid==tasks[current].pid && tasks[i].state!=DEAD) {
                if(foreground_pid[tty]!=a && !tty_restore_pid[tty]) {
                    tty_saved_canon[tty]=tty_canonical[tty];
                    tty_saved_echo[tty]=tty_echo[tty];
                    tty_saved_onlcr[tty]=tty_onlcr[tty];
                    tty_saved_owner[tty]=tty_mode_owner[tty];
                    tty_restore_pid[tty]=a;
                }
                foreground_pid[tty]=a; result=0; break;
            }
            break;
        }
        case SYS_TTY_SPAWN: {
            /* getty: attach a fresh task to a different serial terminal. */
            if(a!=1 && a!=2) { result=OS_EINVAL; break; }
            int tty=(int)a-1;
            if(tty_owner_pid(tty)) { result=OS_EBUSY; break; }
            if(b!=2 && b!=4) { result=OS_EINVAL; break; }
            uint32_t irq_state=save_and_disable_interrupts();
            int slot=free_program_slot();
            result=slot<0 ? OS_EAGAIN : spawn_in_slot(slot,b,programs[b].entry,programs[b].name,0,NULL,NULL,0);
            if(result>0) {
                task_t *child=&tasks[slot];
                if(tty_shell_pid[0]==child->pid) tty_shell_pid[0]=0;
                child->tty=(uint8_t)tty;
                for(int j=0;j<3;j++) fd_set(child,j,tty?30:0,0,0,0);
                if(b==2) tty_shell_pid[tty]=child->pid;
                else foreground_pid[tty]=child->pid;
            }
            restore_interrupts(irq_state);
            break;
        }
        case SYS_SPAWN_TMP: {
            /* A tiny flat Thumb image: PNX1, LE16 code length, LE16 reserved. */
            const char *path=(const char *)(uintptr_t)a;
            char filename[16];
            if(!in_task_range(path,21) || !tmp_name(path,filename)) { result=OS_EINVAL; break; }
            int f=tmp_lookup(filename);
            if(f<0) { result=OS_ENOENT; break; }
            const uint8_t *image=tmp_files[f].data;
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
            break;
        }
        case SYS_SPAWN: {
            if(a<1 || a>=PROGRAM_CAPACITY || !programs[a].entry) { result=OS_ENOENT; break; }
            int argc=(int)b;
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
                while(len<128-total && in_task_range(arg+len,1) && arg[len]) len++;
                if(len==128-total || !in_task_range(arg+len,1)) {
                    result=OS_EINVAL; break;
                }
                total+=len+1;
            }
            if(result<0) break;
            int slot=free_program_slot();
            result=slot<0 ? OS_EAGAIN : spawn_in_slot(slot,a,programs[a].entry,programs[a].name,argc,argv,NULL,0);
            break;
        }
        case SYS_CONSOLE_READY:
#if PICONIX_CONSOLE_USB
            result=tasks[current].tty ? 1 : (stdio_usb_connected() ? 1 : 0);
#else
            result=1;
#endif
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
        case SYS_DUP2:
            if(a<0 || a>=8 || b<0 || b>=8 || tasks[current].fd[a]<0) result=OS_EBADF;
            else {
                fd_set(&tasks[current],b,tasks[current].fd[a],
                       tasks[current].fd_pos[a],tasks[current].fd_flags[a],
                       tasks[current].fd_task_pid[a]);
                result=b;
            }
            break;
        case SYS_OPEN: {
            char path[128];
            result=absolute_path((const char *)(uintptr_t)a,path)<0 ? OS_EINVAL : vfs_open(path,b);
            break;
        }
        case SYS_CLOSE: if(a<3 || a>=8 || tasks[current].fd[a]<0) result=OS_EBADF;
            else {fd_set(&tasks[current],a,-1,0,0,0); result=0;} break;
        case SYS_GETDENTS: result=vfs_getdents(a,(piconix_dirent_t *)(uintptr_t)b,(size_t)c); break;
        case SYS_READ: result=vfs_rw(a,(void *)(uintptr_t)b,(size_t)c,0); break;
        case SYS_WRITE: result=vfs_rw(a,(void *)(uintptr_t)b,(size_t)c,1); break;
        case SYS_IOCTL: result=device_ioctl(a,b,c); break;
        case SYS_FCNTL:
            if(a<0 || a>=8 || tasks[current].fd[a]<0) result=OS_EBADF;
            else if(b==1) result=tasks[current].fd_flags[a]; /* F_GETFL */
            else if(b==2) { /* F_SETFL: only O_NONBLOCK is mutable */
                if(c & ~(OS_O_NONBLOCK | OS_O_RDWR | OS_O_WRONLY | OS_O_CREAT | OS_O_TRUNC | OS_O_APPEND)) result=OS_EINVAL;
                else if(tasks[current].fd[a]!=DEV_CONSOLE && tasks[current].fd[a]!=DEV_UART1) result=OS_EINVAL;
                else {
                    tasks[current].fd_flags[a]=(tasks[current].fd_flags[a] & ~OS_O_NONBLOCK) | (c & OS_O_NONBLOCK);
                    result=0;
                }
            } else result=OS_EINVAL;
            break;
        case SYS_LSEEK:
            if(a<0 || a>=8 || tasks[current].fd[a]<0) result=OS_EBADF;
            else if(c!=0) result=OS_EINVAL; /* SEEK_SET only */
            else if(tasks[current].fd[a]==DEV_MEM) {
                uint32_t pos=(uint32_t)b;
                if((pos>=0x10000000u && pos<0x10200000u) ||
                   (pos>=0x20000000u && pos<0x20040000u)) {
                    tasks[current].fd_pos[a]=pos;
                    result=b;
                } else result=OS_EINVAL;
            } else if(tasks[current].fd[a]>=DEV_TMP_FIRST &&
                      tasks[current].fd[a]<DEV_TMP_FIRST+TMP_FILES && b>=0 && b<=TMP_BYTES) {
                tasks[current].fd_pos[a]=(uint32_t)b; result=b;
            } else result=OS_EINVAL;
            break;
    }
    if(num==SYS_OPEN && result>=0 && result<8 && tasks[current].fd[result]<=127)
        file_id=(int8_t)tasks[current].fd[result];
    /* Only successful operations may expose user data. /dev/trace is excluded
       so the trace reader cannot recursively record its own binary records. */
    const void *preview=NULL;
    unsigned preview_len=0;
    if(num==SYS_OPEN && result>=0) {
        const char *path=(const char *)(uintptr_t)a;
        static const unsigned max_name=16;
        while(preview_len<max_name && in_task_range(path+preview_len,1) &&
              path[preview_len]) preview_len++;
        if(in_task_range(path,preview_len)) preview=path;
    } else if((num==SYS_READ || num==SYS_WRITE) && result>0 && file_id!=99 &&
              in_task_range((const void *)(uintptr_t)b,(unsigned)result)) {
        preview=(const void *)(uintptr_t)b;
        preview_len=(unsigned)result;
    }
    /* A blocking terminal read finishes asynchronously in wake_readers(). */
    if(!(num==SYS_READ && tasks[current].state==BLOCKED) &&
       !(num==SYS_WAITPID && tasks[current].state==BLOCKED))
        trace_record(TRACE_SVC_EXIT, (uint8_t)tasks[current].pid, (uint32_t)num,
                     (uint32_t)result, tasks[current].program_id, file_id, preview, preview_len);
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
    return call(SYS_WAITPID,pid,(uintptr_t)status,(uintptr_t)options);
}
int os_set_foreground(int pid) { return call(SYS_SET_FG,pid,0,0); }
int os_spawn_program(int program_id) { return call(SYS_SPAWN,program_id,0,0); }
int os_getpid(void) { return call(SYS_GETPID,0,0,0); }
int os_get_tty(void) { return call(SYS_GET_TTY,0,0,0); }
int os_kill(int pid) { return call(SYS_KILL,pid,0,0); }
void os_exit(void) { call(SYS_EXIT,0,0,0); for(;;) __wfi(); }
void os_msleep(uint32_t ms) { call(SYS_SLEEP,ms,0,0); }
void os_yield(void) { call(SYS_YIELD,0,0,0); }
int os_open(const char *p,int f) { return call(SYS_OPEN,(uintptr_t)p,f,0); }
int os_close(int fd) { return call(SYS_CLOSE,fd,0,0); }
int os_read(int fd,void *p,size_t n) { return call(SYS_READ,fd,(uintptr_t)p,n); }
int os_write(int fd,const void *p,size_t n) { return call(SYS_WRITE,fd,(uintptr_t)p,n); }
int os_ioctl(int fd,int cmd,int v) { return call(SYS_IOCTL,fd,cmd,v); }
int os_fcntl(int fd,int cmd,int value) { return call(SYS_FCNTL,fd,cmd,value); }

int os_dup2(int oldfd,int newfd) { return call(SYS_DUP2,oldfd,newfd,0); }
int os_unlink(const char *p) { return call(SYS_UNLINK,(uintptr_t)p,0,0); }
