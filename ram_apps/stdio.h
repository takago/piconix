#ifndef PICONIX_STDIO_H
#define PICONIX_STDIO_H
#include <stddef.h>
#include <stdarg.h>
typedef struct { int fd; } FILE;
extern FILE __piconix_stdin, __piconix_stdout, __piconix_stderr;
#define stdin (&__piconix_stdin)
#define stdout (&__piconix_stdout)
#define stderr (&__piconix_stderr)
#define EOF (-1)
int putchar(int c);
int getchar(void);
int puts(const char *s);
char *fgets(char *s, int size, FILE *stream);
int printf(const char *fmt, ...);
int sprintf(char *s, const char *fmt, ...);
int snprintf(char *s, size_t size, const char *fmt, ...);
#endif
