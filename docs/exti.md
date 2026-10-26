# 外部割り込み（EXTI）

`InterruptIn` は F303K8・F446RE・F767ZI の GPIO 入力変化を EXTI で検出します。
立上り、立下り、両エッジを選び、イベント累計と任意のコールバックを利用できます。
ポーリングの間に元のレベルへ戻るセンサにも使えます。

## 基本の使い方

```cpp
#include "nbed.h"

int main() {
  System::Initialize();
  InterruptIn sensor(kPA0, InterruptEdge::kBoth, GpioPull::kUp);
  if (!sensor.Initialize()) {
    System::ErrorHandler();
  }
  uint32_t previous = sensor.GetEdgeCount();
  while (true) {
    const uint32_t current = sensor.GetEdgeCount();
    if (current != previous) {
      // 入力変化を検知。ログ、通信、センサ固有の判定はここで行う。
      previous = current;
    }
  }
}
```

コンストラクタは `InterruptIn(pin, edge, pull, interrupt_priority)` です。

| 引数 | 既定値 | 内容 |
| --- | --- | --- |
| `pin` | なし | 対象 MCU の有効な GPIO。`kNC` は不可 |
| `edge` | `InterruptEdge::kBoth` | `kRising` / `kFalling` / `kBoth` |
| `pull` | `GpioPull::kNone` | `kNone` / `kUp` / `kDown` |
| `interrupt_priority` | `5` | 0〜15。小さい値ほど高優先度 |

`Initialize()` が成功すると直ちに監視を開始します。同じオブジェクトの再呼出しは
何も変更せず `true` を返します。無効なピン・列挙値・優先度、ラインの重複、
共有 IRQ の優先度不一致は GPIO/NVIC を変更せず `false` を返します。

`Read()` は現在の High/Low、`GetEdgeCount()` は ISR で処理したイベントの累計です。
コールバックを設定しなくても累計は増えます。累計は `uint32_t` で周回し、
`current - previous` の符号なし減算で差を取れます（比較間隔で2周以上しないこと）。
未初期化・停止中の `Read()` は `false` です。

`Stop()` または破棄で割り込みとライン予約を解放し、同じプル設定の通常入力へ戻します。
同じ IRQ を使うほかの入力は停止しません。停止後も累計を保持し、
次の `Initialize()` 成功時に0へ戻します。停止・再開しても設定したコールバックは保持します。
初期化前や停止中の保留イベントは、監視開始時に破棄します。

## 任意のコールバック

```cpp
volatile uint32_t notifications = 0;

sensor.SetCallback([] { notifications = notifications + 1U; });
sensor.SetCallback({});  // コールバックだけ解除。監視と累計は継続。
```

`SetCallback()` はキャプチャ付きラムダにも対応します。キャプチャしたオブジェクトは
コールバックの登録期間中、有効である必要があります。
コールバックは累計を加算した直後、割り込み処理内で実行します。
フラグやカウンターの更新に留め、`printf`、待機、ブロッキング通信、動的メモリ確保は避けてください。
`Initialize()`・`Stop()`・`SetCallback()`・破棄はメイン処理から行い、
コールバック内で自分や別の入力の構成を変更しないでください。
設定変更時は短い区間だけ割り込みを禁止し、呼出し前の割り込み許可状態に戻します。

`Read()` と `GetEdgeCount()` は別々に読みます。レベル・累計・時刻の同時スナップショットが
必要な場合は、アプリ側で読取り全体を短いクリティカルセクションにまとめてください。

## ピンと共有割り込みの制約

GPIO の末尾番号が EXTI のライン番号です。PA0 と PB0 は同じ EXTI0 を使うため、
別オブジェクトでも同時に初期化できません。PA8 と PA9 は別ラインなので併用できます。

| ライン | IRQ |
| --- | --- |
| EXTI0 / 1 / 3 / 4 | 各ライン専用 |
| EXTI2 | F4/F7 は `EXTI2`、F3 は `EXTI2_TSC` |
| EXTI5〜9 | `EXTI9_5` を共有 |
| EXTI10〜15 | `EXTI15_10` を共有 |

同じ IRQ に属する入力は同じ優先度を指定します。共有グループで複数の保留ビットが
立った場合は、各ラインを1回ずつ処理します。コールバック中に新しいイベントが
発生した場合は保留ビットを残し、次の割り込みで処理します。

この予約は `InterruptIn` 同士が対象です。通常の GPIO、ADC、PWM、通信機能との
ピン重複や、HAL を直接使った EXTI の競合は検出しません。
監視中のピンを別機能で再初期化しないでください。
nbed が GPIO EXTI の各 IRQ ハンドラーを定義するため、アプリ側で同じハンドラーを
定義したまま併用できません。F3 の EXTI2 と共有する TSC も併用対象外です。
HSE、SWD、基板上の LED・Ethernet などが使うピンは、ボードの配線を確認してください。

EXTI の保留状態は1ビットです。ISR が処理する前に同じラインで複数の変化が起きると
1イベントにまとまります。`GetEdgeCount()` は厳密なパルス総数や立上り／立下りの
内訳を保証しません。高速なパルスの計数・幅計測にはタイマを使用してください。
デバウンス、振動の重複排除、時刻の保存はアプリ側で実装します。

## F446RE で5入力を使う例

| 入力 | コネクタ | GPIO | ライン |
| --- | --- | --- | --- |
| 1 | A0 | PA0 | EXTI0 |
| 2 | A1 | PA1 | EXTI1 |
| 3 | A2 | PA4 | EXTI4 |
| 4 | D7 | PA8 | EXTI8 |
| 5 | D8 | PA9 | EXTI9 |

```cpp
#include <array>
#include "nbed.h"

std::array<InterruptIn, 5> sensors{
    InterruptIn{kPA0, InterruptEdge::kBoth, GpioPull::kUp},
    InterruptIn{kPA1, InterruptEdge::kBoth, GpioPull::kUp},
    InterruptIn{kPA4, InterruptEdge::kBoth, GpioPull::kUp},
    InterruptIn{kPA8, InterruptEdge::kBoth, GpioPull::kUp},
    InterruptIn{kPA9, InterruptEdge::kBoth, GpioPull::kUp},
};
```

`System::Initialize()` の後で各入力の `Initialize()` を確認します。
SW-420 では、従来の GPIO/NVIC 設定・独自 ISR・共有カウンターをこの配列へ置き換え、
既存の校正・静止時間・再受付判定へ `Read()` と `GetEdgeCount()` を渡せます。
センサ用の校正や `Pressed: 1`〜`Pressed: 5` の通知形式はアプリ側の機能です。
センサの電源は3.3 V、GNDは共通とし、DOの電圧と待機レベルを確認してください。
この5入力のコネクタ表は F446RE 用です。

## 実機確認用サンプル

`examples/exti-input/main.cc` は3ボードとも PA0 を使います。
F446RE / F303K8 では A0、F767ZI では Arduino A0 が PA3 なので **PA0 の端子**を使います。
PA0 と GND の間にスイッチをつなぎ、内蔵プルアップで確認できます。

```sh
pio run -e f3-exti-test
pio run -e f4-exti-test
pio run -e f7-exti-test
```

対象環境に `-t upload` を付けて書き込み、921600 bps のモニターで
`level`（現在値）、`events`（ISRで処理した累計）、`callbacks`（通知回数）を確認します。
スイッチの接点では1操作に複数イベント発生する場合があります。
共有 IRQ の確認では PA8 / PA9 の2入力を使い、一方の `Stop()` 後も他方が動くことを確かめます。
ビルド・代替 HAL テストの実施範囲は [v0.9.0 リリースノート](releases/v0.9.0.md)を参照してください。

## 参照

- [ST STM32F4 HAL GPIO](https://github.com/STMicroelectronics/stm32f4xx-hal-driver/blob/v1.8.4/Src/stm32f4xx_hal_gpio.c): GPIO と EXTI の設定・解除
- [ST RM0390（F446）](https://www.st.com/resource/en/reference_manual/rm0390-stm32f446xx-advanced-armbased-32bit-mcus-stmicroelectronics.pdf): EXTI、SYSCFG、共有 IRQ
- [ST UM1724（Nucleo-64）](https://www.st.com/resource/en/user_manual/dm00105823-stm32-nucleo64-boards-mb1136-stmicroelectronics.pdf): F446RE のコネクタ対応
