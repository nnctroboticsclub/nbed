# UART / USART

F303K8の2系統、F446REの6系統、F767ZIの8系統すべてを `Uart` で使用できます。
USARTも非同期のUARTとして使用し、通常送受信・1バイトの受信割り込み・DMA送受信・
`printf` の出力先指定に対応します。既定は8データビット・パリティなし・1ストップビットです。
v0.12.0から `UartConfig` でHALの通常初期化設定と拡張初期化設定を指定できます。

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
- 通信フォーマット、フロー制御、反転などは下記の `UartConfig` で指定します。

## HALの初期化設定

従来のコンストラクタは既定設定で動作します。設定を変更する場合は `UartConfig` を渡します。
`hal` はSTM32 HALの `UART_InitTypeDef` で、HALの定数をそのまま指定します。

```cpp
UartConfig config{.peripheral = kUsart1, .rx = kPA10, .tx = kPA9};
config.hal.BaudRate = 100000;
config.hal.WordLength = UART_WORDLENGTH_9B;
config.hal.Parity = UART_PARITY_EVEN;
config.hal.StopBits = UART_STOPBITS_2;
config.send_timeout = 100;     // HALのタイムアウト、ms
config.receive_timeout = 100;
Uart serial(config);
if (!serial.Initialize()) { System::ErrorHandler(); }
```

この例は8データビット＋偶数パリティ・2ストップビットです。
HALの `WordLength` はパリティを含む長さです。`9B + EVEN/ODD` では通常のバイトAPIを使います。
`9B + NONE` は下記の16ビットバッファのAPIを使います。

`hal` の全フィールド（baud rate、word length、stop bits、parity、TX/RX mode、hardware flow control、
oversampling、F3/F7のone-bit sampling）を指定できます。
F4のone-bit samplingは `config.one_bit_sampling = true;` で指定します。
GPIOのプル・速度も `gpio_pull`・`gpio_speed` で設定できます。
初期化時に定数値・ピン・機能の組み合わせを検証し、未定義の定数や未対応の設定は
HALを呼び出す前に `false` を返します。実際のクロック条件による初期化失敗も呼び出し元へ返します。

### RX/TX反転と拡張設定（F303K8・F767ZI）

```cpp
UartConfig config{.peripheral = kUsart1, .rx = kPA10, .tx = kPA9};
config.advanced.AdvFeatureInit = UART_ADVFEATURE_RXINVERT_INIT;
config.advanced.RxPinLevelInvert = UART_ADVFEATURE_RXINV_ENABLE;
Uart serial(config);
if (!serial.Initialize()) { System::ErrorHandler(); }
```

RX反転だけを有効にする例です。TXも反転する場合は `UART_ADVFEATURE_TXINVERT_INIT` をフラグに加え、
`TxPinLevelInvert = UART_ADVFEATURE_TXINV_ENABLE` を設定します。
レベル反転とデータの論理反転は別の設定です。

`advanced` はHALの `UART_AdvFeatureInitTypeDef` で、全フィールドを渡します。

| 機能 | 初期化フラグ | 設定フィールド |
| --- | --- | --- |
| TX反転 | `UART_ADVFEATURE_TXINVERT_INIT` | `TxPinLevelInvert` |
| RX反転 | `UART_ADVFEATURE_RXINVERT_INIT` | `RxPinLevelInvert` |
| データ論理反転 | `UART_ADVFEATURE_DATAINVERT_INIT` | `DataInvert` |
| RX/TXピン入替 | `UART_ADVFEATURE_SWAP_INIT` | `Swap` |
| overrun検出 | `UART_ADVFEATURE_RXOVERRUNDISABLE_INIT` | `OverrunDisable` |
| RXエラー時のDMA制御 | `UART_ADVFEATURE_DMADISABLEONERROR_INIT` | `DMADisableonRxError` |
| 自動baud rate検出 | `UART_ADVFEATURE_AUTOBAUDRATE_INIT` | `AutoBaudRateEnable` / `AutoBaudRateMode` |
| MSB first | `UART_ADVFEATURE_MSBFIRST_INIT` | `MSBFirst` |

フラグを付けずに対応フィールドだけを有効にした設定は拒否します。
ピン入替では `rx`・`tx` に通常のAF機能のピン名を指定し、HALのSWAPで信号を入れ替えます。
自動baud rate検出はF3ではUSART1、F7ではUSART1/2/3/6に限ります。
7ビットフレームでの0x7F/0x55検出は拒否します。
RXエラー時はHALの設定に加えてnbedのDMAエラー処理も適用され、該当する転送を停止します。

F446REのHALにはこの拡張設定構造体がなく、反転・SWAP・MSB firstなどの設定は提供していません。
F3/F7でも反転の既定値は無効です。

### TX専用・RX専用とRTS/CTS

`hal.Mode = UART_MODE_TX` または `UART_MODE_RX` を指定できます。
使用しない側のピンは `kNC` にできます。無効な方向の送受信APIは `false` を返します。
SWAP時は実際に使う側のAFピンを指定してください。

```cpp
UartConfig config{.peripheral = kUsart1, .rx = kPA10, .tx = kPA9,
                  .cts = kPA11, .rts = kPA12};
config.hal.HwFlowCtl = UART_HWCONTROL_RTS_CTS;
Uart serial(config);
```

CTSのみ・RTSのみも指定できます。指定したフロー制御に対応するピンが必要です。
使用しない制御ピンは `kNC` にします。

| MCU | 周辺機能 | CTS | RTS |
| --- | --- | --- | --- |
| F3/F4/F7 | USART1 | PA11 | PA12 |
| F3/F4 | USART2 | PA0 | PA1 |
| F4 | USART3 | PB13 | PB14 |
| F4 | UART4 | PB0 | PA15 |
| F4 | UART5 | PC9 | PC8 |
| F7 | USART2 | PA0 / PD3 | PA1 / PD4 |
| F7 | USART3 | PB13 / PD11 | PB14 / PD12 |
| F7 | UART4 | PB0 / PB15 | PA15 / PB14 |
| F7 | UART5 | PC9 | PC8 |
| F7 | USART6 | PG13 / PG15 | PG8 / PG12 |
| F7 | UART7 | PE10 / PF9 | PE9 / PF8 |
| F7 | UART8 | PD14 | PD15 |

F446REのUSART6はパッケージにCTS/RTSピンがないため、フロー制御を指定すると失敗します。

### 9データビット・パリティなし

```cpp
UartConfig config{.peripheral = kUsart1, .rx = kPA10, .tx = kPA9};
config.hal.WordLength = UART_WORDLENGTH_9B;
Uart serial(config);
if (!serial.Initialize()) { System::ErrorHandler(); }
const std::array<uint16_t, 3> words{0x100, 0x1ff, 0x55};
if (!serial.SendWords(words)) { System::ErrorHandler(); }
```

`SendWords()` / `ReceiveWords()` は1文字を `uint16_t` で表し、送信値は0〜0x1FFです。
`SendWordsDma()` / `ReceiveWordsDma()` は同じ形式でDMA転送します。DMAはhalfword幅、
HALへ渡す長さは文字数です。F7のRXキャッシュ境界条件はバッファのバイト数で判定します。
1回の転送は1〜65535文字で、バッファは2 byte以上のアラインメントが必要です。

`SetRxWordCallback()` と既存の `EnableRxInterrupt()` で1文字ずつ受信できます。
9ビット・パリティなしではバイト用APIを拒否し、受信にはword用コールバックを使います。
word用APIはそれ以外のフレーム設定では拒否します。`printf` コンソールはバイト用です。

### 対応範囲

| 設定・動作 | F303K8 | F446RE | F767ZI |
| --- | --- | --- | --- |
| baud rate・parity・TX/RX mode・oversampling・one-bit sampling | 対応 | 対応 | 対応 |
| 7ビットフレーム | 対応 | 対象外 | 対応 |
| 8/9ビットフレーム、1/2 stop bits | 対応 | 対応 | 対応 |
| 0.5/1.5 stop bits | 対応 | 対象外 | 対応 |
| 反転・SWAP・MSB first・overrun・DMA on RX error | 対応 | 対象外 | 対応 |
| 自動baud rate検出 | USART1 | 対象外 | USART1/2/3/6 |
| RTS/CTS | ピン表の系統 | USART6以外 | 全系統 |
| 9ビット用の通常転送・DMA・受信割り込み | 対応 | 対応 | 対応 |

HALの通常UART初期化・拡張初期化の設定を公開しています。
同期USART、半二重single wire、LIN、RS485の自動DE、multiprocessor/mute、Stop復帰、
Receive-to-IDLE、循環DMA、Smartcard/IrDAは専用APIとしては未対応です。
`GetHandle()` でHALハンドルへアクセスできますが、初期化後に形式や受信方式を直接変更すると
nbedのバッファ・所有権管理と一致しなくなるため、これらをラップ済みとは扱いません。

## 参照資料

ピンとAFはSTの[公開ピン情報](https://github.com/STMicroelectronics/STM32_open_pin_data/tree/7d1f1514ed5583ec5007ad91236b4e1d377295b1)を
MCUパッケージと照合しています。テストの期待値も同じ版のピン情報に基づきます。
詳細は[F303データシート](https://www.st.com/resource/en/datasheet/stm32f303k8.pdf)、
[F446データシート](https://www.st.com/resource/en/datasheet/stm32f446re.pdf)、
[F767データシート](https://www.st.com/resource/en/datasheet/stm32f767zg.pdf)を参照してください。

ホストテストでは全候補の組み合わせと全256ピン値の検証、クロック・AF・IRQ、
各UARTのDMAストリームと完了・エラー、所有権・破棄・再初期化を確認します。
実機での配線・通信・波形確認は別途必要です。
