# PicoNIX 仕様書

*技術リファレンス*

この文書は，PicoNIXの**現行実装の概要と固定上限**をまとめた開発者向け資料です．操作方法は[ドキュメント案内](../../DOCS.ja.md)を参照してください．本書はPOSIX準拠を宣言するものではありません．記述と動作が異なる場合は，対象リビジョンのソースコードを優先してください．

[全体アーキテクチャ](ARCHITECTURE.md)ではソースファイルの役割を説明しています．

Linuxとの機能の違い，省略した仕組み，上限値の一覧は[Linuxとの違い](DIFFERENCES.md)を参照してください．

## 1．対象と構成

- 対象：Raspberry Pi Pico（RP2040）およびPico W．
- カーネル：Flash上のコードをXIP実行し，データと管理情報をSRAMに置く構成．
- ユーザプログラム：固定サイズの物理RAMスロットで実行．仮想記憶・プロセス間メモリ保護を提供する仕様ではありません．
- ディレクトリ：`/bin`，`/dev`，`/proc`，`/tmp`，`/etc`．

## 2．メモリとタスク

| 項目 | 現行値 |
|---|---|
| ユーザタスクスロット | 6 |
| 1スロット | 24 KiB（24,576バイト） |
| カーネルSRAM領域 | `0x20000000`〜`0x2000DFFF`（56 KiB） |
| タスクスロット領域 | `0x2000E000`〜`0x20031FFF`（144 KiB） |
| 共有メモリ | `0x20032000`〜`0x200327FF`（2 KiB） |
| RAMファイル実データ | `0x20032800`〜`0x20037FFF`（11ファイル×2 KiB） |
| トレースリング領域 | `0x20038000`〜`0x2003BFFF`（16 KiB，カーネル管理） |
| ネットワーク予約領域 | `0x2003C000`〜`0x2003FFFF`（16 KiB） |
| プログラム画像とゼロ初期化領域の検査上限 | 1スロットから4 KiBを除いた範囲 |

最後の4 KiBはスタック用に確保される領域です．ネットワーク予約領域はメモリ配置上の予約であり，PicoのWi-Fiなしビルドで無線機能が使えることを意味しません．カーネルにはユーザタスクとは別のidleタスクがあります．

タスク状態の内部定数は `UNUSED`，`READY`，`SLEEPING`，`DEAD`，`BLOCKED` です．詳細な状態遷移や待機条件は `kernel/kernel.c` を参照してください．

## 3．ファイルディスクリプタとパイプ

- タスクごとのFD番号は0〜15（16個）です．標準入力・出力・エラー出力は通常0，1，2を使用します．
- パイプは最大2本で，各256バイトのリングバッファを使用します．
- `pipe()`，`dup()`，`dup2()`を提供します．FDの複製・継承と参照の解放はカーネルが管理します．
- パイプはバイトストリームです．読み手・書き手の終了，空／満杯，EOF，ブロック時の挙動は[パイプ](commands/PIPE.md)を参照してください．
- シェルのパイプラインの構文制限は[パイプ](commands/PIPE.md)に記載します．POSIXシェルとの完全互換ではありません．

## 4．TTY

USBコンソール，UART1，Wi-Fi端末は別々のデバイスとして扱います（Wi-Fi端末は対応ビルド時）．TTYの行編集（canonical）とraw入力，echo，改行変換（ONLCR）はTTY側の設定です．FD単位の`O_NONBLOCK`は`fcntl()`で制御し，TTY入力モードとは独立しています．PicoNIXのTTY ioctlは独自の簡易インターフェースで，完全なPOSIX termiosではありません．

コマンド番号は `include/piconix.h` の `OS_TTY_*` を，利用例は[TTY](commands/TTY.md)と[端末](commands/TERMINAL.md)を参照してください．

## 5．システムコールとABI

システムコール番号は1〜40番（11番・15番は予約）です．Thumbの `svc #0` を使い，`r0`に番号，`r1`〜`r3`に引数を置き，戻り値を`r0`で受け取ります．番号はPicoNIX内部ABIであり，Linux/POSIXの番号とは異なります．

完全な番号表とユーザAPIの対応は[システムコール](SYSCALLS.md)を参照してください．番号の一次資料は`include/piconix_syscall.h`の`PICONIX_SYSCALL_TABLE`であり，`kernel/kernel.c`のディスパッチと`userland/lib/syscalls.c`がそれを使用します．

## 6．プログラム実行とアセンブラ

Cユーザプログラムは専用のビルド・配置方式で生成します．全ユーザアプリケーションを`-Oz`と`-fPIC`でコンパイルし，24 KiBスロットのうち4 KiBをスタック用に予約します．完全なビルドでは，別のスロットアドレスでも同一のPICイメージになることを検査します．組込みの`as`はCortex-M0+向けの簡易Thumbアセンブラであり，GNU assemblerの代替ではありません．`svc #0`の即値0はシステムコール番号ではありません．`as`の命令，ディレクティブ，入力制限，生成物については[アセンブラ](../development/apps/asm/ASM.md)を参照してください．

## 7．ネットワーク

Pico WビルドではlwIPを使用します．Wi-FiモードはAPまたはSTAをビルド時に選択します．STAモードはSSIDとパスワードを指定し，DHCPでIPv4設定を取得します．状態は`/proc/net/wifi`で確認できます．

| 機能 | 位置・備考 |
|---|---|
| Telnet（TCP 23） | カーネル側のリモート端末 |
| HTTP（TCP 80） | ユーザプログラム`httpd` |
| UDPデバイス制御（UDP 2000） | カーネル側サービス |
| ソケットAPI | ユーザプログラム用のTCP/UDP機能．BSDソケットの完全互換ではありません |
| ループバック | Pico WのAP/STA両モードで`127.0.0.1`（lwIP `lo0`）に対応 |
| ネットワークメモリ診断 | `/proc/net/memory`．Pico版では`MODE: N/A`とアイドルスタックのみ |
| ユーザTCPソケット管理枠 | 4（`kernel/net_socket.h`の`NET_SOCKET_TCP_SLOTS`） |
| UDPデータ転送上限の内部定数 | 512バイト（`NET_SOCKET_UDP_MAX`） |

Pico WではlwIPの`lo0`を通じて`127.0.0.1`宛てにTCP/UDP通信できます（通常のPicoビルドでは非対応）．通信メモリは`/proc/net/memory`で確認できます．ネットワーク予約16 KiBにはlwIPヒープ10 KiB，管理情報を含む境界領域2 KiB，アイドルスタック4 KiBを配置します．pbufプール8個（各512バイト）などの静的プールはカーネルRAM側に残ります．詳細は[メモリレイアウト](MEMORY-LAYOUT.md)，[Wi-Fi](network/WIFI-MODES.md)，[ネットワーク](network/NETWORK.md)，[ソケットプログラミング](../development/apps/c/NETWORK-PROGRAMMING.md)を参照してください．TelnetとUDPデバイス制御は暗号化された認証付きサービスではないため，信頼できるネットワークで使用してください．

Pico Wでは`tcpdump`用の`/dev/net/packet`（16件のIPv4情報記録）と，`ping`用の`/dev/net/icmp`（ICMP Echo専用）を提供します．RAWソケットではありません．詳しくは[ネットワーク仕様](network/NETWORK.md)を参照してください．

## 8．デバイスと疑似ファイル

`/dev`にはコンソール，UART，GPIO，PWM，ADCなどのインターフェースがあり，`/proc`はタスクやシステム状態の観察に使用します．実際に公開されるノードと操作はビルド構成に依存します．[ファイルシステムとノード一覧](FILESYSTEM.md)，[デバイス](devices/DEVICES.md)，[内部構造](INTERNAL.md)を参照してください．

## 9．エラーと互換性

`include/piconix.h`には`OS_EINVAL`，`OS_EBADF`，`OS_EAGAIN`，`OS_EINTR`，`OS_ENOSYS`などの負のエラー値が定義されています．すべての呼び出しがPOSIXの`errno`規約に従うとは限りません．各APIの戻り値は実装と個別ガイドを確認してください．

## 10．仕様の根拠と更新手順

この仕様書の主な根拠は`include/piconix.h`，`kernel/kernel.c`，`kernel/net_socket.h`，`userland/lib/syscalls.c`，`userland/apps/shell.c`，`CMakeLists.txt`です．機能変更時は，(1)実装と定数を確認，(2)本書の上限・構成を更新，(3)個別ガイドと日英版を更新，(4)サンプルを検証，の順で整合性を保ってください．

---

**関連リンク：** [リファレンス目次](README.md) ｜ [学習ロードマップ](../ROADMAP.md) ｜ [English](../../en/reference/SPECIFICATION.md)
