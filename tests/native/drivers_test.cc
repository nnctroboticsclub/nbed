#include <cassert>
#include <cerrno>
#include <cmath>
#include <cstdint>
#include <limits>
#include <vector>

#include "detail/i2c_timing.h"
#include "detail/pwm_timing.h"
#include "hal_stubs.h"
#ifdef STM32F303x8
#include "f3/gpio.h"
#include "f3/pin_mapper.h"
#include "f3/time.h"
#include "f3/timer_reservation.h"
#include "f3/uart.h"
namespace device = nkrc::nbed::stm32::f3;
#elif defined(STM32F767xx)
#include "f7/gpio.h"
#include "f7/pin_mapper.h"
#include "f7/time.h"
#include "f7/timer_reservation.h"
#include "f7/uart.h"
namespace device = nkrc::nbed::stm32::f7;
#else
#include "f4/gpio.h"
#include "f4/pin_mapper.h"
#include "f4/time.h"
#include "f4/timer_reservation.h"
#include "f4/uart.h"
namespace device = nkrc::nbed::stm32::f4;
#endif

extern "C" int _write(int file, const char* ptr, int len);

// Keep arithmetic coverage available even where the host cannot execute an
// unsigned test binary. Split the range to stay within constexpr step limits.
constexpr bool CheckI2cTimings(uint32_t first, uint32_t last) {
  for (uint32_t frequency = first; frequency <= last; frequency += 137) {
    const auto timing = nkrc::nbed::detail::I2cTimingHsi8Mhz(frequency);
    if (!timing) {
      return false;
    }
    const unsigned tick = (((*timing >> 28) & 15) + 1) * 125;
    const unsigned high = (((*timing >> 8) & 255) + 1) * tick + 300;
    const unsigned low = ((*timing & 255) + 1) * tick + 300;
    const unsigned setup = (((*timing >> 20) & 15) + 1) * tick;
    const unsigned hold = ((*timing >> 16) & 15) * tick;
    const bool standard = frequency <= 100'000;
    if (uint64_t{high + low} * frequency < 1'000'000'000ULL ||
        high < (standard ? 4000U : 600U) || low < (standard ? 4700U : 1300U) ||
        setup < (standard ? 1250U : 500U) ||
        hold + (standard ? 1000U : 100U) + 260 + 4 * 125 >
            (standard ? 3450U : 900U)) {
      return false;
    }
  }
  return true;
}

static_assert(CheckI2cTimings(1'000, 100'000));
static_assert(CheckI2cTimings(100'001, 400'000));
static_assert(nkrc::nbed::detail::PwmDutyTicks(0.75F, 0xffffffffU) ==
              0xc0000000ULL);

int main() {
  using namespace device;
  using nkrc::nbed::interface::PinName;
  using namespace nkrc::nbed::detail;

  // Decode every pin value and check the complete package whitelist.
  for (unsigned value = 0; value <= 255; ++value) {
    const auto pin = static_cast<PinName>(value);
    const unsigned port = value >> 4;
    const unsigned index = value & 15;
#ifdef STM32F303x8
    const bool expected =
        port == 0 ||
        (port == 1 && (index <= 1 || (index >= 3 && index <= 7))) ||
        (port == 5 && index <= 1);
#elif defined(STM32F767xx)
    const bool expected = port <= 6 || (port == 7 && index <= 1);
#else
    const bool expected =
        port == 0 || port == 2 || (port == 1 && index != 11) ||
        (port == 3 && index == 2) || (port == 7 && index <= 1);
#endif
    assert(PinMapper::IsValidPin(pin) == expected);
    assert((PinMapper::GetGpioPort(pin) != nullptr) == expected);
    assert(PinMapper::GetGpioPin(pin) == (expected ? 1U << index : 0));
  }
  DigitalOut invalid(PinName::kNC);
  assert(!invalid.Initialize());
  DigitalOut output(PinName::kPA5, true);
  output.Write(true);
  assert(GPIOA->ODR == 0);
  assert(output.Initialize() && output.Initialize());
  assert(GPIOA->ODR == (1U << 5));
  output.Write(false);
  assert(GPIOA->ODR == 0);
  DigitalIn input(PinName::kPB0);
  assert(input.Initialize());
  GPIOB->IDR = 1;
  assert(input.Read());
  output = input;
  assert(GPIOA->ODR == (1U << 5));

  assert(TimerReservation::ClaimPwm(0, 0));
  assert(!TimerReservation::ClaimPwm(0, 0));
  assert(!TimerReservation::ClaimEncoder(0));
  assert(TimerReservation::ClaimPwm(0, 1));
  TimerReservation::ReleasePwm(0, 0);
  TimerReservation::ReleasePwm(0, 1);
  assert(TimerReservation::ClaimEncoder(0));
  assert(!TimerReservation::ClaimPwm(0, 0));
  TimerReservation::ReleaseEncoder(0);
  assert(!TimerReservation::ClaimEncoder(255));
  assert(!TimerReservation::ClaimPwm(0, 4));

  assert(!ComputePwmTiming(64'000'000, 0, 65535));
  assert(!ComputePwmTiming(64'000'000, 64'000'001, 65535));
  const auto full_duty = ComputePwmTiming(65'536'000, 1000, 65'535);
  assert(full_duty && full_duty->actual_hz == 1000 &&
         PwmDutyTicks(1.0F, full_duty->period) <= 65'535);
  for (uint32_t frequency : {1U, 50U, 1000U, 20'000U, 64'000'000U}) {
    auto timing = ComputePwmTiming(64'000'000, frequency, 65535);
    assert(timing && timing->period <= 65535 && timing->prescaler <= 65535);
    assert(timing->actual_hz == 64'000'000ULL / ((timing->prescaler + 1ULL) *
                                                 (timing->period + 1ULL)));
  }
  // These valid CCR values exceed a signed 32-bit long on the MCU.
  assert(PwmDutyTicks(0.75F, 0xffffffffU) == 0xc0000000ULL);
  assert(PwmDutyTicks(1.0F, 0xffffffffU) == 0x100000000ULL);
  assert(PwmDutyTicks(0.5F, 65535) == 32768);

  assert(!I2cTimingHsi8Mhz(0) && !I2cTimingHsi8Mhz(400'001));
  assert(!I2cTimingHsi8Mhz(1));
  for (uint32_t frequency = 1'000; frequency <= 400'000; frequency += 137) {
    const auto timing = I2cTimingHsi8Mhz(frequency);
    assert(timing);
    const unsigned tick = (((*timing >> 28) & 15) + 1) * 125;
    const unsigned high = (((*timing >> 8) & 255) + 1) * tick + 300;
    const unsigned low = ((*timing & 255) + 1) * tick + 300;
    const unsigned setup = (((*timing >> 20) & 15) + 1) * tick;
    const unsigned hold = ((*timing >> 16) & 15) * tick;
    assert(uint64_t{high + low} * frequency >= 1'000'000'000ULL);
    assert(high >= (frequency <= 100'000 ? 4000U : 600U));
    assert(low >= (frequency <= 100'000 ? 4700U : 1300U));
    assert(setup >= (frequency <= 100'000 ? 1250U : 500U));
    // Worst analog-filter delay 260 ns; fast-mode rise assumption 100 ns.
    assert(hold + (frequency <= 100'000 ? 1000U : 100U) + 260 + 4 * 125 <=
           (frequency <= 100'000 ? 3450U : 900U));
  }

  hal_tick = 123;
  assert(Time::Initialize());
#ifdef STM32F767xx
  assert(DWT->LAR == 0xC5ACCE55U);
  static_assert(PinName::kD0 == PinName::kPG9);
  static_assert(PinName::kD11 == PinName::kPA7);
  static_assert(PinName::kD13 == PinName::kPA5);
  static_assert(PinName::kA3 == PinName::kPF3);
#endif
  DWT->CYCCNT = 32'000;
  assert(Time::Microseconds() == 123'500);
  // Interrupt after reading the old cycle baseline forces a consistent retry.
  int samples = 0;
  static int* sample_count = &samples;
  tick_hook = [] {
    ++*sample_count;
    tick_hook = [] {
      ++hal_tick;
      DWT->CYCCNT = 64'000;
      Time::OnSysTick();
    };
  };
  assert(Time::Microseconds() == 124'000 && samples == 1);
  hal_tick = 0xffffffffU;
  DWT->CYCCNT = 0xfffffff0U;
  Time::OnSysTick();
  const auto before_wrap = Time::Microseconds();
  hal_tick = 0;
  DWT->CYCCNT = 64'000;
  Time::OnSysTick();
  assert(Time::Microseconds() == 0x100000000ULL * 1000);
  assert(Time::Microseconds() > before_wrap);
  Timer timer;
  timer.Start();
  DWT->CYCCNT += 640;
  timer.Stop();
  assert(timer.ElapsedMicroseconds() == 10);
  timer.Reset();
  assert(timer.ElapsedMicroseconds() == 0);

  Uart uart(UartPeripheral::kUsart2, PinName::kPA3, PinName::kPA2);
  assert(uart.Initialize());
  Uart duplicate(UartPeripheral::kUsart2, PinName::kPA3, PinName::kPA2);
  assert(!duplicate.Initialize());
  Uart invalid_uart(static_cast<UartPeripheral>(255), PinName::kPA1,
                    PinName::kPA0);
  assert(!invalid_uart.Initialize());
  uart.SetConsole();
  const std::vector<char> message(65535 + 17, 'x');
  assert(_write(1, message.data(), static_cast<int>(message.size())) == 65552);
  assert((tx_sizes == std::vector<uint16_t>{65535, 17}));
  assert(tx_data.size() == message.size());
  assert(_write(1, nullptr, 0) == 0);
  assert(_write(1, nullptr, 1) == -1 && errno == EINVAL);
  tx_sizes.clear();
  fail_tx_call = 1;
  assert(_write(1, message.data(), 100) == -1 && errno == EIO);
  tx_sizes.clear();
  fail_tx_call = 2;
  assert(_write(1, message.data(), 65552) == 65535);
#ifdef STM32F767xx
  Uart console(UartPeripheral::kUsart3, PinName::kPD9, PinName::kPD8);
  assert(console.Initialize() && console.GetHandle().Instance == USART3);
  Uart arduino(UartPeripheral::kUsart6, PinName::kD0, PinName::kD1);
  assert(arduino.Initialize() && arduino.GetHandle().Instance == USART6);
  Uart wrong_pin(UartPeripheral::kUsart3, PinName::kPC5, PinName::kPB10);
  assert(!wrong_pin.Initialize());
#endif
}
