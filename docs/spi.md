# SPI

F446RE、F303K8、F767ZI で、8 bit・全二重の SPI マスターを使用できます。
`#include "nbed.h"` から `Spi`、`SpiPeripheral`、`SpiMode`、`SpiBitOrder` を使います。
同じ SPI を複数のオブジェクトが初期化することはできません。複数の機器をつなぐ場合は、
1 個の `Spi` と機器ごとの CS 出力を使い、同時に選択する機器を 1 個にします。
機器間でモード・速度を変更する API はありません。設定が異なる場合は、オブジェクトを
破棄してから必要な設定で作り直してください。

## 使い方

次は F446RE の例です。コンストラクタのピン順は **SCK、MISO、MOSI** です。
送信するコマンドやバイト数は機器の仕様に合わせます。

```cpp
#include <array>
#include "nbed.h"

int main() {
  System::Initialize();
  Spi spi(kSpi1, kPA5, kPA6, kPA7, 1'000'000,
          SpiMode::kMode0, SpiBitOrder::kMsbFirst);
  DigitalOut cs(kD10, true);  // 非選択時は High（Active Low の機器の場合）
  if (!cs.Initialize() || !spi.Initialize()) {
    System::ErrorHandler();
  }
  const std::array<uint8_t, 2> tx{0x00, 0xff};
  std::array<uint8_t, 2> rx{};
  cs = false;
  const bool success = spi.Transfer(tx, rx, 100);
  cs = true;  // エラー時も CS を戻す
  if (!success) {
    System::ErrorHandler();
  }
  while (true) {
    SleepFor(1s);
  }
}
```

F303K8 の Arduino ピンを使う場合は、SPI の定義を次に置き換えます。
CS の `kD10` は F303K8 では PA11 です。

```cpp
Spi spi(kSpi1, kPB3, kPB4, kPB5, 1'000'000);
```

CS は `Spi` の呼び出しでは変化しません。コマンド送信とデータ受信の間も CS を Low に
保つ必要がある機器では、複数の呼び出しを 1 回の CS 操作で囲めます。機器で必要な
CS 前後の待機時間も呼び出し側で確保してください。3.3 V の信号と共通 GND を使用します。

## API

| 関数 | 動作 |
| --- | --- |
| `Initialize()` | ピン・設定・所有権を確認して初期化。同じオブジェクトの再呼び出しは成功 |
| `Send(tx, timeout_ms = 1000)` | バイト列を送信。受信データは破棄 |
| `Receive(rx, timeout_ms = 1000)` | MOSI から `0xff` を送信しながら受信 |
| `Transfer(tx, rx, timeout_ms = 1000)` | 同じバイト数を同時に送信・受信 |
| `GetFrequencyHz()` | 分周後の SCK 周波数（整数 Hz）。初期化前・失敗時は 0 |

`tx` は `std::span<const uint8_t>`、`rx` は `std::span<uint8_t>` です。
`std::array`、`std::vector`、通常の配列を渡せます。1 回の呼び出しは **1〜65535 byte**。
空の配列、上限を超える配列、`Transfer()` の長さの不一致は `false` を返します。
送受信に同じバッファを使うこともできますが、一部だけ重なるバッファは拒否します。
失敗時には一部だけ通信済みの場合があります。`Receive()` 失敗時の配列内容は未定義です。

通信はブロッキングです。タイムアウトは HAL のミリ秒単位で、1 回の呼び出し全体に
適用します。割り込み処理や割り込み禁止中から呼ばないでください。同じ SPI への
呼び出しは直列に行います。DMA・割り込み転送、スレーブ、16 bit フレーム、TI モード、
CRC、ハードウェア NSS、MISO/MOSI の省略には対応していません。

### モードとビット順

| `SpiMode` | CPOL | CPHA |
| --- | --- | --- |
| `kMode0`（既定） | 0 | 0 |
| `kMode1` | 0 | 1 |
| `kMode2` | 1 | 0 |
| `kMode3` | 1 | 1 |

`SpiBitOrder::kMsbFirst`（既定）または `SpiBitOrder::kLsbFirst` を指定します。
無効な列挙値を渡すと `Initialize()` が失敗します。

### 通信速度

コンストラクタの `frequency_hz` は SCK の上限です。APB クロックを 2、4、8、16、32、64、
128、256 のいずれかで分周し、指定値を超えない最速の設定を選びます。
最も遅い分周でも指定値を超える場合、0 Hz、下記の上限を超える指定は初期化に失敗します。

| MCU / SPI | 元のクロック | 速度指定の上限（3.3 V） |
| --- | --- | --- |
| F446RE SPI1 | PCLK2 | 45 MHz |
| F446RE SPI2 / SPI3 | PCLK1 | 22.5 MHz |
| F303K8 SPI1 | PCLK2 | 24 MHz |
| F767ZI SPI1 / SPI4 / SPI5 / SPI6 | PCLK2 | 54 MHz |
| F767ZI SPI2 / SPI3 | PCLK1 | 27 MHz |

既定のクロックで 1 MHz を指定すると、F446RE は 703,125 Hz、F303K8 は 1 MHz、
F767ZI は 843,750 Hz になります。
F303K8 の既定クロックでの最大実速度は 16 MHz です。相手の上限や配線条件に合う値を指定し、
`System::Initialize()` とクロック設定を済ませてから SPI を初期化してください。
初期化後にクロックを変更する場合は SPI も作り直します。

## 対応ピン

各行の選択肢は組み合わせて使えます。CS は通常の GPIO を別途選びます。
SWD、UART、PWM などと同じピンを同時に使用しないでください。

- [F303K8 の SPI1](pinout.md#f303k8-spi)
- [F446RE の SPI1～3](pinout.md#f446re-spi)
- [F767ZI の SPI1～6](pinout.md#f767zi-spi)

F446RE の LQFP64 では SPI4 の 3 信号を配線できません。F767ZI の SPI6 はピンによって
AF5 または AF8 を使い、nbed が信号ごとに設定します。

F446RE の Arduino D13/D12/D11 は PA5/PA6/PA7、F303K8 は PB3/PB4/PB5 です。
F446RE と F303K8 の SCK は基板上 LED と共用なので、SPI 使用中は LED 用 `DigitalOut` を作りません。
F767ZI の D13/D12/D11 は PA5/PA6/PA7 を想定し、既定の LD1 は PB0 です。
F767ZI の PA7 は Ethernet PHY と接続されているため、SPI 出力の配線を確認してください。
F767ZI のループバック例は PB5（Zio D22 / Morpho）を MOSI に使用します。
PB3/PB4 の SPI 使用中は JTAG・SWO とも競合します（PA13/PA14 の SWD は別です）。
F303K8 の PA5/PA6 は SB16/SB18 により PB7/PB6 と接続される場合があります。
I²C などと併用する際はボードのはんだブリッジ設定を確認してください。

## ループバック確認

外部 SPI 機器を外し、D11（MOSI）と D12（MISO）をジャンパで接続します。
F446RE は PA7–PA6、F303K8 は PB5–PB4 です。
F767ZI は D11 の代わりに PB5（Zio D22 / Morpho）を選び、PA6（D12）へ接続します。
サンプルは CS に D10 を使います。

```sh
pio run -e f4-spi-test
pio run -e f4-spi-test -t upload
pio device monitor -e f4-spi-test
```

F303K8 では環境名を `f3-spi-test` に変えます。成功時には `SPI loopback: PASS` が表示されます。
F767ZI では `f7-spi-test` を使用します。基板上の Ethernet/USB などとの競合と
はんだブリッジについては [F767ZI ガイド](f767zi.md)を参照してください。
サンプルは同時送受信のデータ一致、送信、受信時のダミー送信を確認します。
ビルドと代替 HAL の検証手順は [開発ガイド](development.md#ホスト上の回帰テスト)にあります。
上のループバック手順は実機での通信・波形の確認に使います。

## 参照

- ST [DS10693 STM32F446xC/E データシート](https://www.st.com/resource/en/datasheet/stm32f446re.pdf): AF 表、SPI の速度条件
- ST [DS9866 STM32F303x6/x8 データシート](https://www.st.com/resource/en/datasheet/stm32f303k8.pdf): AF 表、SPI の速度条件
- ST [UM1956 Nucleo-32 ボードマニュアル](https://www.st.com/resource/en/user_manual/dm00231744-stm32-nucleo32-boards-mb1180-stmicroelectronics.pdf): F303K8 の Arduino ピン、はんだブリッジ
