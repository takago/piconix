# PicoNIXで自作プログラムを作る

PicoNIXの外部プログラムは，**通常のPC向け実行ファイルではなく，固定RAMアドレスにリンクした独立バイナリ**です．この文書では，既存の`blink`と`omikuji`を参考に，自作プログラムを追加する手順を説明します．まずは既存プログラムを変更して試し，仕組みを理解してから新しいコマンドを登録することを推奨します．

## 1．最初に理解すること

| 領域 | アドレス | 用途 |
|---|---|---|
| Bank 0 | `0x20000000` | カーネル |
| Bank 1 | `0x20010000` | Shell |
| Bank 2 | `0x20020000` | Blink |
| Bank 3 | `0x20030000` | Omikuji |

各スロットは64 KiBで，末尾の4 KiBをスタックとして予約しています．プログラムはFlashに埋め込まれ，起動時に対応するRAMスロットへ配置されます．ELFローダ，動的リンク，アドレス再配置はありません．**現在の実装には，任意のファイル名からプログラムを起動する汎用ローダもありません．**

## 2．最初の演習：Blinkを変更する

`ram_apps/blink.c`を開き，`SYS_SLEEP`の引数を変更します．

```c
svc3(SYS_SLEEP, 500, 0, 0); /* 500 ms待つ */
```

例えば`250`に変更すれば点滅速度が変わります．ビルドと書き込みの手順は[SETUP.md](SETUP.md)を参照してください．書き込み後は次のように試します．

```text
piconix# blink &
piconix# ps
piconix# kill 2
```

PIDは実行環境によって異なります．フォアグラウンドでは`blink`で起動し，Ctrl+Cで中断できます．

## 3．独立プログラムの基本形

`ram_apps/omikuji.c`と`ram_apps/blink.c`は，どちらも`app_entry()`から開始します．以下はコンソールに一行出力して終了する最小例です．**この例はコードの書き方を示すものであり，ファイルを追加するだけではShellから起動できません**（登録方法は次節）．

```c
/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <stdint.h>

enum { SYS_EXIT = 3, SYS_WRITE = 9 };

static inline int svc3(int n, uintptr_t a, uintptr_t b, uintptr_t c)
{
    register uintptr_t r0 __asm("r0") = (uintptr_t)n;
    register uintptr_t r1 __asm("r1") = a;
    register uintptr_t r2 __asm("r2") = b;
    register uintptr_t r3 __asm("r3") = c;
    __asm volatile("svc #0" : "+r"(r0)
                   : "r"(r1), "r"(r2), "r"(r3) : "memory");
    return (int)r0;
}

__attribute__((used, noreturn, section(".text.app_entry")))
void app_entry(void)
{
    static const char message[] = "Hello from PicoNIX!\r\n";
    svc3(SYS_WRITE, 1, (uintptr_t)message, sizeof(message) - 1);
    svc3(SYS_EXIT, 0, 0, 0);
    for (;;) { }
}
```

`svc #0`はSVC例外を通じてカーネルのシステムコールを呼び出します．この実装では`r0`がシステムコール番号，`r1`〜`r3`が引数，戻り値は`r0`です．番号は**現行ソースの`src/kernel.c`および既存の`ram_apps/*.c`を確認**してください．将来の変更で番号が変わる可能性があります．

### よく使うシステムコール

| 番号 | 名前 | 主な用途 |
|---|---|---|
| 3 | `SYS_EXIT` | 終了コードを指定してタスク終了 |
| 4 | `SYS_SLEEP` | ミリ秒単位で待機 |
| 6 | `SYS_OPEN` | 仮想ファイルを開く |
| 7 | `SYS_CLOSE` | 閉じる |
| 8 | `SYS_READ` | 読み取る（fd 0は標準入力） |
| 9 | `SYS_WRITE` | 書き込む（fd 1は標準出力） |
| 10 | `SYS_IOCTL` | GPIOなどの制御 |
| 15 | `SYS_GETARG` | Omikuji用の簡易引数取得 |

標準入力はフォアグラウンドジョブに割り当てられます．バックグラウンドタスクがUSBコンソールから`read(0, ...)`しようとするとエラーになります．`SYS_GETARG`は一般的な`argc/argv`ではありません．

## 4．新しいコマンドを追加するには

現在の実装では，次の**4か所以上**の登録が必要です．`hello`というプログラムを追加する場合を例に，修正箇所を示します．

1. **ソース**：`ram_apps/hello.c`を作り，`app_entry()`を定義する．
2. **ビルド**：`CMakeLists.txt`の`foreach(APP IN ITEMS shell blink omikuji)`に`hello`を追加する．`APP_ORIGIN`の分岐では`hello`を配置するスロットを明示する．同じBank 3を使う場合は，Omikujiと同時には配置できない．
3. **ファームウェア登録**：`src/main.c`で，既存の`piconix_omikuji_image_start/end`と`register_image(...)`を参考に，生成されたイメージを登録する．`ram_apps/embed_image.S.in`で生成されるシンボル名も確認する．
4. **Shellコマンド**：`ram_apps/shell.c`のコマンド判定・起動処理に`hello`を追加し，`help`も更新する．`SYS_SPAWN`に渡すプログラムIDは`src/main.c`で登録したIDと一致させる．

**注意：** 現在の`register_image`とタスク管理は固定スロットを前提としています．新しいプログラムを既存のBank 3へ追加する場合，単純にプログラムIDを増やすだけで両方を独立に実行できるとは限りません．まずはOmikujiを`hello`に**置き換える**実験から始めるのが安全です．複数プログラムを同じスロットで切り替えるには，Flash上の複数イメージの選択・コピーとスロット占有管理の確認／拡張が必要です．

## 5．ビルドと検査

```bash
chmod +x ./tools/build_sdk.sh
PICO_TOOLCHAIN_PATH="$HOME/.pico-sdk/toolchain/14_2_Rel1" \
LC_ALL=C LANG=C \
PICO_SDK_PATH="$HOME/.pico-sdk/sdk/2.1.1" \
./tools/build_sdk.sh
```

各プログラムのELF／BINは`build/ram_<プログラム名>/`に生成され，`tools/check_slot_image.py`でスロット配置が検査されます．ビルド成功後，`build/piconix.uf2`をPicoに書き込みます（出力先はビルド設定で確認してください）．

## 6．注意事項・トラブルシューティング

- `-ffreestanding -fno-builtin -nostdlib`を使うため，通常の`printf`や`malloc`などのCライブラリ関数はそのまま利用できません．
- Cソースで明示的に呼ばなくても，最適化によって`memcpy`や`__aeabi_uidivmod`などの未解決参照が生成されることがあります．その場合はELFの未解決シンボルと逆アセンブルを確認してください．
- 固定RAMアドレス，スタック領域，`.bss`の大きさには制限があります．
- プログラムの終了時は`SYS_EXIT`を呼びます．`app_entry()`から普通に`return`しないでください．
- バックグラウンド出力はShellの入力行と混ざる場合があります．
- GPIO等へのアクセスは可能な限り`/dev`とシステムコール経由にし，アプリからハードウェアレジスタを直接操作しないようにすると，OSの教材として理解しやすくなります．

詳しいメモリ配置とカーネル構造は[INTERNAL.md](INTERNAL.md)，セットアップは[SETUP.md](SETUP.md)を参照してください．

### Shellの相対パスについて

対話Shellは`cd`・`pwd`・`ls`（引数省略可）と相対パスをサポートしますが，これはShell独自の機能です．RAMアプリから`SYS_OPEN`を直接呼ぶ場合は，従来どおり`/dev/rand`や`/sys/proc/ticks`のような絶対パスを指定してください．
