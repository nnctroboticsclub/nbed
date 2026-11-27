#pragma once

#include <cstdint>

namespace nkrc::nbed::interface {

/// @brief I²C の送受信条件とバッファ。
struct I2CMessage {
  uint16_t address;  ///< 左シフトする前の 7 ビットアドレス。
  uint8_t size;      ///< 転送するバイト数（1～255）。
  uint8_t* data;     ///< 送信元または受信先の配列。size バイト以上を用意する。
};

/// @brief 7 ビットアドレスの I²C マスターとして送受信するインターフェース。
class I2C {
 public:
  /// @brief 派生クラスの初期化を準備する。
  I2C() = default;
  /// @brief 派生クラスのオブジェクトを破棄する。
  virtual ~I2C() = default;
  /// @brief オブジェクトのコピー構築は禁止。
  I2C(const I2C&) = delete;
  /// @brief オブジェクトのコピー代入は禁止。
  I2C& operator=(const I2C&) = delete;
  /// @brief オブジェクトのムーブ構築は禁止。
  I2C(I2C&&) = delete;
  /// @brief オブジェクトのムーブ代入は禁止。
  I2C& operator=(I2C&&) = delete;

  /// @brief SCL・SDA と I²C マスターを設定してバスを使用可能にする。
  /// @return ピン、周辺機能、通信速度の設定に成功すれば true。
  virtual bool Initialize() = 0;

  /// @brief 指定した I²C アドレスへデータを送る。
  /// @param message アドレス、長さと送信バッファ。
  /// @return 転送が完了すれば true。通信中は呼び出し元を待機させる。
  virtual bool Send(const I2CMessage& message) = 0;
  /// @brief 指定した I²C アドレスからデータを読み取る。
  /// @param message アドレス、長さと書き込み可能な受信バッファ。
  /// @return 指定バイト数の受信が完了すれば true。通信中は呼び出し元を待機させる。
  virtual bool Receive(I2CMessage& message) = 0;
};

}  // namespace nkrc::nbed::interface
