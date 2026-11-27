#pragma once
// IWYU pragma: private, include "nbed.h"

#include <cstdint>

#include "interface/analog_in.h"
#include "interface/pin_names.h"
#include "stm32f7xx_hal.h"  // IWYU pragma: export

namespace nkrc::nbed::stm32::f7 {

/// @brief NUCLEO-F767ZI の外部アナログ入力に使う ADC。
enum class AdcPeripheral : uint8_t {
  kAdcAuto,  ///< ピンに対応する ADC を自動選択する。
  kAdc1,     ///< ADC1 を使用する。
  kAdc2,     ///< ADC2 を使用する。
  kAdc3,     ///< ADC3 を使用する。
};

/// @brief 12 ビット ADC を使い、外部ピンの入力電圧を 1 回ずつ測る。
class AnalogIn final : public interface::AnalogIn {
 public:
  /// @brief ピンに応じて ADC を選ぶアナログ入力を作成する。
  /// @param pin 測るピン。共通ピンでは ADC1 を優先する。
  /// @param reference_voltage 実際の ADC 基準電圧（V）。既定は 3.3 V。
  explicit AnalogIn(interface::PinName pin, float reference_voltage = 3.3F);
  /// @brief ADC を明示してアナログ入力を作成する。
  /// @param peripheral 使用する ADC。
  /// @param pin 指定した ADC で変換できるピン。
  /// @param reference_voltage 実際の ADC 基準電圧（V）。既定は 3.3 V。
  AnalogIn(AdcPeripheral peripheral, interface::PinName pin,
           float reference_voltage = 3.3F);
  /// @brief アナログ入力オブジェクトを破棄する。ADC は同じ周辺機能の入力で共有される。
  ~AnalogIn() override = default;
  /// @brief オブジェクトのコピー構築は禁止。
  AnalogIn(const AnalogIn&) = delete;
  /// @brief オブジェクトのコピー代入は禁止。
  AnalogIn& operator=(const AnalogIn&) = delete;
  /// @brief オブジェクトのムーブ構築は禁止。
  AnalogIn(AnalogIn&&) = delete;
  /// @brief オブジェクトのムーブ代入は禁止。
  AnalogIn& operator=(AnalogIn&&) = delete;

  /// @brief ピンと ADC を初期化する。
  /// @return 使用可能になれば true。
  bool Initialize() override;

  /// @brief 1 回変換し、入力電圧を基準電圧に対する比率で返す。
  /// @return 0.0～1.0。未初期化または変換失敗時は 0.0。
  [[nodiscard]] float ReadRatio() override;
  /// @brief 1 回変換し、比率に設定した基準電圧を掛ける。
  /// @return 電圧（V）。未初期化または変換失敗時は 0.0。
  [[nodiscard]] float ReadVoltage() override;

 private:
  AdcPeripheral peripheral_;
  interface::PinName pin_;
  float reference_voltage_;
  uint8_t adc_number_{0};
  uint8_t channel_{0};
};

}  // namespace nkrc::nbed::stm32::f7
