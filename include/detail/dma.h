#pragma once

#include <cstddef>
#include <cstdint>

#include "interface/dma.h"
#if defined(STM32F303x8)
#include "stm32f3xx_hal.h"
#elif defined(STM32F446xx)
#include "stm32f4xx_hal.h"
#elif defined(STM32F767xx)
#include "stm32f7xx_hal.h"
#endif

namespace nkrc::nbed::detail {

// Owns one hardware channel/stream, including its IRQ. The peripheral must
// stop generating requests before Release().
class Dma {
 public:
  Dma() = default;
  ~Dma();
  Dma(const Dma&) = delete;
  Dma& operator=(const Dma&) = delete;
  Dma(Dma&&) = delete;
  Dma& operator=(Dma&&) = delete;

  bool Initialize(interface::DmaRequest request, bool receive = false,
                  bool word = false, uint32_t interrupt_priority = 5);
  void Release();

  [[nodiscard]] DMA_HandleTypeDef& GetHandle() { return handle_; }

  [[nodiscard]] bool IsInitialized() const { return slot_ >= 0; }

  static void HandleInterrupt(unsigned slot);

  // SRAM only. RX with enabled F7 D-cache requires whole, exclusive 32-byte
  // cache lines. Keep buffers alive until completion or abort.
  static bool PrepareBuffer(const void* data, size_t size, bool receive);
  static void CompleteReceive(void* data, size_t size);

  [[nodiscard]] static constexpr bool IsAccessibleAddress(uintptr_t begin,
                                                          size_t size) {
#if defined(STM32F303x8)
    constexpr uintptr_t kBegin = 0x20000000U, kEnd = 0x20003000U;
#elif defined(STM32F446xx)
    constexpr uintptr_t kBegin = 0x20000000U, kEnd = 0x20020000U;
#else
    constexpr uintptr_t kBegin = 0x20020000U, kEnd = 0x20080000U;
#endif
    return size != 0 && begin >= kBegin && begin < kEnd && size <= kEnd - begin;
  }

 private:
  DMA_HandleTypeDef handle_{};
  int slot_{-1};
};

}  // namespace nkrc::nbed::detail
