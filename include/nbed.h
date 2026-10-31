#pragma once

// C++ 標準ライブラリ
#include <chrono>
#include <cstdint>

// 単位系
#include "units.h"  // IWYU pragma: export

// インターフェース
#include "interface/can.h"           // IWYU pragma: export
#include "interface/dma.h"           // IWYU pragma: export
#include "interface/encoder.h"       // IWYU pragma: export
#include "interface/i2c.h"           // IWYU pragma: export
#include "interface/interrupt_in.h"  // IWYU pragma: export
#include "interface/pin_names.h"     // IWYU pragma: export
#include "interface/spi.h"           // IWYU pragma: export
#include "interface/uart.h"          // IWYU pragma: export

// 初心者向けグローバル名前空間
#ifndef NBED_NO_GLOBAL_NAMES
using nkrc::nbed::interface::CanFrameType;
using nkrc::nbed::interface::CanMessage;
using nkrc::nbed::interface::CanRxCallback;
using nkrc::nbed::interface::CanRxFilter;
using nkrc::nbed::interface::DmaRequest;
using nkrc::nbed::interface::DmaStatus;
using nkrc::nbed::interface::GpioInterruptCallback;
using nkrc::nbed::interface::I2CMessage;
using nkrc::nbed::interface::InterruptEdge;
using nkrc::nbed::interface::SpiBitOrder;
using nkrc::nbed::interface::SpiMode;
using nkrc::nbed::interface::UartRxCallback;
using nkrc::nbed::interface::UartRxMessage;
using nkrc::nbed::interface::UartTxMessage;
using nkrc::nbed::interface::UartWordTxMessage;
using nkrc::nbed::interface::UartWordRxMessage;
using nkrc::nbed::interface::UartWordRxCallback;
using enum nkrc::nbed::interface::CanFormat;
using enum nkrc::nbed::interface::CanFrameType;
using enum nkrc::nbed::interface::PinName;
using namespace nkrc::nbed::literals;
using namespace std::chrono_literals;
#endif

// STM32F4シリーズ用の実装
#ifdef STM32F446xx
#include "f4/analog_in.h"     // IWYU pragma: export
#include "f4/can.h"           // IWYU pragma: export
#include "f4/clock.h"         // IWYU pragma: export
#include "f4/dma.h"           // IWYU pragma: export
#include "f4/encoder.h"       // IWYU pragma: export
#include "f4/gpio.h"          // IWYU pragma: export
#include "f4/i2c.h"           // IWYU pragma: export
#include "f4/interrupt_in.h"  // IWYU pragma: export
#include "f4/pwm.h"           // IWYU pragma: export
#include "f4/spi.h"           // IWYU pragma: export
#include "f4/system.h"        // IWYU pragma: export
#include "f4/time.h"          // IWYU pragma: export
#include "f4/uart.h"          // IWYU pragma: export

// 初心者向けグローバル名前空間
#ifndef NBED_NO_GLOBAL_NAMES
using namespace nkrc::nbed::stm32::f4;
using enum nkrc::nbed::stm32::f4::CanPeripheral;
using enum nkrc::nbed::stm32::f4::CanMode;
// using enum nkrc::nbed::stm32::f4::EncoderTimer;
using enum nkrc::nbed::stm32::f4::GpioPull;
using enum nkrc::nbed::stm32::f4::I2cPeripheral;
using enum nkrc::nbed::stm32::f4::PwmTimer;
using enum nkrc::nbed::stm32::f4::PwmChannel;
using enum nkrc::nbed::stm32::f4::SpiPeripheral;
using enum nkrc::nbed::stm32::f4::UartPeripheral;
#endif  // NBED_NO_GLOBAL_NAMES

#ifndef NBED_NO_GLOBAL_FUNCTIONS
inline void SleepFor(std::chrono::seconds sec) {
  nkrc::nbed::stm32::f4::ClockManager::GetClock().Sleep(
      std::chrono::duration_cast<std::chrono::milliseconds>(sec));
}

inline void SleepFor(std::chrono::milliseconds ms) {
  nkrc::nbed::stm32::f4::ClockManager::GetClock().Sleep(ms);
}

inline void SleepFor(uint32_t ms) {
  nkrc::nbed::stm32::f4::ClockManager::GetClock().Sleep(ms);
}
#endif  // NBED_NO_GLOBAL_FUNCTIONS
#endif  // STM32F446xx

// STM32F3シリーズ用の実装
#ifdef STM32F303x8
#include "f3/analog_in.h"     // IWYU pragma: export
#include "f3/can.h"           // IWYU pragma: export
#include "f3/clock.h"         // IWYU pragma: export
#include "f3/dma.h"           // IWYU pragma: export
#include "f3/encoder.h"       // IWYU pragma: export
#include "f3/gpio.h"          // IWYU pragma: export
#include "f3/i2c.h"           // IWYU pragma: export
#include "f3/interrupt_in.h"  // IWYU pragma: export
#include "f3/pwm.h"           // IWYU pragma: export
#include "f3/spi.h"           // IWYU pragma: export
#include "f3/system.h"        // IWYU pragma: export
#include "f3/time.h"          // IWYU pragma: export
#include "f3/uart.h"          // IWYU pragma: export

// 初心者向けグローバル名前空間
#ifndef NBED_NO_GLOBAL_NAMES
using namespace nkrc::nbed::stm32::f3;
using enum nkrc::nbed::stm32::f3::CanPeripheral;
using enum nkrc::nbed::stm32::f3::CanMode;
// using enum nkrc::nbed::stm32::f3::EncoderTimer;
using enum nkrc::nbed::stm32::f3::GpioPull;
using enum nkrc::nbed::stm32::f3::I2cPeripheral;
using enum nkrc::nbed::stm32::f3::PwmTimer;
using enum nkrc::nbed::stm32::f3::PwmChannel;
using enum nkrc::nbed::stm32::f3::SpiPeripheral;
using enum nkrc::nbed::stm32::f3::UartPeripheral;
#endif  // NBED_NO_GLOBAL_NAMES

#ifndef NBED_NO_GLOBAL_FUNCTIONS
inline void SleepFor(std::chrono::seconds sec) {
  nkrc::nbed::stm32::f3::ClockManager::GetClock().Sleep(
      std::chrono::duration_cast<std::chrono::milliseconds>(sec));
}

inline void SleepFor(std::chrono::milliseconds ms) {
  nkrc::nbed::stm32::f3::ClockManager::GetClock().Sleep(ms);
}

inline void SleepFor(uint32_t ms) {
  nkrc::nbed::stm32::f3::ClockManager::GetClock().Sleep(ms);
}
#endif  // NBED_NO_GLOBAL_FUNCTIONS
#endif  // STM32F303x8

// STM32F7シリーズ用の実装
#ifdef STM32F767xx
#include "f7/analog_in.h"     // IWYU pragma: export
#include "f7/can.h"           // IWYU pragma: export
#include "f7/clock.h"         // IWYU pragma: export
#include "f7/dma.h"           // IWYU pragma: export
#include "f7/encoder.h"       // IWYU pragma: export
#include "f7/gpio.h"          // IWYU pragma: export
#include "f7/i2c.h"           // IWYU pragma: export
#include "f7/interrupt_in.h"  // IWYU pragma: export
#include "f7/pwm.h"           // IWYU pragma: export
#include "f7/spi.h"           // IWYU pragma: export
#include "f7/system.h"        // IWYU pragma: export
#include "f7/time.h"          // IWYU pragma: export
#include "f7/uart.h"          // IWYU pragma: export

// 初心者向けグローバル名前空間
#ifndef NBED_NO_GLOBAL_NAMES
using namespace nkrc::nbed::stm32::f7;
using enum nkrc::nbed::stm32::f7::CanPeripheral;
using enum nkrc::nbed::stm32::f7::CanMode;
// using enum nkrc::nbed::stm32::f7::EncoderTimer;
using enum nkrc::nbed::stm32::f7::GpioPull;
using enum nkrc::nbed::stm32::f7::I2cPeripheral;
using enum nkrc::nbed::stm32::f7::PwmTimer;
using enum nkrc::nbed::stm32::f7::PwmChannel;
using enum nkrc::nbed::stm32::f7::SpiPeripheral;
using enum nkrc::nbed::stm32::f7::UartPeripheral;
#endif  // NBED_NO_GLOBAL_NAMES

#ifndef NBED_NO_GLOBAL_FUNCTIONS
inline void SleepFor(std::chrono::seconds sec) {
  nkrc::nbed::stm32::f7::ClockManager::GetClock().Sleep(
      std::chrono::duration_cast<std::chrono::milliseconds>(sec));
}

inline void SleepFor(std::chrono::milliseconds ms) {
  nkrc::nbed::stm32::f7::ClockManager::GetClock().Sleep(ms);
}

inline void SleepFor(uint32_t ms) {
  nkrc::nbed::stm32::f7::ClockManager::GetClock().Sleep(ms);
}
#endif  // NBED_NO_GLOBAL_FUNCTIONS
#endif  // STM32F767xx
