# `top` — CPU負荷とプロセスの監視

`top [seconds]`は，`/proc/cpu`，`/proc/tasks`，`/proc/uptime`，`/proc/context_switches`を参照して，約1秒ごとにCPU負荷とプロセス情報を更新する端末アプリです．`seconds`で更新間隔（1〜60秒）を変更できます．`q`で終了します．

```sh
top
top 2
```

表示項目は，起動時間，システム全体のCPU負荷（Idle，Busy，Console，Task，Net），タスク数，コンテキストスイッチ累計と，`/proc/tasks`のPID，NAME，STATE，TTY，SLOT，MEMORY，SP，PPIDです．画面は幅79列まで使用し，変更された行だけ更新します．80列以上のANSI対応端末を想定しています．

**制限：** CPU使用率は`/proc/cpu`の直近の完了済み1秒サンプルから取得した**概算値**です．プロセスごとのCPU使用率は取得できません．開始直後の計測前はCPU値を表示できません．`top`自身もタスク枠を1つ使用します．最大6タスクのうち空き枠がない場合は起動できません．端末設定は終了時に復元します．

MCP経由で利用する場合は，常駐PTYの対話操作を使い，長時間動作する`top`を単発の`execute_command`で実行しないでください．
