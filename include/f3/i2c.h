#pragma once
// IWYU pragma: private, include "nbed.h"

#include <cstdint>

#include "interface/i2c.h"
#include "interface/pin_names.h"
#include "stm32f3xx_hal.h"  // IWYU pragma: export
#include "stm32f3xx_hal_i2c.h"

namespace nkrc::nbed::stm32::f3 {

/// @brief NUCLEO-F303K8 で使える I²C コントローラー。
enum class I2cPeripheral : uint8_t {
  kI2c1,  ///< I2C1 を使用する。
};

/// @brief 2 本の信号線を使い、I²C マスターとして通信する。
class I2c final : public interface::I2C {
 public:
  /// @brief SCL、SDA と速度を指定して I²C マスターを作る。
  /// @param peripheral 使用する I²C コントローラー。
  /// @param scl クロックピン。
  /// @param sda データピン。
  /// @param frequency_hz SCL の目標周波数（Hz）。既定は 100,000、上限は 400,000。
  I2c(I2cPeripheral peripheral, interface::PinName scl, interface::PinName sda,
      uint32_t frequency_hz = 100'000);
  /// @brief I²C コントローラーを停止し、使用権を解放する。
  ~I2c() override;

  /// @brief オブジェクトのコピー構築は禁止。
  I2c(const I2c&) = delete;
  /// @brief オブジェクトのコピー代入は禁止。
  I2c& operator=(const I2c&) = delete;
  /// @brief オブジェクトのムーブ構築は禁止。
  I2c(I2c&&) = delete;
  /// @brief オブジェクトのムーブ代入は禁止。
  I2c& operator=(I2c&&) = delete;

  /// @brief SCL・SDA と I²C マスターを設定してバスを使用可能にする。
  /// @return ピン、周辺機能、通信速度の設定に成功すれば true。
  bool Initialize() override;

  /// @brief 指定した I²C アドレスへデータを送る。
  /// @param message アドレス、長さと送信バッファ。
  /// @return 転送が完了すれば true。通信中は呼び出し元を待機させる。
  bool Send(const interface::I2CMessage& message) override;
  /// @brief 指定した I²C アドレスからデータを読み取る。
  /// @param message アドレス、長さと書き込み可能な受信バッファ。
  /// @return 指定バイト数の受信が完了すれば true。通信中は呼び出し元を待機させる。
  bool Receive(interface::I2CMessage& message) override;

 private:
  [[nodiscard]] bool InitializeController();
  I2cPeripheral peripheral_;
  interface::PinName scl_;
  interface::PinName sda_;
  uint32_t frequency_hz_;
  I2C_HandleTypeDef handle_{};
  bool initialized_{false};
};

}  // namespace nkrc::nbed::stm32::f3
