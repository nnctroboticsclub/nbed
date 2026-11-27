#pragma once
// IWYU pragma: private, include "nbed.h"

#include <cstdint>

#include "detail/dma.h"
#include "detail/uart_config.h"
#include "interface/pin_names.h"
#include "interface/uart.h"
#include "stm32f7xx_hal.h"  // IWYU pragma: export
#include "stm32f7xx_hal_uart.h"

namespace nkrc::nbed::stm32::f7 {

/// @brief NUCLEO-F767ZI で使える UART／USART。
enum class UartPeripheral : uint8_t {
  kUsart2,  ///< USART2 を使用する。
  kUsart3,  ///< USART3 を使用する。
  kUart4,   ///< UART4 を使用する。
  kUsart6,  ///< USART6 を使用する。
  kUsart1,  ///< USART1 を使用する。
  kUart5,   ///< UART5 を使用する。
  kUart7,   ///< UART7 を使用する。
  kUart8,   ///< UART8 を使用する。
};

/// @brief UART のピン割り当て、通信形式、割り込みとタイムアウトの設定。
struct UartConfig {
  UartPeripheral peripheral{
      UartPeripheral::kUsart2};  ///< 使用する UART または USART。
  interface::PinName rx{
      interface::PinName::
          kNC};  ///< ハードウェア上の RX に対応するピン。使わない場合は kNC。
  interface::PinName tx{
      interface::PinName::
          kNC};  ///< ハードウェア上の TX に対応するピン。使わない場合は kNC。
  interface::PinName cts{
      interface::PinName::
          kNC};  ///< ハードウェアフロー制御の CTS。使わない場合は kNC。
  interface::PinName rts{
      interface::PinName::
          kNC};  ///< ハードウェアフロー制御の RTS。使わない場合は kNC。
  UART_InitTypeDef hal{
      detail::
          DefaultUartInit()};  ///< ボーレート、データ長、パリティ、フロー制御などの HAL 設定。
  UART_AdvFeatureInitTypeDef
      advanced{};  ///< 信号・データ反転、TX／RX 入れ替え、MSB 先行などの HAL 拡張設定。
  uint32_t interrupt_priority{
      0};  ///< 割り込み優先度（0～15）。小さい値ほど優先度が高い。
  uint32_t send_timeout{
      HAL_MAX_DELAY};  ///< 通常送信のタイムアウト（ミリ秒）。HAL_MAX_DELAY は無期限。
  uint32_t receive_timeout{
      HAL_MAX_DELAY};  ///< 通常受信のタイムアウト（ミリ秒）。HAL_MAX_DELAY は無期限。
  uint32_t gpio_pull{GPIO_NOPULL};  ///< 各信号ピンの内部プル抵抗の HAL 設定。
  uint32_t gpio_speed{
      GPIO_SPEED_FREQ_VERY_HIGH};  ///< 各信号ピンの GPIO 出力速度の HAL 設定。
};

/// @brief UART／USART を使い、非同期シリアル通信を行う。
class Uart final : public interface::Uart {
 public:
  /// @brief 詳細設定を使って UART オブジェクトを作る。
  /// @param config HAL の通信形式、ピン、割り込み、タイムアウトの設定。
  explicit Uart(UartConfig config);
  /// @brief 8N1 の送受信 UART を作る。RTS／CTS は使用しない。
  /// @param peripheral 使用する UART または USART。
  /// @param rx 受信用ピン。
  /// @param tx 送信用ピン。
  /// @param baud_rate ボーレート（bps）。既定は 921,600。
  /// @param interrupt_priority 割り込み優先度（0～15）。
  Uart(UartPeripheral peripheral, interface::PinName rx, interface::PinName tx,
       uint32_t baud_rate = 921600, uint32_t interrupt_priority = 0);
  /// @brief 通信と DMA を中断し、UART コントローラーと割り込みを解放する。
  ~Uart() override;
  /// @brief オブジェクトのコピー構築は禁止。
  Uart(const Uart&) = delete;
  /// @brief オブジェクトのコピー代入は禁止。
  Uart& operator=(const Uart&) = delete;
  /// @brief オブジェクトのムーブ構築は禁止。
  Uart(Uart&&) = delete;
  /// @brief オブジェクトのムーブ代入は禁止。
  Uart& operator=(Uart&&) = delete;

  /// @brief UART コントローラー、選んだピン、割り込みを設定する。
  /// @return 1 回目の初期化に成功した場合、または初期化済みの場合に true。
  bool Initialize() override;

  /// @brief バイト列をブロッキング送信する。
  /// @param message 送信する 1～65,535 バイト。
  /// @return 転送完了なら true。9 ビット・パリティなしの場合は SendWords() を使う。
  bool Send(interface::UartTxMessage message) override;
  /// @brief 指定サイズのバイト列をブロッキング受信する。
  /// @param[out] message 1～65,535 バイト分の受信バッファ。
  /// @return 全データを受信できれば true。
  bool Receive(interface::UartRxMessage message) override;
  /// @brief 9 ビット・パリティなしでワード列をブロッキング送信する。
  /// @param message 1 文字を uint16_t で表す 1～65,535 要素の配列。
  /// @return 全要素を送信できれば true。
  bool SendWords(interface::UartWordTxMessage message) override;
  /// @brief 9 ビット・パリティなしでワード列をブロッキング受信する。
  /// @param[out] message 1 文字につき uint16_t 1 要素の受信バッファ。
  /// @return 全要素を受信できれば true。
  bool ReceiveWords(interface::UartWordRxMessage message) override;

  /// @brief バイト列の DMA 送信を開始する。
  /// @param message 送信バッファ。DMA 完了または中断まで保持し、書き換えない。
  /// @return 送信を開始できれば true。完了は GetTxDmaStatus() で確認する。
  bool SendDma(interface::UartTxMessage message) override;
  /// @brief バイト列の DMA 受信を開始する。
  /// @param[out] message DMA 完了または中断まで保持する受信バッファ。
  /// @return 受信を開始できれば true。完了は GetRxDmaStatus() で確認する。
  /// @note 受信割り込みとの併用はできない。F7 のキャッシュ有効時は 32 バイト境界とサイズに注意する。
  bool ReceiveDma(interface::UartRxMessage message) override;
  /// @brief 9 ビット・パリティなしでワード列の DMA 送信を開始する。
  /// @param message 完了または中断まで保持する uint16_t の送信バッファ。
  /// @return DMA 送信が始まれば true。
  bool SendWordsDma(interface::UartWordTxMessage message) override;
  /// @brief 9 ビット・パリティなしでワード列の DMA 受信を開始する。
  /// @param[out] message 完了または中断まで保持する uint16_t の受信バッファ。
  /// @return DMA 受信が始まれば true。
  bool ReceiveWordsDma(interface::UartWordRxMessage message) override;
  /// @brief 送信用 DMA の状態を読む。
  /// @return 未開始、転送中、完了、エラー、中断のいずれか。
  [[nodiscard]] interface::DmaStatus GetTxDmaStatus() const override;
  /// @brief 受信用 DMA の状態を読む。
  /// @return 未開始、転送中、完了、エラー、中断のいずれか。
  [[nodiscard]] interface::DmaStatus GetRxDmaStatus() const override;
  /// @brief 送信と受信の DMA を中断し、DMA の使用権を解放する。
  void StopDma() override;

  /// @brief 1 文字ずつ受信する割り込みを開始する。
  /// @return 受信を開始できれば true。
  /// @note 受信 DMA と併用せず、先に対応する受信コールバックを登録する。
  bool EnableRxInterrupt() override;
  /// @brief 1 バイト受信したときに呼ぶ関数を登録する。
  /// @param callback 割り込み内で実行する処理。受信領域は呼び出し中のみ有効。空の関数で解除する。
  void SetRxCallback(const interface::UartRxCallback& callback) override;
  /// @brief 9 ビット・パリティなしで 1 ワード受信したときに呼ぶ関数を登録する。
  /// @param callback 割り込み内で実行する処理。受信領域は呼び出し中のみ有効。空の関数で解除する。
  void SetRxWordCallback(
      const interface::UartWordRxCallback& callback) override;
  /// @brief 初期化済みのバイト送信 UART を printf の出力先にする。
  void SetConsole() override;

  /// @brief HAL の詳細機能を使うためのハンドルを取得する。
  /// @return このオブジェクトが所有する UART ハンドルへの参照。
  /// @note 初期化後に使う。nbed の転送・割り込みと競合する変更に注意する。
  [[nodiscard]] UART_HandleTypeDef& GetHandle();

  /// @brief 該当するコントローラーの割り込みを HAL に渡す。
  /// @param peripheral 割り込みの発生源。
  /// @note HAL 連携用。通常は IRQ ハンドラーから呼ばれる。
  static void HandleInterrupt(UartPeripheral peripheral);
  /// @brief UART 受信完了を処理して受信コールバックまたは DMA 状態へ反映する。
  /// @param handle HAL コールバックが受け取るハンドル。
  /// @note HAL 連携用。アプリケーションからの通常呼び出しは不要。
  static void HandleRxComplete(UART_HandleTypeDef* handle);
  /// @brief UART 送信完了を DMA 状態へ反映する。
  /// @param handle HAL コールバックが受け取るハンドル。
  /// @note HAL 連携用。アプリケーションからの通常呼び出しは不要。
  static void HandleTxComplete(UART_HandleTypeDef* handle);
  /// @brief UART エラーを受信割り込みと DMA に反映する。
  /// @param handle HAL コールバックが受け取るハンドル。
  /// @note HAL 連携用。アプリケーションからの通常呼び出しは不要。
  static void HandleError(UART_HandleTypeDef* handle);

 private:
  [[nodiscard]] bool IsValidPinPair() const;
  [[nodiscard]] bool IsValidConfig() const;
  [[nodiscard]] IRQn_Type Irq() const;
  static Uart*& InstanceFor(UartPeripheral peripheral);
  void OnRxComplete();

  static Uart* FindHandle(UART_HandleTypeDef* handle);
  [[nodiscard]] interface::DmaRequest DmaRequestFor(bool receive) const;
  bool StartTxDma(const uint8_t* data, size_t bytes, uint16_t count,
                  bool halfword);
  bool StartRxDma(uint8_t* data, size_t bytes, uint16_t count, bool halfword);
  detail::Dma tx_dma_;
  detail::Dma rx_dma_;
  volatile interface::DmaStatus tx_dma_status_{interface::DmaStatus::kIdle};
  volatile interface::DmaStatus rx_dma_status_{interface::DmaStatus::kIdle};
  interface::UartRxMessage rx_dma_buffer_{};
  bool rx_interrupt_enabled_{false};

  UartConfig config_;
  UartPeripheral peripheral_;
  interface::PinName rx_;
  interface::PinName tx_;
  uint32_t interrupt_priority_;
  UART_HandleTypeDef handle_{};
  interface::UartRxCallback rx_callback_;
  interface::UartWordRxCallback rx_word_callback_;
  uint16_t rx_word_{0};
  bool initialized_{false};
};

}  // namespace nkrc::nbed::stm32::f7
