## toio with NimBLE

M5シリーズとtoioを接続して操作するサンプルです。
Visual Studio Codeにplatformioをインストールしてあれば、
projectとして開いてからBuild,uploadするだけで実行できます。

M5シリーズはM5Unifiedに対応していてボタン3つとディスプレイがあれば動くはずです。
Core,Core2で動作確認済み。

### 操作方法


https://twitter.com/coppercele/status/1740823500485783759


M5シリーズの電源を入れるとtoio接続待機モードに入ります。
toioの電源を入れると自動で認識し、画面下に数を表示します。
左ボタンを押すと接続、toio1個あたり5秒ほどかかります。

接続出来たら中ボタンで直進、右ボタンで右回転
すべてのtoioが同時に動きます。

プレイマットがあるならば左ボタンを押すと整列します
(自分は2個しか持ってないので未確認だがバグあるとの報告アリ)

プレイマットの上にいるならば1秒ごとにtoioの座標をディスプレイに表示します。

### 複数接続するときの注意

NimBLEはデフォルトでは２台までしか同時接続できないようになっています。
3台以上接続する場合は

projectRoot/libdeps/NimBLE-Arduino/src/nimconfig.h

内の

#ifndef CONFIG_BT_NIMBLE_MAX_CONNECTIONS
#define CONFIG_BT_NIMBLE_MAX_CONNECTIONS 3
#endif

を10に変更してください。
NimBLEの制限により最大9台接続できます。

https://x.com/mongonta555/status/1741075503791161490

タカオ(Takao) @mongonta555 さん情報ありがとうございます。

7台までは接続確認済みです。

https://x.com/okimoku1/status/1741180149645836690?s=20

おきもく @okimoku1 さん確認ありがとうございます