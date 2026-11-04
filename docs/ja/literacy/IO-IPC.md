# 標準入出力とプロセス間通信

[学習ガイド](LEARNING-PATH.md) | [English](../../en/literacy/IO-IPC.md) | [パイプの詳細](PIPE.md)

## 目的と準備

**目的：** 標準入力（FD 0），標準出力（FD 1），標準エラー（FD 2），リダイレクト，パイプの違いを説明することです．USB接続のPicoだけで実習できます．`/tmp`のファイルは再起動すると消えます．

## 実習

```text
piconix# echo hello
piconix# echo hello > /tmp/hello.txt
piconix# cat /tmp/hello.txt
piconix# cat -n /tmp/hello.txt
piconix# cp /tmp/hello.txt /tmp/backup.txt
piconix# cat /tmp/backup.txt
piconix# echo >> /tmp/hello.txt
piconix# cat -n /tmp/hello.txt
piconix# echo hello | cat
piconix# echo hello | cat -n
piconix# echo hello | grep hello | cat
piconix# lsof
```

`cat`は指定したファイルを表示します．ファイルを省略すると標準入力から読み込みます．`cat -n`は空行も含めて各行に1から始まる番号とタブを付けて表示します．`echo >> /tmp/hello.txt`は空行をファイルの末尾に追加します．その前後の`cat -n`を比べてください．行番号は表示時に付けるだけで，ファイルの内容は変更しません．パイプから入力しても番号は付けられます．

`>`はコマンドの標準出力をファイルに向けます．`|`はコマンド間でバイト列を受け渡します．PicoNIXのシェルではパイプは最大3段で，パイプとリダイレクトの併用には制約があります．Linuxの一般的なシェルと完全に同じではありません．

`cp SRC DST`はRAMファイルを複製します．`/tmp`と`/etc`の直下にある通常ファイルのみ扱い，既存のコピー先は上書きします．同じファイルを指定するとエラーになります．どちらのディレクトリもRAM上にあるため，コピーは再起動すると失われます．書き込み中に失敗するとコピー先が不完全になることがあります．

## 観察・考察

- `cat /tmp/hello.txt`と`echo hello | cat`では，`cat`の入力元はどのように異なりますか．
- `cat -n`で空行にも番号が付く理由を説明できますか．
- ファイルは後から読めますが，パイプの内容はどうなりますか．
- `lsof`で見えるFDは実行時点のものです．終了済みの`echo`が表示されない理由を考えてください．

## 発展

[パイプの実装と制約](PIPE.md)で`pipe()`，`dup2()`，EOFを確認し，[プロセス](PROCESSES.md)に進んでください．

---

## 学習ナビゲーション

| ← 前へ | ↑ 目次へ | 次へ → |
| :--- | :---: | ---: |
| [基本操作](BASICS.md) | [初級コース](BEGINNER.md) | [プロセス](PROCESSES.md) |
