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

Cアプリでは `ioctl()` の `OS_TTY_GET_CANON`，`OS_TTY_GET_ECHO`，`OS_TTY_SET_CANON`，`OS_TTY_SET_ECHO` を使います．`stty` は `OS_TTY_PERSIST` によりコマンド終了後も設定を維持します．フォアグラウンドで起動されたアプリでは，カーネルが実行前のTTY設定を保存し，終了時（Ctrl+Cやkillを含む）に復元します．`edit`，`mem`，`gpio`，`mb`，`pong`，`fbed` は通常終了時のTTY設定復元をカーネルに任せます．起動途中の設定失敗時にはアプリも復旧を試みます．`VMIN` / `VTIME` には対応していません．

## FDごとのノンブロッキング：`fcntl()`

`fcntl(fd, F_GETFL, 0)` でフラグを取得し，`fcntl(fd, F_SETFL, flags)` で `O_NONBLOCK` を設定します．端末のcanonical/echo設定とは独立しています．文字単位の入力が必要なら，`-icanon` と適切なブロッキング設定を組み合わせて使います．

現行実装で `F_SETFL` による `O_NONBLOCK` の変更が可能なのは，USB CDCコンソール，UART1，Wi-Fi TTY（有効な場合），TCP/UDPソケットのFDです．通常ファイル，パイプ，デバイス一般への変更には対応していません．`open()` 時に `O_NONBLOCK` を指定できるのは `/dev/console` と `/dev/uart1` に限られ，`/dev/wifi` は `open()` 後に `fcntl()` で設定します．

TTYの非canonical入力で受信データがない場合，`O_NONBLOCK` が設定された `read()` は **0** を返します（`EAGAIN` 相当の負値ではありません）．したがって0を常に恒久的なEOFと解釈しないでください．ソケットやパイプのI/O動作はTTYと異なるため，[パイプ](PIPE.md)と[ソケットAPI](NETWORK-PROGRAMMING.md)も参照してください．

## 設定の復元と画面制御の分担

`stty`と`reset`は`OS_TTY_PERSIST`を使って意図的な設定変更を保持します．フォアグラウンドタスクの終了や中断では，カーネルが保存済みのTTY設定を復元します．`mb`などの対話型プログラムは，非canonical入力と非ブロッキングFDを組み合わせます．なお，`reset`は`O_NONBLOCK`を変更しません．

カーソル表示，代替画面，文字属性などはTTY設定とは独立しています．アプリケーション側の画面復帰方法，`reset`の具体的な使い方，強制終了時の注意点は[端末の使い方と画面制御](TERMINAL.md)を参照してください．

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


## パイプ・ソケットとTTYの違い

`stty` と `OS_TTY_*` の `ioctl()` は**端末に対する設定**です．パイプや通常のファイル，ソケットに対してcanonical入力やエコーを期待してはいけません．一方，`fcntl()` の `O_NONBLOCK` はFDのI/O動作を制御します．パイプやソケットでは，読み取り可能なデータがない場合に待機するかどうかを，対象のAPIの仕様と合わせて確認してください．

- `icanon` 有効：端末の行入力．無効：文字単位の入力．**パイプのデータには行編集を行いません**．
- `echo`：TTY入力のローカルエコー．パイプやネットワークへのデータ転送とは別です．
- `onlcr`：TTY出力に限るLF→CRLF変換．パイプ・ファイル・ソケットのバイト列には適用しません．
- `VMIN` / `VTIME`：POSIX `termios` の設定は実装されていません．非canonical入力の待機方法は `fcntl()` の設定と `read()` の結果で制御します．
- USB CDC，UART1，Wi-Fi端末は，ビルド構成に応じて独立したTTYとして扱われます．


**実装の参照先：** `kernel/kernel.c` の `tty_canonical`，`tty_echo`，`tty_onlcr`，`tty_saved_*`，`foreground_group` と `OS_TTY_*` の処理，`userland/shell.c` の行編集とパイプ処理．
