#pragma once

#include <cstdint>
#include <functional>

#include "interface/digital_in.h"

namespace nkrc::nbed::interface {

enum class InterruptEdge : uint8_t { kRising, kFalling, kBoth };
using GpioInterruptCallback = std::function<void()>;

class InterruptIn : public DigitalIn {
 public:
  virtual void Stop() = 0;
  virtual void SetCallback(const GpioInterruptCallback& callback) = 0;
  [[nodiscard]] virtual uint32_t GetEdgeCount() const = 0;
};

}  // namespace nkrc::nbed::interface
