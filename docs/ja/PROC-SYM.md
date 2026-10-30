# シンボル情報の階層化

ビルド時にファームウェアFlashへ埋め込んだシンボル情報を，次の読み取り専用の仮想ファイルとして公開します．

- `/proc/sym/kernel`：カーネルのシンボル（絶対アドレス）．
- `/proc/sym/userland/sh`：`sh` のシンボル（タスクスロット相対オフセット）．
- `/proc/sym/userland/<プログラム名>`：登録済みアプリのシンボル．

`mem -s` は `kernel` と `/bin` に登録されたアプリ名を一覧表示します（シンボルを持たないアプリも含まれる場合があります）．`mem -s kernel` と `mem -s sh` は各階層のファイルを利用します．カタログの各行の形式 `app<TAB>address<TAB>type<TAB>name` は変えません．

`ls /proc/sym` および `ls /proc/sym/userland` に対応しています．後者は登録アプリを列挙するため，シンボルがないアプリも表示されることがあります．

## 確認

```
ls /proc/sym
ls /proc/sym/userland
cat /proc/sym/kernel
cat /proc/sym/userland/sh
mem -s
mem -s kernel
mem -s sh
```

**注意**：今回の変更はホスト上の静的テストまで確認しています．Pico SDKによるビルドと実機上の読み出しは未検証です．
