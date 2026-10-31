# DMA と WS2812B

v0.10.0 から F303K8・F446RE・F767ZI で PWM の波形転送と UART の送受信に
DMA を使えます。通常の PWM、UART の `Send()` / `Receive()` は従来どおりです。
DMA は使用時に初期化し、同じチャネル・ストリームの二重予約を拒否します。

## WS2812B を動かす

[`examples/ws2812b/main.cc`](../examples/ws2812b/main.cc) は16個の LED を赤・緑・青へ
500 ms ごとに切り替えます。3ボードとも TIM1 CH1 / PA8 を使用します。
LED 個数は `kLedCount` で変更できます。通常の SRAM に置く静的バッファを使用し、
低い輝度から動作確認します。

```sh
pio run -e f4-ws2812b
pio run -e f4-ws2812b -t upload
# F303K8: -e f3-ws2812b / F767ZI: -e f7-ws2812b
```

接続は PA8 → 5 V 対応のロジックレベル変換器 → 330 Ω → ストリップの DIN です。
ストリップには個数に合う外部5 V電源を使い、マイコン・変換器・電源の GND を共通にします。
3.3 V 信号での直接接続を前提にしません。DOUT と DIN の方向にも注意してください。
F303K8 の PA8 は D9 です。ほかのボードはボードの端子表で PA8 を確認してください。

800 kHz の PWM を使い、GRB の順・各色 MSB から24ビットを出力します。
`GetPeriodTicks()` に対して0ビットは約28%、1ビットは約56%の比較値とし、
High 幅を約0.35 µs / 0.70 µsにします。固定のCPUクロック・CCR値には依存しません。
末尾には300 µs以上の Low と、プリロード用の2要素を含めます。
LED の世代によるタイミング差は使用する部品のデータシートを確認してください。

## PWM の波形転送

```cpp
Pwm output(kTim1, kCh1, kPA8, 800'000);
if (!output.Initialize()) { System::ErrorHandler(); }
// 比較値は0以上、GetPeriodTicks()未満。末尾2要素は必ず0。
static std::array<uint32_t, 5> waveform{20, 40, 20, 0, 0};
if (!output.SendDataDma(waveform)) { System::ErrorHandler(); }
// メインループではほかの処理を続行できる。
if (output.GetDmaStatus() == DmaStatus::kComplete) {
  // バッファを再利用できる。
}
```

サンプルの比較値は生のタイマtickです。WS2812Bには同梱のエンコード例を使用してください。
`SendDataDma()` は開始できれば `true` を返し、転送は非同期に進みます。
未初期化、転送中、空・短すぎるデータ、65535要素超、範囲外の比較値、
末尾2要素が0以外、DMAに使えないメモリ、DMA競合の場合は `false` です。
拒否した入力は転送しません。HAL の開始エラーは状態を `kError` にして資源を解放します。

更新イベントで CCR のプリロードへ転送するため、デューティ0でも転送が継続します。
開始時には2周期の Low が入り、その後に指定した各比較値を1周期ずつ出力します。
末尾2要素はプリロードの待ちを吸収するために必要です。`kComplete` の時点で
最後の非ゼロのパルスまで出力済みとなり、出力は Low を維持します。
Low の期間を数える場合は、末尾ゼロの個数からこの2要素を除いて計算してください。

DMA使用中はそのタイマを占有します。別チャンネルが初期化済みなら開始を拒否し、
資源を保持している間は別チャンネルの初期化も拒否します。
通常のCH出力・High極性・デッドタイム0だけに対応し、N出力・相補ペアのDMAは未対応です。
転送中の周波数・デューティ比・パルス幅の変更は `false` を返します。

`StopDma()` は要求停止、転送中断、Lowへの復帰、DMA資源の解放を行います。
PWMオブジェクト自体は初期化済みのままで、再送や通常PWMへ戻れます。
`Stop()` / デストラクタは DMA と PWM の両方を停止し、タイマ予約も解放します。
転送完了後も資源を保持するため、ほかの用途へ譲る際は `StopDma()` を呼んでください。

## UART の DMA

```cpp
Uart uart(kUsart2, kPA3, kPA2, 115200);
if (!uart.Initialize()) { System::ErrorHandler(); }
alignas(32) static std::array<uint8_t, 32> receive_buffer{};
if (!uart.ReceiveDma(receive_buffer)) { System::ErrorHandler(); }
// 指定した32バイトを受信すると kComplete。
// 完了を確認してから受信バッファを読む。
```

`SendDma(span<const uint8_t>)` / `ReceiveDma(span<uint8_t>)` は非同期の単発転送です。
長さは1〜65535バイトです。TXとRXは別々のDMA資源を使用し、同時に動かせます。
送信ではバッファを完了まで変更しないでください。受信では完了まで読み書きしないでください。
TXはDMAのメモリ読出しに加え、UARTの最後のビットの送信完了をIRQで確認してから
`GetTxDmaStatus()` が `kComplete` になります。

受信は指定バイト数が揃うまで `kBusy` です。IDLE検出、可変長受信、リング・循環転送は
提供していません。アプリ側で期限を設け、必要なら `StopDma()` で中断してください。
[`examples/uart-dma/main.cc`](../examples/uart-dma/main.cc) は32バイト受信→返信の例で、
受信10秒・送信100 msの期限を設けています。環境名は `f3-uart-dma` / `f4-uart-dma` /
`f7-uart-dma`、通信速度は115200 bpsです。既定コンソールを無効にしてビルドします。

受信DMAの完了では既存の `SetRxCallback()` を呼ばず、1バイト受信割り込みを再開しません。
`EnableRxInterrupt()` による受信と受信DMAは排他的です。DMA受信中の `Receive()`、
DMA送信中の `Send()` は `false` です。`printf` と送信DMAの同時利用を避けてください。
`StopDma()` はDMA送受信を止めて予約を解放します。通常の受信割り込みは停止しません。

UARTのエラー時は影響するDMA転送を停止し、状態を `kError` にします。
エラー・中断時の受信バッファは不完全なのでメッセージとして使用しないでください。

## 状態とバッファの寿命

| 状態 | 意味 |
| --- | --- |
| `kIdle` | まだ開始していない |
| `kBusy` | 転送中。バッファを保持する |
| `kComplete` | 転送完了。バッファを再利用できる |
| `kError` | HAL転送エラー。受信内容は破棄する |
| `kAborted` | `StopDma()` / `Stop()` で転送を中断した |

送信バッファは完了・中断まで生存し、内容を変更しない必要があります。
ローカル変数を渡してその関数から戻る使い方は避けてください。
IRQ・SysTickを動かした状態で呼び出し、完了待ちにはアプリ側のタイムアウトを設けます。
同じオブジェクトへの操作はメイン側の1つの実行コンテキストから行ってください。

## DMA の割当て

F303K8はリマップしない既定のDMA1チャネルを使います。HALを直接操作する場合は
SYSCFGのDMAリマップや、nbedが予約したDMAの設定を変更しないでください。

| 要求 | F303K8 | F446RE / F767ZI |
| --- | --- | --- |
| TIM1 update | DMA1 CH5 | DMA2 stream5 / channel6 |
| TIM2 update | DMA1 CH2 | DMA1 stream1 / channel3 |
| TIM3 update | DMA1 CH3 | DMA1 stream2 / channel5 |
| TIM15 update | DMA1 CH5 | 未対応 |
| TIM16 update | DMA1 CH3 | 未対応 |
| TIM17 update | DMA1 CH1 | 未対応 |
| USART1 TX / RX | DMA1 CH4 / CH5 | DMA2 stream7 / stream2、channel4 |
| USART2 TX / RX | DMA1 CH7 / CH6 | DMA1 stream6 / stream5、channel4 |
| USART3 TX / RX | 未対応 | DMA1 stream3 / stream1、channel4 |
| UART4 TX / RX | 未対応 | DMA1 stream4 / stream2、channel4 |
| UART5 TX / RX | 未対応 | DMA1 stream7 / stream0、channel4 |
| USART6 TX / RX | 未対応 | DMA2 stream6 / stream1、channel5 |
| UART7 TX / RX | 未対応 | F767ZIのみ DMA1 stream1 / stream3、channel5 |
| UART8 TX / RX | 未対応 | F767ZIのみ DMA1 stream0 / stream6、channel5 |

v0.11.0から、対応MCUの全UARTでDMAを使用できます。
F7のUART5受信とUART8送信のように、channel番号が異なっても同じstreamは同時に使用できません。

例えばF4/F7のTIM2 PWM DMAとUSART3受信DMA、TIM3 PWM DMAとUART4受信DMAは競合します。
後から開始した側は `false` になり、先に使用している転送を維持します。
予約はnbed内のDMA利用間で管理します。外部HALコードの所有権は検出できません。
自動的な代替ストリーム選択は行いません。

## メモリと F7 のキャッシュ

DMAバッファはF3の通常SRAM12 KiB、F4のSRAM128 KiB、F7のSRAM1/2領域
`0x20020000`〜`0x2007FFFF`に置きます。Flash上の定数配列、CCM、F7のDTCMなど
このAPIの対応領域外は拒否します。付属リンカスクリプトの通常の静的変数・ヒープ・スタックは
対応領域です。PWMのバッファには32 bit境界のアラインメントが必要です。

F7はD-cacheの有効・無効を呼出し側の設定のまま保ちます。有効ならTX開始前にcleanし、
RX開始前にclean/invalidate、完了・中断・エラー時にinvalidateします。
RXは先頭アドレスが32 byte境界、長さも32の倍数で、キャッシュラインをほかの変数と
共有しないバッファに限ります。転送中にキャッシュの設定やメモリ属性を変更しないでください。
DMAバッファを別名のアドレス経由で参照したり、ほかのDMAと共有したりしないでください。

## 互換性・検証範囲

予約APIだった `Pwm::SendDataDma()` の戻り値を `void` から `bool` に変更しました。
呼出し結果を無視していた既存ソースはそのままコンパイルできますが、開始結果を確認してください。
独自の `interface::Pwm` / `interface::Uart` 実装は新メソッドに対応する必要があります。
旧F4の内部用 `Dma1`〜`Dma7` / `DmaManager` を、要求に応じた所有権管理へ置き換えました。
ADC・SPIのDMA、循環転送、PWM相補出力のDMAは対象外です。

ホストテストでは割当て、二重予約、開始失敗、中断・破棄、状態、入力境界、GRBエンコード、
PWMとUARTの競合、F7キャッシュ操作を検証します。実HALでの3系列のビルドも確認します。
実機への書込み・WS2812B点灯・信号の波形・高負荷でのDMA遅延は未検証です。

参照: ST [RM0316](https://www.st.com/resource/en/reference_manual/dm00043574.pdf)、
[RM0390](https://www.st.com/resource/en/reference_manual/dm00135183.pdf)、
[RM0410](https://www.st.com/resource/en/reference_manual/rm0410-stm32f76xxx-and-stm32f77xxx-advanced-armbased-32bit-mcus-stmicroelectronics.pdf)、
[WORLDSEMI製品一覧](https://www.world-semi.com/web/index.php?classid=302&id=299&lanstr=en&topclassid=16)。
