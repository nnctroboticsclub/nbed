#include <array>

#include "detail/ws2812b_encoding.h"
#include "nbed.h"

// Connect PA8 -> 5 V logic level shifter -> 330 ohm -> DIN.
// Power the strip from an appropriate external 5 V supply; share GND.
// Keep these buffers in normal SRAM (the supplied F7 linker script does so).
namespace {
constexpr size_t kLedCount = 16;
std::array<nkrc::nbed::detail::Rgb, kLedCount> colors{};
alignas(32)
    std::array<uint32_t, kLedCount * 24 +
                             nkrc::nbed::detail::kWs2812ResetSlots> waveform{};
}  // namespace

int main() {
  System::Initialize();
  Pwm output(kTim1, kCh1, kPA8, 800'000);
  if (!output.Initialize() || output.GetFrequencyHz() != 800'000) {
    System::ErrorHandler();
  }
  constexpr std::array<nkrc::nbed::detail::Rgb, 3> palette{
      nkrc::nbed::detail::Rgb{16, 0, 0}, {0, 16, 0}, {0, 0, 16}};
  size_t frame = 0;
  while (true) {
    colors.fill(palette[frame++ % palette.size()]);
    if (!nkrc::nbed::detail::EncodeWs2812b(colors, output.GetPeriodTicks(),
                                           waveform) ||
        !output.SendDataDma(waveform)) {
      System::ErrorHandler();
    }
    const uint32_t start = HAL_GetTick();
    while (output.GetDmaStatus() == DmaStatus::kBusy) {
      if (HAL_GetTick() - start > 100) {
        output.StopDma();
        System::ErrorHandler();
      }
    }
    if (output.GetDmaStatus() != DmaStatus::kComplete) {
      System::ErrorHandler();
    }
    SleepFor(500ms);
  }
}
