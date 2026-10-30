/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Compact standalone help. Use direct comparisons/prints to retain PIC. */
#include "stdio.h"
#include "string.h"

int main(int argc, char **argv) {
    if(argc==1) {
        puts("PicoNIX commands (use: help NAME)");
        puts("\nFiles and shell:");
        puts("  sh         Interactive shell");
        puts("  ls         List files / sizes");
        puts("  cat        Read file / standard input");
        puts("  rm         Remove a /tmp file");
        puts("  echo       Print text");
        puts("  pwd        Print working directory");
        puts("  cd         Change working directory");
        puts("  getty      Attach shell to terminal");
        puts("  exit       Leave shell");
        puts("  logout     Leave shell");
        puts("\nSystem and diagnostics:");
        puts("  help       Command list and usage");
        puts("  ps         Show processes");
        puts("  kill       Stop a process");
        puts("  tty        Show controlling terminal");
        puts("  stty       Terminal mode settings");
        puts("  reset      Reset terminal display/settings");
        puts("  lsof       Show open files");
        puts("  trace      Trace system calls");
        puts("  watch      Repeat a command");
        puts("  hexdump    Hex and ASCII dump");
        puts("\nHardware:");
        puts("  gpio       Interactive GPIO editor");
        puts("  pins       Show physical pin layout");
        puts("  fbctl      8x8 framebuffer controller");
        puts("  fbed       Interactive framebuffer editor");
        puts("  pwmctl     Control PWM slice 1");
        puts("  blink      Blink task");
        puts("\nProgramming:");
        puts("  as         Assemble Thumb source");
        puts("  python     Tiny Python interpreter");
        puts("  hello      Print a greeting");
        puts("\nGames and demos:");
        puts("  mb         Moon Buggy game");
        puts("  pong       Pong game");
        puts("  omikuji    Countdown fortune");
        puts("  kitesys    Kanazawa Institute of Tech. Embedded System Test Program");
        puts("\nShell: CMD &, CMD < file, CMD > file, CMD >> file");
        return 0;
    }
    if(argc!=2) { puts("usage: help [command]"); return 1; }
    if(!strcmp(argv[1],"sh")) puts("sh: Interactive shell\nusage: sh");
    else if(!strcmp(argv[1],"blink")) puts("blink: Blink task\nusage: blink [&]");
    else if(!strcmp(argv[1],"omikuji")) puts("omikuji: Countdown fortune\nusage: omikuji [seconds] [&]");
    else if(!strcmp(argv[1],"python")) puts("python: Tiny Python interpreter\nusage: python [file]");
    else if(!strcmp(argv[1],"hello")) puts("hello: Print a greeting\nusage: hello [name]");
    else if(!strcmp(argv[1],"hexdump")) puts("hexdump: Hex and ASCII dump\nusage: hexdump [file [offset [count]]]");
    else if(!strcmp(argv[1],"trace")) puts("trace: Trace system calls\nusage: trace on|off|show [PID]|follow [PID]|run CMD [args...]");
    else if(!strcmp(argv[1],"ps")) puts("ps: Show processes\nusage: ps");
    else if(!strcmp(argv[1],"cat")) puts("cat: Read file / standard input\nusage: cat [file]  (shell: cat > file)");
    else if(!strcmp(argv[1],"watch")) puts("watch: Repeat a command\nusage: watch [seconds] command [args...]");
    else if(!strcmp(argv[1],"pwd")) puts("pwd: Print working directory\nusage: pwd");
    else if(!strcmp(argv[1],"ls")) puts("ls: List files / sizes\nusage: ls [-l] [path]");
    else if(!strcmp(argv[1],"rm")) puts("rm: Remove a /tmp file\nusage: rm path");
    else if(!strcmp(argv[1],"echo")) puts("echo: Print text\nusage: echo [-n] [-e] [text...]\n-e: interpret \\n, \\r, \\t, \\\\, \\033 (octal), \\x1b (hex)\n-n: omit trailing newline");
    else if(!strcmp(argv[1],"kill")) puts("kill: Stop a process\nusage: kill PID");
    else if(!strcmp(argv[1],"tty")) puts("tty: Show controlling terminal\nusage: tty");
    else if(!strcmp(argv[1],"kitesys")) puts("kitesys: Kanazawa Institute of Tech. Embedded System Test Program\nusage: kitesys\nFB display, GP16/17 buttons and GP18 PWM");
    else if(!strcmp(argv[1],"fbctl")) puts("fbctl: 8x8 framebuffer controller\nusage: fbctl on|off|clear|status|test");
    else if(!strcmp(argv[1],"pins")) puts("pins: Show physical pin layout\nusage: pins");
    else if(!strcmp(argv[1],"pwmctl")) puts("pwmctl: Control PWM slice 1\nusage: pwmctl status|freq HZ|a DUTY|b DUTY|off\nDUTY: 0..100%; HZ: 8..100000");
    else if(!strcmp(argv[1],"stty")) puts("stty: Terminal mode settings\nusage: stty [raw|cooked|icanon|-icanon|echo|-echo|onlcr|-onlcr] ...");
    else if(!strcmp(argv[1],"mb")) puts("mb: Moon Buggy game\nusage: mb\nSPACE/j: jump, q: quit (no -2 option)");
    else if(!strcmp(argv[1],"reset")) puts("reset: Reset terminal display/settings\nusage: reset");
    else if(!strcmp(argv[1],"lsof")) puts("lsof: Show open files\nusage: lsof [PID]");
    else if(!strcmp(argv[1],"pong")) puts("pong: Pong game\nusage: pong [-2]\n-2: two-player mode on both terminals");
    else if(!strcmp(argv[1],"gpio")) puts("gpio: Interactive GPIO editor\nusage: gpio  (arrows i/o h/l SP u/d/n q)");
    else if(!strcmp(argv[1],"fbed")) puts("fbed: Interactive framebuffer editor\nusage: fbed\narrows: select, SPACE: toggle, c: clear, q: quit");
    else if(!strcmp(argv[1],"as")) puts("as: Assemble Thumb source\nusage: as file.s  -> /tmp/a.out");
    else if(!strcmp(argv[1],"help")) puts("help: Command list and usage\nusage: help [command]");
    else if(!strcmp(argv[1],"cd")) puts("cd: Change working directory\nusage: cd [path]");
    else if(!strcmp(argv[1],"getty")) puts("getty: Attach shell to terminal\nusage: getty /dev/uart1 sh|python [&]");
    else if(!strcmp(argv[1],"exit")) puts("exit: Leave shell\nusage: exit");
    else if(!strcmp(argv[1],"logout")) puts("logout: Leave shell\nusage: logout");
    else { printf("help: unknown command: %s\n",argv[1]); return 1; }
    return 0;
}
