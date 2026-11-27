# ボード別の周辺機能と対応ピン

ピン番号の頭に `k` を付けて指定します。たとえば表の `PC7` はコードの `kPC7` です。
1 行の各列から、信号ごとに異なるピンを選べます。同じ物理ピンを複数の機能で同時に使うことはできません。
ピン名の後ろの丸括弧には代替機能番号（AF）または ADC チャンネルを示します。nbed がピンに応じて設定します。

表はマイコンのパッケージと nbed の API を基準にしています。コネクターへの引き出し方と、基板上の LED・
ST-LINK・発振器・Ethernet などとの共用は、使用する NUCLEO 基板のユーザーマニュアルで確認してください。
コンソールが使う USART や SWD のピンを再割り当てする場合は、先にボードごとの説明を確認してください。

| ボード | SPI | I²C | UART／USART の数 | CAN | ADC の数 | PWM に使う TIM | エンコーダーに使う TIM |
| --- | --- | --- | --- | --- | --- | --- | --- |
| [NUCLEO-F303K8](#nucleo-f303k8) | 1 | 1 | 2 | 1 | 2 | 1 / 2 / 3 / 15 / 16 / 17 | 1 / 2 / 3 |
| [NUCLEO-F446RE](#nucleo-f446re) | 1～3 | 1～3、FMPI2C1 | 6 | 1～2 | 3 | 1～5 / 8～14 | 1～5 / 8 |
| [NUCLEO-F767ZI](#nucleo-f767zi) | 1～6 | 1～4 | 8 | 1～3 | 3 | 1～5 / 8～14 | 1～5 / 8 |

- F446RE の LQFP64 には、SPI4 用の SCK・MISO・MOSI が引き出されていません。
- PWM とエンコーダーの表は、外部ピンに出力・入力できる通常の TIM を対象にしています。
  F767ZI の低電力タイマ LPTIM1 は今回の実装範囲に含まれません。TIM6／TIM7 は外部チャンネルのない基本タイマです。
- SPI の CS は `DigitalOut` で制御します。UART の表は非同期通信、CAN の表は Classic CAN、
  ADC の表は外部ピンのシングルエンド入力です。

## NUCLEO-F303K8

MCU は STM32F303K8（LQFP32）です。GPIO として指定できるピンは PA0～PA15、PB0～PB1、PB3～PB7、PF0～PF1 です。

[NUCLEO-F303K8 ガイド](f303k8.md)に Arduino コネクターとソルダーブリッジの情報があります。

<a id="f303k8-spi"></a>

### F303K8 の SPI

| 周辺機能 | 列挙値 | SCK | MISO | MOSI |
| --- | --- | --- | --- | --- |
| SPI1 | `kSpi1` | PA5 (AF5) / PB3 (AF5) | PA6 (AF5) / PB4 (AF5) | PA7 (AF5) / PB5 (AF5) |

<a id="f303k8-i2c"></a>

### F303K8 の I²C

| 周辺機能 | 列挙値 | SCL | SDA |
| --- | --- | --- | --- |
| I2C1 | `kI2c1` | PA15 (AF4) / PB6 (AF4) | PA14 (AF4) / PB7 (AF4) |

<a id="f303k8-uart"></a>

### F303K8 の UART

| 周辺機能 | 列挙値 | RX | TX | CTS | RTS |
| --- | --- | --- | --- | --- | --- |
| USART1 | `kUsart1` | PA10 (AF7) / PB7 (AF7) | PA9 (AF7) / PB6 (AF7) | PA11 (AF7) | PA12 (AF7) |
| USART2 | `kUsart2` | PA3 (AF7) / PA15 (AF7) / PB4 (AF7) | PA2 (AF7) / PA14 (AF7) / PB3 (AF7) | PA0 (AF7) | PA1 (AF7) |

<a id="f303k8-can"></a>

### F303K8 の CAN

| 周辺機能 | 列挙値 | RX | TX |
| --- | --- | --- | --- |
| CAN1 | `kCan1` | PA11 (AF9) | PA12 (AF9) |

<a id="f303k8-tim"></a>

### F303K8 の PWM とエンコーダー

エンコーダーには TIM 1 / 2 / 3 の CH1（A 相）と CH2（B 相）を使います。

| 周辺機能 | 列挙値 | CH1 | CH2 | CH3 | CH4 | CH1N | CH2N | CH3N |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| TIM1 | `kTim1` | PA8 (AF6) | PA9 (AF6) | PA10 (AF6) | PA11 (AF11) | PA7 (AF6) / PA11 (AF6) | PA12 (AF6) / PB0 (AF6) | PB1 (AF6) / PF0 (AF6) |
| TIM2 | `kTim2` | PA0 (AF1) / PA5 (AF1) / PA15 (AF1) | PA1 (AF1) / PB3 (AF1) | PA2 (AF1) / PA9 (AF10) | PA3 (AF1) / PA10 (AF10) | — | — | — |
| TIM3 | `kTim3` | PA6 (AF2) / PB4 (AF2) | PA4 (AF2) / PA7 (AF2) / PB5 (AF2) | PB0 (AF2) | PB1 (AF2) / PB7 (AF10) | — | — | — |
| TIM15 | `kTim15` | PA2 (AF9) | PA3 (AF9) | — | — | PA1 (AF9) | — | — |
| TIM16 | `kTim16` | PA6 (AF1) / PA12 (AF1) / PB4 (AF1) | — | — | — | PA13 (AF1) / PB6 (AF1) | — | — |
| TIM17 | `kTim17` | PA7 (AF1) / PB5 (AF10) | — | — | — | PB7 (AF1) | — | — |

<a id="f303k8-adc"></a>

### F303K8 の ADC

ピンだけを指定すると対応 ADC を自動選択します。ADC1 と他の ADC で共通のピンは ADC1 を優先します。

| 周辺機能 | 列挙値 | 入力ピン（チャンネル） |
| --- | --- | --- |
| ADC1 | `kAdc1` | PA0 (1) / PA1 (2) / PA2 (3) / PA3 (4) / PB0 (11) / PB1 (12) |
| ADC2 | `kAdc2` | PA4 (1) / PA5 (2) / PA6 (3) / PA7 (4) |

## NUCLEO-F446RE

MCU は STM32F446RE（LQFP64）です。GPIO として指定できるピンは PA0～PA15、PB0～PB10、PB12～PB15、PC0～PC15、PD2、PH0～PH1 です。

[README の F446RE コネクター早見表](../README.md#基板上でまず使える信号)を参照してください。

<a id="f446re-spi"></a>

### F446RE の SPI

| 周辺機能 | 列挙値 | SCK | MISO | MOSI |
| --- | --- | --- | --- | --- |
| SPI1 | `kSpi1` | PA5 (AF5) / PB3 (AF5) | PA6 (AF5) / PB4 (AF5) | PA7 (AF5) / PB5 (AF5) |
| SPI2 | `kSpi2` | PA9 (AF5) / PB10 (AF5) / PB13 (AF5) / PC7 (AF5) | PB14 (AF5) / PC2 (AF5) | PB15 (AF5) / PC1 (AF7) / PC3 (AF5) |
| SPI3 | `kSpi3` | PB3 (AF6) / PC10 (AF6) | PB4 (AF6) / PC11 (AF6) | PB0 (AF7) / PB2 (AF7) / PB5 (AF6) / PC1 (AF5) / PC12 (AF6) |

<a id="f446re-i2c"></a>

### F446RE の I²C

| 周辺機能 | 列挙値 | SCL | SDA |
| --- | --- | --- | --- |
| I2C1 | `kI2c1` | PB6 (AF4) / PB8 (AF4) | PB7 (AF4) / PB9 (AF4) |
| I2C2 | `kI2c2` | PB10 (AF4) | PB3 (AF4) / PC12 (AF4) |
| I2C3 | `kI2c3` | PA8 (AF4) | PB4 (AF4) / PC9 (AF4) |
| FMPI2C1 | `kFmpi2c1` | PC6 (AF4) | PC7 (AF4) |

<a id="f446re-uart"></a>

### F446RE の UART

| 周辺機能 | 列挙値 | RX | TX | CTS | RTS |
| --- | --- | --- | --- | --- | --- |
| USART1 | `kUsart1` | PA10 (AF7) / PB7 (AF7) | PA9 (AF7) / PB6 (AF7) | PA11 (AF7) | PA12 (AF7) |
| USART2 | `kUsart2` | PA3 (AF7) | PA2 (AF7) | PA0 (AF7) | PA1 (AF7) |
| USART3 | `kUsart3` | PC5 (AF7) / PC11 (AF7) | PB10 (AF7) / PC10 (AF7) | PB13 (AF7) | PB14 (AF7) |
| UART4 | `kUart4` | PA1 (AF8) / PC11 (AF8) | PA0 (AF8) / PC10 (AF8) | PB0 (AF8) | PA15 (AF8) |
| UART5 | `kUart5` | PD2 (AF8) | PC12 (AF8) | PC9 (AF7) | PC8 (AF7) |
| USART6 | `kUsart6` | PC7 (AF8) | PC6 (AF8) | — | — |

<a id="f446re-can"></a>

### F446RE の CAN

| 周辺機能 | 列挙値 | RX | TX |
| --- | --- | --- | --- |
| CAN1 | `kCan1` | PA11 (AF9) / PB8 (AF9) | PA12 (AF9) / PB9 (AF9) |
| CAN2 | `kCan2` | PB5 (AF9) / PB12 (AF9) | PB6 (AF9) / PB13 (AF9) |

<a id="f446re-tim"></a>

### F446RE の PWM とエンコーダー

エンコーダーには TIM 1～5 / 8 の CH1（A 相）と CH2（B 相）を使います。

| 周辺機能 | 列挙値 | CH1 | CH2 | CH3 | CH4 | CH1N | CH2N | CH3N |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| TIM1 | `kTim1` | PA8 (AF1) | PA9 (AF1) | PA10 (AF1) | PA11 (AF1) | PA7 (AF1) / PB13 (AF1) | PB0 (AF1) / PB14 (AF1) | PB1 (AF1) / PB15 (AF1) |
| TIM2 | `kTim2` | PA0 (AF1) / PA5 (AF1) / PA15 (AF1) / PB8 (AF1) | PA1 (AF1) / PB3 (AF1) / PB9 (AF1) | PA2 (AF1) / PB10 (AF1) | PA3 (AF1) / PB2 (AF1) | — | — | — |
| TIM3 | `kTim3` | PA6 (AF2) / PB4 (AF2) / PC6 (AF2) | PA7 (AF2) / PB5 (AF2) / PC7 (AF2) | PB0 (AF2) / PC8 (AF2) | PB1 (AF2) / PC9 (AF2) | — | — | — |
| TIM4 | `kTim4` | PB6 (AF2) | PB7 (AF2) | PB8 (AF2) | PB9 (AF2) | — | — | — |
| TIM5 | `kTim5` | PA0 (AF2) | PA1 (AF2) | PA2 (AF2) | PA3 (AF2) | — | — | — |
| TIM8 | `kTim8` | PC6 (AF3) | PC7 (AF3) | PC8 (AF3) | PC9 (AF3) | PA5 (AF3) / PA7 (AF3) | PB0 (AF3) / PB14 (AF3) | PB1 (AF3) / PB15 (AF3) |
| TIM9 | `kTim9` | PA2 (AF3) | PA3 (AF3) | — | — | — | — | — |
| TIM10 | `kTim10` | PB8 (AF3) | — | — | — | — | — | — |
| TIM11 | `kTim11` | PB9 (AF3) | — | — | — | — | — | — |
| TIM12 | `kTim12` | PB14 (AF9) | PB15 (AF9) | — | — | — | — | — |
| TIM13 | `kTim13` | PA6 (AF9) | — | — | — | — | — | — |
| TIM14 | `kTim14` | PA7 (AF9) | — | — | — | — | — | — |

<a id="f446re-adc"></a>

### F446RE の ADC

ピンだけを指定すると対応 ADC を自動選択します。ADC1 と他の ADC で共通のピンは ADC1 を優先します。

| 周辺機能 | 列挙値 | 入力ピン（チャンネル） |
| --- | --- | --- |
| ADC1 | `kAdc1` | PA0 (0) / PA1 (1) / PA2 (2) / PA3 (3) / PA4 (4) / PA5 (5) / PA6 (6) / PA7 (7) / PB0 (8) / PB1 (9) / PC0 (10) / PC1 (11) / PC2 (12) / PC3 (13) / PC4 (14) / PC5 (15) |
| ADC2 | `kAdc2` | PA0 (0) / PA1 (1) / PA2 (2) / PA3 (3) / PA4 (4) / PA5 (5) / PA6 (6) / PA7 (7) / PB0 (8) / PB1 (9) / PC0 (10) / PC1 (11) / PC2 (12) / PC3 (13) / PC4 (14) / PC5 (15) |
| ADC3 | `kAdc3` | PA0 (0) / PA1 (1) / PA2 (2) / PA3 (3) / PC0 (10) / PC1 (11) / PC2 (12) / PC3 (13) |

## NUCLEO-F767ZI

MCU は STM32F767ZI（LQFP144）です。GPIO として指定できるピンは PA0～PG15（A～G 各ポートの 0～15）、PH0～PH1 です。

[NUCLEO-F767ZI ガイド](f767zi.md)に Arduino／Zio コネクターと基板上の共有信号があります。

<a id="f767zi-spi"></a>

### F767ZI の SPI

| 周辺機能 | 列挙値 | SCK | MISO | MOSI |
| --- | --- | --- | --- | --- |
| SPI1 | `kSpi1` | PA5 (AF5) / PB3 (AF5) / PG11 (AF5) | PA6 (AF5) / PB4 (AF5) / PG9 (AF5) | PA7 (AF5) / PB5 (AF5) / PD7 (AF5) |
| SPI2 | `kSpi2` | PA9 (AF5) / PA12 (AF5) / PB10 (AF5) / PB13 (AF5) / PD3 (AF5) | PB14 (AF5) / PC2 (AF5) | PB15 (AF5) / PC1 (AF5) / PC3 (AF5) |
| SPI3 | `kSpi3` | PB3 (AF6) / PC10 (AF6) | PB4 (AF6) / PC11 (AF6) | PB2 (AF7) / PB5 (AF6) / PC12 (AF6) / PD6 (AF5) |
| SPI4 | `kSpi4` | PE2 (AF5) / PE12 (AF5) | PE5 (AF5) / PE13 (AF5) | PE6 (AF5) / PE14 (AF5) |
| SPI5 | `kSpi5` | PF7 (AF5) | PF8 (AF5) | PF9 (AF5) / PF11 (AF5) |
| SPI6 | `kSpi6` | PA5 (AF8) / PB3 (AF8) / PG13 (AF5) | PA6 (AF8) / PB4 (AF8) / PG12 (AF5) | PA7 (AF8) / PB5 (AF8) / PG14 (AF5) |

<a id="f767zi-i2c"></a>

### F767ZI の I²C

| 周辺機能 | 列挙値 | SCL | SDA |
| --- | --- | --- | --- |
| I2C1 | `kI2c1` | PB6 (AF4) / PB8 (AF4) | PB7 (AF4) / PB9 (AF4) |
| I2C2 | `kI2c2` | PB10 (AF4) / PF1 (AF4) | PB11 (AF4) / PF0 (AF4) |
| I2C3 | `kI2c3` | PA8 (AF4) | PC9 (AF4) |
| I2C4 | `kI2c4` | PB6 (AF11) / PB8 (AF1) / PD12 (AF4) / PF14 (AF4) | PB7 (AF11) / PB9 (AF1) / PD13 (AF4) / PF15 (AF4) |

<a id="f767zi-uart"></a>

### F767ZI の UART

| 周辺機能 | 列挙値 | RX | TX | CTS | RTS |
| --- | --- | --- | --- | --- | --- |
| USART1 | `kUsart1` | PA10 (AF7) / PB7 (AF7) / PB15 (AF4) | PA9 (AF7) / PB6 (AF7) / PB14 (AF4) | PA11 (AF7) | PA12 (AF7) |
| USART2 | `kUsart2` | PA3 (AF7) / PD6 (AF7) | PA2 (AF7) / PD5 (AF7) | PA0 (AF7) / PD3 (AF7) | PA1 (AF7) / PD4 (AF7) |
| USART3 | `kUsart3` | PB11 (AF7) / PC11 (AF7) / PD9 (AF7) | PB10 (AF7) / PC10 (AF7) / PD8 (AF7) | PB13 (AF7) / PD11 (AF7) | PB14 (AF7) / PD12 (AF7) |
| UART4 | `kUart4` | PA1 (AF8) / PA11 (AF6) / PC11 (AF8) / PD0 (AF8) | PA0 (AF8) / PA12 (AF6) / PC10 (AF8) / PD1 (AF8) | PB0 (AF8) / PB15 (AF8) | PA15 (AF8) / PB14 (AF8) |
| UART5 | `kUart5` | PB5 (AF1) / PB8 (AF7) / PB12 (AF8) / PD2 (AF8) | PB6 (AF1) / PB9 (AF7) / PB13 (AF8) / PC12 (AF8) | PC9 (AF7) | PC8 (AF7) |
| USART6 | `kUsart6` | PC7 (AF8) / PG9 (AF8) | PC6 (AF8) / PG14 (AF8) | PG13 (AF8) / PG15 (AF8) | PG8 (AF8) / PG12 (AF8) |
| UART7 | `kUart7` | PA8 (AF12) / PB3 (AF12) / PE7 (AF8) / PF6 (AF8) | PA15 (AF12) / PB4 (AF12) / PE8 (AF8) / PF7 (AF8) | PE10 (AF8) / PF9 (AF8) | PE9 (AF8) / PF8 (AF8) |
| UART8 | `kUart8` | PE0 (AF8) | PE1 (AF8) | PD14 (AF8) | PD15 (AF8) |

<a id="f767zi-can"></a>

### F767ZI の CAN

| 周辺機能 | 列挙値 | RX | TX |
| --- | --- | --- | --- |
| CAN1 | `kCan1` | PA11 (AF9) / PB8 (AF9) / PD0 (AF9) | PA12 (AF9) / PB9 (AF9) / PD1 (AF9) |
| CAN2 | `kCan2` | PB5 (AF9) / PB12 (AF9) | PB6 (AF9) / PB13 (AF9) |
| CAN3 | `kCan3` | PA8 (AF11) / PB3 (AF11) | PA15 (AF11) / PB4 (AF11) |

<a id="f767zi-tim"></a>

### F767ZI の PWM とエンコーダー

エンコーダーには TIM 1～5 / 8 の CH1（A 相）と CH2（B 相）を使います。

| 周辺機能 | 列挙値 | CH1 | CH2 | CH3 | CH4 | CH1N | CH2N | CH3N |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| TIM1 | `kTim1` | PA8 (AF1) / PE9 (AF1) | PA9 (AF1) / PE11 (AF1) | PA10 (AF1) / PE13 (AF1) | PA11 (AF1) / PE14 (AF1) | PA7 (AF1) / PB13 (AF1) / PE8 (AF1) | PB0 (AF1) / PB14 (AF1) / PE10 (AF1) | PB1 (AF1) / PB15 (AF1) / PE12 (AF1) |
| TIM2 | `kTim2` | PA0 (AF1) / PA5 (AF1) / PA15 (AF1) | PA1 (AF1) / PB3 (AF1) | PA2 (AF1) / PB10 (AF1) | PA3 (AF1) / PB11 (AF1) | — | — | — |
| TIM3 | `kTim3` | PA6 (AF2) / PB4 (AF2) / PC6 (AF2) | PA7 (AF2) / PB5 (AF2) / PC7 (AF2) | PB0 (AF2) / PC8 (AF2) | PB1 (AF2) / PC9 (AF2) | — | — | — |
| TIM4 | `kTim4` | PB6 (AF2) / PD12 (AF2) | PB7 (AF2) / PD13 (AF2) | PB8 (AF2) / PD14 (AF2) | PB9 (AF2) / PD15 (AF2) | — | — | — |
| TIM5 | `kTim5` | PA0 (AF2) | PA1 (AF2) | PA2 (AF2) | PA3 (AF2) | — | — | — |
| TIM8 | `kTim8` | PC6 (AF3) | PC7 (AF3) | PC8 (AF3) | PC9 (AF3) | PA5 (AF3) / PA7 (AF3) | PB0 (AF3) / PB14 (AF3) | PB1 (AF3) / PB15 (AF3) |
| TIM9 | `kTim9` | PA2 (AF3) / PE5 (AF3) | PA3 (AF3) / PE6 (AF3) | — | — | — | — | — |
| TIM10 | `kTim10` | PB8 (AF3) / PF6 (AF3) | — | — | — | — | — | — |
| TIM11 | `kTim11` | PB9 (AF3) / PF7 (AF3) | — | — | — | — | — | — |
| TIM12 | `kTim12` | PB14 (AF9) | PB15 (AF9) | — | — | — | — | — |
| TIM13 | `kTim13` | PA6 (AF9) / PF8 (AF9) | — | — | — | — | — | — |
| TIM14 | `kTim14` | PA7 (AF9) / PF9 (AF9) | — | — | — | — | — | — |

<a id="f767zi-adc"></a>

### F767ZI の ADC

ピンだけを指定すると対応 ADC を自動選択します。ADC1 と他の ADC で共通のピンは ADC1 を優先します。

| 周辺機能 | 列挙値 | 入力ピン（チャンネル） |
| --- | --- | --- |
| ADC1 | `kAdc1` | PA0 (0) / PA1 (1) / PA2 (2) / PA3 (3) / PA4 (4) / PA5 (5) / PA6 (6) / PA7 (7) / PB0 (8) / PB1 (9) / PC0 (10) / PC1 (11) / PC2 (12) / PC3 (13) / PC4 (14) / PC5 (15) |
| ADC2 | `kAdc2` | PA0 (0) / PA1 (1) / PA2 (2) / PA3 (3) / PA4 (4) / PA5 (5) / PA6 (6) / PA7 (7) / PB0 (8) / PB1 (9) / PC0 (10) / PC1 (11) / PC2 (12) / PC3 (13) / PC4 (14) / PC5 (15) |
| ADC3 | `kAdc3` | PA0 (0) / PA1 (1) / PA2 (2) / PA3 (3) / PC0 (10) / PC1 (11) / PC2 (12) / PC3 (13) / PF3 (9) / PF4 (14) / PF5 (15) / PF6 (4) / PF7 (5) / PF8 (6) / PF9 (7) / PF10 (8) |

## 開発時の照合

`python scripts/check_pin_routes.py` で、ST のパッケージ別 XML と GPIO の代替機能 XML を取得し、
実装が持つ周辺機能番号・信号・ピン・AF／ADC チャンネルを比較できます。
ST のデータを取得済みの場合は `--data-dir` にチェックアウト先を指定できます。

- [ST の STM32_open_pin_data（照合に使用したリビジョン）](https://github.com/STMicroelectronics/STM32_open_pin_data/tree/45c04599e2e0a788c5ed9b6218dc5a28a36041f7)
- F303K8 の [データシート DS9866](https://www.st.com/resource/en/datasheet/stm32f303k8.pdf)、[リファレンスマニュアル RM0316](https://www.st.com/resource/en/reference_manual/rm0316-stm32f303xbcde-stm32f303x68-stm32f328x8-stm32f358xc-stm32f398xe-advanced-armbased-mcus-stmicroelectronics.pdf)
- F446RE の [データシート DS10693](https://www.st.com/resource/en/datasheet/stm32f446re.pdf)、[リファレンスマニュアル RM0390](https://www.st.com/resource/en/reference_manual/dm00135183.pdf)
- F767ZI の [データシート DS11532](https://www.st.com/resource/en/datasheet/stm32f767zi.pdf)、[リファレンスマニュアル RM0410](https://www.st.com/resource/en/reference_manual/rm0410-stm32f76xxx-and-stm32f77xxx-advanced-armbased-32bit-mcus-stmicroelectronics.pdf)
