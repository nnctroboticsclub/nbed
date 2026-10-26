# C++ の整形・診断設定と移行ガイド

nbed は Google C++ Style Guide を基準にし、STM32 のレジスター、HAL、DMA、
割り込み、newlib に接する箇所には用途を限定した例外を設けています。
整形と静的診断は別の処理です。整形だけでは命名や関数の複雑さは修正されません。

## ファイルの役割と対応バージョン

| ファイル | 役割 |
| --- | --- |
| [.clang-format](../.clang-format) | 空白・改行・include 順など、保存時の C/C++ 整形 |
| [.clangd](../.clangd) | コンパイルDB、静的診断、命名、補完、インレイヒント |
| [.vscode/settings.json](../.vscode/settings.json) | VS Code の整形拡張、保存時整形、コンパイラへの問い合わせ、DB の監視 |
| [scripts/compiledb.py](../scripts/compiledb.py) | PlatformIO の実際のビルド設定から DB を生成 |
| `compile_commands.json` | 各ファイルの実際のコンパイラ、CPU、マクロ、include、言語規格。自動生成・Git 管理外 |

確認に使用したバージョンは **clangd 23.1.0** です。整形にも同じ実行ファイルに
内蔵された clang-format を、エディターと同じ LSP の整形要求で使用しました。
独立した `clang-format` コマンドをインストールしなくても、clangd 拡張から整形できます。

`PackArguments` / `PackParameters` など clang-format 23 の設定を使っています。
古い整形ツールとの互換性はありません。チームでは同じ clangd / clang-format の
バージョンを使用し、更新時には設定の読み込みと整形差分を確認してください。
`BasedOnStyle: Google` から継承する既定値も、バージョン更新で変わる可能性があります。

## .clang-format

設定に記述しない項目は、使用中のバージョンの Google プリセットに従います。
以下は明示的に設定している項目です。整形時の `Standard` は構文の解釈用であり、
実際のビルドを C++23 にする設定は PlatformIO 側とコンパイルDBです。

### 基準・インデント・空白

| 項目 | 値 | 意味 |
| --- | --- | --- |
| `BasedOnStyle` / `Language` / `Standard` | `Google` / `Cpp` / `c++23` | Google を基準に C++23 の構文を整形 |
| `AccessModifierOffset` | `-1` | `public:` 等を通常のメンバーより 1 桁左へ配置 |
| `ColumnLimit` | `80` | 通常の行幅。長い文字列、維持するコメント、整形無効区間などは超える場合がある |
| `IndentWidth` / `TabWidth` / `UseTab` | `2` / `2` / `Never` | 2 スペースで字下げし、タブを挿入しない |
| `ContinuationIndentWidth` | `4` | 継続行の基本の字下げ幅 |
| `IndentCaseLabels` | `true` | `switch` 内の `case` を字下げ |
| `IndentPPDirectives` | `None` | プリプロセッサのディレクティブを字下げしない |
| `NamespaceIndentation` | `None` | 名前空間の内部では追加の字下げをしない |
| `AlignAfterOpenBracket` | `true` | 引数の折り返し位置を開き括弧の後の要素に揃える |
| `BreakBeforeBraces` | `Attach` | 開始ブレースを宣言・制御文と同じ行へ配置 |
| `Cpp11BracedListStyle` | `FunctionCall` | 波括弧の初期化リストを関数の引数に近い形で配置 |
| `DerivePointerAlignment` / `PointerAlignment` | `false` / `Left` | 既存コードから推定せず、`int* ptr` のように型側へ揃える |
| `SpaceBeforeParens` | `ControlStatements` | `if (` には空白を入れ、通常の `Read(` には入れない |
| `SpacesBeforeTrailingComments` | `2` | 行末コメントの前に最低 2 スペース |
| `SpacesInAngles` / `SpacesInParens` | `Never` / `Never` | 山括弧・丸括弧の内側に余分な空白を入れない |

### 改行・引数・短い関数

| 項目 | 値 | 意味 |
| --- | --- | --- |
| `AllowShortBlocksOnASingleLine` | `Empty` | 空の通常ブロックだけ 1 行配置を許可 |
| `AllowShortFunctionsOnASingleLine` | `All` | 行幅に収まる短い関数は 1 行を許可。Google の配置 |
| `AllowShortLambdasOnASingleLine` | `All` | 短いラムダは 1 行を許可 |
| `AllowShortIfStatementsOnASingleLine` | `Never` | 短い分岐でも本体を別行へ配置 |
| `AllowShortLoopsOnASingleLine` | `false` | 短いループでも本体を別行へ配置 |
| `BreakAfterReturnType` | `Automatic` | 戻り値の後の改行を行幅等から判断 |
| `BreakTemplateDeclarations` | `Yes` | テンプレート宣言の次を改行 |
| `PackArguments.BinPack` | `BinPack` | 収まる呼び出し引数は同じ行へ詰める |
| `PackParameters.BinPack` | `BinPack` | 収まる宣言の引数は同じ行へ詰める |
| `PackConstructorInitializers` | `NextLine` | 初期化子が長い場合、次の行にまとめる配置も試す |
| `SeparateDefinitionBlocks` | `Always` | 関数・型などの定義ブロックの間に空行を置く |

ブレースの不足は `.clangd` の `readability-braces-around-statements` が指摘します。
フォーマッターによるブレースの自動挿入や削除は設定していません。

### include の分類

`IncludeBlocks: Regroup` により分類ごとに空行を入れ、
`SortIncludes.Enabled: true` と `IgnoreCase: false` で大文字・小文字を区別して並べます。

| 順番 | 分類 | 例 |
| --- | --- | --- |
| 先頭 | 実装に対応するヘッダー。自動的に優先度 0 | `f4/can.cc` の `"f4/can.h"` |
| 優先度 1 | `.h` で終わる山括弧のヘッダー | `<stdint.h>` |
| 優先度 2 | その他の山括弧、および `<ext/… .h>` | `<array>`、`<chrono>` |
| 優先度 3 | 引用符のヘッダー | `"nbed.h"`、`"stm32f4xx_hal.h"` |

`IncludeCategories.Regex` は表の分類を指定します。`IncludeIsMainRegex` は
`_test`、`-test`、`_unittest`、`-unittest` が付く実装名も対応ヘッダーの判定に含めます。
外部ライブラリと自作ライブラリの引用符 include は、同じ分類内で並びます。
プリプロセッサの条件をまたいだ移動や、依存順序を判断した並べ替えは行いません。
順序依存のある include は、必要に応じて局所的な `clang-format off/on` と理由を使います。

### コメント・空行・改行コード

| 項目 | 値 | 意味 |
| --- | --- | --- |
| `FixNamespaceComments` | `true` | 長い名前空間の閉じ括弧に名前空間コメントを補う・修正する |
| `ReflowComments` | `IndentOnly` | 日本語や表の手動改行を保ち、コメントの字下げのみ調整 |
| `KeepEmptyLines.AtStartOfFile` / `AtStartOfBlock` / `AtEndOfFile` | すべて `false` | ファイル・ブロックの端の余分な空行を除去 |
| `MaxEmptyLinesToKeep` | `1` | 連続した空行は原則 1 行まで |
| `InsertNewlineAtEOF` | `true` | 末尾の改行を確保 |
| `LineEnding` | `LF` | Windows でも整形結果を LF に統一 |

## .clangd

### ビルド情報・補完・表示

| 項目 | 値 | 意味 |
| --- | --- | --- |
| `CompileFlags.CompilationDatabase` | `.` | `.clangd` のあるプロジェクト直下の DB を参照 |
| `Index.Background` | `Build` | バックグラウンドで索引を作り、参照・定義検索を支援 |
| `Index.StandardLibrary` | `true` | 標準ライブラリも索引化 |
| `Completion.ArgumentLists` | `Delimiters` | 関数・テンプレートの補完では括弧を挿入し、仮の引数名は挿入しない |
| `Completion.HeaderInsertion` | `IWYU` | 補完時に必要な include を追加 |
| `Documentation.CommentFormat` | `Doxygen` | ホバー・補完の説明で Doxygen コメントを解釈。ソースのコメントを書き換える設定ではない |
| `Hover.ShowAKA` | `true` | 型エイリアスの実体も表示 |
| `InlayHints.Enabled` | `true` | 以下のインレイヒントを有効化 |
| `InlayHints.BlockEnd` | `true` | 長いブロックの終端を表示 |
| `InlayHints.DeducedTypes` | `true` | `auto` 等の推論された型を表示 |
| `InlayHints.DefaultArguments` | `true` | 省略された既定引数を表示 |
| `InlayHints.Designators` | `true` | 集成体の初期化に対応するフィールド名を表示 |
| `InlayHints.ParameterNames` | `true` | 呼び出しの引数名を表示 |
| `InlayHints.TypeNameLimit` | `24` | 長い型名の表示を制限 |

表示にはエディター側の対応・表示設定も必要です。インレイヒントはソースには保存されません。

CPU、マクロ、include パス、C/C++ の言語規格、例外・RTTI 等は DB に従います。
全ファイルへ `-std=gnu++23` 等を追加すると HAL の C ソースにも影響するため、
共通の `CompileFlags.Add` には重複して記述していません。

### 診断の適用範囲

`If.PathMatch` / `PathExclude` は、`.clangd` から見た相対パス全体に対する正規表現です。
Windows でも区切り文字には `/` を使います。以下の順番でフラグメントを重ねています。

| 範囲 | 設定の意図 |
| --- | --- |
| プロジェクト全体 | DB、索引、補完、表示 |
| `include/`、`src/`、`examples/`、`test/`、`tests/` の C++ | Google の命名・一般的な静的診断。HAL 設定ヘッダーは除外 |
| `include/f4/`、`src/f4/` | HAL とメモリー操作に必要な例外、長い初期化処理の許容 |
| 同じ `f4/` の内部ヘッダー・実装 | 不足 include の提案を停止。private ヘッダーの公開先指定が内部にも作用するため |
| `src/f4/system.cc`、`uart.cc` | C ランタイム・リンカーに要求される特定の名前を許可 |
| `include/stm32f4xx_hal_conf.h` | 設定用 include を維持し、単独解析の前に `stm32f4xx_hal_def.h` を読み込む |
| `include/nbed.h` | 意図的な名前空間の公開・グローバル別名を許可 |

生成物・外部依存がある `build/`、`.pio/`、`.cache/` は、上の自作 C++ の範囲に含めません。
C ファイルのコンパイラ診断は通常どおり有効です。ここでの C++ 命名規約は適用しません。

### include の診断

| 項目 | 値・対象 | 意味 |
| --- | --- | --- |
| `Diagnostics.MissingIncludes` | 通常は `Strict` | 直接 include の不足を提案 |
| `Diagnostics.UnusedIncludes` | 通常は `Strict` | 未使用の include を提案 |
| `Diagnostics.Includes.AnalyzeAngledIncludes` | `false` | 標準ライブラリ以外の山括弧 include の未使用検出を控える |
| `Diagnostics.Includes.IgnoreHeader` | `stm32… .h`、`core_cm… .h`、`bits/… .h` | HAL/CMSIS の集約 include と libstdc++ の実装内部について include 診断を抑制 |
| `f4/` 内部の `MissingIncludes` | `None` | 内部の private ヘッダー利用に対して `nbed.h` を追加する提案等を避ける |
| HAL 設定ヘッダーの `MissingIncludes` / `UnusedIncludes` | `None` / `None` | 必要な機能選択用 include を削除候補にしない |

libstdc++ の内部ヘッダーは、例えば chrono リテラルの提供元として
`<bits/chrono.h>` を直接追加する提案を避けるための例外です。
標準ライブラリを変更した場合にはこの除外も再検討してください。
include 診断の例外は、未定義の型や識別子など、コンパイラのエラーを隠しません。

既存の `IWYU pragma: private`、`export`、`keep` も公開ヘッダーの境界を表します。
これらは `NOLINT` と役割が異なるため維持しています。

### 有効にするチェック

`Diagnostics.ClangTidy.Add` は次のグループ・チェックを指定しています。

| チェック | 主な目的 |
| --- | --- |
| `bugprone-*` | ミスを生みやすい式・寿命・分岐など |
| `clang-analyzer-*` | 対応している静的解析チェックの選択 |
| `cppcoreguidelines-*` | 初期化・型・境界などの確認 |
| `google-build-namespaces` | 名前空間の使い方 |
| `google-build-using-namespace` | 広すぎる using-directive の抑制 |
| `google-explicit-constructor` | 意図しない暗黙変換を防ぐコンストラクター |
| `google-global-names-in-headers` | ヘッダーによるグローバル名前空間の汚染防止 |
| `modernize-*` | 利用できるモダン C++ の書き方 |
| `performance-*` | 不要なコピーなどの性能上の問題 |
| `readability-*` | 命名、制御文、複雑さなど |

`FastCheckFilter: Strict` により、clangd が高速と判断したチェックを実行します。
Add に書いた全チェックが必ず実行されるわけではありません。重い解析を含む完全な
品質ゲートや Google 規約の完全な検証ではなく、エディターでの実用性を優先しています。
clangd に含まれないチェックもあります。単独の clang-tidy に `.clangd` の設定が
そのまま適用されるわけではないため、CI 導入時は `.clang-tidy` を別途用意してください。

### チェックの例外

共通の `Remove` は次の 6 個です。

| チェック | 理由 |
| --- | --- |
| `cppcoreguidelines-avoid-const-or-ref-data-members` | ハードウェアを参照で共有する設計や、初期化後に変えないメンバーを許可 |
| `cppcoreguidelines-avoid-magic-numbers` | マジックナンバーの別名チェックも無効化 |
| `cppcoreguidelines-pro-type-vararg` | `printf` 等の C API を使用 |
| `modernize-use-std-print` | 裸のマイコンで `<print>` の提供・実装負荷を前提にしない |
| `modernize-use-trailing-return-type` | 通常の戻り値を先に書く Google の表記を維持 |
| `readability-magic-numbers` | 通信設定・単位換算・ピン指定・サンプル等の数値の直書きを許可 |

マジックナンバーの警告は整数・浮動小数点数ともにプロジェクト全体で無効です。
レジスター操作に限らず、通信速度、配列の要素、単位の換算係数などにも多数の警告が
出るため、値ごとの許可リストや個別の `NOLINT` を増やす運用は採用しません。
Google を基準とする命名・整形の方針は維持し、意味が分かりにくい値や繰り返し使う
設定値には、レビューで必要性を判断して `constexpr` の名前付き定数を使います。
この変更は数値リテラルの命名要求だけが対象で、型・境界等の他の診断は継続します。

`f4/` に限って追加で無効にするチェックは次のとおりです。
この範囲外のアプリケーションやインターフェースでは引き続き検出します。

| チェック | 理由 |
| --- | --- |
| `bugprone-easily-swappable-parameters` | 同型のピン番号・設定値を持つハードウェア API |
| `cppcoreguidelines-avoid-c-arrays`、`modernize-avoid-c-arrays` | HAL との C 配列の受け渡し |
| `cppcoreguidelines-avoid-non-const-global-variables` | ISR と共有する周辺機器状態 |
| `cppcoreguidelines-pro-bounds-array-to-pointer-decay` | C API への配列の受け渡し |
| `cppcoreguidelines-pro-bounds-constant-array-index` | ハードウェアのチャンネル番号等によるアクセス |
| `cppcoreguidelines-pro-bounds-pointer-arithmetic` | DMA 等のバッファ操作 |
| `cppcoreguidelines-pro-type-cstyle-cast` | 外部の HAL/CMSIS マクロに含まれる C のキャスト |
| `cppcoreguidelines-pro-type-reinterpret-cast` | 固定番地・リンカー定義アドレス |
| `performance-no-int-to-ptr` | メモリーマップドレジスター等、整数として規定された番地の利用 |

これらはファイル単位の例外なので、同じファイルの手書きコードにも作用します。
上位ロジックまで `f4/` に置かず、外部 API と接する範囲を小さく保ってください。

### リテラルの接尾辞

`readability-uppercase-literal-suffix` は有効にしたまま、
`NewSuffixes: 'U;L;UL;LU;LL;ULL;LLU;Z;UZ;ZU'` で検査する接尾辞を限定しています。
このリストとの大文字・小文字を無視した比較で一致する接尾辞だけが、指定の形へ修正する対象です。

- `0.5f` と `0.5F` は両方を許可し、どちらかへの統一を要求しません。
- `7l`、`7ll`、`7ul` 等は、数字の `1` と紛らわしい小文字の `l` を含むため警告します。
- 整数の `u` は従来どおり `U` を推奨します。`UL` / `ULL` 等の組合せも大文字です。
- C++23 のサイズ型の接尾辞は `Z` / `UZ` / `ZU`、long double は `L` を使います。

`f` と `F` は C++ 上で同じ float 型です。Google の公式の記述例も小文字の `f` を使うため、
この点を大文字にする警告は不要と判断しています。ソースの数値や型を一括変更する設定ではありません。

### 複雑さ・制御文・名前の長さ

`CheckOptions` のキーは、それぞれ次のチェック名を接頭辞に持ちます。

| チェック / オプション | 通常 | `f4/` | 意図 |
| --- | --- | --- | --- |
| `readability-function-size.LineThreshold` | 100 | 150 | 関数の行数 |
| 同 `StatementThreshold` | 50 | 100 | 文の数。HAL マクロの展開も数に影響 |
| 同 `ParameterThreshold` | 6 | 8 | 引数の数 |
| 同 `BranchThreshold` | 15 | 15 | 分岐の数 |
| 同 `NestingThreshold` | 4 | 4 | ネストの深さ |
| `readability-function-cognitive-complexity.IgnoreMacros` | `true` | 同じ | 外部マクロの構造は認知的複雑さの計算から除外 |
| `readability-braces-around-statements.ShortStatementLines` | 0 | 同じ | 短い分岐・ループにもブレースを要求 |
| `cppcoreguidelines-avoid-do-while.IgnoreMacros` | `true` | 同じ | 安全な関数風マクロの `do … while (0)` は許可 |
| `readability-simplify-boolean-expr.IgnoreMacros` | `true` | 同じ | 外部マクロ内部の論理式を変更対象にしない |
| `readability-identifier-length.MinimumVariableNameLength` | 2 | 同じ | 通常の変数名の最低文字数 |
| 同 `MinimumParameterNameLength` | 2 | 同じ | 引数名の最低文字数 |
| 同 `MinimumLoopCounterNameLength` | 1 | 同じ | `i` / `j` 等のループ変数は許可 |
| `readability-implicit-bool-conversion.AllowIntegerConditions` | `true` | 同じ | `if (mask)` のような条件式は許可 |
| 同 `AllowPointerConditions` | `true` | 同じ | `if (ptr)` は許可 |

これらは Google の必須の数値規則ではなく、このプロジェクトのレビュー目安です。
`Ignored…Names` 等、チェック側の既定の例外もあるため、最低文字数は絶対的な禁止ではありません。
条件式以外の不用意な整数・ポインターと bool の変換は引き続き診断します。

### 命名規則

以下はすべて `readability-identifier-naming.` のオプションです。
`lower_case` は `snake_case` 形式を意味する正しい設定値です。

| 対象 / キー | 規則 | 例 |
| --- | --- | --- |
| `ClassCase`、`StructCase`、`UnionCase`、`EnumCase` | `CamelCase` | `CanMessage` |
| `TypeAliasCase`、`TypedefCase` | `CamelCase` | `CanRxCallback` |
| `TemplateParameterCase` | `CamelCase` | `ElementType`、`T` |
| `ValueTemplateParameterCase` | `lower_case` | `buffer_size` |
| `ValueTemplateParameterIgnoredRegexp` | `k` + 大文字から始まる名前も許可 | `kBufferSize` |
| `FunctionCase` | `CamelCase` | `ReadValue()` |
| `NamespaceCase` | `lower_case` | `nkrc::nbed::interface` |
| `VariableCase`、`GlobalVariableCase` | `lower_case` | `received_count` |
| `ParameterCase`、`ConstantParameterCase` | `lower_case` | `input_value` |
| `MemberCase` | `lower_case` | struct の `message_size` |
| `PrivateMemberCase` / `Suffix`、`ProtectedMemberCase` / `Suffix` | `lower_case` + `_` | `message_size_` |
| `ClassMemberCase` / `Suffix` | `lower_case` + `_` | static メンバーの `instance_count_` |
| `GlobalConstantCase` / `Prefix`、`StaticConstantCase` / `Prefix` | `CamelCase` + 接頭辞 `k` | `kDefaultRate` |
| `ClassConstantCase` / `Prefix` | 同じ | static const の `kChannelCount` |
| `ConstexprVariableCase` / `Prefix` | 同じ | `kBufferSize` |
| `EnumConstantCase` / `Prefix` | 同じ | `kStandard` |
| `LocalConstantCase` | `lower_case` | 実行時に決まる const の `received_size` |
| `LocalConstantIgnoredRegexp` | `k` + 大文字から始まる名前も許可 | 局所的な定数の `kLimit` |
| `MacroDefinitionCase` | `UPPER_CASE` | `NBED_NO_GLOBAL_NAMES` |

class は private/protected のデータを持ち、struct は public のデータを持つ設計を
想定しています。ツールは public な class データと struct データをこの設定だけで
完全には区別できません。また `ClassMember` は「static メンバー」の分類です。
通常の class のフィールドすべてを意味するわけではありません。
汎用の `ConstantCase` は const メンバーの末尾 `_` を妨げるため設定していません。

`FunctionIgnoredRegexp` は `main`、割り込み・HAL コールバック、newlib のフック、
`begin` / `end` / `size` / `data` 等の STL 互換名を許可します。
`IgnoreMainLikeFunctions: false` により、単に main に似たシグネチャの関数まで除外しません。

`system.cc` / `uart.cc` ではこれを C ABI 用の名前一覧で上書きし、
`initialise_monitor_handles` も許可します。`VariableIgnoredRegexp` は
`_end`、`_estack`、`_Min_Stack_Size` だけを除外します。
`bugprone-reserved-identifier.AllowedIdentifiers` もリンカーの名前と
`__io_putchar` / `__io_getchar` に限定しています。任意の予約名を許可する設定ではありません。

## NOLINT と既存コードの扱い

元のチェック名なし `NOLINT` 147 か所を見直し、139 か所を削除しました。
残る 8 か所は理由とチェック名を指定した `NOLINTNEXTLINE(check-name)` にしています。

| 場所 | 残す理由 |
| --- | --- |
| `clock.cc` のクロック上限式 | 複数の上限を同じ形で比較し、ハードウェア制約を一覧しやすくする |
| `encoder.cc` の TIM1/TIM2 分岐 | 別の周辺機器の代替機能名が、同じ数値に展開される |
| `pwm.cc` のピン適合判定 | タイマー・チャンネル・ピンの対応表を 1 か所で確認する |
| `system.cc` の `_wait` | newlib が要求する `int*` の ABI を維持する |
| `system.cc` のリンカーアドレス 2 か所 | `uintptr_t` を明示して番地の幅を確認しやすくする |
| `time.cc` のサンプリングループ | 少なくとも 1 回読み、割り込みが入った場合に再試行する |
| `uart.cc` の送信バッファ | HAL の引数は非 const だが、送信処理はバッファを書き換えない |

`NOLINT` はコンパイラの構文・型エラーを直す仕組みではありません。
特定の診断を抑える必要がある場合だけ、理由を通常のコメントに書き、次の行へ
`NOLINTNEXTLINE(正確なチェック名)` を置きます。長い行末注釈より、再整形時も対象行を保ちやすくなります。

設定導入時の整形対象は `include/`、`src/`、`examples/` にある自作の C/C++ 50 ファイルです。
外部ライブラリ、SDK、生成物、Python、Markdown、リンカースクリプトには
C++ 用フォーマッターを適用していません。意図的な `clang-format off/on` 区間は維持しています。
命名の変更やロジックの書き換え、診断の自動修正は一括適用していません。
既存の単位表現の引数名等には、整形と別に検討すべき診断が残る場合があります。

## 日常の整形と確認

VS Code では clangd 拡張を使用し、C/C++ の保存時整形を有効にしています。
コマンドパレットの「ドキュメントのフォーマット」でも同じ設定を使えます。
他の拡張を使用する場合は、選択した整形エンジンが clang-format 23 の設定に対応するか確認します。

一括整形を継続して行う場合は、独立した **clang-format 23.1.0** を用意すると便利です。
インストール済み clangd と独立したコマンドなので、こちらのバージョンも確認します。
プロジェクト直下で、PowerShell なら次を実行します。

```powershell
clang-format --version
$sourceFiles = Get-ChildItem -LiteralPath include, src, examples -Recurse -File |
  Where-Object { $_.Extension -in '.c', '.cc', '.cpp', '.cxx', '.h', '.hpp' }
$sourceFiles | ForEach-Object {
  clang-format --style=file -i $_.FullName
  if ($LASTEXITCODE -ne 0) { throw "Formatting failed: $($_.FullName)" }
}
```

整形済みかだけ確認する場合は、`-i` を `--dry-run --Werror` に置き換えます。
整形後は差分を確認し、開発ガイドに記載した全環境のビルドを実行します。
include の並べ替えにも依存順序の影響があり得るため、ビルドの確認は省略しません。

## 他のプロジェクトへの移行

1. `.clang-format`、`.clangd` と、必要な `.vscode/settings.json` の項目をコピーします。
   配置先は新しいプロジェクトのルートです。親フォルダの `.clangd`、入れ子の設定、
   個人の clangd `config.yaml` も設定結果に影響します。
2. clangd と整形エンジンを同じバージョンに揃えます。VS Code の選択中プロファイルで
   clangd 拡張が有効か確認してください。表示設定などを共有する必要はありません。
3. PlatformIO なら `scripts/compiledb.py` をコピーし、`extra_scripts` に
   `pre:scripts/compiledb.py` を追加します。配置先が直下なら `pre:compiledb.py` に合わせます。
   ライブラリのビルド設定用 `extra_script.py` は別の役割なので上書きしません。
   詳細は [DB 生成スクリプトの説明](../scripts/README.md)を参照してください。
4. コピー先でビルド、または `pio run -e 環境名 -t compiledb` を実行して DB を再生成します。
   旧 PC の DB は絶対パスを含むためコピーして再利用しません。DB の標準配置はルートです。
5. `clangd.arguments` の `--query-driver` を、DB に記録されたコンパイラのパスに合わせます。
   現在は `${userHome}/.platformio/packages/toolchain-gccarmnoneeabi/bin/arm-none-eabi-*` です。
   PlatformIO Core の保存先、RISC-V / AVR 等への変更、コンパイラのパッケージ名の違いには調整が必要です。
   この許可範囲は実行して問い合わせるコンパイラを選ぶため、必要なツールチェーンに限定します。
6. `.clangd` の `PathMatch` / `PathExclude` をコピー先の構成に合わせます。
   アプリが `app/`、ライブラリが `lib/` 配下などの場合は、自作コードの範囲を追加します。
   STM32F4 以外では `f4/` の例外と HAL 設定ヘッダーの強制 include を削除・置換してください。
   `nbed.h` の公開 API 例外、リンカーの名前一覧も、そのまま他のライブラリへ持ち込まないでください。
7. `compile_commands.json` をエディターの監視除外に含めないようにします。
   `build/` 等を監視対象から外していても、ルートの DB を監視する現在の構成なら更新できます。
8. 最初に DB を更新し、必要なら `clangd: Restart language server` を実行します。
   `<chrono>` 等の標準ヘッダー、ハードウェアの型、補完、整形を確認してから通常作業に移ります。

共有設定に PC のユーザー名や nbed の絶対パスは書いていません。
ただし `.clangd` の自作コードの配置、`f4/`、`nbed.h`、HAL のファイル名は
**このプロジェクト固有の相対パス・名前**です。コピーするだけで全構成に適用される設定ではありません。
`.clangd` は VS Code の `${workspaceFolder}` 等の変数を展開しません。
`${userHome}` を使う `clangd.arguments` は vscode-clangd 拡張側の設定です。

VS Code のワークスペース・フォルダー設定は通常ユーザー設定より優先されます。
共有の `clangd.arguments` をプロファイルへ書くだけでは上書きできない場合があるため、
コピー先のワークスペース設定を確認して変更します。clangd 自体の個人設定は
Windows の `%LocalAppData%/clangd/config.yaml` 等にあり、プロジェクトの `.clangd` より優先されます。
複数ルートのワークスペースでは、DB と設定を各プロジェクトのルートに配置してください。

## 設定導入時の確認結果（v0.3.1）

Windows / clangd 23.1.0 で、次を確認しています。

| 確認 | 結果 |
| --- | --- |
| 設定の読み込みと、命名・分岐・組み込み例外を含む確認用コード | 設定エラーなし。意図した診断と抑制を確認 |
| 自作 C/C++ 50 ファイルの整形 | 実行済み。再度整形しても差分なし |
| 同じ 50 ファイルのエディター診断 | コンパイラのエラーなし。HAL 設定ヘッダーも単独で解析可能 |
| 8 か所の `NOLINTNEXTLINE` | 整形後も抑制対象の行に作用することを確認 |
| 全 8 個の PlatformIO 環境 | ビルド成功 |
| 整形前後のソース内容の比較 | 空白・include 順序・コメント以外のコードテキスト差分なし |

既存コードには命名、bool 変換、`auto` の提案などが残っています。
HAL マクロ由来の `-Wdeprecated-volatile` も 1 件確認しています。
これらを消すための包括的な抑制や API の変更は行っていません。
設定導入時のビルド確認には実機への書込み・動作確認は含みません。

## 参照

- [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html)
- [clang-format の設定項目](https://clang.llvm.org/docs/ClangFormatStyleOptions.html)
- [clangd の設定・条件付きフラグメント](https://clangd.llvm.org/config)
- [clang-tidy の NOLINT](https://clang.llvm.org/extra/clang-tidy/index.html#suppressing-undesired-diagnostics)
- [命名規則の各オプション](https://clang.llvm.org/extra/clang-tidy/checks/readability/identifier-naming.html)
- [マクロ内の do-while の扱い](https://clang.llvm.org/extra/clang-tidy/checks/cppcoreguidelines/avoid-do-while.html)
- [予約識別子の限定的な許可](https://clang.llvm.org/extra/clang-tidy/checks/bugprone/reserved-identifier.html)
- [マジックナンバーの診断](https://clang.llvm.org/extra/clang-tidy/checks/readability/magic-numbers.html)
- [リテラル接尾辞の診断と NewSuffixes](https://clang.llvm.org/extra/clang-tidy/checks/readability/uppercase-literal-suffix.html)

オンラインの LLVM 資料は開発中の版を指すことがあるため、対応バージョンの表記と、
実際に使用する clangd / clang-format での動作を併せて確認してください。
