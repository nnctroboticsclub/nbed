#include <array>
#include <cassert>
#include <cmath>
#include <limits>

#include "analog_i2c_hal_stubs.h"
#include "detail/i2c_timing.h"
#if defined(STM32F303x8)
#include "f3/analog_in.h"
#include "f3/i2c.h"
#include "f3/pin_routes.h"
namespace device = nkrc::nbed::stm32::f3;
#elif defined(STM32F446xx)
#include "f4/analog_in.h"
#include "f4/i2c.h"
#include "f4/pin_routes.h"
namespace device = nkrc::nbed::stm32::f4;
#else
#include "f7/analog_in.h"
#include "f7/i2c.h"
#include "f7/pin_routes.h"
namespace device = nkrc::nbed::stm32::f7;
#endif

using namespace device;
using enum nkrc::nbed::interface::PinName;
using nkrc::nbed::interface::I2CMessage;

constexpr bool CheckTimings(uint32_t first, uint32_t last) {
  for (uint32_t rate = first; rate <= last; rate += 137) {
    const auto timing = nkrc::nbed::detail::I2cTimingHsi16Mhz(rate);
    if (!timing) {
      return false;
    }
    const uint32_t tick = ((*timing >> 28) + 1) * 125;
    const uint32_t high = (((*timing >> 8) & 255) + 1) * tick + 350;
    const uint32_t low = ((*timing & 255) + 1) * tick + 350;
    const uint32_t setup = (((*timing >> 20) & 15) + 1) * tick;
    const uint32_t hold = ((*timing >> 16) & 15) * tick;
    const bool standard = rate <= 100'000;
    if (uint64_t{high + low} * rate < 2'000'000'000ULL ||
        high < (standard ? 8000U : 1200U) || low < (standard ? 9400U : 2600U) ||
        setup < (standard ? 2500U : 1000U)) {
      return false;
    }
    // Lower and upper SDA hold bounds from RM0410, in half-nanoseconds.
    if (hold + 100U + 3U * 125U < 600U ||
        hold + (standard ? 2000U : 200U) + 520U + 4U * 125U >
            (standard ? 6900U : 1800U)) {
      return false;
    }
  }
  return true;
}

static_assert(CheckTimings(2000, 100'000));
static_assert(CheckTimings(100'001, 400'000));
static_assert(!nkrc::nbed::detail::I2cTimingHsi16Mhz(0));
static_assert(!nkrc::nbed::detail::I2cTimingHsi16Mhz(1000));
static_assert(!nkrc::nbed::detail::I2cTimingHsi16Mhz(400'001));

void CheckI2c() {
#if defined(STM32F303x8)
  constexpr auto scl = kPA15;
  constexpr auto sda = kPB7;
  constexpr auto alternative_sda = kPA14;
#else
  constexpr auto scl = kPB8;
  constexpr auto sda = kPB7;
  constexpr auto alternative_sda = kPB9;
#endif
  {
    I2c bus(I2cPeripheral::kI2c1, scl, sda);
    std::array<uint8_t, 2> data{};
    I2CMessage message{.address = 0x50, .size = 2, .data = data.data()};
    assert(!bus.Send(message));
#if defined(STM32F446xx)
    i2c_init_status = HAL_ERROR;
#else
    i2c_filter_status = HAL_ERROR;
#endif
    assert(!bus.Initialize() && i2c_deinits == 1);
    i2c_init_status = i2c_filter_status = HAL_OK;
    assert(bus.Initialize() && bus.Initialize());
    assert(last_i2c_init.Instance == I2C1);
#if defined(STM32F446xx)
    assert(last_i2c_init.Init.ClockSpeed == 100'000);
#else
    assert(i2c_clocks[0] == 1);
#endif
    I2c duplicate(I2cPeripheral::kI2c1, scl, alternative_sda);
    assert(!duplicate.Initialize());
    assert(bus.Send(message) && bus.Receive(message) &&
           last_i2c_address == 0xa0);
    message.address = 0x80;
    assert(!bus.Send(message) && !bus.Receive(message));
    message.address = 0x50;
    message.data = nullptr;
    assert(!bus.Send(message));
    message.data = data.data();
    message.size = 0;
    assert(!bus.Receive(message));
    message.size = 2;
    i2c_transfer_status = HAL_ERROR;
    assert(!bus.Send(message) && !bus.Receive(message));
    i2c_transfer_status = HAL_OK;
  }
  I2c wrong(static_cast<I2cPeripheral>(255), scl, sda);
  I2c wrong_pin(I2cPeripheral::kI2c1, sda, scl);
  assert(!wrong.Initialize() && !wrong_pin.Initialize());

  const std::array instances{
      I2C1,
#if defined(STM32F446xx)
      I2C2,
      I2C3,
      FMPI2C1,
#elif defined(STM32F767xx)
      I2C2,
      I2C3,
      I2C4,
#endif
  };
  // Exercise each independently selectable pair and verify both AF values.
  for (const auto& clock : kI2cPins) {
    if (clock.signal != PinSignal::kScl) {
      continue;
    }
    for (const auto& data : kI2cPins) {
      if (data.peripheral != clock.peripheral ||
          data.signal != PinSignal::kSda) {
        continue;
      }
      const auto index = clock.peripheral - 1;
      I2c bus(static_cast<I2cPeripheral>(index), clock.pin, data.pin, 400'000);
      gpio_inits.clear();
      assert(bus.Initialize() && last_i2c_init.Instance == instances[index]);
      assert(gpio_inits.size() == 2 &&
             gpio_inits[0].second.Alternate == clock.value &&
             gpio_inits[1].second.Alternate == data.value);
      assert(gpio_inits[0].second.Mode == GPIO_MODE_AF_OD);
      if (index == 3) {
        assert(i2c_clocks[3] == 1 &&
               last_i2c_init.Init.Timing ==
                   *nkrc::nbed::detail::I2cTimingHsi16Mhz(400'000));
        uint8_t value{};
        I2CMessage message{.address = 0x30, .size = 1, .data = &value};
        assert(bus.Send(message) && bus.Receive(message) &&
               last_i2c_address == 0x60);
      }
    }
  }
}

void CheckAdc() {
  AnalogIn failing(kPA0);
  adc_init_status = HAL_ERROR;
  assert(!failing.Initialize());
  adc_init_status = HAL_OK;
  assert(failing.Initialize() && last_adc_init.Instance == ADC1);
  assert(last_adc_init.Init.ClockPrescaler == ADC_CLOCK_SYNC_PCLK_DIV4);

  const std::array instances{
      ADC1,
      ADC2,
#ifndef STM32F303x8
      ADC3,
#endif
  };
  for (const auto& route : kAdcPins) {
    AnalogIn analog(static_cast<AdcPeripheral>(route.peripheral), route.pin);
    assert(analog.Initialize() && analog.ReadRatio() > 0.5F);
    assert(adc_channels.back().first == instances[route.peripheral - 1] &&
           adc_channels.back().second == route.value);
  }

#ifdef STM32F767xx
  AnalogIn a(PinName::kPB1), b(PinName::kPF3), c(PinName::kPF5);
  adc_channels.clear();
  assert(a.Initialize() && b.Initialize() && c.Initialize());
  assert(a.ReadRatio() > 0.5F && b.ReadRatio() > 0.5F);
  assert(adc_channels.size() == 2);
  assert(adc_channels[0].first == ADC1 && adc_channels[0].second == 9);
  assert(adc_channels[1].first == ADC3 && adc_channels[1].second == 9);
  (void)a.ReadRatio();
  (void)b.ReadRatio();
  assert(adc_channels.size() == 2);  // Cache is independent for each ADC.
  assert(c.ReadRatio() > 0.5F && adc_channels.back().second == 15);
  assert(std::abs(a.ReadVoltage() - 2048.0F / 4095.0F * 3.3F) < 0.001F);
  const auto stops = adc_stops;
  adc_poll_status = HAL_ERROR;
  assert(a.ReadRatio() == 0 && adc_stops == stops + 1);
  adc_poll_status = HAL_OK;
#endif
  AnalogIn invalid(kPA8), invalid_ref(kPA0, 0);
  AnalogIn invalid_unit(static_cast<AdcPeripheral>(255), kPA0);
  AnalogIn nan(kPA0, std::numeric_limits<float>::quiet_NaN());
  assert(!invalid.Initialize() && !invalid_ref.Initialize() &&
         !nan.Initialize() && !invalid_unit.Initialize());
}

int main() {
  CheckI2c();
  CheckAdc();
}
