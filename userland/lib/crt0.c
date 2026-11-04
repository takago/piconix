/* Common freestanding entry point. Kernel passes argc and argv in r0/r1. */
#include "unistd.h"
#include "stdio.h"
extern int main(int argc, char *argv[]);
__attribute__((used, noreturn, section(".text.app_entry")))
void app_entry(int argc, char *argv[]) {
    int status=main(argc, argv);
    fflush(0);
    _exit(status);
}
