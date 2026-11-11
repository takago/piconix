# `tictactoe`：三目並べ

`tictactoe`はPLAYERとCOMが交互に着手する3×3の三目並べです．空きマスの縦・横・斜めに3個並べると勝ちで，盤面が埋まると引分けです．COMは全合法手を探索するため，COMは最善手を選び，PLAYERが先手・後手のどちらでも負けないように応手します．

```sh
tictactoe          # PLAYER先手（X）
tictactoe -c       # COM先手（X），PLAYER後手（O）
```

| キー | 操作 |
|---|---|
| 矢印キー，`h`／`j`／`k`／`l` | マスの選択（左／下／上／右） |
| SPACE | 選択した空きマスに自分の記号を置く |
| `r` | 先手設定を維持して再対局 |
| `q` | 終了 |

引数なしではPLAYERがX（先手）でCOMがO（後手），`-c`ではPLAYERがO（後手）でCOMがX（先手）です．座標は横`a`〜`c`，縦`1`〜`3`です．直近の着手は`[X]`または`[O]`，選択中のマスは反転表示です．着手数と`Invalid moves: PLAYER n  COM 0`を表示し，占有済みのマスを選択した場合のみ不正着手を加算します．合法な着手後にCOMは約400ms後に応手します．

MCPでは**`execute_command`を使用せず**，常駐PTYの`terminal_send`でシェルから起動して`terminal_read`で画面を読み取ってください．`terminal_send({"key":"SPACE"})`で着手し，送信直後にCOMの着手が完了しているとは限らないため，再度`terminal_read`で`Turn: PLAYER`と盤面を確認してください．

[コマンド一覧](README.md) ｜ [English](../../../en/reference/commands/TICTACTOE.md)
