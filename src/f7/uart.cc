#if defined(STM32F767xx)

#include "f7/uart.h"

#include <algorithm>
#include <array>
#include <cerrno>
#include <limits>
#include <optional>

#include "f7/pin_mapper.h"
#include "stm32f7xx_hal.h"  // IWYU pragma: keep
#include "stm32f7xx_hal_gpio.h"
#include "stm32f7xx_hal_gpio_ex.h"
#include "stm32f7xx_hal_rcc.h"

namespace nkrc::nbed::stm32::f7 {
namespace {
struct UartHardware {
  USART_TypeDef* instance;
  IRQn_Type irq;
};

// Keep this order aligned with UartPeripheral, preserving existing enum values.
const std::array kHardware{
    UartHardware{USART2, USART2_IRQn},
    UartHardware{USART3, USART3_IRQn},
    UartHardware{UART4, UART4_IRQn},
    UartHardware{USART6, USART6_IRQn},
    UartHardware{USART1, USART1_IRQn},
    UartHardware{UART5, UART5_IRQn},
    UartHardware{UART7, UART7_IRQn},
    UartHardware{UART8, UART8_IRQn},
};
std::array<Uart*, kHardware.size()> instances{};

struct UartPin {
  UartPeripheral peripheral;
  interface::PinName pin;
  bool receive;
  uint32_t alternate;
};

// ST package pinout and GPIO alternate-function tables; see docs/uart.md.
constexpr std::array kPins{
    UartPin{UartPeripheral::kUsart1, interface::PinName::kPA10,
            true, GPIO_AF7_USART1},
    UartPin{UartPeripheral::kUsart1, interface::PinName::kPB15,
            true, GPIO_AF4_USART1},
    UartPin{UartPeripheral::kUsart1, interface::PinName::kPB7,
            true, GPIO_AF7_USART1},
    UartPin{UartPeripheral::kUsart1, interface::PinName::kPA9,
            false, GPIO_AF7_USART1},
    UartPin{UartPeripheral::kUsart1, interface::PinName::kPB14,
            false, GPIO_AF4_USART1},
    UartPin{UartPeripheral::kUsart1, interface::PinName::kPB6,
            false, GPIO_AF7_USART1},
    UartPin{UartPeripheral::kUsart2, interface::PinName::kPA3,
            true, GPIO_AF7_USART2},
    UartPin{UartPeripheral::kUsart2, interface::PinName::kPD6,
            true, GPIO_AF7_USART2},
    UartPin{UartPeripheral::kUsart2, interface::PinName::kPA2,
            false, GPIO_AF7_USART2},
    UartPin{UartPeripheral::kUsart2, interface::PinName::kPD5,
            false, GPIO_AF7_USART2},
    UartPin{UartPeripheral::kUsart3, interface::PinName::kPB11,
            true, GPIO_AF7_USART3},
    UartPin{UartPeripheral::kUsart3, interface::PinName::kPC11,
            true, GPIO_AF7_USART3},
    UartPin{UartPeripheral::kUsart3, interface::PinName::kPD9,
            true, GPIO_AF7_USART3},
    UartPin{UartPeripheral::kUsart3, interface::PinName::kPB10,
            false, GPIO_AF7_USART3},
    UartPin{UartPeripheral::kUsart3, interface::PinName::kPC10,
            false, GPIO_AF7_USART3},
    UartPin{UartPeripheral::kUsart3, interface::PinName::kPD8,
            false, GPIO_AF7_USART3},
    UartPin{UartPeripheral::kUart4, interface::PinName::kPA1,
            true, GPIO_AF8_UART4},
    UartPin{UartPeripheral::kUart4, interface::PinName::kPA11,
            true, GPIO_AF6_UART4},
    UartPin{UartPeripheral::kUart4, interface::PinName::kPC11,
            true, GPIO_AF8_UART4},
    UartPin{UartPeripheral::kUart4, interface::PinName::kPD0,
            true, GPIO_AF8_UART4},
    UartPin{UartPeripheral::kUart4, interface::PinName::kPA0,
            false, GPIO_AF8_UART4},
    UartPin{UartPeripheral::kUart4, interface::PinName::kPA12,
            false, GPIO_AF6_UART4},
    UartPin{UartPeripheral::kUart4, interface::PinName::kPC10,
            false, GPIO_AF8_UART4},
    UartPin{UartPeripheral::kUart4, interface::PinName::kPD1,
            false, GPIO_AF8_UART4},
    UartPin{UartPeripheral::kUart5, interface::PinName::kPB12,
            true, GPIO_AF8_UART5},
    UartPin{UartPeripheral::kUart5, interface::PinName::kPB5,
            true, GPIO_AF1_UART5},
    UartPin{UartPeripheral::kUart5, interface::PinName::kPB8,
            true, GPIO_AF7_UART5},
    UartPin{UartPeripheral::kUart5, interface::PinName::kPD2,
            true, GPIO_AF8_UART5},
    UartPin{UartPeripheral::kUart5, interface::PinName::kPB13,
            false, GPIO_AF8_UART5},
    UartPin{UartPeripheral::kUart5, interface::PinName::kPB6,
            false, GPIO_AF1_UART5},
    UartPin{UartPeripheral::kUart5, interface::PinName::kPB9,
            false, GPIO_AF7_UART5},
    UartPin{UartPeripheral::kUart5, interface::PinName::kPC12,
            false, GPIO_AF8_UART5},
    UartPin{UartPeripheral::kUsart6, interface::PinName::kPC7,
            true, GPIO_AF8_USART6},
    UartPin{UartPeripheral::kUsart6, interface::PinName::kPG9,
            true, GPIO_AF8_USART6},
    UartPin{UartPeripheral::kUsart6, interface::PinName::kPC6,
            false, GPIO_AF8_USART6},
    UartPin{UartPeripheral::kUsart6, interface::PinName::kPG14,
            false, GPIO_AF8_USART6},
    UartPin{UartPeripheral::kUart7, interface::PinName::kPA8,
            true, GPIO_AF12_UART7},
    UartPin{UartPeripheral::kUart7, interface::PinName::kPB3,
            true, GPIO_AF12_UART7},
    UartPin{UartPeripheral::kUart7, interface::PinName::kPE7,
            true, GPIO_AF8_UART7},
    UartPin{UartPeripheral::kUart7, interface::PinName::kPF6,
            true, GPIO_AF8_UART7},
    UartPin{UartPeripheral::kUart7, interface::PinName::kPA15,
            false, GPIO_AF12_UART7},
    UartPin{UartPeripheral::kUart7, interface::PinName::kPB4,
            false, GPIO_AF12_UART7},
    UartPin{UartPeripheral::kUart7, interface::PinName::kPE8,
            false, GPIO_AF8_UART7},
    UartPin{UartPeripheral::kUart7, interface::PinName::kPF7,
            false, GPIO_AF8_UART7},
    UartPin{UartPeripheral::kUart8, interface::PinName::kPE0,
            true, GPIO_AF8_UART8},
    UartPin{UartPeripheral::kUart8, interface::PinName::kPE1,
            false, GPIO_AF8_UART8},
};

std::optional<uint32_t> AlternateFunction(UartPeripheral peripheral,
                                         interface::PinName pin, bool receive) {
  for (const auto& route : kPins) {
    if (route.peripheral == peripheral && route.pin == pin &&
        route.receive == receive) {
      return route.alternate;
    }
  }
  return std::nullopt;
}

Uart*& Console() {
  static Uart* instance = nullptr;
  return instance;
}

void EnableUartClock(UartPeripheral peripheral) {
  switch (peripheral) {
    case UartPeripheral::kUsart2:
      __HAL_RCC_USART2_CONFIG(RCC_USART2CLKSOURCE_PCLK1);
      __HAL_RCC_USART2_CLK_ENABLE();
      break;
    case UartPeripheral::kUsart3:
      __HAL_RCC_USART3_CONFIG(RCC_USART3CLKSOURCE_PCLK1);
      __HAL_RCC_USART3_CLK_ENABLE();
      break;
    case UartPeripheral::kUart4:
      __HAL_RCC_UART4_CONFIG(RCC_UART4CLKSOURCE_PCLK1);
      __HAL_RCC_UART4_CLK_ENABLE();
      break;
    case UartPeripheral::kUsart6:
      __HAL_RCC_USART6_CONFIG(RCC_USART6CLKSOURCE_PCLK2);
      __HAL_RCC_USART6_CLK_ENABLE();
      break;
    case UartPeripheral::kUsart1:
      __HAL_RCC_USART1_CONFIG(RCC_USART1CLKSOURCE_PCLK2);
      __HAL_RCC_USART1_CLK_ENABLE();
      break;
    case UartPeripheral::kUart5:
      __HAL_RCC_UART5_CONFIG(RCC_UART5CLKSOURCE_PCLK1);
      __HAL_RCC_UART5_CLK_ENABLE();
      break;
    case UartPeripheral::kUart7:
      __HAL_RCC_UART7_CONFIG(RCC_UART7CLKSOURCE_PCLK1);
      __HAL_RCC_UART7_CLK_ENABLE();
      break;
    case UartPeripheral::kUart8:
      __HAL_RCC_UART8_CONFIG(RCC_UART8CLKSOURCE_PCLK1);
      __HAL_RCC_UART8_CLK_ENABLE();
      break;
  }
}
}  // namespace

Uart::Uart(UartPeripheral peripheral, interface::PinName rx,
           interface::PinName tx, uint32_t baud_rate,
           uint32_t interrupt_priority)
    : peripheral_(peripheral),
      rx_(rx),
      tx_(tx),
      baud_rate_(baud_rate),
      interrupt_priority_(interrupt_priority) {}

Uart::~Uart() {
  StopDma();
  if (!initialized_) {
    return;
  }
  HAL_NVIC_DisableIRQ(Irq());
  (void)HAL_UART_DeInit(&handle_);
  if (InstanceFor(peripheral_) == this) {
    InstanceFor(peripheral_) = nullptr;
  }
  if (Console() == this) {
    Console() = nullptr;
  }
  initialized_ = false;
}

bool Uart::Initialize() {
  if (initialized_) {
    return true;
  }
  Uart*& instance = InstanceFor(peripheral_);
  if (!IsValidPinPair() || baud_rate_ == 0 || interrupt_priority_ > 15U ||
      (instance != nullptr && instance != this)) {
    return false;
  }
  instance = this;
  EnableUartClock(peripheral_);
  handle_.Instance = kHardware[static_cast<unsigned>(peripheral_)].instance;
  handle_.Init.BaudRate = baud_rate_;
  handle_.Init.WordLength = UART_WORDLENGTH_8B;
  handle_.Init.StopBits = UART_STOPBITS_1;
  handle_.Init.Parity = UART_PARITY_NONE;
  handle_.Init.Mode = UART_MODE_TX_RX;
  handle_.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  handle_.Init.OverSampling = UART_OVERSAMPLING_16;
  handle_.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  handle_.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  GPIO_InitTypeDef gpio{};
  gpio.Mode = GPIO_MODE_AF_PP;
  gpio.Pull = GPIO_NOPULL;
  gpio.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  const auto configure_pin = [&](interface::PinName pin, bool receive) {
    gpio.Alternate = *AlternateFunction(peripheral_, pin, receive);
    return PinMapper::ConfigureGpio(pin, gpio);
  };
  if (!configure_pin(rx_, true) || !configure_pin(tx_, false)) {
    instance = nullptr;
    return false;
  }
  if (HAL_UART_Init(&handle_) != HAL_OK) {
    instance = nullptr;
    return false;
  }
  HAL_NVIC_SetPriority(Irq(), interrupt_priority_, 0);
  HAL_NVIC_EnableIRQ(Irq());
  initialized_ = true;
  return true;
}

bool Uart::Send(interface::UartTxMessage message) {
  return initialized_ && tx_dma_status_ != interface::DmaStatus::kBusy &&
         message.size() <= std::numeric_limits<uint16_t>::max() &&
         HAL_UART_Transmit(
             &handle_,
             // HAL transmit accepts a mutable pointer but does not modify
             // the TX buffer.
             // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
             const_cast<uint8_t*>(message.data()),
             static_cast<uint16_t>(message.size()), HAL_MAX_DELAY) == HAL_OK;
}

bool Uart::Receive(interface::UartRxMessage message) {
  return initialized_ && !rx_interrupt_enabled_ &&
         rx_dma_status_ != interface::DmaStatus::kBusy &&
         message.size() <= std::numeric_limits<uint16_t>::max() &&
         HAL_UART_Receive(&handle_, message.data(),
                          static_cast<uint16_t>(message.size()),
                          HAL_MAX_DELAY) == HAL_OK;
}

bool Uart::EnableRxInterrupt() {
  if (!initialized_ || rx_dma_status_ == interface::DmaStatus::kBusy) {
    return false;
  }
  if (HAL_UART_Receive_IT(&handle_, &rx_byte_, 1) != HAL_OK) {
    return false;
  }
  rx_interrupt_enabled_ = true;
  return true;
}

void Uart::SetRxCallback(const interface::UartRxCallback& callback) {
  rx_callback_ = callback;
}

UART_HandleTypeDef& Uart::GetHandle() { return handle_; }

void Uart::SetConsole() {
  if (initialized_) {
    Console() = this;
  }
}

void Uart::HandleInterrupt(UartPeripheral peripheral) {
  Uart* const uart = InstanceFor(peripheral);
  if (uart != nullptr && uart->initialized_) {
    HAL_UART_IRQHandler(&uart->handle_);
  }
}

void Uart::HandleRxComplete(UART_HandleTypeDef* handle) {
  Uart* uart = FindHandle(handle);
  if (uart != nullptr) {
    uart->OnRxComplete();
  }
}

void Uart::OnRxComplete() {
  if (rx_dma_status_ == interface::DmaStatus::kBusy) {
    detail::Dma::CompleteReceive(rx_dma_buffer_.data(), rx_dma_buffer_.size());
    rx_dma_buffer_ = {};
    rx_dma_status_ = interface::DmaStatus::kComplete;
    // DMA reception is one-shot; polling status owns the completed buffer.
    return;
  }
  if (!rx_interrupt_enabled_) {
    return;
  }
  if (rx_callback_) {
    rx_callback_(interface::UartRxMessage{&rx_byte_, 1});
  }
  (void)EnableRxInterrupt();
}

#include "detail/uart_dma_impl.h"

bool Uart::IsValidPinPair() const {
  return PinMapper::IsValidPinPair(rx_, tx_) &&
         AlternateFunction(peripheral_, rx_, true).has_value() &&
         AlternateFunction(peripheral_, tx_, false).has_value();
}

IRQn_Type Uart::Irq() const {
  return kHardware[static_cast<unsigned>(peripheral_)].irq;
}

Uart*& Uart::InstanceFor(UartPeripheral peripheral) {
  const auto index = static_cast<unsigned>(peripheral);
  // Invalid inputs must not alias an existing peripheral's owner.
  static Uart* invalid = nullptr;
  return index < instances.size() ? instances[index] : invalid;
}

extern "C" {
// newlib supplies a complete output buffer. Send it in HAL-sized chunks so
// printf does not acquire/release the UART for every character.
int _write(int file, const char* ptr, int len) {
  (void)file;
  if (len < 0 || (len > 0 && ptr == nullptr)) {
    errno = EINVAL;
    return -1;
  }
  if (Console() == nullptr) {
    return len;
  }
  int written = 0;
  while (written < len) {
    const auto size = static_cast<uint16_t>(
        std::min<int>(len - written, std::numeric_limits<uint16_t>::max()));
    if (!Console()->Send(interface::UartTxMessage{
            reinterpret_cast<const uint8_t*>(ptr + written), size})) {
      errno = EIO;
      return written == 0 ? -1 : written;
    }
    written += size;
  }
  return written;
}

void USART2_IRQHandler() { Uart::HandleInterrupt(UartPeripheral::kUsart2); }

void USART3_IRQHandler() { Uart::HandleInterrupt(UartPeripheral::kUsart3); }

void UART4_IRQHandler() { Uart::HandleInterrupt(UartPeripheral::kUart4); }

void USART6_IRQHandler() { Uart::HandleInterrupt(UartPeripheral::kUsart6); }

void USART1_IRQHandler() { Uart::HandleInterrupt(UartPeripheral::kUsart1); }

void UART5_IRQHandler() { Uart::HandleInterrupt(UartPeripheral::kUart5); }

void UART7_IRQHandler() { Uart::HandleInterrupt(UartPeripheral::kUart7); }

void UART8_IRQHandler() { Uart::HandleInterrupt(UartPeripheral::kUart8); }

void HAL_UART_TxCpltCallback(UART_HandleTypeDef* huart) {
  Uart::HandleTxComplete(huart);
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef* huart) {
  Uart::HandleError(huart);
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef* huart) {
  Uart::HandleRxComplete(huart);
}

int __io_putchar(int ch) {
  if (Console() == nullptr) {
    return ch;
  }
  const uint8_t byte = static_cast<uint8_t>(ch);
  (void)Console()->Send(interface::UartTxMessage{&byte, 1});
  return ch;
}

int __io_getchar() {
  if (Console() == nullptr) {
    return -1;
  }
  uint8_t byte = 0;
  return Console()->Receive(interface::UartRxMessage{&byte, 1})
             ? static_cast<int>(byte)
             : -1;
}
}  // extern "C"

}  // namespace nkrc::nbed::stm32::f7

#endif
