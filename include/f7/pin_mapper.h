#pragma once

#include "interface/pin_names.h"
#include "stm32f7xx_hal.h"  // IWYU pragma: export

namespace nkrc::nbed::stm32::f7 {

/// @brief NUCLEO-F767ZI の物理ピン名を HAL の GPIO ポートとビット位置に変換する。
class PinMapper {
 public:
  /// @brief MCU パッケージに実装された GPIO かを確認する。
  /// @param pin_name 確認するピン。
  /// @return kNC 以外の実装された GPIO なら true。
  [[nodiscard]] static bool IsValidPin(interface::PinName pin_name);

  /// @brief 実装された異なる 2 本の GPIO かを確認する。
  /// @param first 1 本目のピン。
  /// @param second 2 本目のピン。
  /// @return 2 本とも有効で、互いに異なる場合に true。
  [[nodiscard]] static bool IsValidPinPair(interface::PinName first,
                                           interface::PinName second);

  /// @brief ピンの GPIO ポートを取得する。
  /// @param pin_name ピン。
  /// @return ポート。無効なピンでは nullptr。
  [[nodiscard]] static GPIO_TypeDef* GetGpioPort(interface::PinName pin_name);

  /// @brief ピンの HAL 用ビットマスクを取得する。
  /// @param pin_name ピン。
  /// @return 1 ビットのマスク。無効なピンでは 0。
  [[nodiscard]] static uint16_t GetGpioPin(interface::PinName pin_name);

  /// @brief GPIO ポートの共有クロックを有効にする。
  /// @param port 対象 GPIO ポート。
  static void EnableGpioClock(GPIO_TypeDef* port);

  /// @brief ポートのクロックを有効にし、物理ピンを 1 本設定する。
  /// @param pin_name 設定対象の GPIO。
  /// @param config モード、プル、速度、代替機能の HAL 設定。Pin は自動設定する。
  /// @return ピンが有効で GPIO の設定を行った場合に true。
  [[nodiscard]] static bool ConfigureGpio(interface::PinName pin_name,
                                          const GPIO_InitTypeDef& config);
};

}  // namespace nkrc::nbed::stm32::f7
