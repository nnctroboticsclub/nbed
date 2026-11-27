#pragma once

#include <cstdint>

namespace nkrc::nbed::stm32::f4 {

/// @brief PWM とエンコーダーがタイマを共有するときの内部の所有権管理。
class TimerReservation final {
 public:
  /// @brief タイマの使用権は静的メソッドで管理する。
  TimerReservation() = delete;

  /// @brief PWM がチャンネルを使えるか確認して予約する。
  /// @param timer PwmTimer の列挙値。
  /// @param channel 0 から数えたチャンネル番号。
  /// @return 予約できれば true。
  [[nodiscard]] static bool ClaimPwm(uint8_t timer, uint8_t channel);
  /// @brief PWM が予約していたチャンネルを解放する。
  /// @param timer PwmTimer の列挙値。
  /// @param channel 0 から数えたチャンネル番号。
  static void ReleasePwm(uint8_t timer, uint8_t channel);

  /// @brief エンコーダー用にタイマ全体を予約する。
  /// @param timer EncoderTimer の列挙値。
  /// @return 予約できれば true。
  [[nodiscard]] static bool ClaimEncoder(uint8_t timer);
  /// @brief エンコーダー用に予約したタイマを解放する。
  /// @param timer EncoderTimer の列挙値。
  static void ReleaseEncoder(uint8_t timer);
};

}  // namespace nkrc::nbed::stm32::f4
