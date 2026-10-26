# 開発環境統合の検証結果

検証日: 2026-10-17

## 移行元と移行先

- 移行元: `V:\2_robotics\5_library\Nbed_dev`
- 移行先: `V:\2_robotics\5_library\Nbed`
- Nbed の基点: `25d19b219cb51189a9f9642fe2d944e23e64b3a7`
- 作業ブランチ: `codex/integrate-development-environment`
- origin: `https://github.com/nnctroboticsclub/Nbed.git`

ライブラリの Git 履歴を複製し、独立した `.git` を持つ作業フォルダを作成しました。
サブモジュールはありません。旧フォルダ内のファイルは変更していません。
元の7つのサンプルは、移行後のコードとのバイト単位の一致を確認しました。

## 確認結果

PlatformIO Core 6.2.0 で確認しました。

| 確認項目 | 結果 |
| --- | --- |
| 既存の7環境 + `f4-blink` | 全8環境のビルド成功 |
| 環境別の成果物 | 全8環境で `firmware.elf` と `firmware.bin` の生成を確認 |
| 引数なしの `pio run` | 既定の `f4-blink` のみビルド成功 |
| サンプルのソース選択 | 各環境につき対応する `main.cc` だけを選択 |
| 開発するライブラリの参照先 | 全環境でこの作業フォルダの `src/f4/` の13ファイルを参照 |
| `NBED_NO_DEFAULT_SERIAL` | 総合テスト・UARTテストだけでライブラリ本体にも適用 |
| 通常ビルド時の clangd 情報 | 環境別・統合版の生成成功。統合版に全8サンプルを確認 |
| `pio run -e f4-blink -t compiledb` | ビルド情報の生成・統合成功 |
| 配布パッケージ作成 | `pio pkg pack` 成功。許可したライブラリ用ファイル49件のみを収録 |
| 別プロジェクトからの利用 | パッケージをインストールし、LED サンプルのビルド成功 |
| 差分の空白検査 | `git diff --check` 成功 |

STM32CubeF4 の `stm32f4xx_hal_flash_ex.c` では、未使用の `Banks` 引数に関する警告が
出ますが、ビルドは成功しています。

確認用ログ・パッケージ・別プロジェクトは `.cache/` 以下にあり、Git の管理対象から外しています。

## 検証の範囲

実機への書込みと、LED・UART・CAN などの実機動作は確認していません。

## clangd の標準ヘッダ検索の修正

通常ビルドで生成したデータベースには `arm-none-eabi-g++` というコマンド名だけが記録され、
VS Code の `PATH` にそのコンパイラがない環境で `chrono` が見つからない問題がありました。
生成後に PlatformIO の検索パスでコンパイラを解決し、環境別・統合版の両方に絶対パスを
保存するよう修正しました。既存の `--query-driver` 設定はそのまま使用できます。

VS Code が使用中の clangd 23.1.0 と同じ起動設定で確認しました。

- 修正前の `examples/f4-blink/main.cc`: `chrono` 未検出を再現、チェックエラー3件。
- 通常ビルドと `compiledb` による再生成の成功を確認。
- 修正後の `examples/f4-blink/main.cc` と `src/f4/system.cc`: ともにチェックエラー0件。
- 生成されたデータベースの全エントリが、実在するコンパイラの絶対パスを参照することを確認。
- `pio run` の成功を確認。

## DB の監視と改良版スクリプトの取り込み

yamamoto-mdc の開発用 `extra_script.py` を参考に、DB 読込みの検証、パスの正規化、
ソースファイル順の安定化、空の統合結果による上書き防止を取り込みました。
別の PlatformIO を起動する処理は採用せず、同じ SCons プロセスで生成します。
Nbed 直下のライブラリ用 `extra_script.py` は変更せず、`scripts/compiledb.py` に反映しました。

clangd の参照先を、監視対象のプロジェクト直下の `compile_commands.json` に変更しました。
環境別と従来の統合版は `build/compiledb/` にも保存します。
VS Code は DB 変更時の自動再起動を有効にしています。設定反映には一度ウィンドウの再読み込みが必要です。

- 通常ビルドと明示的な `compiledb` の実行成功を確認。
- 同じ設定でのビルド・再生成では、ルート DB の内容と更新日時が変わらないことを確認。
- UART 環境への切替で、ライブラリ本体の全13コマンドに `NBED_NO_DEFAULT_SERIAL` が反映されることを確認。
- 上記の DB 更新で、Windows のファイル監視にルート DB の変更イベントが届くことを確認。
- ルート DB を使った clangd チェックで、LED サンプルのエラー0件を確認。

VS Code 拡張による自動再起動は設定と監視先の実装を照合しました。
ウィンドウの再読み込み後の実際の VS Code 上の再起動動作は、この検証では未確認です。

## 通常プロジェクト・ライブラリ開発プロジェクトへの汎用化

`scripts/compiledb.py` を単独でコピーできる形に整理しました。
`build/` の固定参照を外し、PlatformIO の `PROJECT_BUILD_DIR` を使用します。
プロジェクト構成の識別情報をキャッシュに保存し、他環境のコンパイラでキャッシュを
再解釈しないようにしました。JSONの公開はOSのプロセス間ロックで直列化します。

Windows / Python 3.11 / PlatformIO Core 6.2.0 / SCons 4.11.1 での検証:

| 検証 | 結果 |
| --- | --- |
| 単体テスト | 22件成功 |
| 実際のPlatformIOを起動する統合テスト | 8件成功 |
| Nbedの全8環境 | ビルド成功、環境別と統合DBの生成成功 |
| clangdによるLEDサンプルの解析 | エラー0件 |

統合テストでは、nativeプラットフォームを使い、次を確認しました。

- `src/`・`lib/` の通常構成と、`src/`・`include/`・`examples/`・自己参照ライブラリの構成。
- プロジェクトパスの空白、日本語パスでのコンパイルDB生成。
- ビルド先省略時の `.pio/build`、空白を含む独自の相対パス、プロジェクト外の絶対パス。
- 通常ビルド、`compiledb`、`buildprog`、`upload`、デバッグ用ビルドターゲット。
- 内容が同じDBの更新日時を保持すること、クリーンでルートDBを変更しないこと。
- 環境切替時の優先順位と、2プロセスによる公開後も両環境のソースを保持すること。

nativeの書込みターゲットは短いサンプルプログラムの実行で確認し、実機にはアクセスしていません。
テストのログは `.cache/portable-final-unit.log`、`.cache/portable-final-integration.log`、
`.cache/portable-debug-integration.log`、`.cache/portable-final-all-envs.log`、
`.cache/portable-final-clangd.log` にあります。

導入方法・必要な環境・未検証範囲は [スクリプトの説明](../scripts/README.md)を参照してください。
