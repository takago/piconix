/* Common freestanding entry point. Kernel passes argc and argv in r0/r1. */
#include "unistd.h"
extern int main(int argc, char *argv[]);
__attribute__((used, noreturn, section(".text.app_entry")))
void app_entry(int argc, char *argv[]) {
    _exit(main(argc, argv));
}
