#pragma once

#include <algorithm>
#include <cstdint>
#include <span>

namespace nkrc::nbed::detail {

struct Rgb {
  uint8_t red, green, blue;
};

// At 800 kHz: T0H ~= 0.35 us, T1H ~= 0.70 us. Include 300 us low and
// two additional zero entries for the PWM preload pipeline.
inline constexpr size_t kWs2812ResetSlots = 242;

inline bool EncodeWs2812b(std::span<const Rgb> colors, uint64_t period,
                          std::span<uint32_t> waveform) {
  if (period < 4 || period > 0xFFFFFFFFULL ||
      waveform.size() < kWs2812ResetSlots ||
      colors.size() > (waveform.size() - kWs2812ResetSlots) / 24 ||
      waveform.size() != colors.size() * 24 + kWs2812ResetSlots) {
    return false;
  }
  const auto zero = static_cast<uint32_t>((period * 28 + 50) / 100);
  const auto one = static_cast<uint32_t>((period * 56 + 50) / 100);
  size_t offset = 0;
  for (const Rgb color : colors) {
    const uint32_t grb =
        (uint32_t{color.green} << 16) | (uint32_t{color.red} << 8) | color.blue;
    for (int bit = 23; bit >= 0; --bit) {
      waveform[offset++] = (grb & (1UL << bit)) != 0 ? one : zero;
    }
  }
  std::fill(waveform.begin() + offset, waveform.end(), 0U);
  return true;
}

}  // namespace nkrc::nbed::detail
