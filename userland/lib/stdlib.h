/* ユーザプログラム用の最小stdlib宣言． */
#ifndef PICONIX_STDLIB_H
#define PICONIX_STDLIB_H
/* ユーザランタイムの全シンボルは各PICイメージ内にリンクする． */
#ifndef PICONIX_HOST_TEST
#pragma GCC visibility push(hidden)
#endif
int atoi(const char *s);
long strtol(const char *s, char **endptr, int base);
unsigned long strtoul(const char *s, char **endptr, int base);
int abs(int value);
#ifndef PICONIX_HOST_TEST
#pragma GCC visibility pop
#endif
#endif
