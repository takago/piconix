# システムコール開発ガイド：`getuptime()`を実際に追加する

このガイドは**実装を伴う演習**です．起動からの経過時間をミリ秒で返す`getuptime()`をPicoNIXに追加し，Cアプリから呼び出します．既存の`/proc/ticks`でも同種の情報は得られますが，**SVCの経路を学ぶためにあえて新しい入口を作ります**．Cの基本と[カーネル読解ガイド](../kernel/KERNEL-READING-GUIDE.md)を学習した人を対象にしています．

## 1．全体像と演習前の確認

```text
labuptime.c: getuptime() → userland/lib/syscalls.c: call()
  → svc #0 → kernel/kernel.c: os_svc_dispatch()
  → SYS_GETUPTIME → ticks → r0へ結果 → Cプログラム
```

ホストPC上の**PicoNIXリポジトリ**で，変更前に次を確認します．

```sh
grep -n 'SYS_SHUTDOWN_TCP\|SYS_GETUPTIME' include/piconix_syscall.h
grep -n 'case SYS_FLASH_ID\|case SYS_GETPID' kernel/kernel.c
grep -n 'unsigned flash_id' userland/lib/{syscalls.c,unistd.h}
```

現行ソースではシステムコール番号は1～39で，11と15は予約済みです．演習では未使用の**40**を使います．番号の衝突を避けるため，改造後の最新版では必ず再確認してください．ABIは**r0＝番号，r1～r3＝引数，SVC実行後r0＝戻り値**です．番号と表示名の正本は`include/piconix_syscall.h`です．

## 2．番号表を追加する（共有ヘッダ）

`include/piconix_syscall.h`の`PICONIX_SYSCALL_TABLE(X)`末尾を次のように変更します．**前の行へ継続用のバックスラッシュを付ける**必要があります．

```c
    X(SYS_SHUTDOWN_TCP,  39, "shutdown") \
    X(SYS_GETUPTIME,    40, "getuptime")
```

このマクロはカーネルとユーザ側の両方で利用されます．既存の番号は変更しません．

## 3．カーネルへ処理を追加する

`kernel/kernel.c`の`os_svc_dispatch(uint32_t *f)`内の`switch(num)`へ，`SYS_FLASH_ID`などの近くに次の`case`を追加します．`ticks`は1 ms周期で進む既存の`uint32_t`カウンタです．

```c
case SYS_GETUPTIME:
    result = (int)ticks; /* 32ビットのビット列をr0へ返す */
    break;
```

`os_svc_dispatch()`では`result`が復帰時の`r0`へ返されます．`ticks`は符号なし32ビットで約49.7日後に周回するため，呼び出し側も`unsigned`として解釈します．このAPIはエラー値を別途返さないという設計です．ユーザポインタを渡さないので，ポインタ境界検査はこの演習では不要です．**ポインタを受け取る別のSVCを作るときは**`in_task_range()`で長さと参照範囲を必ず確認してください．

## 4．ユーザC APIを公開する

`userland/lib/syscalls.c`に，既存の`call()`を使ったラッパーを追加します．

```c
unsigned getuptime(void) {
    return (unsigned)call(SYS_GETUPTIME, 0, 0, 0);
}
```

`userland/lib/unistd.h`の公開プロトタイプ一覧へ次を追加します．

```c
unsigned getuptime(void); /* 起動後ミリ秒，32ビットで周回 */
```

これでユーザCプログラムから`#include "unistd.h"`で呼び出せます．**ヘッダ宣言だけを追加してもSVCは動作しません**．共有番号表，カーネル，ラッパーの3つの一致が必要です．

## 5．試験用コマンドを追加する

この演習は[デバイスドライバ開発ガイド](../drivers/DRIVER-DEVELOPMENT.md)と共通の確認プログラム`labuptime`を使います．両演習を適用した後のCソースは以下のとおりです．デバイス実装前にSVCだけを試す場合は`open()`以下を省略し，`printf()`までを使ってください．

```c
/* Compare the new syscall with the new read-only device node. */
#include "unistd.h"
#include "fcntl.h"
#include "stdio.h"

int main(void) {
    printf("getuptime: %u ms\n", getuptime());
    int fd = open("/dev/uptime", O_RDONLY);
    if (fd < 0) { printf("open: %d\n", fd); return 1; }
    char bytes[8];
    int n;
    while ((n = read(fd, bytes, sizeof bytes)) > 0) {
        if (write(1, bytes, (unsigned)n) < 0) break;
    }
    close(fd);
    return n < 0 ? 1 : 0;
}
```

CMakeの`foreach(APP IN ITEMS ...)`に`labuptime`を追記し，`kernel/main.c`で埋め込みイメージを宣言・登録します．現行版の例ではID **51**を使っています．

```c
extern const unsigned char piconix_labuptime_image_start[], piconix_labuptime_image_end[];
/* main()のregister_image群の末尾 */
register_image(51,"labuptime",piconix_labuptime_image_start,piconix_labuptime_image_end);
```

詳しい登録位置とUF2書き込み操作は[Cアプリケーション開発](../apps/c/PROGRAMMING.md)の第3～4節を参照してください．登録ID49，50，51を別の演習で使用している場合は衝突を解決してから進めます．

## 6．完成した教材用パッチで再現する

手入力のほかに，**変更箇所をすべて収録した参考パッチ**[`docs/examples/uptime_lab.patch`](../../../examples/uptime_lab.patch)を用意しました．パッチにはシステムコール，独立した仮想デバイスドライバ，検証用コマンドの変更が含まれます．**学習用として明示的に適用する場合だけ**使用してください．普段のPicoNIX本体への採用を要求するものではありません．

```sh
# ほかの未コミット変更がない作業ブランチで
 git switch -c exercise/uptime-lab
 git apply --check docs/examples/uptime_lab.patch
 git apply docs/examples/uptime_lab.patch
 git diff --check
 python3 tools/run_tests.py
 PICO_SDK_PATH="$HOME/.pico-sdk/sdk/2.1.1" \
 PICO_TOOLCHAIN_PATH="$HOME/.pico-sdk/toolchain/14_2_Rel1" \
 ./tools/build_sdk.sh -DPICO_BOARD=pico -DCMAKE_BUILD_TYPE=MinSizeRel
```

`git apply --check`に失敗する場合は**その時点で止めます**．当該パッチはこの教材で確認したソース版を対象としており，後のバージョンでは行の位置や番号が変わる場合があります．また，通常Picoで練習した後にPico Wのビルドも確認すると，両基板への影響を把握できます．

## 7．実機で確認する

ファームウェアを書き込んだ後，次をPicoNIXのシェルで試します．この出力は**想定例**です．

```text
piconix# cat /proc/ticks
15327
piconix# labuptime
getuptime: 15410 ms
15.410 s
piconix# trace run labuptime
...
```

出力値は実行時刻によって変わります．`trace`の結果には新しい`getuptime`システムコールが現れるか確認します（[トレース操作ガイド](../../guides/TRACE.md)）．カーネルが古いUF2のままだと新番号は`OS_ENOSYS`となる可能性があります．`labuptime`自体が見つからなければCMake・イメージ登録・UF2更新を再確認してください．

**確認課題**：1．`SYS_GETUPTIME`の呼出番号はどこで共有されますか．2．`getuptime()`が32ビットで周回することをどう扱いますか．3．`trace`で観察できるのはどの境界ですか．4．同じ値を`/proc/ticks`から取得する場合との違いを説明してください．

## 8．演習後の反映・注意点

実運用のAPIとして採用するなら[システムコール仕様](../../reference/SYSCALLS.md)と[Cライブラリ仕様](../apps/c/LIBC.md)の日英両方に番号，型，戻り値，周回条件を追記します．演習を終了して変更を戻すときは，別ブランチで作業していれば元のブランチへ戻れます．`git apply -R`を使う場合，演習後に他のソース変更を混ぜていないことを確認してください．

**検証範囲について：** 参考パッチは現行ソースへの適用とCの構文検査を行っています．Pico SDKを用いたファームウェアビルドと実機動作はこの場では未検証です．

---

[カーネル拡張ガイド](../kernel/KERNEL-DEVELOPMENT.md) ｜ [ドライバ追加演習](../drivers/DRIVER-DEVELOPMENT.md) ｜ [English](../../../en/development/syscalls/SYSCALL-DEVELOPMENT.md)
