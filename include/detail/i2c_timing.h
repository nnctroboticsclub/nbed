#pragma once

#include <algorithm>
#include <cstdint>
#include <optional>

namespace nkrc::nbed::detail {

// STM32F3 I2C v2, 8 MHz HSI, analog filter enabled, digital filter off.
// RM0316 I2C timing equations. Include the minimum two-cycle synchronizer
// delay on each edge and the minimum 50 ns analog filter delay. Board rise
// and fall times extend the period, so the requested rate is an upper bound
// at the nominal HSI frequency. For fast mode, assume rise <= 100 ns and
// fall <= 300 ns. SDADEL=0 fits both modes' data-valid windows and avoids
// excessive data hold at the larger prescalers used for low bitrates.
constexpr std::optional<uint32_t> I2cTimingHsi8Mhz(uint32_t frequency_hz) {
  if (frequency_hz == 0 || frequency_hz > 400'000) {
    return std::nullopt;
  }
  const bool standard = frequency_hz <= 100'000;
  const uint64_t period_ns =
      (1'000'000'000ULL + frequency_hz - 1) / frequency_hz;
  for (uint32_t divider = 1; divider <= 16; ++divider) {
    const uint32_t tick_ns = divider * 125;
    const auto ceil_ticks = [tick_ns](uint64_t ns) {
      return static_cast<uint32_t>((ns + tick_ns - 1) / tick_ns);
    };
    const uint32_t total = ceil_ticks(period_ns - 600);
    const uint32_t high =
        std::max(ceil_ticks(standard ? 3'700 : 300), total / 2);
    const uint32_t low =
        std::max(ceil_ticks(standard ? 4'400 : 1'000), total - high);
    const uint32_t setup = ceil_ticks(standard ? 1'250 : 500);
    const uint32_t hold = 0;
    if (high >= 1 && high <= 256 && low >= 1 && low <= 256 && setup >= 1 &&
        setup <= 16 && hold <= 15) {
      return ((divider - 1) << 28) | ((setup - 1) << 20) | (hold << 16) |
             ((high - 1) << 8) | (low - 1);
    }
  }
  return std::nullopt;
}

// F7 I2C and F446 FMPI2C with a fixed 16 MHz HSI kernel clock. Use half-nanoseconds
// to represent its 62.5 ns period exactly. Analog filter: 50..260 ns; DNF=0.
// Standard-mode rise <= 1000 ns, fast-mode rise <= 100 ns, fall <= 300 ns.
constexpr std::optional<uint32_t> I2cTimingHsi16Mhz(uint32_t frequency_hz) {
  if (frequency_hz == 0 || frequency_hz > 400'000) {
    return std::nullopt;
  }
  const bool standard = frequency_hz <= 100'000;
  const uint64_t period = (2'000'000'000ULL + frequency_hz - 1) / frequency_hz;
  constexpr uint32_t kEdgeDelay = 350;  // 2 HSI cycles + 50 ns, in 0.5 ns.
  for (uint32_t divider = 1; divider <= 16; ++divider) {
    const uint32_t tick = divider * 125;
    const auto ceil_ticks = [tick](uint64_t half_ns) {
      return static_cast<uint32_t>((half_ns + tick - 1) / tick);
    };
    const uint32_t total = ceil_ticks(period - 2 * kEdgeDelay);
    const uint32_t high = std::max(
        ceil_ticks((standard ? 8'000 : 1'200) - kEdgeDelay), total / 2);
    const uint32_t low = std::max(
        ceil_ticks((standard ? 9'400 : 2'600) - kEdgeDelay), total - high);
    const uint32_t setup = ceil_ticks(standard ? 2'500 : 1'000);
    // RM0410: fall(300 ns) - AFmin(50 ns) - 3*HSI(62.5 ns)
    // leaves 62.5 ns of required SDA hold, unlike the F3's 8 MHz clock.
    const uint32_t hold = ceil_ticks(125);
    const uint32_t hold_max = standard ? 3'880 : 580;
    if (high >= 1 && high <= 256 && low >= 1 && low <= 256 && setup >= 1 &&
        setup <= 16 && hold <= 15 && hold * tick <= hold_max) {
      return ((divider - 1) << 28) | ((setup - 1) << 20) | (hold << 16) |
             ((high - 1) << 8) | (low - 1);
    }
  }
  return std::nullopt;
}

}  // namespace nkrc::nbed::detail
