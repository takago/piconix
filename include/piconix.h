#ifndef PICONIX_H
#define PICONIX_H
#include <stdint.h>
#include <stddef.h>
#define OS_MAX_TASKS 3
#define OS_TASK_BYTES (64u * 1024u)
#define OS_O_RDONLY 0
#define OS_O_WRONLY 1
#define OS_O_RDWR 2
#define OS_O_NONBLOCK 4
#define OS_GPIO_INPUT 1
#define OS_GPIO_OUTPUT 2
#define OS_GPIO_PULL_UP 3
#define OS_GPIO_PULL_DOWN 4
#define OS_GPIO_NO_PULL 5
#define OS_WNOHANG 1
#define OS_EINVAL (-22)
#define OS_EBADF (-9)
#define OS_EAGAIN (-11)
#define OS_ENOSYS (-38)
#define OS_ESRCH (-3)
#define OS_EMFILE (-24)
typedef void (*os_entry_t)(void);
#define OS_TASK_UNUSED 0
#define OS_TASK_READY 1
#define OS_TASK_SLEEPING 2
#define OS_TASK_DEAD 3
#define OS_TASK_BLOCKED 4
#define OS_EINTR (-4)
#define OS_EIO (-5)
#define OS_ENOENT (-2)
typedef struct {
    int pid;
    int state;
    char name[16];
    uint32_t slot_base, slot_end, entry, saved_sp;
} os_task_info_t;

void os_init(void);
// Poll SDK USB stdio input from the idle task; UART builds are a no-op.
void os_console_poll(void);
void os_init_idle(os_entry_t entry);
int os_spawn(os_entry_t entry);
int os_spawn_named(os_entry_t entry, const char *name);
int os_run(int program_id);
int os_start_program(int program_id); // boot-time launch before scheduler starts
void os_register_program(int program_id, os_entry_t entry, const char *name);
// Experimental: copy a slot-linked image from flash into the assigned RAM slot.
// Does NOT itself change the linked entry point or enable RAM execution.
int os_register_ram_image(int program_id, const void *image, size_t size,
                          size_t zero_start, size_t zero_size);
int os_task_info(int pid, os_task_info_t *info);
void os_start(void) __attribute__((noreturn));
int os_getpid(void);
int os_kill(int pid);
int os_waitpid(int pid, int *status, int options);
int os_set_foreground(int pid);
void os_exit(void) __attribute__((noreturn));
void os_sleep(uint32_t milliseconds);
void os_yield(void);
int os_open(const char *path, int flags);
int os_close(int fd);
int os_read(int fd, void *buf, size_t n);
int os_write(int fd, const void *buf, size_t n);
int os_ioctl(int fd, int cmd, int value);
#endif
