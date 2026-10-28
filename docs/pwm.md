# PWM の相補出力・極性・デッドタイム

相補出力・HAL の出力設定は v0.8.0 から利用できます。
既存の4引数の `Pwm` は通常出力として使えます。

## N 出力だけを使う

F303K8 の TIM15 CH1N（PA1）の例です。ピンだけで自動判定せず、`kCh1N` を明示します。

```cpp
Pwm output(kTim15, kCh1N, kPA1, 1'000);
if (!output.Initialize() || !output.SetDutyCycle(0.25F)) {
  System::ErrorHandler();
}
```

`kCh1N` / `kCh2N` / `kCh3N` は、対応するハードウェアとピンだけで使えます。
TIM2/3 に N 出力はなく、TIM15/16/17 で使える N 出力は CH1N だけです。
存在しない組み合わせは `Initialize()` が `false` を返します。

## CHx と CHxN を同時に使う

CH1 と CH1N は同じ比較レジスタを共有します。1つのオブジェクトに両方のピンを渡します。

```cpp
Pwm pair(kTim15, kCh1, kPA2, 20'000,
         {.complementary_pin = kPA1, .dead_time = 8});
if (!pair.Initialize() || !pair.SetDutyCycle(0.25F)) {
  System::ErrorHandler();
}
```

この例で PA2 を使う場合は、ビルド設定に `-DNBED_NO_DEFAULT_SERIAL` が必要です。
`Stop()` とデストラクタは両方の出力を停止します。
CH1 と CH1N を別々の `Pwm` オブジェクトで初期化することはできません。
同じタイマの別チャンネルとは、周波数・デッドタイム・OSSR/OSSI 設定を揃えて併用できます。
別チャンネルが使用中の `SetFrequencyHz()` は拒否されます。ペアだけなら周波数を変更できます。

**デューティ比とパルス幅は CHx 側の比較値を基準にします。** 既定極性で `0.25F` を指定すると、
CHx は約25%、CHxN は約75%の High 期間になります。デッドタイム分も波形に反映されます。
N 出力だけを使った場合も同じ基準で、N 側の High 率を直接25%にする指定ではありません。
初期化時の比較値は0です。N 出力の初期レベルもその反転関係になります。

## HAL の出力設定

5番目の引数は `Pwm::Configuration` です。次のフィールドは STM32 HAL の定数・値を受け取ります。

| フィールド | 既定値 | 用途 |
| --- | --- | --- |
| `complementary_pin` | `kNC` | 通常出力に追加する N 出力ピン。N 単独の場合は指定しない |
| `polarity` | `TIM_OCPOLARITY_HIGH` | 通常出力の極性。`TIM_OCPOLARITY_LOW` で反転 |
| `complementary_polarity` | `TIM_OCNPOLARITY_HIGH` | N 出力の極性。`TIM_OCNPOLARITY_LOW` も指定可能 |
| `idle_state` | `TIM_OCIDLESTATE_RESET` | 高機能タイマの通常出力のアイドル状態 |
| `complementary_idle_state` | `TIM_OCNIDLESTATE_RESET` | 高機能タイマの N 出力のアイドル状態 |
| `dead_time` | `0` | HAL の `DeadTime` と同じ DTG 符号値（0〜255） |
| `off_state_run_mode` | `TIM_OSSR_DISABLE` | 高機能タイマの OSSR。`TIM_OSSR_ENABLE` も指定可能 |
| `off_state_idle_mode` | `TIM_OSSI_DISABLE` | 高機能タイマの OSSI。`TIM_OSSI_ENABLE` も指定可能 |

例えば、通常のPWM端子自体を反転する場合は次のように設定します。

```cpp
Pwm inverted(kTim2, kCh1, kPA0, 1'000,
             {.polarity = TIM_OCPOLARITY_LOW});
```

アイドル状態は、出力許可・MOE・OSSR/OSSI などの条件に応じてハードウェアが適用します。
アイドル値の指定だけで `Stop()` 後の端子レベルが無条件に固定されるという意味ではありません。
TIM2/3 はデッドタイム・アイドル状態・OSSR/OSSI の非既定設定に対応せず、初期化時に拒否します。
極性を個別に反転した場合、CHx/CHxN が論理的な反転関係になるとは限りません。

開始・停止には `HAL_TIM_PWM_Start/Stop` と `HAL_TIMEx_PWMN_Start/Stop`、
出力設定には `HAL_TIM_PWM_ConfigChannel` と `HAL_TIMEx_ConfigBreakDeadTime` を使います。
ブレーク入力は無効、ロックはオフ、自動出力は無効として初期化します。
DMA、ブレーク入力の設定、割り込み転送、センターアラインなどの追加 API は今回含めていません。

## デッドタイムの単位

`dead_time` はナノ秒や単純なカウンタ tick 数ではなく、HAL が受け取る8 bitの DTG 符号値です。
カウンタのプリスケーラとは独立した `tDTS` を使い、128以上では符号化が非線形になります。

| DTG | デッドタイム |
| --- | --- |
| 0〜127 | `DTG × tDTS` |
| 128〜191 | `(64 + (DTG & 63)) × 2 × tDTS` |
| 192〜223 | `(32 + (DTG & 31)) × 8 × tDTS` |
| 224〜255 | `(32 + (DTG & 31)) × 16 × tDTS` |

nbed の PWM は `ClockDivision = DIV1` を使います。例えば既定の F303K8 の TIM15 は64 MHzなので、
`dead_time = 8` は125 nsに相当します。実際の波形は使用クロックと実機で確認してください。

## 対応ピン

| MCU | タイマ | CH1N | CH2N | CH3N |
| --- | --- | --- | --- | --- |
| F303K8 | TIM1（AF6） | PA7 / PA11 | PA12 / PB0 | PB1 |
| F303K8 | TIM15（AF9） | PA1 | — | — |
| F303K8 | TIM16（AF1） | PA13 / PB6 | — | — |
| F303K8 | TIM17（AF1） | PB7 | — | — |
| F446RE | TIM1（AF1） | PA7 / PB13 | PB0 / PB14 | PB1 / PB15 |
| F767ZI | TIM1（AF1） | PA7 / PB13 / PE8 | PB0 / PB14 / PE10 | PB1 / PB15 / PE12 |

同じピンをほかの出力や周辺機能と併用しないでください。PA13 は SWD、F767ZI の PA7 は Ethernet PHY と共用です。
F303K8 の PB6/PB7 は、Nucleo-32 のはんだブリッジによって PA6/PA5 にも接続される場合があります。

## サンプルと検証

| 環境 | CH1 | CH1N |
| --- | --- | --- |
| `f3-pwm-complementary` | TIM15 / PA2 | PA1 |
| `f4-pwm-complementary` | TIM1 / PA8 | PA7 |
| `f7-pwm-complementary` | TIM1 / PE9 | PE8 |

```sh
pio run -e f3-pwm-complementary
```

20 kHz、CH1基準25%、`dead_time = 8` の波形測定用です。実機での波形確認は未実施です。
ホストテストでは全ピン候補、極性などの HAL 設定値、所有権、設定共有、開始失敗時の解放、
別チャンネルを維持した停止を確認します。

2026-10-23 の確認結果:

- 全19環境でビルド成功。
- 配布パッケージの109ファイルが作業ツリーと一致し、別の F3/F4/F7 プロジェクトでも
  N 単独・ペア出力を含む構成でビルド成功。グローバル名・グローバル待機関数・既定コンソールは無効化。
- 回帰テスト35件を実行して成功。F7 の PWM 出力設定1件はコンパイル成功ですが、
  最終実行は Windows のアプリ制御（4551）によりスキップ。任意統合テスト8件もスキップ。
- Windows の保護設定は変更していません。実機での書込み・波形測定は未実施です。

参照: ST [F303K8 データシート](https://www.st.com/resource/en/datasheet/stm32f303k8.pdf)、
[F446RE データシート](https://www.st.com/resource/en/datasheet/stm32f446re.pdf)、
[F767ZI データシート](https://www.st.com/resource/en/datasheet/stm32f767zi.pdf)、
[HAL の相補出力・デッドタイム設定](https://github.com/STMicroelectronics/stm32f3xx-hal-driver/blob/master/Src/stm32f3xx_hal_tim_ex.c)。

## DMA 波形転送

v0.10.0から通常のCH出力に `SendDataDma()` を使用できます。
タイマを占有する単発転送で、CHxN・相補ペアには使用できません。
WS2812Bの例、戻り値・状態・末尾ゼロ・バッファ寿命は [DMA ガイド](dma.md)を参照してください。
