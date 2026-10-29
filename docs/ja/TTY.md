# 端末設定と対話型入力

[English](../en/TTY.md) | [概要](README.md) | [UART1と端末](UART.md) | [Cアプリ開発](PROGRAMMING.md)

PicoNIXでは，端末の**入力モード**とファイルディスクリプタ（FD）の**ブロッキング設定**を別々に扱います．POSIX `termios` の完全実装ではありません．

## 端末設定：`stty` と `ioctl()`

```text
stty
stty -icanon -echo
stty icanon echo
stty raw
stty cooked
```

- `stty`：標準入力端末の `icanon`（行入力）と `echo` の状態を表示します．
- `icanon` / `-icanon`：行単位 / 文字単位の入力を切り替えます．
- `echo` / `-echo`：入力文字のエコーを有効 / 無効にします．
- `raw` は `-icanon -echo`，`cooked` は `icanon echo` の簡易指定です．POSIXの完全なraw/cooked設定ではありません．

Cアプリでは `ioctl()` の `OS_TTY_GET_CANON`，`OS_TTY_GET_ECHO`，`OS_TTY_SET_CANON`，`OS_TTY_SET_ECHO` を使います．`stty` は `OS_TTY_PERSIST` によりコマンド終了後も設定を維持します．通常はアプリ内だけで設定を一時変更し，終了時に復元する方が安全です．`VMIN` / `VTIME` には対応していません．

## FDごとのノンブロッキング：`fcntl()`

`fcntl(fd, F_GETFL, 0)` でフラグを取得し，`fcntl(fd, F_SETFL, flags)` で `O_NONBLOCK` を設定します．端末のcanonical/echo設定とは独立しています．文字単位の入力が必要なら，`-icanon` と適切なブロッキング設定を組み合わせて使います．

## `mb`（moonbuggy）

```text
mb
```

SPACEでジャンプ，`q`で終了します．`mb` は標準入力を文字単位・非エコー・非ブロッキングに切り替え，`read()` と `msleep()` で入力と画面更新を扱います．通常終了時に保存した端末設定とFDフラグを復元します．

## `reset`（端末表示の初期化）

```text
reset
```

`reset` は標準入力の `icanon` と `echo` を有効にし，ANSIエスケープシーケンスで文字属性・カーソル・画面を初期化します．`O_NONBLOCK` は変更しません．端末入力ができない場合は実行できないため，復旧手段として万能ではありません．

## 注意点と関連資料

端末モードは端末側で共有されます．非標準設定のままシェルに戻ると入力が難しくなるため，対話型アプリでは終了時の復元を徹底してください．端末の所属，`getty`，Ctrl+C／Ctrl+D，USB CDCとUART1の相違は[UART1と端末](UART.md)を参照してください．

## TTY出力の改行変換（ONLCR）

通常，USB CDCコンソールとUART1のTTYでは，LF（`\n`）をCRLF（`\r\n`）に変換します．既存のCRLFを二重変換せず，CR直後のLFはそのまま出力します．ファイルやGPIO等のデバイスには適用されません．

```text
stty             # onlcr または -onlcr を表示
stty -onlcr      # TTY出力変換を無効化
stty onlcr       # TTY出力変換を有効化
```

Cプログラムでは `ioctl(fd, OS_TTY_GET_ONLCR, 0)` と `ioctl(fd, OS_TTY_SET_ONLCR, 0 または 1)` を利用できます．設定はTTYごとに独立しており，canonical入力とエコー設定とは別です．`raw` コマンドでも出力変換は自動的に変更しません．

## TAB入力

TAB（`0x09`）はシェルの行編集とcanonical入力で受け付け，ファイルへはTAB文字のまま保存します．画面表示ではTAB幅を4桁固定とし，次の4桁境界までスペースで描画します．シェルのカーソル移動・削除・履歴再表示もこの表示幅に対応します．

## シェルでのCtrl+L

シェルの行編集中にCtrl+Lを押すと，ANSIシーケンスで画面を消去してプロンプトと入力中の行を再表示します．編集中の文字列，履歴から選んだ内容，カーソル位置は保持します．`reset` と異なりTTY設定は変更しません．
