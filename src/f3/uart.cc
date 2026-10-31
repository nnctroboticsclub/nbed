#if defined(STM32F303x8)

#include "f3/uart.h"

#include <algorithm>
#include <cerrno>
#include <limits>

#include "f3/pin_mapper.h"
#include "stm32f3xx_hal.h"  // IWYU pragma: keep
#include "stm32f3xx_hal_gpio.h"
#include "stm32f3xx_hal_gpio_ex.h"
#include "stm32f3xx_hal_rcc.h"

namespace nkrc::nbed::stm32::f3 {
namespace {
Uart*& Usart2Instance() {
  static Uart* instance = nullptr;
  return instance;
}

Uart*& Usart1Instance() {
  static Uart* instance = nullptr;
  return instance;
}

Uart*& Console() {
  static Uart* instance = nullptr;
  return instance;
}

void EnableUartClock(UartPeripheral peripheral) {
  if (peripheral == UartPeripheral::kUsart2) {
    __HAL_RCC_USART2_CLK_ENABLE();
  } else {
    __HAL_RCC_USART1_CLK_ENABLE();
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
  if (peripheral_ == UartPeripheral::kUsart2) {
    handle_.Instance = USART2;
  } else {
    handle_.Instance = USART1;
  }
  handle_.Init.BaudRate = baud_rate_;
  handle_.Init.WordLength = UART_WORDLENGTH_8B;
  handle_.Init.StopBits = UART_STOPBITS_1;
  handle_.Init.Parity = UART_PARITY_NONE;
  handle_.Init.Mode = UART_MODE_TX_RX;
  handle_.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  handle_.Init.OverSampling = UART_OVERSAMPLING_16;
  GPIO_InitTypeDef gpio{};
  gpio.Mode = GPIO_MODE_AF_PP;
  gpio.Pull = GPIO_NOPULL;
  gpio.Speed = GPIO_SPEED_FREQ_HIGH;
  gpio.Alternate = GPIO_AF7_USART2;
  if (!PinMapper::ConfigureGpio(rx_, gpio) ||
      !PinMapper::ConfigureGpio(tx_, gpio)) {
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
  using interface::PinName;
  if (!PinMapper::IsValidPinPair(rx_, tx_)) {
    return false;
  }
  if (peripheral_ == UartPeripheral::kUsart2) {
    return (rx_ == PinName::kPA3 || rx_ == PinName::kPA15 ||
            rx_ == PinName::kPB4) &&
           (tx_ == PinName::kPA2 || tx_ == PinName::kPA14 ||
            tx_ == PinName::kPB3);
  }
  return peripheral_ == UartPeripheral::kUsart1 &&
         (rx_ == PinName::kPA10 || rx_ == PinName::kPB7) &&
         (tx_ == PinName::kPA9 || tx_ == PinName::kPB6);
}

IRQn_Type Uart::Irq() const {
  return peripheral_ == UartPeripheral::kUsart2 ? USART2_IRQn : USART1_IRQn;
}

Uart*& Uart::InstanceFor(UartPeripheral peripheral) {
  if (peripheral == UartPeripheral::kUsart2) {
    return Usart2Instance();
  }
  if (peripheral == UartPeripheral::kUsart1) {
    return Usart1Instance();
  }
  static Uart* invalid = nullptr;
  return invalid;
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

void USART1_IRQHandler() { Uart::HandleInterrupt(UartPeripheral::kUsart1); }

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

}  // namespace nkrc::nbed::stm32::f3

#endif
