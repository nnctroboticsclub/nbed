#pragma once

#include <cstdint>
#include <functional>

#include "interface/digital_in.h"

namespace nkrc::nbed::interface {

/// @brief 割り込みで検出する信号変化。
enum class InterruptEdge : uint8_t {
  kRising,   ///< Low から High への立上り。
  kFalling,  ///< High から Low への立下り。
  kBoth,     ///< 立上りと立下りの両方。
};
/// @brief エッジ検出時に呼び出す引数なしの関数型。
using GpioInterruptCallback = std::function<void()>;

/// @brief 信号のエッジを GPIO 外部割り込み（EXTI）で検出するインターフェース。
class InterruptIn : public DigitalIn {
 public:
  /// @brief エッジの検出を停止し、EXTI ラインを解放する。
  virtual void Stop() = 0;
  /// @brief エッジ検出時に呼ぶ関数を登録する。
  /// @param callback 割り込み内で実行する短い処理。空の関数で解除する。
  virtual void SetCallback(const GpioInterruptCallback& callback) = 0;
  /// @brief 処理したエッジの累計を返す。
  /// @return Initialize() 以降の累計。32 ビットで周回する。
  [[nodiscard]] virtual uint32_t GetEdgeCount() const = 0;
};

}  // namespace nkrc::nbed::interface
