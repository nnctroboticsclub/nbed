#pragma once
// IWYU pragma: private, include "nbed.h"

#include <cstdint>
#include <span>

#include "detail/dma.h"
#include "interface/pin_names.h"
#include "interface/pwm.h"
#include "stm32f7xx_hal.h"  // IWYU pragma: export

namespace nkrc::nbed::stm32::f7 {

enum class PwmTimer : uint8_t { kTim1, kTim2, kTim3 };
enum class PwmChannel : uint8_t { kCh1, kCh2, kCh3, kCh4, kCh1N, kCh2N, kCh3N };

// A PWM output. Channels on the same timer necessarily share a frequency.
class Pwm final : public interface::Pwm {
 public:
  // Internal shared state; exposed only because the F7 translation unit owns
  // one state object per hardware timer.
  struct TimerState;

  struct Configuration {
    // For a CHx + CHxN pair, keep one owner of their shared compare register.
    interface::PinName complementary_pin{interface::PinName::kNC};
    uint32_t polarity{TIM_OCPOLARITY_HIGH};
    uint32_t complementary_polarity{TIM_OCNPOLARITY_HIGH};
    uint32_t idle_state{TIM_OCIDLESTATE_RESET};
    uint32_t complementary_idle_state{TIM_OCNIDLESTATE_RESET};
    // HAL DeadTime / DTG encoding, 0..255 (not a duration in nanoseconds).
    // All channels on one timer must specify the same value.
    uint32_t dead_time{0};
    uint32_t off_state_run_mode{TIM_OSSR_DISABLE};
    uint32_t off_state_idle_mode{TIM_OSSI_DISABLE};
  };

  Pwm(PwmTimer timer, PwmChannel channel, interface::PinName pin,
      uint32_t frequency_hz = 50);
  Pwm(PwmTimer timer, PwmChannel channel, interface::PinName pin,
      uint32_t frequency_hz, const Configuration& configuration);
  ~Pwm() override;
  Pwm(const Pwm&) = delete;
  Pwm& operator=(const Pwm&) = delete;
  Pwm(Pwm&&) = delete;
  Pwm& operator=(Pwm&&) = delete;

  bool Initialize() override;

  bool SetPulseWidthUs(uint32_t pulse_width_us) override;
  bool SetFrequencyHz(uint32_t frequency_hz) override;
  bool SetDutyCycle(float duty_cycle) override;

  [[nodiscard]] uint32_t GetFrequencyHz() const override;

  bool SendDataDma(std::span<const uint32_t> data) override;
  [[nodiscard]] interface::DmaStatus GetDmaStatus() const override;
  [[nodiscard]] uint64_t GetPeriodTicks() const override;
  void StopDma() override;

  void Stop() override;

 private:
  [[nodiscard]] TimerState& State() const;
  [[nodiscard]] uint32_t HalChannel() const;
  [[nodiscard]] bool IsValidPin() const;
  [[nodiscard]] bool IsValidConfiguration() const;
  [[nodiscard]] bool IsValidComplementaryPin(interface::PinName pin) const;
  [[nodiscard]] uint32_t ComplementaryAlternateFunction() const;
  [[nodiscard]] uint32_t AlternateFunction() const;
  [[nodiscard]] uint32_t TimerClockHz() const;

  static void OnDmaComplete(DMA_HandleTypeDef* handle);
  static void OnDmaError(DMA_HandleTypeDef* handle);
  void FinishDma(interface::DmaStatus status);
  detail::Dma dma_;
  volatile interface::DmaStatus dma_status_{interface::DmaStatus::kIdle};

  PwmTimer timer_;
  PwmChannel channel_;
  interface::PinName pin_;
  uint32_t frequency_hz_;
  Configuration configuration_{};
  bool initialized_{false};
};

}  // namespace nkrc::nbed::stm32::f7
