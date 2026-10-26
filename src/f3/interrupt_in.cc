#if defined(STM32F303x8)

#include "f3/interrupt_in.h"

namespace nkrc::nbed::stm32::f3 {

// Out-of-line construction also links the IRQ handlers from the static library.
InterruptIn::InterruptIn(interface::PinName pin, interface::InterruptEdge edge,
                         GpioPull pull, uint32_t interrupt_priority)
    : InterruptInput(pin, edge, pull, interrupt_priority) {}

}  // namespace nkrc::nbed::stm32::f3

// NOLINTBEGIN(readability-identifier-naming)
extern "C" void EXTI0_IRQHandler() {
  nkrc::nbed::stm32::f3::InterruptIn::HandleInterrupt(1U << 0);
}

extern "C" void EXTI1_IRQHandler() {
  nkrc::nbed::stm32::f3::InterruptIn::HandleInterrupt(1U << 1);
}

extern "C" void EXTI2_TSC_IRQHandler() {
  nkrc::nbed::stm32::f3::InterruptIn::HandleInterrupt(1U << 2);
}

extern "C" void EXTI3_IRQHandler() {
  nkrc::nbed::stm32::f3::InterruptIn::HandleInterrupt(1U << 3);
}

extern "C" void EXTI4_IRQHandler() {
  nkrc::nbed::stm32::f3::InterruptIn::HandleInterrupt(1U << 4);
}

extern "C" void EXTI9_5_IRQHandler() {
  nkrc::nbed::stm32::f3::InterruptIn::HandleInterrupt(0x03e0U);
}

extern "C" void EXTI15_10_IRQHandler() {
  nkrc::nbed::stm32::f3::InterruptIn::HandleInterrupt(0xfc00U);
}

// NOLINTEND(readability-identifier-naming)

#endif
