/* すべてのユーザアプリに共通する入口．カーネルがr0/r1に渡す
 * argc/argvをmainへ引き継ぎ，終了時に出力を流してexitする． */
/* ユーザプログラム共通の独立した入口．カーネルはargc/argvをr0/r1で渡す． */
#include "unistd.h"
#include "stdio.h"
extern int main(int argc, char *argv[]);
/* アプリ起動時にmainを呼び出し，終了コードをカーネルへ返す入口． */
__attribute__((used, noreturn, section(".text.app_entry")))
void app_entry(int argc, char *argv[]) {
    int status=main(argc, argv);
    fflush(0);
    _exit(status);
}
