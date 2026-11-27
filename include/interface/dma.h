#pragma once

#include <cstdint>

namespace nkrc::nbed::interface {

/// @brief 非同期 DMA 転送の現在の状態。
enum class DmaStatus : uint8_t {
  kIdle,      ///< 転送開始前。
  kBusy,      ///< 転送中。
  kComplete,  ///< 転送完了。
  kError,     ///< 転送エラー。
  kAborted,   ///< 転送を中断した状態。
};

/// @brief PWM の更新または UART の送受信に対応する DMA 要求。
enum class DmaRequest : uint8_t {
  kTim1Update,   ///< TIM1 の更新要求。
  kTim2Update,   ///< TIM2 の更新要求。
  kTim3Update,   ///< TIM3 の更新要求。
  kTim15Update,  ///< TIM15 の更新要求。
  kTim16Update,  ///< TIM16 の更新要求。
  kTim17Update,  ///< TIM17 の更新要求。
  kUsart1Tx,     ///< USART1 の送信要求。
  kUsart1Rx,     ///< USART1 の受信要求。
  kUsart2Tx,     ///< USART2 の送信要求。
  kUsart2Rx,     ///< USART2 の受信要求。
  kUsart3Tx,     ///< USART3 の送信要求。
  kUsart3Rx,     ///< USART3 の受信要求。
  kUart4Tx,      ///< UART4 の送信要求。
  kUart4Rx,      ///< UART4 の受信要求。
  kUsart6Tx,     ///< USART6 の送信要求。
  kUsart6Rx,     ///< USART6 の受信要求。
  kUart5Tx,      ///< UART5 の送信要求。
  kUart5Rx,      ///< UART5 の受信要求。
  kUart7Tx,      ///< UART7 の送信要求。
  kUart7Rx,      ///< UART7 の受信要求。
  kUart8Tx,      ///< UART8 の送信要求。
  kUart8Rx,      ///< UART8 の受信要求。
  kTim4Update,   ///< TIM4 の更新要求。
  kTim5Update,   ///< TIM5 の更新要求。
  kTim8Update,   ///< TIM8 の更新要求。
};

}  // namespace nkrc::nbed::interface
