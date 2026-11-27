#pragma once
// IWYU pragma: private, include "nbed.h"

#include <cstdint>
#include <span>

#include "interface/pin_names.h"
#include "interface/spi.h"
#include "stm32f4xx_hal.h"  // IWYU pragma: export
#include "stm32f4xx_hal_spi.h"

namespace nkrc::nbed::stm32::f4 {

/// @brief NUCLEO-F446RE で使える SPI コントローラー。
enum class SpiPeripheral : uint8_t {
  kSpi1,  ///< SPI1 を使用する。
  kSpi2,  ///< SPI2 を使用する。
  kSpi3,  ///< SPI3 を使用する。
};

/// @brief 8 ビット全二重 SPI マスター。CS は DigitalOut などで操作する。
class Spi final : public interface::Spi {
 public:
  /// @brief SPI の信号線と通信条件を指定する。
  /// @param peripheral 使用する SPI コントローラー。
  /// @param sck クロック出力ピン。
  /// @param miso スレーブからのデータ入力ピン。
  /// @param mosi スレーブへのデータ出力ピン。
  /// @param frequency_hz SCK の上限（Hz）。既定は 1,000,000。
  /// @param mode 相手機器の CPOL と CPHA に合わせる。
  /// @param bit_order 相手機器と揃えるビット順。
  Spi(SpiPeripheral peripheral, interface::PinName sck, interface::PinName miso,
      interface::PinName mosi, uint32_t frequency_hz = 1'000'000,
      interface::SpiMode mode = interface::SpiMode::kMode0,
      interface::SpiBitOrder bit_order = interface::SpiBitOrder::kMsbFirst);
  /// @brief SPI コントローラーを停止し、使用権を解放する。
  ~Spi() override;

  /// @brief オブジェクトのコピー構築は禁止。
  Spi(const Spi&) = delete;
  /// @brief オブジェクトのコピー代入は禁止。
  Spi& operator=(const Spi&) = delete;
  /// @brief オブジェクトのムーブ構築は禁止。
  Spi(Spi&&) = delete;
  /// @brief オブジェクトのムーブ代入は禁止。
  Spi& operator=(Spi&&) = delete;

  /// @brief SPI コントローラーと 3 本の信号線を初期化する。
  /// @return 設定に成功した場合、または初期化済みの場合に true。
  bool Initialize() override;
  /// @brief 分周後の SCK 周波数を返す。
  /// @return 実際に設定した整数 Hz。未初期化では 0。
  [[nodiscard]] uint32_t GetFrequencyHz() const override;
  /// @brief バイト列を送信し、同時に受信した値を破棄する。
  /// @param data 送信する 1～65,535 バイト。
  /// @param timeout_ms HAL のタイムアウト（ミリ秒）。
  /// @return 全バイトを転送できれば true。失敗時も途中まで転送される場合がある。
  bool Send(std::span<const uint8_t> data, uint32_t timeout_ms = 1000) override;
  /// @brief 0xff をクロックに合わせて送信しながら受信する。
  /// @param[out] data 1～65,535 バイト分の受信バッファ。
  /// @param timeout_ms HAL のタイムアウト（ミリ秒）。
  /// @return 全バイトを受信できれば true。失敗時のバッファ内容は不定。
  bool Receive(std::span<uint8_t> data, uint32_t timeout_ms = 1000) override;
  /// @brief 等しい長さの送信列と受信列を同時に転送する。
  /// @param tx 送る 1～65,535 バイト。
  /// @param[out] rx tx と同じ長さの格納先。別領域または完全に同じ領域を指定できる。
  /// @param timeout_ms HAL のタイムアウト（ミリ秒）。
  /// @return 転送が完了すれば true。部分的に重なる領域は false。
  bool Transfer(std::span<const uint8_t> tx, std::span<uint8_t> rx,
                uint32_t timeout_ms = 1000) override;

 private:
  SpiPeripheral peripheral_;
  interface::PinName sck_;
  interface::PinName miso_;
  interface::PinName mosi_;
  uint32_t frequency_hz_;
  interface::SpiMode mode_;
  interface::SpiBitOrder bit_order_;
  SPI_HandleTypeDef handle_{};
  uint32_t actual_frequency_hz_{0};
  bool initialized_{false};
};

}  // namespace nkrc::nbed::stm32::f4
