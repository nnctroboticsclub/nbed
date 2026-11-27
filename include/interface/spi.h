#pragma once

#include <cstdint>
#include <span>

namespace nkrc::nbed::interface {

/// @brief SCK の待機レベル（CPOL）とデータ取得エッジ（CPHA）。
enum class SpiMode : uint8_t {
  kMode0,  ///< CPOL=0、CPHA=0。
  kMode1,  ///< CPOL=0、CPHA=1。
  kMode2,  ///< CPOL=1、CPHA=0。
  kMode3,  ///< CPOL=1、CPHA=1。
};
/// @brief 1 バイト内で先に送るビット。
enum class SpiBitOrder : uint8_t {
  kMsbFirst,  ///< 最上位ビットから送る。
  kLsbFirst,  ///< 最下位ビットから送る。
};

/// @brief 8 ビット全二重 SPI マスターのインターフェース。CS は GPIO で操作する。
class Spi {
 public:
  /// @brief 派生クラスの初期化を準備する。
  Spi() = default;
  /// @brief 派生クラスのオブジェクトを破棄する。
  virtual ~Spi() = default;
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
  virtual bool Initialize() = 0;
  /// @brief 分周後の SCK 周波数を返す。
  /// @return 実際に設定した整数 Hz。未初期化では 0。
  [[nodiscard]] virtual uint32_t GetFrequencyHz() const = 0;

  // Each call accepts 1..65535 bytes. Transfer buffers must have equal sizes;
  // use separate buffers or exactly the same buffer (partial overlap is invalid).
  // Timeout is in HAL milliseconds. A failed call may have transferred a prefix.
  /// @brief バイト列を送信し、同時に受信した値を破棄する。
  /// @param data 送信する 1～65,535 バイト。
  /// @param timeout_ms HAL のタイムアウト（ミリ秒）。
  /// @return 全バイトを転送できれば true。失敗時も途中まで転送される場合がある。
  virtual bool Send(std::span<const uint8_t> data,
                    uint32_t timeout_ms = 1000) = 0;
  // Sends 0xff for every received byte. On failure the buffer is unspecified.
  /// @brief 0xff をクロックに合わせて送信しながら受信する。
  /// @param[out] data 1～65,535 バイト分の受信バッファ。
  /// @param timeout_ms HAL のタイムアウト（ミリ秒）。
  /// @return 全バイトを受信できれば true。失敗時のバッファ内容は不定。
  virtual bool Receive(std::span<uint8_t> data, uint32_t timeout_ms = 1000) = 0;
  /// @brief 等しい長さの送信列と受信列を同時に転送する。
  /// @param tx 送る 1～65,535 バイト。
  /// @param[out] rx tx と同じ長さの格納先。別領域または完全に同じ領域を指定できる。
  /// @param timeout_ms HAL のタイムアウト（ミリ秒）。
  /// @return 転送が完了すれば true。部分的に重なる領域は false。
  virtual bool Transfer(std::span<const uint8_t> tx, std::span<uint8_t> rx,
                        uint32_t timeout_ms = 1000) = 0;
};

}  // namespace nkrc::nbed::interface
