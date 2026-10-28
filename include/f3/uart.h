#pragma once
// IWYU pragma: private, include "nbed.h"

#include <cstdint>

#include "detail/dma.h"
#include "interface/pin_names.h"
#include "interface/uart.h"
#include "stm32f3xx_hal.h"  // IWYU pragma: export
#include "stm32f3xx_hal_uart.h"

namespace nkrc::nbed::stm32::f3 {

enum class UartPeripheral : uint8_t { kUsart1, kUsart2 };

class Uart final : public interface::Uart {
 public:
  Uart(UartPeripheral peripheral, interface::PinName rx, interface::PinName tx,
       uint32_t baud_rate = 921600, uint32_t interrupt_priority = 0);
  ~Uart() override;
  Uart(const Uart&) = delete;
  Uart& operator=(const Uart&) = delete;
  Uart(Uart&&) = delete;
  Uart& operator=(Uart&&) = delete;

  bool Initialize() override;

  bool Send(interface::UartTxMessage message) override;
  bool Receive(interface::UartRxMessage message) override;

  bool SendDma(interface::UartTxMessage message) override;
  bool ReceiveDma(interface::UartRxMessage message) override;
  [[nodiscard]] interface::DmaStatus GetTxDmaStatus() const override;
  [[nodiscard]] interface::DmaStatus GetRxDmaStatus() const override;
  void StopDma() override;

  bool EnableRxInterrupt() override;
  void SetRxCallback(const interface::UartRxCallback& callback) override;
  void SetConsole() override;

  [[nodiscard]] UART_HandleTypeDef& GetHandle();

  static void HandleInterrupt(UartPeripheral peripheral);
  static void HandleRxComplete(UART_HandleTypeDef* handle);
  static void HandleTxComplete(UART_HandleTypeDef* handle);
  static void HandleError(UART_HandleTypeDef* handle);

 private:
  [[nodiscard]] bool IsValidPinPair() const;
  [[nodiscard]] IRQn_Type Irq() const;
  static Uart*& InstanceFor(UartPeripheral peripheral);
  void OnRxComplete();

  static Uart* FindHandle(UART_HandleTypeDef* handle);
  [[nodiscard]] interface::DmaRequest DmaRequestFor(bool receive) const;
  detail::Dma tx_dma_;
  detail::Dma rx_dma_;
  volatile interface::DmaStatus tx_dma_status_{interface::DmaStatus::kIdle};
  volatile interface::DmaStatus rx_dma_status_{interface::DmaStatus::kIdle};
  interface::UartRxMessage rx_dma_buffer_{};
  bool rx_interrupt_enabled_{false};

  UartPeripheral peripheral_;
  interface::PinName rx_;
  interface::PinName tx_;
  uint32_t baud_rate_;
  uint32_t interrupt_priority_;
  UART_HandleTypeDef handle_{};
  interface::UartRxCallback rx_callback_;
  uint8_t rx_byte_{0};
  bool initialized_{false};
};

}  // namespace nkrc::nbed::stm32::f3
