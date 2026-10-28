#pragma once

#include <cstdint>
#include <functional>
#include <span>

#include "interface/dma.h"

namespace nkrc::nbed::interface {

using UartTxMessage = std::span<const uint8_t>;
using UartRxMessage = std::span<uint8_t>;
using UartRxCallback = std::function<void(UartRxMessage)>;

class Uart {
 public:
  Uart() = default;
  virtual ~Uart() = default;
  Uart(const Uart&) = delete;
  Uart& operator=(const Uart&) = delete;
  Uart(Uart&&) = delete;
  Uart& operator=(Uart&&) = delete;

  virtual bool Initialize() = 0;

  virtual bool Send(UartTxMessage message) = 0;
  virtual bool Receive(UartRxMessage message) = 0;

  virtual bool SendDma(UartTxMessage message) = 0;
  virtual bool ReceiveDma(UartRxMessage message) = 0;
  [[nodiscard]] virtual DmaStatus GetTxDmaStatus() const = 0;
  [[nodiscard]] virtual DmaStatus GetRxDmaStatus() const = 0;
  virtual void StopDma() = 0;

  virtual bool EnableRxInterrupt() = 0;
  virtual void SetRxCallback(const UartRxCallback& callback) = 0;

  // Routes the C/C++ standard I/O console through this UART.
  virtual void SetConsole() = 0;
};

}  // namespace nkrc::nbed::interface
