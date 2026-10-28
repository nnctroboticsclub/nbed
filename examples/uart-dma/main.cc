#include <array>

#include "nbed.h"

namespace {
alignas(32) std::array<uint8_t, 32> buffer{};
}

// USART2: PA3 RX, PA2 TX. Receive exactly 32 bytes, then echo by DMA.
// NBED_NO_DEFAULT_SERIAL prevents the console from owning this UART.
int main() {
  System::Initialize();
  Uart uart(kUsart2, kPA3, kPA2, 115200);
  if (!uart.Initialize()) {
    System::ErrorHandler();
  }
  while (true) {
    if (!uart.ReceiveDma(buffer)) {
      System::ErrorHandler();
    }
    const uint32_t start = HAL_GetTick();
    while (uart.GetRxDmaStatus() == DmaStatus::kBusy) {
      if (HAL_GetTick() - start > 10'000) {
        break;
      }
    }
    if (uart.GetRxDmaStatus() == DmaStatus::kBusy) {
      uart.StopDma();
      continue;
    }
    if (uart.GetRxDmaStatus() != DmaStatus::kComplete ||
        !uart.SendDma(buffer)) {
      System::ErrorHandler();
    }
    const uint32_t tx_start = HAL_GetTick();
    while (uart.GetTxDmaStatus() == DmaStatus::kBusy) {
      if (HAL_GetTick() - tx_start > 100) {
        uart.StopDma();
        System::ErrorHandler();
      }
    }
    if (uart.GetTxDmaStatus() != DmaStatus::kComplete) {
      System::ErrorHandler();
    }
  }
}
