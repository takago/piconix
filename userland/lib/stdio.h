#ifndef PICONIX_STDIO_H
#define PICONIX_STDIO_H
#include <stddef.h>
#include <stdarg.h>
/* All user-space runtime symbols are linked within the same PIC image. */
#ifndef PICONIX_HOST_TEST
#pragma GCC visibility push(hidden)
#endif
/* One 64-byte buffer per stream, used for input OR output. */
typedef struct {
    int fd;
    unsigned char buf[64];
    unsigned char pos, len, flags, saved_count;
    unsigned char saved[3];
} FILE;
extern FILE __piconix_stdin, __piconix_stdout, __piconix_stderr;
#define stdin (&__piconix_stdin)
#define stdout (&__piconix_stdout)
#define stderr (&__piconix_stderr)
#define EOF (-1)
#define PICONIX_BUFSIZ 64
#define _IOFBF 0
#define _IOLBF 1
#define _IONBF 2
/* Uses the built-in 64-byte buffer; external buffers are unsupported. */
int setvbuf(FILE *stream, char *buf, int mode, size_t size);
FILE *fopen(const char *path, const char *mode);
int fclose(FILE *stream);
size_t fread(void *ptr, size_t size, size_t count, FILE *stream);
size_t fwrite(const void *ptr, size_t size, size_t count, FILE *stream);
int fflush(FILE *stream);
int feof(FILE *stream);
int ferror(FILE *stream);
int fprintf(FILE *stream, const char *fmt, ...);
int fscanf(FILE *stream, const char *fmt, ...);
int putchar(int c);
int getchar(void);
int puts(const char *s);
char *fgets(char *s, int size, FILE *stream);
int printf(const char *fmt, ...);
int scanf(const char *fmt, ...);
int sscanf(const char *text, const char *fmt, ...);
int sprintf(char *s, const char *fmt, ...);
int snprintf(char *s, size_t size, const char *fmt, ...);
#ifndef PICONIX_HOST_TEST
#pragma GCC visibility pop
#endif
#endif
