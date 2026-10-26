# nbed 開発ガイド

## フォルダ構成

```text
nbed/
├── library.json          # 配布するライブラリの定義と logger 依存
├── include/              # 公開ヘッダと HAL 設定
├── src/                  # ライブラリ本体
├── linker/               # リンカスクリプト
├── extra_script.py       # 利用側にも適用するライブラリのビルド設定
├── platformio.ini        # nbed 自体を開発するための PlatformIO 設定
├── examples/             # 開発・実機確認用プログラム
├── scripts/compiledb.py  # 開発用 clangd ビルド情報の生成
├── docs/                 # 開発資料
└── .vscode/              # 共有するエディタ設定・タスク
```

`src/` はライブラリ専用です。アプリケーションの `main()` は `examples/` に置きます。
`platformio.ini` の `src_dir = examples` と環境ごとのソースフィルタにより、
選んだサンプルだけをアプリケーションとしてビルドします。

`lib_deps = nbed=symlink://.` は、このフォルダ自体を nbed ライブラリとして参照する指定です。
コピーを作らないため、`include/`・`src/`・`extra_script.py` の変更が直接反映されます。
Windows の管理者権限や、OS のシンボリックリンク作成権限は必要ありません。
logger は `library.json` の依存関係から自動的にインストールされます。

`extra_script.py` の `Import`・`DefaultEnvironment` と、インポートされる環境変数は
PlatformIO/SCons が実行時に提供します。通常のPythonとして解析するPylanceには見えないため、
該当箇所だけ `pyright: ignore[reportUndefinedVariable]` を付けています。
ビルド処理や、それ以外のPythonコードの診断は変更しません。

## 準備と最初のビルド

VS Code + PlatformIO IDE、または PlatformIO Core を使います。
このフォルダを開いて実行してください。初回は依存パッケージの取得に通信が必要です。

```sh
pio run
pio run -e f4-blink -t upload
pio device monitor -e f4-blink
```

既定の `f4-blink` は基板上の PA5 LED を 500 ms ごとに点灯・消灯します。
書込みには ST-LINK を使います。シリアルモニターは 921600 bps です。
この LED サンプル自体はシリアル出力しません。

開発環境は、移行元と同じ次のバージョンに固定しています。

| 項目 | バージョン・設定 |
| --- | --- |
| ボード | NUCLEO-F446RE / NUCLEO-F303K8 / NUCLEO-F767ZI |
| Platform | ststm32 20.0.0 |
| STM32CubeF4 | 1.28.3 |
| STM32CubeF3 | 1.11.5 |
| STM32CubeF7 | 1.17.2 |
| GCC パッケージ | 1.140201.0 |
| logger | v0.3.0 |

## サンプルと実機確認

| PlatformIO 環境名 | 内容 |
| --- | --- |
| `f4-blink` | 基板上 LED の点滅。既定の環境 |
| `00-f4-all-test` | 各機能の総合確認 |
| `01-f4-digital-test` | LED・ユーザーボタン |
| `02-f4-analog-test` | PA0 のアナログ入力 |
| `03-f4-pwm-test` | PB4 の PWM 出力 |
| `04-f4-uart-test` | USART2 の受信データを返信 |
| `05-f4-can-test` | CAN1 から CAN2 への送受信 |
| `06-f4-nbed-example` | 機能別の使用例。初期状態では `CanExample()` を実行 |
| `f3-blink` | NUCLEO-F303K8 の PB3 LED 点滅 |
| `f3-all-test` | F303K8 の全機能・CAN 内部ループバック |
| `f3-pwm-test` | F303K8 の TIM15（2出力）/ TIM16 / TIM17 の PWM 波形確認 |
| `f4-spi-test` | F446RE の SPI1 ループバック（D11–D12 を接続） |
| `f3-spi-test` | F303K8 の SPI1 ループバック（D11–D12 を接続） |
| `f7-blink` | F767ZI の PB0 LED・既定 USART3 コンソール |
| `f7-all-test` | F767ZI の各機能・CAN1 内部ループバック |
| `f7-spi-test` | F767ZI の SPI1 ループバック（PB5–PA6 を接続） |
| `f3-pwm-complementary` | F303K8 の TIM15 CH1/CH1N（PA2/PA1） |
| `f4-pwm-complementary` | F446RE の TIM1 CH1/CH1N（PA8/PA7） |
| `f7-pwm-complementary` | F767ZI の TIM1 CH1/CH1N（PE9/PE8） |
| `f3-exti-test` | F303K8 の PA0 入力・EXTI 累計・コールバック |
| `f4-exti-test` | F446RE の PA0 入力・EXTI 累計・コールバック |
| `f7-exti-test` | F767ZI の PA0 入力・EXTI 累計・コールバック |

F303K8 の配線・クロック・ピン表は [F303K8 ガイド](f303k8.md)を参照してください。
F767ZI の設定・ピン表・検証条件は [F767ZI ガイド](f767zi.md)を参照してください。

```sh
pio run -e 04-f4-uart-test
pio run -e 04-f4-uart-test -t upload
pio device monitor -e 04-f4-uart-test
```

UART・CAN・アナログ入力などの例には、それぞれに合う配線と周辺機器が必要です。
サンプルのピン指定と [ライブラリガイド](../README.md)を確認してから書き込んでください。
`00-f4-all-test` と `04-f4-uart-test` の `NBED_NO_DEFAULT_SERIAL` は移行元から引き継いでいます。

全22環境をビルドするには、既定環境を上書きして実行します。

```sh
pio run -e f4-blink -e 00-f4-all-test -e 01-f4-digital-test -e 02-f4-analog-test -e 03-f4-pwm-test -e 04-f4-uart-test -e 05-f4-can-test -e 06-f4-nbed-example -e f3-blink -e f3-all-test -e f3-pwm-test -e f4-spi-test -e f3-spi-test -e f7-blink -e f7-all-test -e f7-spi-test -e f3-pwm-complementary -e f4-pwm-complementary -e f7-pwm-complementary -e f3-exti-test -e f4-exti-test -e f7-exti-test
```

これらは実機確認用プログラムです。ビルドの成功だけでは実機の動作確認は完了しません。

新しい例は `examples/<環境名>/main.cc` に作成し、`platformio.ini` に
`[env:<環境名>]` と `build_src_filter = -<*> +<${this.__env__}/>` を追加します。
このソースフィルタが同名のフォルダを選択します。

## ホスト上の回帰テスト

```sh
python -m unittest discover -s tests -v
```

コンパイルDBのテスト、MCU別ビルド設定、F3/F4/F7のドライバロジックを確認します。
ドライバテストにはホスト用 C++23 コンパイラが必要です。既定は `g++`、別のコンパイラは
環境変数 `CXX` で指定できます。コンパイラがない場合は、そのテストだけスキップされます。
Windows のアプリ制御が生成した実行ファイルを拒否する場合も、理由を表示して実行だけ
スキップします。数値計算の `static_assert` はコンパイル時に検証されます。
EXTI では全16ラインのハンドラー、同番号の重複、共有 IRQ と優先度、同時保留、停止・破棄・再開、
コールバックと保留ビット、割り込み許可状態の復元を確認します。[EXTI ガイド](exti.md)に使用例があります。
HAL を小さな代替実装に置き換え、全ピン値の判定、GPIO、タイマの競合、PWM/I²C の
計算、計時の割り込みと周回、UART のまとめ送り・分割・エラー、SPI のモード・分周・
ピン設定・バッファ境界・所有権・送受信とエラーを確認します。
F3 の PWM では TIM15/16/17 の全ピン候補と AF、チャンネル数・重複予約、周波数共有、
APB の分周、デューティ比・パルス幅・周波数変更、HAL 失敗後の再初期化も確認します。
F3/F4/F7 の出力設定テストは、N 出力の全ピン候補、ペアの所有権、極性・アイドル・デッドタイムの
HAL 設定値、設定の競合、開始失敗時の解放と別チャンネルを保った停止も確認します。
相補出力の実機確認用サンプルと設定例は [PWM 出力設定ガイド](pwm.md)にあります。
F7 ではクロック・Flash 待機数・OverDrive、I²C の HSI16 タイミングと所有権、ADC1/ADC3 の
独立したチャンネルキャッシュ、DWT のアンロック、USART3/6 と F7 固有の SPI ピンも確認します。
実機での波形・通信確認は別途必要です。

## VS Code と clangd

整形・命名・組み込み向けの例外と、他プロジェクトへの移行手順は
[C++ の整形・診断設定ガイド](code-style.md)にまとめています。

このDB生成処理は他のPlatformIOプロジェクトにもコピーして使えます。
通常の開発環境・ライブラリ開発環境での導入方法と対応範囲は
[再利用スクリプトの説明](../scripts/README.md)にまとめています。

通常のビルドで、環境別のコンパイル情報を `build/compiledb/<環境名>/` に保存し、
`build/compiledb/compile_commands.json` にまとめます。同じ内容をプロジェクト直下の
`compile_commands.json` にも出力し、clangd は直下の DB を参照します。
同じライブラリソースについては、最後にビルドした環境の設定を優先します。
MCU 別のソースフィルタにより `src/f3/`、`src/f4/`、`src/f7/` は対応する環境だけでコンパイルされ、
それぞれの CPU マクロ・HAL パスが DB に残ります。
初回は一度ビルドしてから補完・診断を利用してください。

`files.watcherExclude` では大量の生成物がある `build/**` を監視対象から外しています。
clangd が読む DB はこの除外の外側に置き、VS Code の clangd 拡張が監視する
標準のパス `compile_commands.json` を使います。
`clangd.onConfigChanged = restart` と `clangd.onConfigChangedForceEnable = true` により、
DB 変更後に自動で再起動し、開いているファイルの診断も更新します。
DB は内容が変わった場合だけ一時ファイルから置き換えるため、変更のないビルドでは
自動再起動が発生せず、書込み途中の JSON を読み込むことも防ぎます。
この設定に切り替えた直後は、一度 VS Code の `Developer: Reload Window` を実行してください。

DB 統合処理は yamamoto-mdc の改良版 `extra_script.py` の方式を取り入れ、
パスの正規化、安定した並び順、不正な JSON の読み飛ばし、空の結果による上書き防止を
加えています。生成は同じ SCons プロセスで行い、別の PlatformIO の起動は行いません。

生成スクリプトは、PlatformIO のツールチェーン検索パスからコンパイラの絶対パスを
記録します。VS Code の環境変数 `PATH` に ARM コンパイラがなくても、clangd が
`--query-driver` を通して C++ 標準ヘッダの場所を取得できるようにするためです。
ユーザー名などの実パスは生成物だけに記録し、共有設定には固定しません。

`chrono` などが見つからない場合は、VS Code の「nbed: clangd用ビルド情報を更新（既定環境）」タスク、
または次のコマンドでビルド情報を再生成します。

```sh
pio run -e f4-blink -t compiledb
```

更新後も診断が残る場合は、コマンドパレットから `clangd: Restart language server` を実行します。

VS Code のタスクは `V:/7_script/vscode/projects/robotics/.vscode/tasks.json` を基に統合しています。
`Ctrl+Shift+B` は `platformio.ini` の `default_envs` をビルドし、現在の対象は `f4-blink` です。
通常ビルド・書き込み・ビルド情報の更新・通常クリーンに加え、次のタスクを使えます。

- 「nbed: 生成物とキャッシュを全削除」: 全環境の `build/`・`.cache/`・旧 `.pio/build/` とルートの生成DBを削除。
- 「nbed: 完全再ビルド（既定環境）」: 全削除が成功してから既定環境をビルド。
- 「nbed: シリアルモニター（LED点滅）」: 従来どおり `f4-blink` のシリアル設定で接続。

全削除には `.vscode/clean.py` を使い、`.pio/libdeps/`、`include/`、`src/`、`examples/` は保持します。
全環境のビルド結果を消しますが、再ビルドするのは既定環境だけです。
PlatformIO標準の実行ファイルを直接使うため、システムのPATHやターミナルの種類に依存しません。
Coreの保存先を変えたPCでは、タスクの実行ファイルのパスを合わせてください。
PlatformIO のターミナルからも同じ操作を実行できます。

## ライブラリとしての配布

`library.json` の `export.include` で、配布物を `include/`、`src/`、`linker/`、
`extra_script.py`、`library.json`、`README.md`、`LICENSE` に限定しています。
開発用の `platformio.ini`、エディタ設定、サンプル、生成物はライブラリパッケージに含めません。
Git リポジトリには開発環境も保存されます。

```sh
pio pkg pack . --output build/nbed.tar.gz
```

ライブラリ利用側の設定は [README](../README.md#platformio-に残す設定)を参照してください。
利用側で開発用の `scripts/compiledb.py` を指定する必要はありません。

## リポジトリ名の変更

2026年10月23日に、リポジトリ名とライブラリ名を `Nbed` から `nbed` に変更しました。
GitHub 上の既存リポジトリをリネームし、コミット履歴・ブランチ・タグ・リリースを引き継いでいます。
利用側の `lib_deps` と既存の clone の接続先は `https://github.com/nnctroboticsclub/nbed.git` に更新してください。
過去のリリース文書は GitHub の公開本文を基に、見出しをリリース名へ揃えています。
当時の製品名・設定・検証結果は履歴として残しています。

## Nbed-dev からの移行

ライブラリの Git 履歴を継承し、Nbed-dev の7つのプログラムを
`src/<環境名>/src/main.cc` から `examples/<環境名>/main.cc` へ移しました。
環境名とコードはそのまま引き継いでいます。
移行前の README にあった未コミットのリンク修正と、VS Code の共有設定も反映しています。

今後は、この nbed フォルダ内でライブラリ・サンプル・開発設定をまとめて変更・コミットします。
旧 Nbed-dev とその中の Nbed サブモジュールは別の作業コピーなので、自動同期はされません。

移行時の確認項目と結果は [統合の検証結果](migration-verification.md)にまとめています。

構成に使った PlatformIO の仕様は、公式の
[ローカルライブラリ参照](https://docs.platformio.org/en/stable/core/userguide/pkg/cmd_install.html#local-folder)と
[ライブラリマニフェスト](https://docs.platformio.org/page/librarymanager/config.html)を参照してください。
