#ifndef PICONIX_STDLIB_H
#define PICONIX_STDLIB_H
/* All user-space runtime symbols are linked within the same PIC image. */
#ifndef PICONIX_HOST_TEST
#pragma GCC visibility push(hidden)
#endif
int atoi(const char *s);
long strtol(const char *s, char **endptr, int base);
int abs(int value);
#ifndef PICONIX_HOST_TEST
#pragma GCC visibility pop
#endif
#endif
