#include <array>
#include <cassert>
#include <memory>
#include <vector>

#include "hal_stubs.h"
#ifdef STM32F303x8
#include "f3/pin_mapper.h"
#include "f3/uart.h"
namespace device = nkrc::nbed::stm32::f3;
#elif defined(STM32F767xx)
#include "f7/pin_mapper.h"
#include "f7/uart.h"
namespace device = nkrc::nbed::stm32::f7;
#else
#include "f4/pin_mapper.h"
#include "f4/uart.h"
namespace device = nkrc::nbed::stm32::f4;
#endif

using namespace device;
using nkrc::nbed::interface::PinName;
using nkrc::nbed::interface::UartRxMessage;
using enum UartPeripheral;
using enum PinName;
using enum nkrc::nbed::interface::DmaStatus;

extern "C" {
void USART1_IRQHandler();
void USART2_IRQHandler();
#ifndef STM32F303x8
void USART3_IRQHandler();
void UART4_IRQHandler();
void UART5_IRQHandler();
void USART6_IRQHandler();
#endif
#ifdef STM32F767xx
void UART7_IRQHandler();
void UART8_IRQHandler();
#endif
void HAL_UART_RxCpltCallback(UART_HandleTypeDef*);
void HAL_UART_TxCpltCallback(UART_HandleTypeDef*);
void HAL_UART_ErrorCallback(UART_HandleTypeDef*);
int _write(int, const char*, int);
}

struct Unit {
  UartPeripheral peripheral;
  unsigned number;
  USART_TypeDef* instance;
  IRQn_Type irq;
  void (*handler)();
  DMA_Instance* tx;
  DMA_Instance* rx;
  uint32_t channel;
};

const std::array kUnits{
#ifdef STM32F303x8
    Unit{kUsart1, 1, USART1, USART1_IRQn, USART1_IRQHandler,
         DMA1_Channel4, DMA1_Channel5, 0},
    Unit{kUsart2, 2, USART2, USART2_IRQn, USART2_IRQHandler,
         DMA1_Channel7, DMA1_Channel6, 0},
#else
    Unit{kUsart1, 1, USART1, USART1_IRQn, USART1_IRQHandler,
         DMA2_Stream7, DMA2_Stream2, DMA_CHANNEL_4},
    Unit{kUsart2, 2, USART2, USART2_IRQn, USART2_IRQHandler,
         DMA1_Stream6, DMA1_Stream5, DMA_CHANNEL_4},
    Unit{kUsart3, 3, USART3, USART3_IRQn, USART3_IRQHandler,
         DMA1_Stream3, DMA1_Stream1, DMA_CHANNEL_4},
    Unit{kUart4, 4, UART4, UART4_IRQn, UART4_IRQHandler,
         DMA1_Stream4, DMA1_Stream2, DMA_CHANNEL_4},
    Unit{kUart5, 5, UART5, UART5_IRQn, UART5_IRQHandler,
         DMA1_Stream7, DMA1_Stream0, DMA_CHANNEL_4},
    Unit{kUsart6, 6, USART6, USART6_IRQn, USART6_IRQHandler,
         DMA2_Stream6, DMA2_Stream1, DMA_CHANNEL_5},
#ifdef STM32F767xx
    Unit{kUart7, 7, UART7, UART7_IRQn, UART7_IRQHandler,
         DMA1_Stream1, DMA1_Stream3, DMA_CHANNEL_5},
    Unit{kUart8, 8, UART8, UART8_IRQn, UART8_IRQHandler,
         DMA1_Stream0, DMA1_Stream6, DMA_CHANNEL_5},
#endif
#endif
};

struct PinCase {
  UartPeripheral peripheral;
  PinName pin;
  bool receive;
  uint32_t alternate;
};

#include "uart_pin_cases.h"

std::vector<PinCase> Pins(UartPeripheral peripheral, bool receive) {
  std::vector<PinCase> pins;
  for (const auto& pin : kPinCases) {
    if (pin.peripheral == peripheral && pin.receive == receive) {
      pins.push_back(pin);
    }
  }
  assert(!pins.empty());
  return pins;
}

void CheckPin(const PinCase& expected, unsigned index) {
  const auto& [port, gpio] = gpio_inits[index];
  assert(port == PinMapper::GetGpioPort(expected.pin));
  assert(gpio.Pin == PinMapper::GetGpioPin(expected.pin));
  assert(gpio.Alternate == expected.alternate);
  assert(gpio.Mode == GPIO_MODE_AF_PP && gpio.Pull == GPIO_NOPULL);
}

void TestPins(const Unit& unit) {
  const auto rx_pins = Pins(unit.peripheral, true);
  const auto tx_pins = Pins(unit.peripheral, false);
  for (const auto& rx : rx_pins) {
    for (const auto& tx : tx_pins) {
      gpio_inits.clear();
      uart_clocks.fill(false);
      uart_clock_sources.fill(0);
      {
        Uart uart(unit.peripheral, rx.pin, tx.pin, 115200, 9);
        const auto calls = uart_init_calls;
        assert(uart.Initialize() && uart.Initialize());
        assert(uart_init_calls == calls + 1);
        assert(uart.GetHandle().Instance == unit.instance);
        assert(uart.GetHandle().Init.BaudRate == 115200);
        assert(gpio_inits.size() == 2);
        CheckPin(rx, 0);
        CheckPin(tx, 1);
        for (unsigned number = 1; number <= 8; ++number) {
          assert(uart_clocks[number] == (number == unit.number));
        }
#ifdef STM32F767xx
        const auto apb = unit.number == 1 || unit.number == 6 ? 2U : 1U;
        assert(uart_clock_sources[unit.number] == apb);
#endif
        assert(irq_enabled[unit.irq] && irq_priority[unit.irq] == 9);
        Uart duplicate(unit.peripheral, rx.pin, tx.pin);
        assert(!duplicate.Initialize());
        unit.handler();
        assert(last_uart_irq == &uart.GetHandle());
        const auto irq_calls = uart_irq_calls;
        Uart::HandleInterrupt(static_cast<UartPeripheral>(255));
        assert(uart_irq_calls == irq_calls);
      }
      assert(!irq_enabled[unit.irq]);
      const auto calls = uart_irq_calls;
      unit.handler();
      assert(uart_irq_calls == calls);
    }
  }
  // Check every encoded pin in both roles, including NC and unbonded pins.
  for (unsigned value = 0; value < 256; ++value) {
    const auto pin = static_cast<PinName>(value);
    bool valid_rx = false, valid_tx = false;
    for (const auto& expected : rx_pins) {
      valid_rx |= expected.pin == pin;
    }
    for (const auto& expected : tx_pins) {
      valid_tx |= expected.pin == pin;
    }
    {
      Uart uart(unit.peripheral, pin, tx_pins.front().pin);
      assert(uart.Initialize() == valid_rx);
    }
    {
      Uart uart(unit.peripheral, rx_pins.front().pin, pin);
      assert(uart.Initialize() == valid_tx);
    }
  }
  const auto rx = rx_pins.front().pin, tx = tx_pins.front().pin;
  Uart invalid(static_cast<UartPeripheral>(255), rx, tx);
  Uart zero_baud(unit.peripheral, rx, tx, 0);
  Uart bad_priority(unit.peripheral, rx, tx, 115200, 16);
  assert(!invalid.Initialize() && !zero_baud.Initialize() &&
         !bad_priority.Initialize());
  Uart retry(unit.peripheral, rx, tx);
  uart_init_status = HAL_ERROR;
  assert(!retry.Initialize());
  uart_init_status = HAL_OK;
  Uart replacement(unit.peripheral, rx, tx);
  assert(replacement.Initialize());
}

void TestTransfers(const Unit& unit) {
  const auto rx = Pins(unit.peripheral, true).front().pin;
  const auto tx = Pins(unit.peripheral, false).front().pin;
  alignas(32) std::array<uint8_t, 32> buffer{};
  {
    Uart uart(unit.peripheral, rx, tx);
    assert(!uart.Send(buffer) && !uart.Receive(buffer));
    assert(!uart.SendDma(buffer) && !uart.ReceiveDma(buffer));
    assert(uart.Initialize());
    assert(uart.Send(buffer) && last_uart_io == &uart.GetHandle());
    assert(uart.Receive(buffer) && last_uart_io == &uart.GetHandle());
    uart.SetConsole();
    assert(_write(1, "ok", 2) == 2);
    assert(last_uart_io == &uart.GetHandle());
    assert(uart.SendDma(buffer) && uart.ReceiveDma(buffer));
    assert(uart.GetHandle().hdmatx->Instance == unit.tx);
    assert(uart.GetHandle().hdmarx->Instance == unit.rx);
    assert(uart.GetHandle().hdmatx->Init.Channel == unit.channel);
    assert(uart.GetHandle().hdmarx->Init.Channel == unit.channel);
    assert(!uart.Send(buffer) && !uart.Receive(buffer));
    assert(!uart.EnableRxInterrupt());
    auto other = uart.GetHandle();
    HAL_UART_TxCpltCallback(&other);
    HAL_UART_RxCpltCallback(&other);
    assert(uart.GetTxDmaStatus() == kBusy && uart.GetRxDmaStatus() == kBusy);
    HAL_UART_TxCpltCallback(&uart.GetHandle());
    HAL_UART_RxCpltCallback(&uart.GetHandle());
    assert(uart.GetTxDmaStatus() == kComplete);
    assert(uart.GetRxDmaStatus() == kComplete);
    assert(uart.SendDma(buffer) && uart.ReceiveDma(buffer));
    uart.GetHandle().ErrorCode = HAL_UART_ERROR_DMA;
    HAL_UART_ErrorCallback(&uart.GetHandle());
    assert(uart.GetTxDmaStatus() == kError && uart.GetRxDmaStatus() == kError);
    uart.StopDma();
    uart_dma_tx_status = HAL_ERROR;
    assert(!uart.SendDma(buffer));
    assert(uart.GetHandle().hdmatx == nullptr);
    uart_dma_tx_status = HAL_OK;
    assert(uart.SendDma(buffer) && uart.ReceiveDma(buffer));
    uart.StopDma();
    assert(uart.GetTxDmaStatus() == kAborted);
    assert(uart.GetRxDmaStatus() == kAborted);
    unsigned callbacks = 0;
    uart.SetRxCallback([&](UartRxMessage message) {
      assert(message.size() == 1 && message.front() == 0x5a);
      ++callbacks;
    });
    uart_it_status = HAL_ERROR;
    assert(!uart.EnableRxInterrupt());
    uart_it_status = HAL_OK;
    assert(uart.EnableRxInterrupt());
    assert(!uart.ReceiveDma(buffer));
    *uart_it_data = 0x5a;
    const auto calls = uart_it_calls;
    HAL_UART_RxCpltCallback(&other);
    assert(callbacks == 0);
    HAL_UART_RxCpltCallback(&uart.GetHandle());
    assert(callbacks == 1 && uart_it_calls == calls + 1);
  }
  // Destruction releases the UART and the DMA streams, including active DMA.
  {
    Uart uart(unit.peripheral, rx, tx);
    assert(uart.Initialize() && uart.SendDma(buffer) &&
           uart.ReceiveDma(buffer));
  }
  {
    Uart uart(unit.peripheral, rx, tx);
    assert(uart.Initialize() && uart.SendDma(buffer) &&
           uart.ReceiveDma(buffer));
  }
}

int main() {
#ifndef STM32F303x8
  static_assert(static_cast<unsigned>(kUsart2) == 0);
  static_assert(static_cast<unsigned>(kUsart3) == 1);
  static_assert(static_cast<unsigned>(kUart4) == 2);
#ifdef STM32F767xx
  static_assert(static_cast<unsigned>(kUsart6) == 3);
  {
    alignas(32) std::array<uint8_t, 32> buffer{};
    Uart first(kUart5, kPD2, kPC12);
    Uart second(kUart8, kPE0, kPE1);
    assert(first.Initialize() && second.Initialize());
    assert(first.ReceiveDma(buffer));
    // Channel 4/5 still share the same physical DMA1 stream 0.
    assert(!second.SendDma(buffer) && first.GetRxDmaStatus() == kBusy);
    first.StopDma();
    assert(second.SendDma(buffer));
  }
#endif
#endif
  for (const auto& unit : kUnits) {
    TestPins(unit);
    TestTransfers(unit);
  }
  // All UART owners and interrupt handlers must work independently at once.
  std::vector<std::unique_ptr<Uart>> owners;
  for (const auto& unit : kUnits) {
    auto owner = std::make_unique<Uart>(unit.peripheral,
        Pins(unit.peripheral, true).front().pin,
        Pins(unit.peripheral, false).front().pin);
    assert(owner->Initialize());
    owners.push_back(std::move(owner));
  }
  for (unsigned i = 0; i < kUnits.size(); ++i) {
    kUnits[i].handler();
    assert(last_uart_irq == &owners[i]->GetHandle());
  }
}
