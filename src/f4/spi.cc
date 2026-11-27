#if defined(STM32F446xx)

#include "f4/spi.h"

#include <array>

#include "detail/spi_master.h"
#include "f4/pin_mapper.h"
#include "f4/pin_routes.h"
#include "stm32f4xx_hal_rcc.h"

namespace nkrc::nbed::stm32::f4 {
namespace {

struct SpiHardware {
  SPI_TypeDef* instance;
  bool apb2;
};

const std::array kHardware{
    SpiHardware{SPI1, true},
    SpiHardware{SPI2, false},
    SpiHardware{SPI3, false},
};
std::array<Spi*, kHardware.size()> owners{};

void EnableSpiClock(SpiPeripheral peripheral) {
  switch (peripheral) {
    case SpiPeripheral::kSpi1:
      __HAL_RCC_SPI1_CLK_ENABLE();
      break;
    case SpiPeripheral::kSpi2:
      __HAL_RCC_SPI2_CLK_ENABLE();
      break;
    case SpiPeripheral::kSpi3:
      __HAL_RCC_SPI3_CLK_ENABLE();
      break;
  }
}

}  // namespace

Spi::Spi(SpiPeripheral peripheral, interface::PinName sck,
         interface::PinName miso, interface::PinName mosi,
         uint32_t frequency_hz, interface::SpiMode mode,
         interface::SpiBitOrder bit_order)
    : peripheral_(peripheral),
      sck_(sck),
      miso_(miso),
      mosi_(mosi),
      frequency_hz_(frequency_hz),
      mode_(mode),
      bit_order_(bit_order) {}

Spi::~Spi() {
  if (initialized_) {
    (void)HAL_SPI_DeInit(&handle_);
    owners[static_cast<unsigned>(peripheral_)] = nullptr;
  }
}

bool Spi::Initialize() {
  if (initialized_) {
    return true;
  }
  const auto index = static_cast<unsigned>(peripheral_);
  if (index >= kHardware.size() || owners[index] != nullptr || sck_ == miso_ ||
      sck_ == mosi_ || miso_ == mosi_) {
    return false;
  }
  const auto number = static_cast<uint8_t>(index + 1U);
  const std::array routes{
      detail::FindPinRoute(kSpiPins, number, PinSignal::kSck, sck_),
      detail::FindPinRoute(kSpiPins, number, PinSignal::kMiso, miso_),
      detail::FindPinRoute(kSpiPins, number, PinSignal::kMosi, mosi_),
  };
  for (const auto* route : routes) {
    if (route == nullptr || !PinMapper::IsValidPin(route->pin)) {
      return false;
    }
  }
  const auto& hardware = kHardware[index];
  const auto timing = detail::ComputeSpiTiming(
      hardware.apb2 ? HAL_RCC_GetPCLK2Freq() : HAL_RCC_GetPCLK1Freq(),
      frequency_hz_, hardware.apb2 ? 45'000'000 : 22'500'000);
  if (!timing || !detail::ConfigureSpiMaster(handle_, timing->prescaler, mode_,
                                             bit_order_)) {
    return false;
  }
  owners[index] = this;
  handle_.Instance = hardware.instance;
  EnableSpiClock(peripheral_);
  if (HAL_SPI_Init(&handle_) != HAL_OK) {
    (void)HAL_SPI_DeInit(&handle_);
    owners[index] = nullptr;
    return false;
  }

  GPIO_InitTypeDef gpio{};
  gpio.Mode = GPIO_MODE_AF_PP;
  gpio.Pull = GPIO_NOPULL;
  gpio.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  for (const auto* route : routes) {
    gpio.Alternate = route->value;
    (void)PinMapper::ConfigureGpio(route->pin, gpio);
  }
  // CS をアサートする前に、SCK をモードで指定した待機レベルにする。
  __HAL_SPI_ENABLE(&handle_);
  actual_frequency_hz_ = timing->actual_hz;
  initialized_ = true;
  return true;
}

uint32_t Spi::GetFrequencyHz() const { return actual_frequency_hz_; }

bool Spi::Send(std::span<const uint8_t> data, uint32_t timeout_ms) {
  return initialized_ && detail::SpiSend(handle_, data, timeout_ms);
}

bool Spi::Receive(std::span<uint8_t> data, uint32_t timeout_ms) {
  return initialized_ && detail::SpiReceive(handle_, data, timeout_ms);
}

bool Spi::Transfer(std::span<const uint8_t> tx, std::span<uint8_t> rx,
                   uint32_t timeout_ms) {
  return initialized_ && detail::SpiTransfer(handle_, tx, rx, timeout_ms);
}

}  // namespace nkrc::nbed::stm32::f4

#endif
