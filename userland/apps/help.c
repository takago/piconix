/* SPDX-License-Identifier: GPL-3.0-or-later */
/* 標準コマンドの使用方法を表示する． */

/* PICイメージを小さくするため，直接比較とprintで実装した単独ヘルプ． */
#include "stdio.h"
#include "string.h"

/* 登録コマンドと利用方法の簡易案内を表示する． */
int main(int argc, char **argv) {
    if(argc==1) {
        puts("PicoNIX commands (use: help NAME)");
        puts("\nFiles and shell:");
        puts("  sh         Interactive shell");
        puts("  ls         List files / sizes");
        puts("  tree       Display directory tree");
        puts("  fd         Interactive directory/file browser");
        puts("  find       Search directory entries");
        puts("  edit       Edit a /tmp or /etc file");
        puts("  vi         Modal /tmp or /etc editor (vi-like)");
        puts("  dd         Copy binary data / RAM blocks");
        puts("  cat        Read file / standard input");
        puts("  cp         Copy a RAM file");
        puts("  grep       Filter lines by matching text");
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
        puts("  mem        Live memory viewer");
        puts("\nHardware:");
        puts("  gpio       Interactive GPIO editor");
        puts("  i2cctl     I2C1 scan/read/write/combined transfer");
        puts("  bh1750     BH1750 illuminance sensor (I2C1)");
        puts("  pins       Show physical pin layout");
        puts("  fbctl      8x8 framebuffer controller");
        puts("  fbed       Interactive framebuffer editor");
        puts("  pwmctl     Control PWM slice 1");
        puts("  blink      Blink task");
        puts("\nProgramming:");
        puts("  as         Assemble Thumb source");
        puts("  python     Tiny Python interpreter");
        puts("  net        Network, AP clients and ARP diagnostics");
        puts("  ip         Show IP interfaces (read-only)");
        puts("  nc         TCP/UDP network client or listener");
        puts("  ping       Send ICMP Echo requests (Pico W)");
        puts("  tcpdump    Observe IPv4 traffic (Pico W)");
        puts("  httpd      HTTP/1.0 server on TCP port 80");
        puts("  scroll     ASCII LED scrolling display (stdin/UDP)");
        puts("  cc         Small C-subset compiler (C to Thumb assembly)");
        puts("  mcpd       Experimental MCP HTTP server on TCP 8080");
        puts("  tcp_echod  TCP echo server (default port 7)");
        puts("  udp_echod  UDP echo server (default port 7)");
        puts("  hello      Print a greeting");
        puts("\nGames and demos:");
        puts("  mb         Moon Buggy game");
        puts("  pong       Pong game");
        puts("  omikuji    Countdown fortune");
        puts("  kitesys    Kanazawa Institute of Tech. Embedded System Test Program");
        puts("\nManual: ls /man ; cat /man/README ; cat /man/agent");
        puts("Languages: /man/languages/c, /man/languages/asm, /man/languages/python");
        puts("\nShell: CMD &, CMD < file, CMD > file, CMD >> file");
        return 0;
    }
    if(argc!=2) { puts("usage: help [command]"); return 1; }
    if(!strcmp(argv[1],"sh")) puts("sh: Interactive shell\nusage: sh");
    else if(!strcmp(argv[1],"bh1750")) puts("bh1750: Read BH1750 sensor on I2C1\nusage: bh1750 [count] (0 = continuous)");
    else if(!strcmp(argv[1],"i2cctl")) puts("i2cctl: I2C1 on GP26/GP27\nusage: i2cctl scan | read ADDR COUNT | write ADDR BYTE... | xfer ADDR READ_COUNT [BYTE...]");
    else if(!strcmp(argv[1],"blink")) puts("blink: Blink task\nusage: blink [&]");
    else if(!strcmp(argv[1],"omikuji")) puts("omikuji: Countdown fortune\nusage: omikuji [-i | seconds]\nDefault: 10 seconds (1..30). -i: repeat numbers from stdin until EOF.\nExamples: omikuji &, omikuji 3, echo 3 | omikuji -i.");
    else if(!strcmp(argv[1],"python")) puts("python: Tiny Python interpreter\nusage: python [file]");
    else if(!strcmp(argv[1],"hello")) puts("hello: Print a greeting\nusage: hello [name]");
    else if(!strcmp(argv[1],"hexdump")) puts("hexdump: Hex and ASCII dump\nusage: hexdump [file [offset [count]]]");
    else if(!strcmp(argv[1],"trace")) puts("trace: Trace system calls\nusage: trace on|off|show [PID]|follow [PID]|run CMD [args...]");
    else if(!strcmp(argv[1],"ps")) puts("ps: Show processes\nusage: ps");
    else if(!strcmp(argv[1],"grep")) puts("grep: Filter matching lines from a file or standard input\nusage: grep PATTERN [file]\nLiteral substring; lines over 128 bytes are skipped");
    else if(!strcmp(argv[1],"ping")) puts("ping: ICMP Echo tests (Pico W)\nusage: ping [-c COUNT] IPv4\nDefault: 4 probes; numerical IPv4 only\nexample: ping -c 2 192.168.4.2");
    else if(!strcmp(argv[1],"tcpdump")) puts("tcpdump: Observe IPv4 packets (Pico W)\nusage: tcpdump [-x] [-c COUNT]\n-x: show DATA length and up to 16 payload bytes in HEX and ASCII on an indented second line\n-c: stop after COUNT packets; Ctrl+C to stop\nexample: tcpdump -c 10");
    else if(!strcmp(argv[1],"cp")) puts("cp: Copy a /tmp or /etc RAM file\nusage: cp SRC DST\nOverwrites DST; refuses the same file\nexample: cp /etc/index.html /tmp/index.html");
    else if(!strcmp(argv[1],"cat")) puts("cat: Display a file or standard input\nusage: cat [-n] [file]\n-n: Number all lines, including empty lines\nexample: cat -n /tmp/test.py\nexample: echo hello | cat -n");
    else if(!strcmp(argv[1],"fd")) puts("fd: Interactive file browser\nusage: fd [directory]\nArrows: select/navigate, Enter: open, Backspace: parent, e: edit /tmp or /etc, q: quit");
    else if(!strcmp(argv[1],"watch")) puts("watch: Repeat a command\nusage: watch [seconds] command [args...]");
    else if(!strcmp(argv[1],"pwd")) puts("pwd: Print working directory\nusage: pwd");
    else if(!strcmp(argv[1],"tree")) puts("tree: Display directory tree\nusage: tree [-L 0..8] [path]");
    else if(!strcmp(argv[1],"tcp_echod")) puts("tcp_echod [PORT]: TCP echo server (default 7)");
    else if(!strcmp(argv[1],"udp_echod")) puts("udp_echod [PORT]: UDP echo server (default 7)");
    else if(!strcmp(argv[1],"httpd")) puts("httpd: HTTP/1.0 server on TCP 80 (HTML, framebuffer/PWM APIs)");
    else if(!strcmp(argv[1],"scroll")) puts("scroll: LED text ticker\nusage: scroll [-u [PORT]] (default UDP 2001)");
    else if(!strcmp(argv[1],"cc")) puts("cc: compile a tiny C subset to Thumb assembly\nusage: cc source.c\nthen: as /tmp/cc.s ; ./a.out");
    else if(!strcmp(argv[1],"mcpd")) puts("mcpd: Experimental Streamable HTTP MCP server (Pico W)\nusage: mcpd [--shell] (TCP 8080)\nAgent manual: cat /man/agent");
    else if(!strcmp(argv[1],"ip")) puts("ip: Interface addresses (Pico W)\nusage: ip [addr]\nShows loopback and actual Wi-Fi interface names; no configuration support");
    else if(!strcmp(argv[1],"net")) puts("net: Wi-Fi network status\nusage: net [-a|-r]\n-a: AP clients, -r: ARP cache");
    else if(!strcmp(argv[1],"nc")) puts("nc: Minimal IPv4 netcat\nusage: nc [-u] [-N] -l PORT | nc [-u] [-N] IPv4 PORT\n-N: exit on stdin EOF in UDP mode (not TCP)");
    else if(!strcmp(argv[1],"find")) puts("find: Search directory entries\nusage: find [path] [-name pattern]");
    else if(!strcmp(argv[1],"dd")) puts("dd: block copy\nusage: dd [if=FILE] [of=FILE] [bs=1..256] [count=N] [skip=N] [seek=N]\nif=-: stdin; of=-: stdout (also defaults)");
    else if(!strcmp(argv[1],"vi")) puts("vi: Modal /tmp or /etc editor\nusage: vi file\ni/a/o insert, Esc normal, :w, :q, :q!, :wq");
    else if(!strcmp(argv[1],"edit")) puts("edit: Edit a /tmp or /etc file\nusage: edit /tmp/FILE or /etc/FILE\nCtrl+S: save; Ctrl+Q: quit (twice to discard unsaved changes); Ctrl+C: abort");
    else if(!strcmp(argv[1],"mem")) puts("mem: Live memory viewer\nusage: mem [address] | mem -s [app]\ninteractive: arrows, PgUp/PgDn, g goto, q quit");
    else if(!strcmp(argv[1],"ls")) puts("ls: List files / sizes\nusage: ls [-la] [path]\n-l: show type, ID and size; -a: include . and ..");
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
    else if(!strcmp(argv[1],"gpio")) puts("gpio: Interactive GPIO editor\nusage: gpio\nArrows: select; i/o: input/output; h/l/SPACE: output level\nu/d/n: pull up/down/none for input; q: quit");
    else if(!strcmp(argv[1],"fbed")) puts("fbed: Interactive framebuffer editor\nusage: fbed\narrows: select, SPACE: toggle, c: clear, q: quit");
    else if(!strcmp(argv[1],"as")) puts("as: Assemble Thumb source\nusage: as file.s  -> /tmp/a.out");
    else if(!strcmp(argv[1],"help")) puts("help: Command list and usage\nusage: help [command]\nDetailed manual: cat /man/README");
    else if(!strcmp(argv[1],"cd")) puts("cd: Change working directory\nusage: cd [path]");
    else if(!strcmp(argv[1],"getty")) puts("getty: Attach shell to terminal\nusage: getty /dev/ttyS1 sh|python [&]");
    else if(!strcmp(argv[1],"exit")) puts("exit: Leave shell\nusage: exit");
    else if(!strcmp(argv[1],"logout")) puts("logout: Leave shell\nusage: logout");
    else { printf("help: unknown command: %s\n",argv[1]); return 1; }
    return 0;
}
