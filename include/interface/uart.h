#pragma once

#include <cstdint>
#include <functional>
#include <span>

#include "interface/dma.h"

namespace nkrc::nbed::interface {

using UartTxMessage = std::span<const uint8_t>;
using UartRxMessage = std::span<uint8_t>;
using UartRxCallback = std::function<void(UartRxMessage)>;
using UartWordTxMessage = std::span<const uint16_t>;
using UartWordRxMessage = std::span<uint16_t>;
using UartWordRxCallback = std::function<void(UartWordRxMessage)>;

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

  // 9-bit data without parity uses one uint16_t per character.
  virtual bool SendWords(UartWordTxMessage) { return false; }
  virtual bool ReceiveWords(UartWordRxMessage) { return false; }

  virtual bool SendDma(UartTxMessage message) = 0;
  virtual bool ReceiveDma(UartRxMessage message) = 0;
  virtual bool SendWordsDma(UartWordTxMessage) { return false; }
  virtual bool ReceiveWordsDma(UartWordRxMessage) { return false; }
  [[nodiscard]] virtual DmaStatus GetTxDmaStatus() const = 0;
  [[nodiscard]] virtual DmaStatus GetRxDmaStatus() const = 0;
  virtual void StopDma() = 0;

  virtual bool EnableRxInterrupt() = 0;
  virtual void SetRxCallback(const UartRxCallback& callback) = 0;
  virtual void SetRxWordCallback(const UartWordRxCallback&) {}

  // Routes the C/C++ standard I/O console through this UART.
  virtual void SetConsole() = 0;
};

}  // namespace nkrc::nbed::interface
