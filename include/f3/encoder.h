#pragma once
// IWYU pragma: private, include "nbed.h"

#include <cstdint>

#include "interface/encoder.h"
#include "interface/pin_names.h"
#include "stm32f3xx_hal.h"  // IWYU pragma: export
#include "stm32f3xx_hal_tim.h"

namespace nkrc::nbed::stm32::f3 {

/// @brief NUCLEO-F303K8 で A 相・B 相の計数に使える TIM。
enum class EncoderTimer : uint8_t {
  kTim1,  ///< TIM1 を使用する。
  kTim2,  ///< TIM2 を使用する。
  kTim3,  ///< TIM3 を使用する。
};

/// @brief タイマの CH1 と CH2 を使い、A 相・B 相エンコーダーを読み取る。
class Encoder final : public interface::Encoder {
 public:
  /// @brief A 相・B 相エンコーダーから位置を計測するオブジェクトを作る。
  /// @param timer 使用するタイマ。PWM と同時に使用できない。
  /// @param channel_a A 相を接続する CH1 のピン。
  /// @param channel_b B 相を接続する CH2 のピン。
  /// @param counts_per_revolution 両相の両エッジを含む 1 回転当たりのカウント数。
  Encoder(EncoderTimer timer, interface::PinName channel_a,
          interface::PinName channel_b, uint32_t counts_per_revolution);
  /// @brief カウントを停止し、タイマの使用権を解放する。
  ~Encoder() override;

  /// @brief オブジェクトのコピー構築は禁止。
  Encoder(const Encoder&) = delete;
  /// @brief オブジェクトのコピー代入は禁止。
  Encoder& operator=(const Encoder&) = delete;
  /// @brief オブジェクトのムーブ構築は禁止。
  Encoder(Encoder&&) = delete;
  /// @brief オブジェクトのムーブ代入は禁止。
  Encoder& operator=(Encoder&&) = delete;

  /// @brief 2 相入力とカウンターを設定し、計数を始める。
  /// @return 計数できる状態になれば true。
  bool Initialize() override;

  /// @brief A 相・B 相の両エッジを数えた現在の値を返す。
  /// @return 符号付きカウント。カウンター幅で周回し、未初期化では 0。
  [[nodiscard]] int GetCount() override;
  /// @brief カウントと初期設定の 1 回転当たりカウント数から角度を計算する。
  /// @return 累積角度。未初期化では 0 度。
  [[nodiscard]] Angle GetAngle() override;

  /// @brief 現在の位置を原点としてカウントを 0 にする。
  void ResetCount() override;

 private:
  EncoderTimer timer_;
  interface::PinName channel_a_;
  interface::PinName channel_b_;
  uint32_t counts_per_revolution_;
  TIM_HandleTypeDef handle_{};
  bool initialized_{false};
};

}  // namespace nkrc::nbed::stm32::f3
