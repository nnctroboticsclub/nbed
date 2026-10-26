#pragma once
// IWYU pragma: private, include "nbed.h"

#include "f4/gpio.h"
#include "f4/pin_mapper.h"
#include "detail/interrupt_input.h"

namespace nkrc::nbed::stm32::f4 {

class InterruptIn final : public detail::InterruptInput<PinMapper, GpioPull> {
 public:
  explicit InterruptIn(
      interface::PinName pin,
      interface::InterruptEdge edge = interface::InterruptEdge::kBoth,
      GpioPull pull = GpioPull::kNone, uint32_t interrupt_priority = 5);
};

}  // namespace nkrc::nbed::stm32::f4
