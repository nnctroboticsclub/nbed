#if defined(STM32F767xx)

#include "f7/gpio.h"

#include "f7/pin_mapper.h"
#include "stm32f7xx_hal.h"  // IWYU pragma: keep
#include "stm32f7xx_hal_gpio.h"

namespace nkrc::nbed::stm32::f7 {
namespace {
uint32_t ToHalPull(GpioPull pull) {
  if (pull == GpioPull::kUp) {
    return GPIO_PULLUP;
  }
  if (pull == GpioPull::kDown) {
    return GPIO_PULLDOWN;
  }
  return GPIO_NOPULL;
}
}  // namespace

DigitalOut::DigitalOut(interface::PinName pin, bool initial_value)
    : pin_(pin), initial_value_(initial_value) {}

bool DigitalOut::Initialize() {
  if (initialized_) {
    return true;
  }
  if (!PinMapper::IsValidPin(pin_)) {
    return false;
  }
  GPIO_TypeDef* const port = PinMapper::GetGpioPort(pin_);
  const uint16_t pin = PinMapper::GetGpioPin(pin_);
  PinMapper::EnableGpioClock(port);
  HAL_GPIO_WritePin(port, pin, initial_value_ ? GPIO_PIN_SET : GPIO_PIN_RESET);
  GPIO_InitTypeDef config{};
  config.Mode = GPIO_MODE_OUTPUT_PP;
  config.Pull = GPIO_NOPULL;
  config.Speed = GPIO_SPEED_FREQ_LOW;
  if (!PinMapper::ConfigureGpio(pin_, config)) {
    return false;
  }
  port_ = port;
  pin_mask_ = pin;
  initialized_ = true;
  return true;
}

void DigitalOut::Write(bool value) {
  if (initialized_) {
    HAL_GPIO_WritePin(port_, pin_mask_, value ? GPIO_PIN_SET : GPIO_PIN_RESET);
  }
}

bool DigitalOut::Read() const {
  return initialized_ && HAL_GPIO_ReadPin(port_, pin_mask_) == GPIO_PIN_SET;
}

DigitalIn::DigitalIn(interface::PinName pin, GpioPull pull)
    : pin_(pin), pull_(pull) {}

bool DigitalIn::Initialize() {
  if (initialized_) {
    return true;
  }
  if (!PinMapper::IsValidPin(pin_) || static_cast<unsigned>(pull_) > 2) {
    return false;
  }
  GPIO_InitTypeDef config{};
  config.Mode = GPIO_MODE_INPUT;
  config.Pull = ToHalPull(pull_);
  if (!PinMapper::ConfigureGpio(pin_, config)) {
    return false;
  }
  port_ = PinMapper::GetGpioPort(pin_);
  pin_mask_ = PinMapper::GetGpioPin(pin_);
  initialized_ = true;
  return true;
}

bool DigitalIn::Read() const {
  return initialized_ && HAL_GPIO_ReadPin(port_, pin_mask_) == GPIO_PIN_SET;
}

}  // namespace nkrc::nbed::stm32::f7

#endif
