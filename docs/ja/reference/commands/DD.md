# dd — バイナリコピーとRAM転送

*操作ガイド*

`dd`はファイル，デバイス，パイプの間でバイト列をコピーします．通常のファイルを扱う場合でも，**`of=`で指定した出力先の内容を上書きし得る**ため，最初はコピー先に新しいファイル名を指定してください．`if=`，`of=`，`bs=`，`count=`，`skip=`，`seek=`を扱います．`if=-`は標準入力，`of=-`は標準出力で，省略時もそれぞれ同じFDを使います．`bs`は1～256バイト，`skip`と`seek`は`bs`単位のブロック数です．値は10進数または`0x`付き16進数を使用できます．

## まず通常ファイルをコピーする

```sh
dd if=/tmp/test.py of=/tmp/copy.py bs=128
dd if=/dev/mem of=/tmp/dump.bin bs=1 skip=0x2000E000 count=256
hexdump /tmp/dump.bin
```

最初のコマンドは `/tmp/test.py` を `/tmp/copy.py` にコピーします．`bs=128` は1回の読み書きの単位を128バイトにする指定です．`hexdump` はコピーされたバイト列を確認するためのコマンドです．2番目の例は通常のコピーより危険性が高いメモリ読出しなので，以下の注意事項を読んでから使用してください．

## メモリ・デバイスを扱うときの注意

`/dev/mem`のオフセットは絶対アドレスです．`bs=1`なら`skip=`と`seek=`をバイトアドレスとして指定できます．`/dev/mem`から読み出す場合は`count=`が必須です．書き込み可能範囲はユーザスロット`0x2000E000`～`0x20031FFF`，共有RAM（SHM）`0x20032000`～`0x200327FF`，および`/proc/ramfiles`で確認できるRAMファイルのデータ領域です．タスク領域への書き込みは実行中のコードやデータを破壊するため，十分注意してください．

```sh
# 共有RAMの書込み・読出し例
dd if=/tmp/test.py of=/dev/shm bs=1 count=256
dd if=/dev/shm of=/tmp/shm.bin bs=1 count=256
```

`/dev/mem`によるRAMファイルデータの直接変更は，ファイル名・長さなどの管理情報を更新しません．通常のファイルの作成・転送には`of=/tmp/名前`を使用してください．現在のRAMファイルは11個，各2048バイトです．データ領域は`0x20032800`から固定配置されています．全タスク共通の2 KiB RAMは`/dev/shm`（相対オフセット）または`/dev/mem`（絶対アドレス`0x20032000`から）でアクセスできます．

## ネットワーク転送の前提

以降の `nc` を使う例は**ネットワーク接続したPico W**を対象とします．Linux側とPicoNIX側で使用するIPアドレスを確認し，受信側を先に起動します．転送先がRAMファイルの場合は容量制限にも注意してください．

## Linux → PicoNIX：ファイル転送

まずPicoNIX側で受信を開始します．

```sh
# PicoNIX側
nc -N -l 9000 | dd if=- of=/tmp/data.bin bs=1
```

別の端末でLinux側から送信します．以下のIPアドレスは例なので，実際のPico Wのアドレスに置き換えてください．

```sh
# Linux側
nc -N 192.168.4.1 9000 < data.bin
```

```sh
# PicoNIX側：転送結果を確認
ls -l /tmp/data.bin
hexdump /tmp/data.bin
```

Linux側`nc`の`-N`の有無や意味はnetcatの実装によって異なります．送信後に終了しない場合は，Linux側のnetcatのヘルプも確認してください．

## PicoNIX → Linux：RAMをバイナリ転送

```sh
# Linux側：先に受信待ち
nc -l 9000 > dump.bin
```

```sh
# PicoNIX側：256バイトを送信
# 192.168.4.2はLinux側のIPアドレス例
dd if=/dev/mem bs=1 skip=0x2000E000 count=256 | nc -N 192.168.4.2 9000
```

```sh
# Linux側：受信サイズと内容を確認
wc -c dump.bin
hexdump -C dump.bin
```

## Linux → PicoNIX：共有RAMへ転送

```sh
# PicoNIX側：共有RAM先頭へ256バイト受信
nc -N -l 9000 | dd if=- of=/dev/shm bs=1 count=256
```

```sh
# Linux側：ちょうど256バイトを送信する例
head -c 256 data.bin | nc -N 192.168.4.1 9000
```

転送量と受信の終了条件を確認してください．`/dev/shm`は全タスクで共有されるため，他のプログラムが利用中なら内容を上書きしないでください．ユーザスロットへの直接書き込みは通常運用では推奨しません．現行の`/dev/mem`は任意のRAMへの無制限書き込みを提供するものではありません．Pico W以外の通常のPicoではWi-Fi経由の`nc`転送は利用できません．

## うまく転送できないとき

受信が終わらないときは，両側の `nc` の終了条件と `-N` の対応を確認します．内容がおかしい場合は `count=`，`bs=`，送受信サイズを確認してください．`/dev/mem` の読み出しに失敗した場合は，指定アドレスが許可範囲内か，`count=` が指定されているかを確認します．

---

**関連リンク：** [パイプ操作](PIPE.md) ｜ [操作ガイド一覧](README.md) ｜ [English](../../../en/reference/commands/DD.md) ｜ [文書一覧](../../README.md) ｜ [nc](../network/NC.md) ｜ [デバイス](../devices/README.md)
