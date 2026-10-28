#include <array>
#include <cassert>

#include "can_hal_stubs.h"
#ifdef STM32F303x8
#include "f3/can.h"
#include "f3/pin_mapper.h"
namespace device = nkrc::nbed::stm32::f3;
#elif defined(STM32F767xx)
#include "f7/can.h"
#include "f7/pin_mapper.h"
namespace device = nkrc::nbed::stm32::f7;
#else
#include "f4/can.h"
#include "f4/pin_mapper.h"
namespace device = nkrc::nbed::stm32::f4;
#endif

using namespace device;
using nkrc::nbed::interface::PinName;
using enum nkrc::nbed::interface::PinName;

constexpr auto kCan1Rx = std::array{
    kPA11,
#ifndef STM32F303x8
    kPB8,
#ifdef STM32F767xx
    kPD0,
#endif
#endif
};
constexpr auto kCan1Tx = std::array{
    kPA12,
#ifndef STM32F303x8
    kPB9,
#ifdef STM32F767xx
    kPD1,
#endif
#endif
};

void CheckDeinit(nkrc::nbed::interface::PinName rx,
                 nkrc::nbed::interface::PinName tx) {
  // Only the selected pin on each port may be released, including on failure.
  uint32_t rx_mask = 0, tx_mask = 0;
  const auto rx_port = PinMapper::GetGpioPort(rx);
  const auto tx_port = PinMapper::GetGpioPort(tx);
  for (const auto& [port, pin] : gpio_deinits) {
    assert(port == rx_port || port == tx_port);
    if (port == rx_port) {
      rx_mask |= pin;
    }
    if (port == tx_port) {
      tx_mask |= pin;
    }
  }
  if (rx_port == tx_port) {
    assert(rx_mask == (PinMapper::GetGpioPin(rx) | PinMapper::GetGpioPin(tx)));
  } else {
    assert(rx_mask == PinMapper::GetGpioPin(rx));
    assert(tx_mask == PinMapper::GetGpioPin(tx));
  }
}

int main() {
  for (auto rx : kCan1Rx) {
    for (auto tx : kCan1Tx) {
      gpio_inits.clear();
      gpio_deinits.clear();
      Can can(CanConfig{.rx = rx, .tx = tx, .interrupt_priority = 7});
      assert(can.Initialize());
      assert(can1_clock && gpio_inits.size() == 2);
      for (unsigned i = 0; i < 2; ++i) {
        const auto pin = i == 0 ? rx : tx;
        const auto& [port, config] = gpio_inits[i];
        assert(port == PinMapper::GetGpioPort(pin));
        assert(config.Pin == PinMapper::GetGpioPin(pin));
        assert(config.Alternate == 9 && config.Mode == GPIO_MODE_AF_PP);
      }
#ifdef STM32F303x8
      constexpr auto irq = CAN_RX0_IRQn;
#else
      constexpr auto irq = CAN1_RX0_IRQn;
#endif
      assert(irq_enabled[irq] && irq_priority[irq] == 7);
      const auto calls = can_init_calls;
      assert(can.Initialize() && can_init_calls == calls);
      Can duplicate(CanPeripheral::kCan1, rx, tx);
      assert(!duplicate.Initialize());
      assert(can.Start());
      can.Stop();
      assert(!irq_enabled[irq]);
      CheckDeinit(rx, tx);
      assert(can.Start());
      can.Stop();
      // HAL initialization and filter failures must both release both pins.
      for (bool fail_filter : {false, true}) {
        gpio_deinits.clear();
        can_init_status = fail_filter ? HAL_OK : HAL_ERROR;
        can_filter_status = fail_filter ? HAL_ERROR : HAL_OK;
        assert(!can.Initialize());
        CheckDeinit(rx, tx);
        assert(!irq_enabled[irq]);
        can_init_status = can_filter_status = HAL_OK;
        Can retry(CanPeripheral::kCan1, rx, tx);
        assert(retry.Initialize());
      }
    }
  }

  // Reject reversed roles, unrelated pins, NC, duplicates and other CAN units.
  constexpr auto candidates = std::array{
      kPA11, kPA12, kPB5, kPB6, kNC, kPA0,
      static_cast<PinName>(0x18), static_cast<PinName>(0x19),
      static_cast<PinName>(0x30), static_cast<PinName>(0x31),
      static_cast<PinName>(0x1c), static_cast<PinName>(0x1d),
      static_cast<PinName>(0xfe)};
  for (auto rx : candidates) {
    for (auto tx : candidates) {
      const bool valid_rx =
          std::find(kCan1Rx.begin(), kCan1Rx.end(), rx) != kCan1Rx.end();
      const bool valid_tx =
          std::find(kCan1Tx.begin(), kCan1Tx.end(), tx) != kCan1Tx.end();
      Can can(CanPeripheral::kCan1, rx, tx);
      const auto calls = can_init_calls;
      assert(can.Initialize() == (valid_rx && valid_tx));
      if (!valid_rx || !valid_tx) {
        assert(can_init_calls == calls);
      }
#ifndef STM32F303x8
      Can can2(CanPeripheral::kCan2, rx, tx);
      const bool valid_can2 = (rx == kPB5 || rx == kPB12) &&
                              (tx == kPB6 || tx == kPB13);
      assert(can2.Initialize() == valid_can2);
      if (valid_can2) {
        gpio_deinits.clear();
        can2.Stop();
        CheckDeinit(rx, tx);
      }
#endif
    }
  }
  Can invalid_unit(static_cast<CanPeripheral>(0xff), kPA11, kPA12);
  assert(!invalid_unit.Initialize());
  Can invalid_priority(CanConfig{.rx = kPA11, .tx = kPA12,
                                 .interrupt_priority = 16});
  assert(!invalid_priority.Initialize());
#ifndef STM32F303x8
  // Stopping CAN1 with mixed ports must preserve a running CAN2's clock.
  Can first(CanPeripheral::kCan1, kPA11, kPB9);
  Can second(CanPeripheral::kCan2, kPB5, kPB13);
  assert(first.Start() && second.Start());
  first.Stop();
  assert(can1_clock && can2_clock && irq_enabled[CAN2_RX0_IRQn]);
  second.Stop();
  assert(!can2_clock && !irq_enabled[CAN2_RX0_IRQn]);
#endif
}
