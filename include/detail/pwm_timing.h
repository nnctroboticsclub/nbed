#pragma once

#include <cstdint>
#include <optional>

namespace nkrc::nbed::detail {

struct Timing {
  uint32_t prescaler;  // PSCレジスタ値（分周比 - 1）
  uint32_t period;     // ARRレジスタ値
  uint32_t actual_hz;
};

constexpr std::optional<Timing> ComputePwmTiming(uint32_t clock_hz,
                                                 uint32_t frequency_hz,
                                                 uint32_t max_period) {
  if (frequency_hz == 0 || max_period == 0) {
    return std::nullopt;
  }
  const uint64_t ticks_per_period = clock_hz / frequency_hz;
  if (ticks_per_period == 0) {
    return std::nullopt;
  }
  // PWM mode 1 needs CCR > ARR to hold the active level at 100% duty.
  const uint64_t counter_range = max_period;
  const uint64_t divider =
      (ticks_per_period + counter_range - 1ULL) / counter_range;  // ceil
  if (divider == 0 || divider > 0x10000ULL) {
    return std::nullopt;
  }
  const uint64_t counter_ticks = ticks_per_period / divider;
  if (counter_ticks == 0 || counter_ticks > counter_range) {
    return std::nullopt;
  }
  return Timing{
      .prescaler = static_cast<uint32_t>(divider - 1ULL),
      .period = static_cast<uint32_t>(counter_ticks - 1ULL),
      .actual_hz = static_cast<uint32_t>(clock_hz / (divider * counter_ticks)),
  };
}

// A 32-bit timer can have a compare value above LONG_MAX. Avoid lround(),
// whose signed 32-bit return type cannot represent those valid values.
constexpr uint64_t PwmDutyTicks(float duty_cycle, uint32_t period) {
  return static_cast<uint64_t>(static_cast<double>(duty_cycle) *
                                   (static_cast<double>(period) + 1.0) +
                               0.5);
}

}  // namespace nkrc::nbed::detail
