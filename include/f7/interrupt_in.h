#pragma once
// IWYU pragma: private, include "nbed.h"

#include "detail/interrupt_input.h"
#include "f7/gpio.h"
#include "f7/pin_mapper.h"

namespace nkrc::nbed::stm32::f7 {

/// @brief GPIO のエッジを EXTI で検出する。
class InterruptIn final : public detail::InterruptInput<PinMapper, GpioPull> {
 public:
  /// @brief 外部割り込み入力に使うピンと条件を指定する。
  /// @param pin 割り込みを検出する入力ピン。同じ末尾番号のピンは EXTI ラインを共有する。
  /// @param edge 立上り、立下り、または両方。
  /// @param pull ピンの内部プル抵抗。
  /// @param interrupt_priority 割り込み優先度（0～15）。同じ IRQ を共有する入力で揃える。
  explicit InterruptIn(
      interface::PinName pin,
      interface::InterruptEdge edge = interface::InterruptEdge::kBoth,
      GpioPull pull = GpioPull::kNone, uint32_t interrupt_priority = 5);
};

}  // namespace nkrc::nbed::stm32::f7
