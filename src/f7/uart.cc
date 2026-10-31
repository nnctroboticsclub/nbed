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

enum class UartSignal { kRx, kTx, kCts, kRts };

struct UartPin {
  UartPeripheral peripheral;
  interface::PinName pin;
  UartSignal signal;
  uint32_t alternate;
};

// ST package pinout and GPIO alternate-function tables; see docs/uart.md.
constexpr std::array kPins{
    UartPin{UartPeripheral::kUart4, interface::PinName::kPA0,
            UartSignal::kTx, GPIO_AF8_UART4},
    UartPin{UartPeripheral::kUsart2, interface::PinName::kPA0,
            UartSignal::kCts, GPIO_AF7_USART2},
    UartPin{UartPeripheral::kUart4, interface::PinName::kPA1,
            UartSignal::kRx, GPIO_AF8_UART4},
    UartPin{UartPeripheral::kUsart2, interface::PinName::kPA1,
            UartSignal::kRts, GPIO_AF7_USART2},
    UartPin{UartPeripheral::kUsart1, interface::PinName::kPA10,
            UartSignal::kRx, GPIO_AF7_USART1},
    UartPin{UartPeripheral::kUart4, interface::PinName::kPA11,
            UartSignal::kRx, GPIO_AF6_UART4},
    UartPin{UartPeripheral::kUsart1, interface::PinName::kPA11,
            UartSignal::kCts, GPIO_AF7_USART1},
    UartPin{UartPeripheral::kUart4, interface::PinName::kPA12,
            UartSignal::kTx, GPIO_AF6_UART4},
    UartPin{UartPeripheral::kUsart1, interface::PinName::kPA12,
            UartSignal::kRts, GPIO_AF7_USART1},
    UartPin{UartPeripheral::kUart4, interface::PinName::kPA15,
            UartSignal::kRts, GPIO_AF8_UART4},
    UartPin{UartPeripheral::kUart7, interface::PinName::kPA15,
            UartSignal::kTx, GPIO_AF12_UART7},
    UartPin{UartPeripheral::kUsart2, interface::PinName::kPA2,
            UartSignal::kTx, GPIO_AF7_USART2},
    UartPin{UartPeripheral::kUsart2, interface::PinName::kPA3,
            UartSignal::kRx, GPIO_AF7_USART2},
    UartPin{UartPeripheral::kUart7, interface::PinName::kPA8,
            UartSignal::kRx, GPIO_AF12_UART7},
    UartPin{UartPeripheral::kUsart1, interface::PinName::kPA9,
            UartSignal::kTx, GPIO_AF7_USART1},
    UartPin{UartPeripheral::kUart4, interface::PinName::kPB0,
            UartSignal::kCts, GPIO_AF8_UART4},
    UartPin{UartPeripheral::kUsart3, interface::PinName::kPB10,
            UartSignal::kTx, GPIO_AF7_USART3},
    UartPin{UartPeripheral::kUsart3, interface::PinName::kPB11,
            UartSignal::kRx, GPIO_AF7_USART3},
    UartPin{UartPeripheral::kUart5, interface::PinName::kPB12,
            UartSignal::kRx, GPIO_AF8_UART5},
    UartPin{UartPeripheral::kUart5, interface::PinName::kPB13,
            UartSignal::kTx, GPIO_AF8_UART5},
    UartPin{UartPeripheral::kUsart3, interface::PinName::kPB13,
            UartSignal::kCts, GPIO_AF7_USART3},
    UartPin{UartPeripheral::kUart4, interface::PinName::kPB14,
            UartSignal::kRts, GPIO_AF8_UART4},
    UartPin{UartPeripheral::kUsart1, interface::PinName::kPB14,
            UartSignal::kTx, GPIO_AF4_USART1},
    UartPin{UartPeripheral::kUsart3, interface::PinName::kPB14,
            UartSignal::kRts, GPIO_AF7_USART3},
    UartPin{UartPeripheral::kUart4, interface::PinName::kPB15,
            UartSignal::kCts, GPIO_AF8_UART4},
    UartPin{UartPeripheral::kUsart1, interface::PinName::kPB15,
            UartSignal::kRx, GPIO_AF4_USART1},
    UartPin{UartPeripheral::kUart7, interface::PinName::kPB3,
            UartSignal::kRx, GPIO_AF12_UART7},
    UartPin{UartPeripheral::kUart7, interface::PinName::kPB4,
            UartSignal::kTx, GPIO_AF12_UART7},
    UartPin{UartPeripheral::kUart5, interface::PinName::kPB5,
            UartSignal::kRx, GPIO_AF1_UART5},
    UartPin{UartPeripheral::kUart5, interface::PinName::kPB6,
            UartSignal::kTx, GPIO_AF1_UART5},
    UartPin{UartPeripheral::kUsart1, interface::PinName::kPB6,
            UartSignal::kTx, GPIO_AF7_USART1},
    UartPin{UartPeripheral::kUsart1, interface::PinName::kPB7,
            UartSignal::kRx, GPIO_AF7_USART1},
    UartPin{UartPeripheral::kUart5, interface::PinName::kPB8,
            UartSignal::kRx, GPIO_AF7_UART5},
    UartPin{UartPeripheral::kUart5, interface::PinName::kPB9,
            UartSignal::kTx, GPIO_AF7_UART5},
    UartPin{UartPeripheral::kUart4, interface::PinName::kPC10,
            UartSignal::kTx, GPIO_AF8_UART4},
    UartPin{UartPeripheral::kUsart3, interface::PinName::kPC10,
            UartSignal::kTx, GPIO_AF7_USART3},
    UartPin{UartPeripheral::kUart4, interface::PinName::kPC11,
            UartSignal::kRx, GPIO_AF8_UART4},
    UartPin{UartPeripheral::kUsart3, interface::PinName::kPC11,
            UartSignal::kRx, GPIO_AF7_USART3},
    UartPin{UartPeripheral::kUart5, interface::PinName::kPC12,
            UartSignal::kTx, GPIO_AF8_UART5},
    UartPin{UartPeripheral::kUsart6, interface::PinName::kPC6,
            UartSignal::kTx, GPIO_AF8_USART6},
    UartPin{UartPeripheral::kUsart6, interface::PinName::kPC7,
            UartSignal::kRx, GPIO_AF8_USART6},
    UartPin{UartPeripheral::kUart5, interface::PinName::kPC8,
            UartSignal::kRts, GPIO_AF7_UART5},
    UartPin{UartPeripheral::kUart5, interface::PinName::kPC9,
            UartSignal::kCts, GPIO_AF7_UART5},
    UartPin{UartPeripheral::kUart4, interface::PinName::kPD0,
            UartSignal::kRx, GPIO_AF8_UART4},
    UartPin{UartPeripheral::kUart4, interface::PinName::kPD1,
            UartSignal::kTx, GPIO_AF8_UART4},
    UartPin{UartPeripheral::kUsart3, interface::PinName::kPD11,
            UartSignal::kCts, GPIO_AF7_USART3},
    UartPin{UartPeripheral::kUsart3, interface::PinName::kPD12,
            UartSignal::kRts, GPIO_AF7_USART3},
    UartPin{UartPeripheral::kUart8, interface::PinName::kPD14,
            UartSignal::kCts, GPIO_AF8_UART8},
    UartPin{UartPeripheral::kUart8, interface::PinName::kPD15,
            UartSignal::kRts, GPIO_AF8_UART8},
    UartPin{UartPeripheral::kUart5, interface::PinName::kPD2,
            UartSignal::kRx, GPIO_AF8_UART5},
    UartPin{UartPeripheral::kUsart2, interface::PinName::kPD3,
            UartSignal::kCts, GPIO_AF7_USART2},
    UartPin{UartPeripheral::kUsart2, interface::PinName::kPD4,
            UartSignal::kRts, GPIO_AF7_USART2},
    UartPin{UartPeripheral::kUsart2, interface::PinName::kPD5,
            UartSignal::kTx, GPIO_AF7_USART2},
    UartPin{UartPeripheral::kUsart2, interface::PinName::kPD6,
            UartSignal::kRx, GPIO_AF7_USART2},
    UartPin{UartPeripheral::kUsart3, interface::PinName::kPD8,
            UartSignal::kTx, GPIO_AF7_USART3},
    UartPin{UartPeripheral::kUsart3, interface::PinName::kPD9,
            UartSignal::kRx, GPIO_AF7_USART3},
    UartPin{UartPeripheral::kUart8, interface::PinName::kPE0,
            UartSignal::kRx, GPIO_AF8_UART8},
    UartPin{UartPeripheral::kUart8, interface::PinName::kPE1,
            UartSignal::kTx, GPIO_AF8_UART8},
    UartPin{UartPeripheral::kUart7, interface::PinName::kPE10,
            UartSignal::kCts, GPIO_AF8_UART7},
    UartPin{UartPeripheral::kUart7, interface::PinName::kPE7,
            UartSignal::kRx, GPIO_AF8_UART7},
    UartPin{UartPeripheral::kUart7, interface::PinName::kPE8,
            UartSignal::kTx, GPIO_AF8_UART7},
    UartPin{UartPeripheral::kUart7, interface::PinName::kPE9,
            UartSignal::kRts, GPIO_AF8_UART7},
    UartPin{UartPeripheral::kUart7, interface::PinName::kPF6,
            UartSignal::kRx, GPIO_AF8_UART7},
    UartPin{UartPeripheral::kUart7, interface::PinName::kPF7,
            UartSignal::kTx, GPIO_AF8_UART7},
    UartPin{UartPeripheral::kUart7, interface::PinName::kPF8,
            UartSignal::kRts, GPIO_AF8_UART7},
    UartPin{UartPeripheral::kUart7, interface::PinName::kPF9,
            UartSignal::kCts, GPIO_AF8_UART7},
    UartPin{UartPeripheral::kUsart6, interface::PinName::kPG12,
            UartSignal::kRts, GPIO_AF8_USART6},
    UartPin{UartPeripheral::kUsart6, interface::PinName::kPG13,
            UartSignal::kCts, GPIO_AF8_USART6},
    UartPin{UartPeripheral::kUsart6, interface::PinName::kPG14,
            UartSignal::kTx, GPIO_AF8_USART6},
    UartPin{UartPeripheral::kUsart6, interface::PinName::kPG15,
            UartSignal::kCts, GPIO_AF8_USART6},
    UartPin{UartPeripheral::kUsart6, interface::PinName::kPG8,
            UartSignal::kRts, GPIO_AF8_USART6},
    UartPin{UartPeripheral::kUsart6, interface::PinName::kPG9,
            UartSignal::kRx, GPIO_AF8_USART6},
};

std::optional<uint32_t> AlternateFunction(UartPeripheral peripheral,
                                         interface::PinName pin,
                                         UartSignal signal) {
  for (const auto& route : kPins) {
    if (route.peripheral == peripheral && route.pin == pin &&
        route.signal == signal) {
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

Uart::Uart(UartConfig config)
    : config_(config),
      peripheral_(config.peripheral),
      rx_(config.rx),
      tx_(config.tx),
      interrupt_priority_(config.interrupt_priority) {}

Uart::Uart(UartPeripheral peripheral, interface::PinName rx,
           interface::PinName tx, uint32_t baud_rate,
           uint32_t interrupt_priority)
    : Uart(UartConfig{.peripheral = peripheral,
                      .rx = rx,
                      .tx = tx,
                      .hal = detail::DefaultUartInit(baud_rate),
                      .interrupt_priority = interrupt_priority}) {}

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
  if (!IsValidPinPair() || !IsValidConfig() ||
      interrupt_priority_ > 15U ||
      (instance != nullptr && instance != this)) {
    return false;
  }
  instance = this;
  EnableUartClock(peripheral_);
  handle_.Instance = kHardware[static_cast<unsigned>(peripheral_)].instance;
  handle_.Init = config_.hal;
  handle_.AdvancedInit = config_.advanced;
  GPIO_InitTypeDef gpio{};
  gpio.Mode = GPIO_MODE_AF_PP;
  gpio.Pull = config_.gpio_pull;
  gpio.Speed = config_.gpio_speed;
  const auto configure_pin = [&](interface::PinName pin, UartSignal signal) {
    if (pin == interface::PinName::kNC) {
      return true;
    }
    gpio.Alternate = *AlternateFunction(peripheral_, pin, signal);
    return PinMapper::ConfigureGpio(pin, gpio);
  };
  if (!configure_pin(rx_, UartSignal::kRx) ||
      !configure_pin(tx_, UartSignal::kTx) ||
      !configure_pin(config_.cts, UartSignal::kCts) ||
      !configure_pin(config_.rts, UartSignal::kRts)) {
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
  return initialized_ && (config_.hal.Mode & UART_MODE_TX) != 0 &&
         !detail::IsUartWordTransfer(config_.hal) && tx_dma_status_ != interface::DmaStatus::kBusy &&
         message.size() <= std::numeric_limits<uint16_t>::max() &&
         HAL_UART_Transmit(
             &handle_,
             // HAL transmit accepts a mutable pointer but does not modify
             // the TX buffer.
             // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
             const_cast<uint8_t*>(message.data()),
             static_cast<uint16_t>(message.size()),
             config_.send_timeout) == HAL_OK;
}

bool Uart::Receive(interface::UartRxMessage message) {
  return initialized_ && (config_.hal.Mode & UART_MODE_RX) != 0 &&
         !detail::IsUartWordTransfer(config_.hal) && !rx_interrupt_enabled_ &&
         rx_dma_status_ != interface::DmaStatus::kBusy &&
         message.size() <= std::numeric_limits<uint16_t>::max() &&
         HAL_UART_Receive(&handle_, message.data(),
                          static_cast<uint16_t>(message.size()),
                          config_.receive_timeout) == HAL_OK;
}

bool Uart::EnableRxInterrupt() {
  if (!initialized_ || (config_.hal.Mode & UART_MODE_RX) == 0 ||
      rx_dma_status_ == interface::DmaStatus::kBusy) {
    return false;
  }
  if (HAL_UART_Receive_IT(&handle_,
                         reinterpret_cast<uint8_t*>(&rx_word_), 1) != HAL_OK) {
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
  if (initialized_ && !detail::IsUartWordTransfer(config_.hal) &&
      (config_.hal.Mode & UART_MODE_TX) != 0) {
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
  if (detail::IsUartWordTransfer(config_.hal)) {
    if (rx_word_callback_) {
      rx_word_callback_(interface::UartWordRxMessage{&rx_word_, 1});
    }
  } else if (rx_callback_) {
    rx_callback_(interface::UartRxMessage{
        reinterpret_cast<uint8_t*>(&rx_word_), 1});
  }
  (void)EnableRxInterrupt();
}

#include "detail/uart_words_impl.h"
#include "detail/uart_dma_impl.h"

bool Uart::IsValidPinPair() const {
  using interface::PinName;
  bool swapped = false;
  swapped = (config_.advanced.AdvFeatureInit &
             UART_ADVFEATURE_SWAP_INIT) != 0 &&
            config_.advanced.Swap == UART_ADVFEATURE_SWAP_ENABLE;
  const bool needs_rx = (config_.hal.Mode &
                          (swapped ? UART_MODE_TX : UART_MODE_RX)) != 0;
  const bool needs_tx = (config_.hal.Mode &
                          (swapped ? UART_MODE_RX : UART_MODE_TX)) != 0;
  const bool needs_cts = (config_.hal.HwFlowCtl & UART_HWCONTROL_CTS) != 0;
  const bool needs_rts = (config_.hal.HwFlowCtl & UART_HWCONTROL_RTS) != 0;
  const auto valid = [&](PinName pin, UartSignal signal, bool required) {
    return pin == PinName::kNC ? !required :
        PinMapper::IsValidPin(pin) &&
        AlternateFunction(peripheral_, pin, signal).has_value();
  };
  if (!valid(rx_, UartSignal::kRx, needs_rx) ||
      !valid(tx_, UartSignal::kTx, needs_tx) ||
      !valid(config_.cts, UartSignal::kCts, needs_cts) ||
      !valid(config_.rts, UartSignal::kRts, needs_rts) ||
      (!needs_cts && config_.cts != PinName::kNC) ||
      (!needs_rts && config_.rts != PinName::kNC)) {
    return false;
  }
  const std::array pins{rx_, tx_, config_.cts, config_.rts};
  for (unsigned i = 0; i < pins.size(); ++i) {
    for (unsigned j = i + 1; j < pins.size(); ++j) {
      if (pins[i] != PinName::kNC && pins[i] == pins[j]) {
        return false;
      }
    }
  }
  return true;
}

bool Uart::IsValidConfig() const {
  const auto index = static_cast<unsigned>(peripheral_);
  if (index >= kHardware.size()) {
    return false;
  }
  USART_TypeDef* const instance = kHardware[index].instance;
  if (!detail::IsValidUartInit(config_.hal, instance) ||
      (config_.gpio_pull != GPIO_NOPULL && config_.gpio_pull != GPIO_PULLUP &&
       config_.gpio_pull != GPIO_PULLDOWN) ||
      (config_.gpio_speed != GPIO_SPEED_FREQ_LOW &&
       config_.gpio_speed != GPIO_SPEED_FREQ_MEDIUM &&
       config_.gpio_speed != GPIO_SPEED_FREQ_HIGH
#if !defined(STM32F303x8)
       && config_.gpio_speed != GPIO_SPEED_FREQ_VERY_HIGH
#endif
      )) {
    return false;
  }
  return detail::IsValidUartAdvanced(config_.advanced, config_.hal, instance);
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
