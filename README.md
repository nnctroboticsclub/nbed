# nbed

nbed は、**STM32シリーズ** で GPIO、EXTI、ADC、PWM、I²C、SPI、UART、CAN、
エンコーダを扱うための C++ ライブラリです。STM32 HAL の初期化手順を隠しつつ、使用する
タイマ・通信機能・ピンをコードに明示して、間違った組み合わせは `Initialize()` の失敗として
検出します。

対応ボードは **NUCLEO-F446RE / NUCLEO-F303K8 / NUCLEO-F767ZI** です。このページの基本例と対応ピン表は
F446RE 向けです。F303K8 の設定・ピン表・実機確認手順は
[F303K8 ガイド](docs/f303k8.md)を参照してください。
F767ZI の設定・対応ピン・キャッシュとメモリ配置は [F767ZI ガイド](docs/f767zi.md)にあります。
v0.12.0 はプレリリースです。変更内容と検証範囲は
[v0.12.0 リリースノート](docs/releases/v0.12.0.md)にあります。

初心者はまず `#include "nbed.h"` と `System::Initialize()` だけ覚えれば大丈夫です。  
こちらの [サンプル](#最初に動かす) にチュートリアルとしてLチカの例があります。

## 目次

- [nbed](#nbed)
  - [目次](#目次)
  - [このリポジトリで開発する](#このリポジトリで開発する)
  - [最初に動かす](#最初に動かす)
    - [必要なもの](#必要なもの)
  - [共通の考え方](#共通の考え方)
    - [ピン名の書き方](#ピン名の書き方)
    - [初期化の順番](#初期化の順番)
    - [`printf` の出力先](#printf-の出力先)
  - [GPIO（LED・ボタン）](#gpioledボタン)
    - [出力: LED を点灯する](#出力-led-を点灯する)
    - [入力: スイッチを読む](#入力-スイッチを読む)
  - [外部割り込み（EXTI）](#外部割り込みexti)
  - [アナログ入力（ADC）](#アナログ入力adc)
  - [PWM（サーボ・モータドライバ）](#pwmサーボモータドライバ)
    - [PWM で特に重要な制約](#pwm-で特に重要な制約)
  - [DMA と WS2812B](#dma-と-ws2812b)
  - [I2C（センサ）](#i2cセンサ)
  - [SPI（センサ・周辺機器）](#spiセンサ周辺機器)
  - [UART（PC・外部機器とのシリアル通信）](#uartpc外部機器とのシリアル通信)
    - [既定の `printf`](#既定の-printf)
    - [任意の UART を使う](#任意の-uart-を使う)
  - [CAN](#can)
  - [エンコーダ](#エンコーダ)
  - [時間待ち・計時・単位](#時間待ち計時単位)
    - [待機](#待機)
    - [経過時間](#経過時間)
    - [単位付きの値](#単位付きの値)
  - [対応ピン一覧](#対応ピン一覧)
    - [基板上でまず使える信号](#基板上でまず使える信号)
    - [GPIO](#gpio)
    - [ADC1](#adc1)
    - [PWM](#pwm)
    - [I²C](#ic)
    - [SPI](#spi)
    - [UART](#uart)
    - [CAN](#can-1)
    - [エンコーダ](#エンコーダ-1)
  - [上級者向け: マクロとビルドの仕組み](#上級者向け-マクロとビルドの仕組み)
    - [マクロ](#マクロ)
    - [PlatformIO に残す設定](#platformio-に残す設定)
    - [クロックを変更する](#クロックを変更する)
  - [制約と未対応機能](#制約と未対応機能)
  - [コントリビューター](#コントリビューター)
  - [ライセンス](#ライセンス)
  - [バージョン履歴](#バージョン履歴)

## このリポジトリで開発する

ライブラリ本体と PlatformIO の開発環境を、このリポジトリでまとめて管理します。
このフォルダを VS Code で開き、PlatformIO IDE を有効にしてください。
サブモジュールの初期化や、別の Nbed-dev リポジトリは不要です。

```sh
git clone https://github.com/nnctroboticsclub/nbed.git
cd nbed
pio run
```

既定では、外部配線なしで使える基板上 LED の点滅例 `f4-blink` をビルドします。
NUCLEO-F446RE に書き込む場合は `pio run -e f4-blink -t upload` を実行します。
`include/`・`src/` を編集すると、次のビルドにその変更が直接反映されます。
ほかのサンプルの選び方、全環境のビルド、フォルダ構成は
[開発ガイド](docs/development.md)を参照してください。

別のアプリケーションから nbed を使う場合は、従来どおりそのプロジェクトの
`lib_deps` に nbed を指定します。設定例は
[PlatformIO に残す設定](#platformio-に残す設定)にあります。

## 最初に動かす

### 必要なもの

- NUCLEO-F446RE
- ST-LINK 用 USB ケーブル
- PlatformIO が使える環境（VS Code + PlatformIO IDE など）

最初はNucleo基板上の LED（`PA5`）を点滅させるだけの、次のプログラムがおすすめです。

```cpp
#include "nbed.h"

int main() {
  System::Initialize(); // クロック等の初期化

  DigitalOut led(kPA5); // LEDを定義

  led.Initialize();     // LEDを初期化

  while (true) {
    led = true;       // 点灯 (led.Write(true) とも書ける)
    SleepFor(500ms);  // 500ms 待つ
  
    led = false;      // 消灯 (led.Write(false) とも書ける)
    SleepFor(500ms);  // 500ms 待つ
  }
}
```

`System::Initialize()` は、HAL、クロック、時間計測を初期化します。DMAは転送の使用時に初期化します。`main()` の先頭で
**一度だけ**呼び出してください。`SleepFor()` の `ms` と `s` は、それぞれミリ秒・秒を表します。

`Initialize()` は、初期化に失敗した場合に `false` を返します。  
`false` なら、選んだピンや周辺機能の組み合わせが使えない、同じ資源を
すでに使っている、または設定値が不正です。最初のうちは戻り値を必ず確認しましょう。

```cpp
DigitalOut output(kPA5);
if (!output.Initialize()) {
  System::ErrorHandler();  // 割り込みを止めて停止する既定のエラー処理
}
```

## 共通の考え方

### ピン名の書き方

`kPA5` は「GPIOA の 5 番ピン」、`kPB8` は「GPIOB の 8 番ピン」です。STM32 のポート名に加え、
NUCLEO-F446RE の Arduino 互換コネクタ名も `kD1` や `kA3` のように指定できます。どちらも同じ
ピンを表す別名なので、すべての機能で混在なく使用できます。基板上の信号名と対応表は
[対応ピン一覧](#対応ピン一覧)を確認してください。

### 初期化の順番

1. `System::Initialize()` を呼ぶ。
2. 使用したい機能のオブジェクトを作る。
3. `Initialize()` が成功したことを確認する。
4. 読み書きや通信を始める。

同じオブジェクトに対して `Initialize()` を複数回呼んでも問題ありません。いっぽうで、同じ
UART・I²C・CAN 周辺機能を二つのオブジェクトで同時に初期化することはできません。

### `printf` の出力先

既定では `System::Initialize()` が USART2（RX: `PA3`、TX: `PA2`、921600 bps）をコンソールに
設定します。そのため `printf("value = %d\\n", value);` は ST-LINK の仮想 COM ポートへ出力
できます。詳しい切り替え方は [UART](#uartpc外部機器とのシリアル通信) と
[マクロ](#マクロ)を参照してください。

## GPIO（LED・ボタン）

### 出力: LED を点灯する

NUCLEO-F446RE のユーザー LED LD2 は `PA5` です。`true` は High、`false` は Low を出力します。
この基板では High で LED が点灯します。

```cpp
DigitalOut led(kPA5);

led.Initialize();

led.Write(true);   // 点灯
led.Write(false);  // 消灯
led = true;        // Write(true) と同じ
```

コンストラクタの 2 番目の引数は、初期化した直後の出力値です。たとえば
`DigitalOut motor_enable(kPB0, false);` とすれば、初期化時に Low を出力します。

### 入力: スイッチを読む

`DigitalIn` の `Read()` はピンの現在の電圧を `bool` で返します。外付けスイッチは、押した時に
High になる接続か Low になる接続かを回路図で確認してください。基板上の B1 USER は `PC13` に
接続されています。

```cpp
DigitalIn button(kPC13, GpioPull::kNone);

button.Initialize();

if (button.Read()) {
  // High のときに実行する処理
}
```

未接続の入力は High/Low が安定しません。外部抵抗がないスイッチには `GpioPull::kUp` または
`GpioPull::kDown` を選び、押していない時の電圧を決めます。

```cpp
DigitalIn switch_input(kPB1, GpioPull::kUp);
// スイッチで PB1 を GND へつなぐ回路なら、押した時は false（Low）
```

`DigitalOut` には `DigitalIn` をそのまま代入できます。

```cpp
led = button;  // button の現在の値を LED へ出力
```

## 外部割り込み（EXTI）

`InterruptIn` は入力の立上り・立下り・両エッジを EXTI で検出します。短いパルスが
メインループの読取り前に戻っても、`GetEdgeCount()` の変化から検知できます。
F303K8・F446RE・F767ZI で同じ API を使用できます。

```cpp
InterruptIn sensor(kPA0, InterruptEdge::kBoth, GpioPull::kUp);
if (!sensor.Initialize()) {
  System::ErrorHandler();
}

const uint32_t edges = sensor.GetEdgeCount();  // ISRで処理したイベントの累計
const bool high = sensor.Read();              // 現在の入力レベル
```

GPIO の末尾番号が同じピンは同時に使用できません（PA0 と PB0 はともに EXTI0）。
F446RE の5入力には PA0 / PA1 / PA4 / PA8 / PA9 を使えます。
`Stop()` またはオブジェクトの破棄でラインを解放します。
コールバック、共有 IRQ の優先度、SW-420 の移行例、実機試験は [EXTI ガイド](docs/exti.md)を参照してください。

## アナログ入力（ADC）

`AnalogIn` は ADC1 の 12 bit 変換を使います。`ReadRatio()` は `0.0` から `1.0` の比率、
`ReadVoltage()` は指定した基準電圧から求めた電圧を返します。

```cpp
AnalogIn battery(kPA0);  // 基準電圧は既定で 3.3 V

battery.Initialize();

const float ratio = battery.ReadRatio();      // 例: 0.50
const float voltage = battery.ReadVoltage();  // 例: 約 1.65 V
```

基準電圧が 3.3 V 以外の基板では、コンストラクタの 2 番目の引数で指定します。

```cpp
AnalogIn sensor(kPA1, 3.0F);
```

ADC 端子に入力できるのは **0 V から基準電圧まで**です。測りたい電圧が 3.3 V を超える場合は、
必ず抵抗分圧回路を用意してください。ADC は読み出すたびに一回変換するため、高速連続サンプリング
や ADC の DMA はこのライブラリの対象外です。

## PWM（サーボ・モータドライバ）

`Pwm` はタイマの一つのチャンネルを PWM 出力にします。サーボなら 50 Hz で初期化し、パルス幅を
指定する方法が分かりやすいです。

F303K8 は TIM1/2/3 に加え、v0.7.0 から TIM15（2チャンネル）と TIM16/17（各1チャンネル）に
対応しています。ピンとコンソールとの競合条件は [F303K8 の PWM ガイド](docs/f303k8.md#pwm)を参照してください。

v0.8.0 から `kCh1N` などの相補出力、通常/N出力のペア、HAL の極性・アイドル状態・
デッドタイム設定にも対応しています。使い方と対応ピンは [PWM 出力設定ガイド](docs/pwm.md)にあります。

```cpp
Pwm servo(kTim3, kCh1, kPB4, 50);  // TIM3 CH1 / PB4 / 50 Hz

servo.Initialize();

servo.SetPulseWidthUs(1'000);  // 1.0 ms
SleepFor(1s);
servo.SetPulseWidthUs(1'500);  // 1.5 ms
SleepFor(1s);
servo.SetPulseWidthUs(2'000);  // 2.0 ms
```

デューティ比で指定することもできます。`0.0F` から `1.0F` の範囲で、範囲外の値は 0 または 1 に
丸められます。

```cpp
pwm.SetDutyCycle(0.25F);  // 25 %
pwm.SetFrequencyHz(20'000);
```

### PWM で特に重要な制約

- コンストラクタの「タイマ・チャンネル・ピン」は、[対応ピン一覧](#対応ピン一覧)の組み合わせを
  そのまま使います。たとえば `kTim3, kCh1, kPB4` は使えますが、`kTim3, kCh1, kPA5` は使えません。
- 同じタイマの複数チャンネルは使えますが、**周波数を共有**します。最初のチャンネルと異なる
  周波数で初期化すると `false` になります。
- 同じタイマで PWM とエンコーダを併用することはできません。競合した `Initialize()` は `false` を
  返します。
- `SetFrequencyHz()` は、そのタイマで PWM を一つだけ使っている時に限り変更できます。
- `Stop()` を呼ぶか、`Pwm` オブジェクトが破棄されると出力を停止してタイマを解放します。

サーボやモータは NUCLEO の 3.3 V ピンから給電しないでください。外部電源を使い、GND だけは
NUCLEO と接続します。

## DMA と WS2812B

`Pwm::SendDataDma()` でPWMの比較値の配列を非同期に出力できます。
`Uart::SendDma()` / `ReceiveDma()` は通常の送受信に追加して選べる非同期転送です。
`DmaStatus` で完了・エラー・中断を確認します。

3ボード共通の [WS2812B点灯例](examples/ws2812b/main.cc)と
[UART DMA受信・返信例](examples/uart-dma/main.cc)を同梱しています。
配線、バッファの保持、F7のキャッシュ、DMAの競合は [DMA ガイド](docs/dma.md)を参照してください。

## I2C（センサ）

`I2c` は I²C マスターです。SCL、SDA、通信速度を指定してから、送受信用の `I2CMessage` を渡します。
アドレスは **7 bit の値をそのまま**書きます。`0x68 << 1` のように左シフトする必要はありません。

```cpp
#include <array>

I2c i2c(kI2c1, kPB6, kPB7, 400'000);

i2c.Initialize();

std::array<uint8_t, 2> command{0x75, 0x00};
I2CMessage write_message{
    .address = 0x68,
    .size = static_cast<uint8_t>(command.size()),
    .data = command.data(),
};
if (!i2c.Send(write_message)) {
  System::ErrorHandler();
}

std::array<uint8_t, 1> received{};
I2CMessage read_message{
    .address = 0x68,
    .size = static_cast<uint8_t>(received.size()),
    .data = received.data(),
};
if (!i2c.Receive(read_message)) {
  System::ErrorHandler();
}
```

対応速度は 400 kHz 以下です。長い配線や複数のデバイスをつなぐ場合は、モジュール側の
抵抗だけに頼らず、適切なプルアップ抵抗を SCL/SDA に入れてください。`Send()` と `Receive()` は
完了まで待つ（ブロッキングする）関数です。

## SPI（センサ・周辺機器）

8 bit の SPI マスターです。送信、受信、同時送受信に対応し、モード 0〜3 と MSB/LSB 順を
選べます。次は F446RE の例です。CS は `DigitalOut` で操作します。

```cpp
Spi spi(kSpi1, kPA5, kPA6, kPA7, 1'000'000);  // SCK, MISO, MOSI
DigitalOut cs(kD10, true);
if (!cs.Initialize() || !spi.Initialize()) {
  System::ErrorHandler();
}

const std::array<uint8_t, 2> tx{0x00, 0xff};
std::array<uint8_t, 2> rx{};
cs = false;
const bool success = spi.Transfer(tx, rx, 100);  // タイムアウト 100 ms
cs = true;
if (!success) {
  System::ErrorHandler();
}
```

`std::array` には `<array>` を include します。通信速度は指定値を超えない分周値を選び、
`GetFrequencyHz()` で実速度を取得できます。F303K8 は `kPB3, kPB4, kPB5` が
Arduino コネクタの SPI1 に相当します。対応ピン・モード・制約・ループバック確認は
[SPI ガイド](docs/spi.md)を参照してください。

## UART（PC・外部機器とのシリアル通信）

F303K8の2系統、F446REの6系統、F767ZIの8系統すべてに対応します。
F446REのUSART6の使用例と、全系列のピン・AF一覧は [UARTガイド](docs/uart.md)を参照してください。
`UartConfig` でHALの通信フォーマット・RTS/CTS・F3/F7の反転やMSB firstも指定できます。
9ビット・パリティなしにはword用の通常転送・DMA・受信コールバックを使用します。

### 既定の `printf`

何も設定しなければ、`System::Initialize()` が USART2（RX: `PA3`、TX: `PA2`、921600 bps）を
初期化します。ST-LINK の仮想 COM ポートを開けば、次のような出力を確認できます。

```cpp
#include <cstdio>

printf("Hello, nbed!\\n");
```

この既定設定が USART2 を使用中なので、同じ USART2 を自分で初期化する場合は
`NBED_NO_DEFAULT_SERIAL` を定義してください。定義方法は[マクロ](#マクロ)を参照します。

### 任意の UART を使う

`Send()` は `std::span` を受け取るため、`std::array` や `std::vector` など連続したバイト列を
直接渡せます。

```cpp
#include <array>

Uart device(kUsart3, kPC11, kPB10, 115'200);

device.Initialize();

const std::array<uint8_t, 3> message{'O', 'K', '\\n'};
device.Send(message);
```

自分で初期化した UART を `printf` の出力先にするには `SetConsole()` を呼びます。

```cpp
Uart pc(kUsart2, kPA3, kPA2, 921'600);
if (pc.Initialize()) {
  pc.SetConsole();
}
```

受信割り込みを使う時は、コールバックを設定してから `EnableRxInterrupt()` を呼びます。
コールバックには **受信した 1 バイト**が渡され、割り込み処理の中で実行されます。コールバック内で
`SleepFor()`、`printf()`、長い送信、動的メモリ確保などの時間のかかる処理は行わず、値を保存する
だけにしてください。

```cpp
volatile uint8_t latest_byte = 0;

device.SetRxCallback([](UartRxMessage message) {
  latest_byte = message[0];
});
if (!device.EnableRxInterrupt()) {
  System::ErrorHandler();
}
```

`Receive()` は指定したバイト数を受信するまで待つブロッキング関数です。

## CAN

nbed の CAN は Classic CAN（最大 8 byte/frame）です。NUCLEO のピンだけでは CAN バスに
つなげません。**別途 CAN トランシーバ**（例: 3.3 V 対応品）が必要です。CANH/CANL の配線、
両端の終端抵抗、共通 GND を正しく接続してください。CAN FD には対応していません。

```cpp
Can can(kCan1, kPB8, kPB9);  // CANを定義

can.Initialize();  // CANを初期化

can.Start();  // CANの送受信を開始

// 送信するメッセージを作成 (ID: 0x123, データ長: 3, データ: {0x4E, 0x42, 0x45})
CanMessage message{
    .id = 0x123,
    .size = 3,
    .data = {0x4E, 0x42, 0x45},
};

can.Send(message);  // 送信
```

既定では標準 ID・拡張 ID とも受信します。標準 ID `0x123` だけを通すには、受信開始前または後に
フィルタを設定できます。

```cpp
can.SetRxFilter(0x123, 0x7ff);
```

受信を取りこぼしにくくするには、ポーリングの `Receive()` より受信割り込みを推奨します。
UART と同様、コールバックは割り込み文脈で短く終える必要があります。

```cpp
// 受信したか同課のフラグ (割り込みで書き換えられるので volatile)
volatile bool has_message = false;

// 受信割り込みのコールバックを設定
can.SetRxCallback([](const CanMessage&) {
  has_message = true;
});

can.EnableRxInterrupt();  // 受信割り込みを有効化
```

`CanConfig` を使うと、ループバックや割り込み優先度も指定できます。外部配線なしの動作確認には
`kLoopback` が便利です。

```cpp
Can test_can({
    .peripheral = kCan1,
    .rx = kPB8,
    .tx = kPB9,
    .bitrate = 1'000'000,
    .mode = kLoopback,
});
```

## エンコーダ

`Encoder` は A/B 相のインクリメンタルエンコーダをタイマのエンコーダモードで読みます。
4 番目の引数には、`GetCount()` が 1 回転で進む**実効カウント数**を指定してください。エンコーダの
仕様にある PPR と、逓倍後のカウント数は異なることがあります。

```cpp
Encoder encoder(EncoderTimer::kTim3, kPB4, kPB5, 2048);

encoder.Initialize();

const int count = encoder.GetCount();
const float angle_deg = encoder.GetAngle().GetAsDegree();

encoder.ResetCount();
```

タイマ 1/2/3 を使うため、同じタイマを PWM とエンコーダに同時使用することはできません。
`TIM1` と `TIM3` は 16 bit、`TIM2` は 32 bit のカウンタです。非常に長く回転させる場合は、
カウンタの周回を考慮して定期的に `GetCount()` を読み取ってください。

## 時間待ち・計時・単位

### 待機

`SleepFor()` は CPU を待機させます。制御周期の厳密さが必要な処理や、割り込みコールバックの中では
使わないでください。

```cpp
SleepFor(10ms);
SleepFor(1s);
```

### 経過時間

`Timer` はマイクロ秒単位の経過時間を測ります。

```cpp
Timer timer;
timer.Start();
// 計測したい処理
timer.Stop();

const uint64_t elapsed_us = timer.ElapsedMicroseconds();
```

### 単位付きの値

`Angle`、`Velocity`、`Current` には変換用の型とリテラルがあります。

```cpp
const Angle target = 90.0_deg;
const Velocity speed = 120.0_rpm;
const Current limit = 500.0_mA;

const float radians = target.GetAsRadian();
```

## 対応ピン一覧

以下は NUCLEO-F446RE / STM32F446RE で nbed が受け付ける組み合わせです。表にない
組み合わせを指定すると、初期化は失敗します。

### 基板上でまず使える信号

| 用途 | STM32 ピン | NUCLEO の信号 |
| --- | --- | --- |
| LD2 ユーザー LED | `PA5` | Arduino D13 |
| B1 USER ボタン | `PC13` | B1 |
| 既定の UART RX | `PA3` | Arduino D0 |
| 既定の UART TX | `PA2` | Arduino D1 |
| I²C1 SCL | `PB8` | Arduino D15 |
| I²C1 SDA | `PB9` | Arduino D14 |
| ADC の例 | `PA0` | Arduino A0 |

Arduino 互換コネクタの表記を使うこともできます。例えばNucleoに搭載されている LED は
`kD13`（`kPA5` と同じ）、UART の送信は `kD1`（`kPA2` と同じ）、アナログ入力 A3 は
`kA3`（`kPB0` と同じ）です。

| Arduino 表記 | STM32 ピン | Arduino 表記 | STM32 ピン |
| --- | --- | --- | --- |
| `kD0` | `kPA3` | `kD1` | `kPA2` |
| `kD2` | `kPA10` | `kD3` | `kPB3` |
| `kD4` | `kPB5` | `kD5` | `kPB4` |
| `kD6` | `kPB10` | `kD7` | `kPA8` |
| `kD8` | `kPA9` | `kD9` | `kPC7` |
| `kD10` | `kPB6` | `kD11` | `kPA7` |
| `kD12` | `kPA6` | `kD13` | `kPA5` |
| `kD14` | `kPB9` | `kD15` | `kPB8` |
| `kA0` | `kPA0` | `kA1` | `kPA1` |
| `kA2` | `kPA4` | `kA3` | `kPB0` |
| `kA4` | `kPC1` | `kA5` | `kPC0` |

基板のコネクタ上の詳しい場所は、ST の
[NUCLEO-64 ユーザーマニュアル](https://www.st.com/resource/en/user_manual/dm00105823.pdf)も参照してください。

### GPIO

デジタル入出力には `PA0–PA15`、`PB0–PB10`、`PB12–PB15`、`PC0–PC15`、`PD2`、`PH0–PH1` を
使えます。`kNC` は未接続を表す予約値で、初期化には使えません。

### ADC1

| ADC1 の入力 | 対応ピン |
| --- | --- |
| CH0–CH7 | `PA0–PA7` |
| CH8–CH9 | `PB0–PB1` |
| CH10–CH15 | `PC0–PC5` |

### PWM

| タイマ | チャンネル | 対応ピン |
| --- | --- | --- |
| TIM1 | CH1 / CH2 / CH3 / CH4 | `PA8` / `PA9` / `PA10` / `PA11` |
| TIM2 | CH1 | `PA0`, `PA5`, `PA15`, `PB8` |
| TIM2 | CH2 | `PA1`, `PB3`, `PB9` |
| TIM2 | CH3 | `PA2`, `PB10` |
| TIM2 | CH4 | `PA3`, `PB2` |
| TIM3 | CH1 | `PA6`, `PB4`, `PC6` |
| TIM3 | CH2 | `PA7`, `PB5`, `PC7` |
| TIM3 | CH3 | `PB0`, `PC8` |
| TIM3 | CH4 | `PB1`, `PC9` |

### I²C

| 周辺機能 | SCL / SDA の組み合わせ |
| --- | --- |
| I2C1 | `PB6` / `PB7`、`PB8` / `PB9` |
| I2C2 | `PB10` / `PB3`、`PB10` / `PC12` |
| I2C3 | `PA8` / `PB4`、`PA8` / `PC9` |

### SPI

| 周辺機能 | SCK | MISO | MOSI |
| --- | --- | --- | --- |
| SPI1 | `PA5`, `PB3` | `PA6`, `PB4` | `PA7`, `PB5` |
| SPI2 | `PA9`, `PB10`, `PB13`, `PC7` | `PB14`, `PC2` | `PB15`, `PC1`, `PC3` |
| SPI3 | `PB3`, `PC10` | `PB4`, `PC11` | `PB0`, `PB2`, `PB5`, `PC1`, `PC12` |

各列のピンは組み合わせて使えます。CS は別の GPIO で制御します。
PA5 は基板上 LED と共用なので、SPI1 の SCK として使う場合は LED 出力を初期化しません。

### UART

| 周辺機能 | RX | TX |
| --- | --- | --- |
| USART1 | `PA10` または `PB7` | `PA9` または `PB6` |
| USART2 | `PA3` | `PA2` |
| USART3 | `PC5` または `PC11` | `PB10` または `PC10` |
| UART4 | `PA1` または `PC11` | `PA0` または `PC10` |
| UART5 | `PD2` | `PC12` |
| USART6 | `PC7` | `PC6` |

各行のRX/TX候補を個別に選べます。USART6は `Uart serial(kUsart6, kPC7, kPC6, 115200);` で指定します。

### CAN

| 周辺機能 | RX | TX |
| --- | --- | --- |
| CAN1 | `PA11` または `PB8` | `PA12` または `PB9` |
| CAN2 | `PB5` または `PB12` | `PB6` または `PB13` |

同じCANのRX候補とTX候補は個別に選べます。たとえば
`Can can(kCan1, kPA11, kPB9);` も使用できます。

### エンコーダ

| タイマ | A 相（CH1） | B 相（CH2） |
| --- | --- | --- |
| TIM1 | `PA8` | `PA9` |
| TIM2 | `PA0`, `PA5`, `PA15`, `PB8` | `PA1`, `PB3`, `PB9` |
| TIM3 | `PA6`, `PB4`, `PC6` | `PA7`, `PB5`, `PC7` |

## 上級者向け: マクロとビルドの仕組み

### マクロ

マクロは `platformio.ini` の `build_flags` で定義します。ライブラリ本体にも影響させたい
`NBED_NO_DEFAULT_SERIAL` は、ソースコード内の `#define` ではなく**ビルドフラグで定義**して
ください。

```ini
[env:my_board]
build_flags =
  -DNBED_NO_DEFAULT_SERIAL
```

| マクロ | 変更される仕様 | 使う場面 |
| --- | --- | --- |
| `NBED_NO_DEFAULT_SERIAL` | `System::Initialize()` が USART2 / `PA3` / `PA2` の既定コンソールを作らなくなる。`printf` の出力先も自動では設定されない。 | USART2 を通信相手に使う、別の UART をコンソールにする。 |
| `NBED_NO_GLOBAL_NAMES` | `kPA5`、`kTim3`、`DigitalOut`、`CanMessage` などの短い名前をグローバル名前空間へ出さない。 | 大きな C++ プロジェクトで名前の衝突を防ぐ。 |
| `NBED_NO_GLOBAL_FUNCTIONS` | グローバルの `SleepFor()` を出さない。 | 同名の関数との衝突を防ぐ。 |

nbed の名前空間は部内共通の `nkrc` 配下に置いています。
抽象インターフェースは `nkrc::nbed::interface`、STM32F4 の実装は
`nkrc::nbed::stm32::f4`、単位系とリテラルは `nkrc::nbed` と
`nkrc::nbed::literals` です。
通常は `nbed.h` が短い名前を公開するので、初心者は名前空間を意識する必要はありません。

`NBED_NO_GLOBAL_NAMES` を使う場合は、型と列挙値を完全修飾します。

```cpp
#define NBED_NO_GLOBAL_NAMES
#include "nbed.h"

nkrc::nbed::stm32::f4::DigitalOut led(
    nkrc::nbed::interface::PinName::kPA5);
```

`NBED_NO_GLOBAL_FUNCTIONS` を使った場合の待機は、次のように書けます。

```cpp
nkrc::nbed::stm32::f4::ClockManager::GetClock().Sleep(std::chrono::milliseconds{100});
```

### PlatformIO に残す設定

nbed の `library.json` は `extra_script.py` を呼び、ライブラリの配置場所に応じて次を自動設定します。

- nbed の `include` ディレクトリを、アプリ・STM32 HAL・ライブラリ自身の検索パスへ追加する。
- F446RE は `-O3`、F303K8 はサイズを抑える `-Os` を使い、LTO、C++23、警告、
  セクション削除、浮動小数点 `printf` を、必要なビルド環境すべてへ
  同じように適用する。
- PlatformIO 標準の `-Os` と `-std=gnu++14` を外す。
- MCU に合う同梱リンカスクリプトを最終リンクへ指定し、変更時に再リンクする。
- 対象 MCU の実装だけをコンパイルする。未対応 MCU はビルド設定時にエラーにする。

つまり、これらのフラグやリンカスクリプトを利用側の `platformio.ini` にコピーする必要はありません。
ただし PlatformIO がボードと STM32Cube フレームワークを選ぶ処理は、ライブラリを読み込むより前に
行われます。そのため、利用側には少なくとも次の設定が必要です。

```ini
[env:nucleo_f446re]
platform = ststm32@20.0.0
framework = stm32cube
board = nucleo_f446re
platform_packages =
  toolchain-gccarmnoneeabi@1.140201.0
  framework-stm32cubef4@1.28.3
board_build.stm32cube.custom_config_header = yes
lib_deps = https://github.com/nnctroboticsclub/nbed.git
```

`custom_config_header = yes` は、nbed に同梱した `stm32f4xx_hal_conf.h` を STM32Cube HAL が使う
ための指定です。この一行はライブラリのスクリプトだけでは代替できません。HAL 自身のビルド開始前に
PlatformIO が読む設定だからです。

上記のバージョンはこの開発リポジトリで検証した組み合わせです。別のバージョンを使う場合も、
ビルドが通ることと実機での動作を確認してください。最適化・LTO・リンカスクリプトの設定を独自に
上書きする場合は、nbed のスクリプトと重複しないよう注意が必要です。

### クロックを変更する

既定では、NUCLEO-F446RE の ST-LINK MCO から供給される 8 MHz HSE を使い、180 MHz のシステム
クロックに設定します。別基板では発振器や分周値に合わせて `Clock::Configuration` を渡します。

```cpp
Clock::Configuration clock{};
clock.oscillator_source = Clock::OscillatorSource::kHse;
clock.hse_mode = Clock::HseMode::kCrystal;
clock.hse_frequency_hz = 8'000'000;

System::Initialize(clock);
```

`hse_frequency_hz` は HAL が使う `HSE_VALUE` と一致する必要があります。8 MHz 以外の
外部発振器では、ビルド設定にも `-DHSE_VALUE=周波数` を指定してください。

PWM と CAN の実際の周波数はクロック設定に依存します。クロックを変更したら、通信速度・PWM の
周波数・実機上のタイミングを必ず再確認してください。

## 制約と未対応機能

- 対象 MCU は STM32F446RE、STM32F303K8（LQFP32）、STM32F767ZI（LQFP144）です。
  ほかの STM32 型番は未対応です。F3/F7 の名前空間は `nkrc::nbed::stm32::f3` / `nkrc::nbed::stm32::f7` です。
- ADC DMA、SPI DMA・割り込み転送・スレーブはこの API では提供していません。
- GPIO と異なる周辺機能のピン重複は一律に検出しません。ピンを複数の機能へ割り当てないでください。
- `AnalogIn::ReadRatio()` は未初期化・変換失敗時にも0を返すため、有効な0 Vと区別できません。
- I²C と UART の通常送受信は完了まで無期限に待つ場合があります。割り込み内では呼び出さないでください。
- PWM DMA と UART DMA のバッファは完了・中断まで保持してください。循環転送・UARTの可変長受信・
  相補PWMのDMAは未対応です。[DMA ガイド](docs/dma.md)に割当て・メモリ・制約をまとめています。
- I²C、SPI、UART の通常送受信、CAN の `Receive()` はブロッキングです。周期処理を止めたくない場合は
  タイムアウト設計や受信割り込みを検討してください。
- UART/CAN の受信コールバックは割り込み処理です。共有データをメインループで読む場合は、必要に
  応じて `volatile`、排他、コピーの一貫性を考慮してください。

## コントリビューター

- Yunoshin Tani — 初期開発・メンテナンス

## ライセンス

このプロジェクトは [MIT License](LICENSE) のもとで公開されています。利用、改変、再配布の条件は
[`LICENSE`](LICENSE) を参照してください。

## バージョン履歴

公開済みの本文と日本時間の公開日は [リリース履歴](docs/releases/README.md)を参照してください。

- v0.1.0 (2026-10-05) : 初版公開
- v0.2.0 (2026-10-06) : Arduino互換ピンへの対応
- v0.2.1 (2026-10-06) : nbed.h が include されている場合の clangd の補完の不具合を修正
- v0.3.0 (2026-10-18) : 部内共通名前空間 `nkrc` への移行と開発環境の統合。
  `nbed::interface` を `nkrc::nbed::interface`、`nbed::f4` を
  `nkrc::nbed::stm32::f4`、単位系の `nbed` を `nkrc::nbed` に変更しました。
  旧名前空間を明示するコードは修正が必要です。`nbed.h` 経由の初心者向けの短い名前は維持しています。
  PlatformIO の開発環境・実機確認用サンプル・コンパイルDB生成を同梱し、
  clangd の補完・整形・診断設定、interface ヘッダーの直接 include、単位系の命名を整理しました。
- v0.3.1 (2026-10-18) : VS Code の共有設定・推奨拡張・ビルドタスクを整理。
  既定環境のビルドや生成物・キャッシュを削除する完全再ビルドに対応し、
  PlatformIO 固有の Python コードに対する Pylance の警告を必要な行だけ抑制しました。
  公開 API と実行時の動作に変更はありません。
- v0.4.0 (2026-10-23) : STM32F303K8 向けに GPIO、ADC、PWM、I²C、UART、CAN、
  エンコーダ、計時を追加。MCU 別のビルド設定・サンプル・ピン表を整備しました。
  GPIO・ADC の重複処理と UART コンソール出力を最適化し、PWM の丸め、長時間計時の
  周回、待機時間の境界条件を修正しました。既存の F4 公開 API は維持しています。
  詳細は [v0.4.0 リリースノート](docs/releases/v0.4.0.md)を参照してください。
- v0.5.0 (2026-10-23) : F446RE の SPI1/2/3 と F303K8 の SPI1 に対応。
  8 bit マスターの送信・受信・同時送受信、モード0〜3、MSB/LSB順、タイムアウトを追加しました。
  SPI の使い方・ピン表、ループバック用サンプル、回帰テストを整備しました。
  詳細は [v0.5.0 リリースノート](docs/releases/v0.5.0.md)を参照してください。
- v0.6.0 (2026-10-23) : STM32F767ZI 向けに GPIO、ADC1/ADC3、PWM、I²C、SPI、UART、
  Classic CAN、エンコーダ、計時を追加。216 MHz クロック、F7 HAL 設定・専用リンカ、
  Arduino ピン名、サンプル、回帰テストを整備しました。
  詳細は [v0.6.0 リリースノート](docs/releases/v0.6.0.md)を参照してください。
- v0.6.1 (2026-10-23) : リポジトリ名とライブラリ名を `Nbed` から `nbed` に統一。
  `library.json` にリポジトリ情報を追加し、依存設定・ガイド・VS Code タスク・表示名を更新しました。
  公開 API・名前空間・`NBED_...` マクロは維持しています。
  詳細は [v0.6.1 リリースノート](docs/releases/v0.6.1.md)を参照してください。
- v0.7.0 (2026-10-23) : F303K8 の TIM15/16/17 に PWM 出力を追加。
  APB2 クロック、各タイマのチャンネル数・ピン・AF、所有権の管理に対応しました。
  PWM 波形確認用サンプルと回帰テストを追加しています。
  詳細は [v0.7.0 リリースノート](docs/releases/v0.7.0.md)を参照してください。
- v0.8.0 (2026-10-24) : F3/F4/F7 の PWM に相補出力と CHx/CHxN のペア出力を追加。
  HAL の極性・アイドル状態・デッドタイム・OSSR/OSSI 設定に対応しました。
  出力設定ガイド、3系列の波形確認用サンプルと回帰テストを追加しています。
  詳細は [v0.8.0 リリースノート](docs/releases/v0.8.0.md)を参照してください。
- v0.9.0 (2026-10-26) : F3/F4/F7 の外部割り込み入力、イベント累計、コールバックを追加。
  共有 IRQ とラインの所有権を管理し、3系列のサンプル・回帰テスト・EXTI ガイドを追加しました。
  GPIO の設定値検証とリリース文書も整理しています。
  詳細は [v0.9.0 リリースノート](docs/releases/v0.9.0.md)を参照してください。
- v0.10.0 (2026-10-28) : F3/F4/F7 の PWM DMA と UART DMA送受信を追加。
  DMA資源の予約・IRQ・停止・エラー・F7キャッシュ処理に対応し、WS2812BとUART DMAの例を追加しました。
  詳細は [v0.10.0 リリースノート](docs/releases/v0.10.0.md)を参照してください。
- v0.10.1 (2026-10-28) : F446RE・F767ZIのCANピン検証を修正。
  PA11 RX・PB9 TXなど、同じCANのRX/TX候補を個別に選択でき、異なるGPIOポートの初期化・停止にも対応しました。
  詳細は [v0.10.1 リリースノート](docs/releases/v0.10.1.md)を参照してください。
- v0.11.0 (2026-11-01) : F446RE・F767ZIの未実装UARTを追加。
  対応MCUの全UARTで通常送受信・受信割り込み・DMAに対応し、F7のピン別AF設定を追加しました。
  詳細は [v0.11.0 リリースノート](docs/releases/v0.11.0.md)を参照してください。
- v0.12.0 (2026-11-01) : UARTのHAL基本・拡張初期化設定を公開。
  反転、MSB first、ピン入替、RTS/CTS、通信フォーマット、9ビット用転送・DMA・受信割り込みに対応しました。
  詳細は [v0.12.0 リリースノート](docs/releases/v0.12.0.md)を参照してください。
