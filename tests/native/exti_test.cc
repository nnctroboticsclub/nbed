#include <cassert>
#include <cstdint>
#include <type_traits>

#include "hal_stubs.h"
#ifdef STM32F303x8
#include "f3/interrupt_in.h"
namespace device = nkrc::nbed::stm32::f3;
extern "C" void EXTI2_TSC_IRQHandler();
#elif defined(STM32F767xx)
#include "f7/interrupt_in.h"
namespace device = nkrc::nbed::stm32::f7;
extern "C" void EXTI2_IRQHandler();
#else
#include "f4/interrupt_in.h"
namespace device = nkrc::nbed::stm32::f4;
extern "C" void EXTI2_IRQHandler();
#endif

extern "C" void EXTI0_IRQHandler();
extern "C" void EXTI1_IRQHandler();
extern "C" void EXTI3_IRQHandler();
extern "C" void EXTI4_IRQHandler();
extern "C" void EXTI9_5_IRQHandler();
extern "C" void EXTI15_10_IRQHandler();

int main() {
  using namespace device;
  using nkrc::nbed::interface::InterruptEdge;
  using enum nkrc::nbed::interface::PinName;
  static_assert(!std::is_copy_constructible_v<InterruptIn>);
  static_assert(!std::is_move_constructible_v<InterruptIn>);
  static_assert(std::is_base_of_v<nkrc::nbed::interface::DigitalIn, InterruptIn>);

  InterruptIn invalid(kNC);
  InterruptIn invalid_edge(kPA0, static_cast<InterruptEdge>(255));
  InterruptIn invalid_pull(kPA0, InterruptEdge::kBoth, static_cast<GpioPull>(255));
  InterruptIn invalid_priority(kPA0, InterruptEdge::kBoth, GpioPull::kNone, 16);
  assert(!invalid.Initialize() && !invalid_edge.Initialize());
  assert(!invalid_pull.Initialize() && !invalid_priority.Initialize());
  assert(gpio_inits.empty() && exti_mask == 0);
  assert(!invalid.Read() && invalid.GetEdgeCount() == 0);
  invalid.Stop();
  DigitalIn digital_invalid(kPA0, static_cast<GpioPull>(255));
  assert(!digital_invalid.Initialize() && gpio_inits.empty());

  // All 16 lines, the F3-specific vector name, and each independent IRQ group.
  for (unsigned line = 0; line < 16; ++line) {
    const auto pin = static_cast<nkrc::nbed::interface::PinName>(line);
    const uint32_t mask = 1U << line;
    InterruptIn input(pin, InterruptEdge::kRising, GpioPull::kDown, 3);
    assert(input.Initialize() && input.Initialize());
    assert(exti_mask == mask && exti_rising == mask && exti_falling == 0);
    assert(gpio_inits.back().second.Pull == GPIO_PULLDOWN);
    GPIOA->IDR = mask;
    assert(input.Read());
    GPIOA->IDR = 0;
    assert(!input.Read());
    exti_pending = mask;
    switch (line) {
      case 0: EXTI0_IRQHandler(); break;
      case 1: EXTI1_IRQHandler(); break;
      case 2:
#ifdef STM32F303x8
        EXTI2_TSC_IRQHandler();
#else
        EXTI2_IRQHandler();
#endif
        break;
      case 3: EXTI3_IRQHandler(); break;
      case 4: EXTI4_IRQHandler(); break;
      default:
        if (line < 10) {
          EXTI9_5_IRQHandler();
        } else {
          EXTI15_10_IRQHandler();
        }
    }
    assert(input.GetEdgeCount() == 1 && exti_pending == 0);
    input.Stop();
    assert(exti_mask == 0 && exti_rising == 0 && !input.Read());
    assert(input.GetEdgeCount() == 1);
  }

  unsigned calls = 0;
  {
    InterruptIn pa0(kPA0);
    InterruptIn pb0(kPB0);
    assert(pa0.Initialize() && !pb0.Initialize());
    pa0.Stop();
    assert(pb0.Initialize());
    assert(gpio_inits.back().first == GPIOB);
  }
  InterruptIn first(kPA8, InterruptEdge::kBoth, GpioPull::kUp, 5);
  first.SetCallback([&calls] { ++calls; });
  exti_pending = 1U << 8;  // Initialization discards stale events.
  primask = 1;
  assert(first.Initialize() && primask == 1);
  primask = 0;
  assert(exti_pending == 0 && irq_priority[EXTI9_5_IRQn] == 5);
  assert(exti_rising == (1U << 8) && exti_falling == (1U << 8));
  const auto configured = gpio_inits.size();
  InterruptIn duplicate(kPA8);
  assert(!duplicate.Initialize());
  InterruptIn different_priority(kPA9, InterruptEdge::kBoth, GpioPull::kNone, 4);
  assert(!different_priority.Initialize() && gpio_inits.size() == configured);
  assert(irq_priority[EXTI9_5_IRQn] == 5);
  const auto clears = irq_clears[EXTI9_5_IRQn];
  {
    InterruptIn second(kPA9, InterruptEdge::kFalling);
    assert(second.Initialize() && irq_clears[EXTI9_5_IRQn] == clears);
    assert((exti_rising & (1U << 9)) == 0);
    assert((exti_falling & (1U << 9)) != 0);
    exti_pending = (1U << 8) | (1U << 9) | (1U << 0);
    EXTI9_5_IRQHandler();
    assert(calls == 1 && first.GetEdgeCount() == 1);
    assert(second.GetEdgeCount() == 1 && exti_pending == (1U << 0));
    EXTI9_5_IRQHandler();
    assert(calls == 1);  // No callback without a pending bit.
    first.Stop();
    assert(irq_enabled[EXTI9_5_IRQn] && exti_mask == (1U << 9));
    exti_pending = 1U << 9;
    EXTI9_5_IRQHandler();
    assert(second.GetEdgeCount() == 2);
  }
  assert(!irq_enabled[EXTI9_5_IRQn] && exti_mask == 0);
  first.Stop();
  assert(first.Initialize() && first.GetEdgeCount() == 0);
  first.SetCallback({});
  exti_pending = 1U << 8;
  EXTI9_5_IRQHandler();
  assert(calls == 1 && first.GetEdgeCount() == 1);
  // New events during the callback must survive this dispatch.
  first.SetCallback([] { exti_pending |= 1U << 8; });
  exti_pending = 1U << 8;
  EXTI9_5_IRQHandler();
  assert(first.GetEdgeCount() == 2 && exti_pending == (1U << 8));
  primask = 1;
  first.Stop();
  assert(primask == 1 && exti_pending == 0 && !irq_enabled[EXTI9_5_IRQn]);
  primask = 0;
  assert(duplicate.Initialize());  // Stop releases the line.
}
