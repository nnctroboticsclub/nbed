#if defined(STM32F767xx)

#include "f7/i2c.h"

#include <array>

#include "detail/i2c_timing.h"
#include "f7/pin_mapper.h"
#include "f7/pin_routes.h"
#include "stm32f7xx_hal_gpio.h"
#include "stm32f7xx_hal_rcc.h"
#include "stm32f7xx_hal_rcc_ex.h"

namespace nkrc::nbed::stm32::f7 {
namespace {

const std::array kI2cInstances{I2C1, I2C2, I2C3, I2C4};
std::array<I2c*, 4> owners{};

void EnableI2cClock(I2cPeripheral peripheral) {
  switch (peripheral) {
    case I2cPeripheral::kI2c1:
      __HAL_RCC_I2C1_CONFIG(RCC_I2C1CLKSOURCE_HSI);
      __HAL_RCC_I2C1_CLK_ENABLE();
      break;
    case I2cPeripheral::kI2c2:
      __HAL_RCC_I2C2_CONFIG(RCC_I2C2CLKSOURCE_HSI);
      __HAL_RCC_I2C2_CLK_ENABLE();
      break;
    case I2cPeripheral::kI2c3:
      __HAL_RCC_I2C3_CONFIG(RCC_I2C3CLKSOURCE_HSI);
      __HAL_RCC_I2C3_CLK_ENABLE();
      break;
    case I2cPeripheral::kI2c4:
      __HAL_RCC_I2C4_CONFIG(RCC_I2C4CLKSOURCE_HSI);
      __HAL_RCC_I2C4_CLK_ENABLE();
      break;
  }
}

bool IsValidMessage(const interface::I2CMessage& message) {
  return message.address <= 0x7fU && message.size != 0 &&
         message.data != nullptr;
}

}  // namespace

I2c::I2c(I2cPeripheral peripheral, interface::PinName scl,
         interface::PinName sda, uint32_t frequency_hz)
    : peripheral_(peripheral),
      scl_(scl),
      sda_(sda),
      frequency_hz_(frequency_hz) {}

I2c::~I2c() {
  if (!initialized_) {
    return;
  }
  (void)HAL_I2C_DeInit(&handle_);
  owners[static_cast<unsigned>(peripheral_)] = nullptr;
}

bool I2c::Initialize() {
  if (initialized_) {
    return true;
  }
  const auto index = static_cast<unsigned>(peripheral_);
  if (index >= owners.size() || owners[index] != nullptr ||
      !PinMapper::IsValidPinPair(scl_, sda_) || frequency_hz_ == 0 ||
      frequency_hz_ > 400'000) {
    return false;
  }
  const auto number = static_cast<uint8_t>(index + 1U);
  const auto* scl =
      detail::FindPinRoute(kI2cPins, number, PinSignal::kScl, scl_);
  const auto* sda =
      detail::FindPinRoute(kI2cPins, number, PinSignal::kSda, sda_);
  if (scl == nullptr || sda == nullptr) {
    return false;
  }

  owners[index] = this;
  if (!InitializeController()) {
    owners[index] = nullptr;
    return false;
  }
  GPIO_InitTypeDef gpio{};
  gpio.Mode = GPIO_MODE_AF_OD;
  gpio.Pull = GPIO_PULLUP;
  gpio.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  gpio.Alternate = scl->value;
  (void)PinMapper::ConfigureGpio(scl_, gpio);
  gpio.Alternate = sda->value;
  (void)PinMapper::ConfigureGpio(sda_, gpio);
  initialized_ = true;
  return true;
}

bool I2c::InitializeController() {
  const auto timing = detail::I2cTimingHsi16Mhz(frequency_hz_);
  if (!timing) {
    return false;
  }
  EnableI2cClock(peripheral_);
  handle_.Instance = kI2cInstances[static_cast<unsigned>(peripheral_)];
  handle_.Init.Timing = *timing;
  handle_.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
  handle_.Init.OwnAddress1 = 0;
  handle_.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  handle_.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  handle_.Init.OwnAddress2 = 0;
  handle_.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  handle_.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&handle_) != HAL_OK ||
      HAL_I2CEx_ConfigAnalogFilter(&handle_, I2C_ANALOGFILTER_ENABLE) !=
          HAL_OK ||
      HAL_I2CEx_ConfigDigitalFilter(&handle_, 0) != HAL_OK) {
    (void)HAL_I2C_DeInit(&handle_);
    return false;
  }
  return true;
}

bool I2c::Send(const interface::I2CMessage& message) {
  if (!initialized_ || !IsValidMessage(message)) {
    return false;
  }
  const auto address = static_cast<uint16_t>(message.address << 1);
  return HAL_I2C_Master_Transmit(&handle_, address, message.data, message.size,
                                 HAL_MAX_DELAY) == HAL_OK;
}

bool I2c::Receive(interface::I2CMessage& message) {
  if (!initialized_ || !IsValidMessage(message)) {
    return false;
  }
  const auto address = static_cast<uint16_t>(message.address << 1);
  return HAL_I2C_Master_Receive(&handle_, address, message.data, message.size,
                                HAL_MAX_DELAY) == HAL_OK;
}

}  // namespace nkrc::nbed::stm32::f7

#endif
