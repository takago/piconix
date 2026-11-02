# dd — バイナリコピーとRAM転送

[English](../en/DD.md) | [文書一覧](DOCS.md) | [nc](NC.md) | [デバイス](DEVICES.md)

`dd`はファイル，デバイス，パイプの間でバイト列をコピーします．`if=`，`of=`，`bs=`，`count=`，`skip=`，`seek=`を扱います．`if=-`は標準入力，`of=-`は標準出力で，省略時もそれぞれ同じFDを使います．`bs`は1～256バイト，`skip`と`seek`は`bs`単位のブロック数です．値は10進数または`0x`付き16進数を使用できます．

```sh
dd if=/tmp/test.py of=/tmp/copy.py bs=128
dd if=/dev/mem of=/tmp/dump.bin bs=1 skip=0x20010000 count=256
hexdump /tmp/dump.bin
```

`/dev/mem`のオフセットは絶対アドレスです．`bs=1`なら`skip=`と`seek=`をバイトアドレスとして指定できます．`/dev/mem`から読み出す場合は`count=`が必須です．書き込み可能範囲は現行のユーザスロット`0x20010000`～`0x20033FFF`と，`/proc/ramfiles`で確認できるRAMファイルのデータ領域です．タスク領域への書き込みは実行中のコードやデータを破壊するため，十分注意してください．

```sh
# 空きスロットと確認できた場合に限り，RAMへ書き戻す例
dd if=/tmp/dump.bin of=/dev/mem bs=1 seek=0x20010000 count=256
```

`/dev/mem`によるRAMファイルデータの直接変更は，ファイル名・長さなどの管理情報を更新しません．通常のファイルの作成・転送には`of=/tmp/名前`を使用してください．現在のRAMファイルは1ファイル最大1024バイトです．ファイル領域の固定配置，2 KiB化，SHMは今後の改造案であり，この版では未実装です．

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
dd if=/dev/mem bs=1 skip=0x20010000 count=256 | nc -N 192.168.4.2 9000
```

```sh
# Linux側：受信サイズと内容を確認
wc -c dump.bin
hexdump -C dump.bin
```

## Linux → PicoNIX：RAMへ転送

```sh
# PicoNIX側：書き込み先は事前に確認した安全な領域に変更すること
nc -N -l 9000 | dd if=- of=/dev/mem bs=1 seek=0x20010000 count=256
```

```sh
# Linux側：ちょうど256バイトを送信する例
head -c 256 data.bin | nc -N 192.168.4.1 9000
```

転送量・受信の終了条件・転送先が空きスロットであることを確認してください．現行の`/dev/mem`は任意のRAMへの無制限書き込みを提供するものではありません．Pico W以外の通常のPicoではWi-Fi経由の`nc`転送は利用できません．
