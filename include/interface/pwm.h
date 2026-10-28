#pragma once

#include <cstdint>
#include <span>

#include "interface/dma.h"

namespace nkrc::nbed::interface {

class Pwm {
 public:
  Pwm() = default;
  virtual ~Pwm() = default;
  Pwm(const Pwm&) = delete;
  Pwm& operator=(const Pwm&) = delete;
  Pwm(Pwm&&) = delete;
  Pwm& operator=(Pwm&&) = delete;

  virtual bool Initialize() = 0;

  virtual bool SetPulseWidthUs(uint32_t pulse_width_us) = 0;
  virtual bool SetFrequencyHz(uint32_t frequency_hz) = 0;
  virtual bool SetDutyCycle(float duty_cycle) = 0;

  // Raw CCR ticks, ending with two zero entries. Buffer lives until done/abort.
  virtual bool SendDataDma(std::span<const uint32_t> data) = 0;
  [[nodiscard]] virtual DmaStatus GetDmaStatus() const = 0;
  [[nodiscard]] virtual uint64_t GetPeriodTicks() const = 0;
  virtual void StopDma() = 0;

  [[nodiscard]] virtual uint32_t GetFrequencyHz() const = 0;

  virtual void Stop() = 0;
};

}  // namespace nkrc::nbed::interface
