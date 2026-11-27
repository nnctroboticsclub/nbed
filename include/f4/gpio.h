#pragma once
// IWYU pragma: private, include "nbed.h"

#include <cstdint>

#include "interface/digital_in.h"
#include "interface/digital_out.h"
#include "interface/pin_names.h"
#include "stm32f4xx_hal.h"

namespace nkrc::nbed::stm32::f4 {

/// @brief GPIO 入力に使う内部プル抵抗。
enum class GpioPull : uint8_t {
  kNone,  ///< 内部抵抗なし。
  kUp,    ///< プルアップ。
  kDown,  ///< プルダウン。
};

/// @brief GPIO 出力ピンに High／Low を出力する。
class DigitalOut final : public interface::DigitalOut {
 public:
  // DigitalIn の現在の入力値をそのまま出力へ書き込めるようにする。
  using interface::DigitalOut::operator=;

  /// @brief デジタル出力ピンと初期出力を指定する。
  /// @param pin 出力ピン。
  /// @param initial_value Initialize() で設定する出力レベル。既定は Low。
  explicit DigitalOut(interface::PinName pin, bool initial_value = false);

  /// @brief ピンを出力に設定し、指定した初期値を出力する。
  /// @return 使用できる状態になれば true。
  bool Initialize() override;

  /// @brief 出力レベルを設定する。未初期化なら何もしない。
  /// @param value true で High、false で Low。
  void Write(bool value) override;
  /// @brief ピンで読み取った現在のレベルを返す。
  /// @return High なら true。Low または未初期化では false。
  [[nodiscard]] bool Read() const override;

  /// @brief 指定した論理値を出力する。
  /// @param value true で High、false で Low。
  /// @return この出力オブジェクト。
  DigitalOut& operator=(bool value) override {
    Write(value);
    return *this;
  }

 private:
  interface::PinName pin_;
  bool initial_value_;
  bool initialized_{false};
  GPIO_TypeDef* port_{nullptr};
  uint16_t pin_mask_{0};
};

/// @brief GPIO 入力ピンから High／Low を読み取る。
class DigitalIn final : public interface::DigitalIn {
 public:
  /// @brief デジタル入力ピンを指定する。
  /// @param pin 入力ピン。
  /// @param pull 未接続時の論理レベルを決める内部プル抵抗。
  explicit DigitalIn(interface::PinName pin, GpioPull pull = GpioPull::kNone);

  /// @brief 指定したピンをデジタル入力に設定する。
  /// @return 使用できる状態になれば true。
  bool Initialize() override;

  /// @brief 入力ピンの現在のレベルを読む。
  /// @return High なら true。Low または未初期化では false。
  [[nodiscard]] bool Read() const override;

 private:
  interface::PinName pin_;
  GpioPull pull_;
  bool initialized_{false};
  GPIO_TypeDef* port_{nullptr};
  uint16_t pin_mask_{0};
};

}  // namespace nkrc::nbed::stm32::f4
