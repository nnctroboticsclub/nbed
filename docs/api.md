# 日本語 API リファレンス

利用するファイルで `#include "nbed.h"` を指定します。クラスや関数にカーソルを置くと、
対応する Doxygen コメントをエディターで確認できます。引数の単位、戻り値、
初期化の順番、割り込みで呼ばれる関数の注意点を記載しています。

## HTML を生成する

Doxygen を起動できる環境で、nbed のリポジトリ直下から実行します。
初回は出力先の親ディレクトリ `build` を作成します。設定は Doxygen 1.18.0 で確認しています。

```powershell
New-Item -ItemType Directory -Path build -Force | Out-Null
doxygen Doxyfile
```

macOS／Linux では `mkdir -p build` を実行してから Doxygen を起動します。

`build/doxygen/html/index.html` をブラウザーで開きます。名前空間には `f3`、`f4`、`f7` の
各ボード用クラスと、`interface` の共通 API があります。`PinName` と `SleepFor()` の表示対象は
`Doxyfile` の `PREDEFINED` で選びます。既定は `STM32F446xx` です。F303K8 は
`STM32F303x8`、F767ZI は `STM32F767xx` を指定して再生成できます。

配線を選ぶときは [ボード別の対応ピン](pinout.md)、最初のプログラムを書くときは
[LED 点滅の例](../README.md#最初に動かす)を参照してください。
