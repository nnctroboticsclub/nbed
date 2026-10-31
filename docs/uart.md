# UART / USART

F303K8の2系統、F446REの6系統、F767ZIの8系統すべてを `Uart` で使用できます。
USARTも非同期のUARTとして使用し、通常送受信・1バイトの受信割り込み・DMA送受信・
`printf` の出力先指定に対応します。設定は8データビット・パリティなし・1ストップビットです。

## F446REのUSART6を使う

```cpp
#include <array>
#include "nbed.h"

int main() {
  System::Initialize();
  Uart serial(kUsart6, kPC7, kPC6, 115200);  // RX, TXの順
  if (!serial.Initialize()) {
    System::ErrorHandler();
  }
  const std::array<uint8_t, 3> message{'O', 'K', '\n'};
  if (!serial.Send(message)) {
    System::ErrorHandler();
  }
  while (true) {}
}
```

`kUsart6` が正式な指定名です。相手のTXをPC7（RX）、RXをPC6（TX）へ接続し、GNDも接続します。
基板のピン位置はNUCLEO-F446REの回路図・コネクタ表を確認してください。
既定コンソールのUSART2とは別に使用できます。

## 対応ピン

同じ行のRX候補とTX候補を個別に選べます。異なるポート・異なるAF番号の組み合わせにも対応します。
表は対応するMCUパッケージで使用できるピンです。ボード上の配線や、ほかの機能との共用は別途確認してください。

### F303K8（LQFP32）

| 指定名 | RX | TX | AF |
| --- | --- | --- | --- |
| `kUsart1` | PA10 / PB7 | PA9 / PB6 | 7 |
| `kUsart2` | PA3 / PA15 / PB4 | PA2 / PA14 / PB3 | 7 |

PA14はSWCLKです。デバッグ接続を維持する場合は、USART2のTXにPA2またはPB3を選んでください。
PB3/PB4/PA15もデバッグ機能と共用するため、使用するデバッグ設定を確認してください。

### F446RE（LQFP64）

| 指定名 | RX | TX | AF |
| --- | --- | --- | --- |
| `kUsart1` | PA10 / PB7 | PA9 / PB6 | 7 |
| `kUsart2` | PA3 | PA2 | 7 |
| `kUsart3` | PC5 / PC11 | PB10 / PC10 | 7 |
| `kUart4` | PA1 / PC11 | PA0 / PC10 | 8 |
| `kUart5` | PD2 | PC12 | 8 |
| `kUsart6` | PC7 | PC6 | 8 |

### F767ZI（LQFP144）

括弧内はそのピンのAF番号です。nbedがRX/TXそれぞれの番号を設定します。

| 指定名 | RX | TX |
| --- | --- | --- |
| `kUsart1` | PA10 / PB7（AF7）、PB15（AF4） | PA9 / PB6（AF7）、PB14（AF4） |
| `kUsart2` | PA3 / PD6（AF7） | PA2 / PD5（AF7） |
| `kUsart3` | PB11 / PC11 / PD9（AF7） | PB10 / PC10 / PD8（AF7） |
| `kUart4` | PA1 / PC11 / PD0（AF8）、PA11（AF6） | PA0 / PC10 / PD1（AF8）、PA12（AF6） |
| `kUart5` | PD2 / PB12（AF8）、PB5（AF1）、PB8（AF7） | PC12 / PB13（AF8）、PB6（AF1）、PB9（AF7） |
| `kUsart6` | PC7 / PG9（AF8） | PC6 / PG14（AF8） |
| `kUart7` | PE7 / PF6（AF8）、PA8 / PB3（AF12） | PE8 / PF7（AF8）、PA15 / PB4（AF12） |
| `kUart8` | PE0（AF8） | PE1（AF8） |

たとえば `Uart serial(kUsart1, kPB15, kPA9, 115200);` は、RXをAF4、TXをAF7で初期化します。
UART7のPA15/PB3/PB4はデバッグ機能と共用するため、デバッグ設定を確認してください。

## 送受信と割り込み

- `Send()` / `Receive()` は指定バッファの転送が終わるまで待機します。
- `SetRxCallback()` と `EnableRxInterrupt()` で1バイトずつ受信し、コールバック後に次の受信を開始します。
  コールバックは割り込み内で実行するため、短時間で戻ってください。
- 同じUARTを複数の `Uart` オブジェクトで初期化すると、後から初期化した側は `false` になります。
  所有しているオブジェクトの破棄でUARTの所有権を解放します。
- DMAは全系統に対応します。バッファの寿命、固定ストリームの競合、F7キャッシュの条件は
  [DMAガイド](dma.md#uart-の-dma)を参照してください。
- 同期USART、フロー制御、パリティ・データ長・ストップビットの変更は提供していません。

## 参照資料

ピンとAFはSTの[公開ピン情報](https://github.com/STMicroelectronics/STM32_open_pin_data/tree/7d1f1514ed5583ec5007ad91236b4e1d377295b1)を
MCUパッケージと照合しています。テストの期待値も同じ版のピン情報に基づきます。
詳細は[F303データシート](https://www.st.com/resource/en/datasheet/stm32f303k8.pdf)、
[F446データシート](https://www.st.com/resource/en/datasheet/stm32f446re.pdf)、
[F767データシート](https://www.st.com/resource/en/datasheet/stm32f767zg.pdf)を参照してください。

ホストテストでは全候補の組み合わせと全256ピン値の検証、クロック・AF・IRQ、
各UARTのDMAストリームと完了・エラー、所有権・破棄・再初期化を確認します。
実機での配線・通信・波形確認は別途必要です。
