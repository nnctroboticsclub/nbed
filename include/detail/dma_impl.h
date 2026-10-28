// Compiled once by the selected MCU backend's dma.cc.
#include <array>
#include <limits>

#include "detail/dma.h"

namespace nkrc::nbed::detail {
namespace {

using interface::DmaRequest;

struct Route {
  DmaRequest request;
  unsigned slot;
  uint32_t channel;
};

#if defined(STM32F303x8)
// RM0316: default routing (SYSCFG DMA remapping must remain at reset).
const std::array kInstances{DMA1_Channel1, DMA1_Channel2, DMA1_Channel3,
                            DMA1_Channel4, DMA1_Channel5, DMA1_Channel6,
                            DMA1_Channel7};
constexpr std::array kIrqs{DMA1_Channel1_IRQn, DMA1_Channel2_IRQn,
                           DMA1_Channel3_IRQn, DMA1_Channel4_IRQn,
                           DMA1_Channel5_IRQn, DMA1_Channel6_IRQn,
                           DMA1_Channel7_IRQn};
constexpr std::array kRoutes{Route{DmaRequest::kTim1Update, 4, 0},
                             Route{DmaRequest::kTim2Update, 1, 0},
                             Route{DmaRequest::kTim3Update, 2, 0},
                             Route{DmaRequest::kTim15Update, 4, 0},
                             Route{DmaRequest::kTim16Update, 2, 0},
                             Route{DmaRequest::kTim17Update, 0, 0},
                             Route{DmaRequest::kUsart1Tx, 3, 0},
                             Route{DmaRequest::kUsart1Rx, 4, 0},
                             Route{DmaRequest::kUsart2Tx, 6, 0},
                             Route{DmaRequest::kUsart2Rx, 5, 0}};
#else
// RM0390 / RM0410. Use a fixed stream per request so conflicts are explicit.
const std::array kInstances{
    DMA1_Stream0, DMA1_Stream1, DMA1_Stream2, DMA1_Stream3,
    DMA1_Stream4, DMA1_Stream5, DMA1_Stream6, DMA1_Stream7,
    DMA2_Stream0, DMA2_Stream1, DMA2_Stream2, DMA2_Stream3,
    DMA2_Stream4, DMA2_Stream5, DMA2_Stream6, DMA2_Stream7};
constexpr std::array kIrqs{
    DMA1_Stream0_IRQn, DMA1_Stream1_IRQn, DMA1_Stream2_IRQn, DMA1_Stream3_IRQn,
    DMA1_Stream4_IRQn, DMA1_Stream5_IRQn, DMA1_Stream6_IRQn, DMA1_Stream7_IRQn,
    DMA2_Stream0_IRQn, DMA2_Stream1_IRQn, DMA2_Stream2_IRQn, DMA2_Stream3_IRQn,
    DMA2_Stream4_IRQn, DMA2_Stream5_IRQn, DMA2_Stream6_IRQn, DMA2_Stream7_IRQn};
constexpr std::array kRoutes{
    Route{DmaRequest::kTim1Update, 13, DMA_CHANNEL_6},
    Route{DmaRequest::kTim2Update, 1, DMA_CHANNEL_3},
    Route{DmaRequest::kTim3Update, 2, DMA_CHANNEL_5},
    Route{DmaRequest::kUsart2Tx, 6, DMA_CHANNEL_4},
    Route{DmaRequest::kUsart2Rx, 5, DMA_CHANNEL_4},
    Route{DmaRequest::kUsart3Tx, 3, DMA_CHANNEL_4},
    Route{DmaRequest::kUsart3Rx, 1, DMA_CHANNEL_4},
    Route{DmaRequest::kUart4Tx, 4, DMA_CHANNEL_4},
    Route{DmaRequest::kUart4Rx, 2, DMA_CHANNEL_4},
#if defined(STM32F767xx)
    Route{DmaRequest::kUsart6Tx, 14, DMA_CHANNEL_5},
    Route{DmaRequest::kUsart6Rx, 9, DMA_CHANNEL_5},
#endif
};
#endif

std::array<Dma*, kInstances.size()> owners{};

class InterruptLock {
 public:
  InterruptLock() : primask_(__get_PRIMASK()) { __disable_irq(); }

  ~InterruptLock() { __set_PRIMASK(primask_); }

 private:
  uint32_t primask_;
};

}  // namespace

Dma::~Dma() { Release(); }

bool Dma::Initialize(DmaRequest request, bool receive, bool word,
                     uint32_t interrupt_priority) {
  if (slot_ >= 0 || interrupt_priority > 15U) {
    return false;
  }
  for (const auto& route : kRoutes) {
    if (route.request != request) {
      continue;
    }
    {
      InterruptLock lock;
      if (owners[route.slot] != nullptr) {
        return false;
      }
      owners[route.slot] = this;
      slot_ = static_cast<int>(route.slot);
    }
    HAL_NVIC_DisableIRQ(kIrqs[route.slot]);
    __HAL_RCC_DMA1_CLK_ENABLE();
#if !defined(STM32F303x8)
    __HAL_RCC_DMA2_CLK_ENABLE();
#endif
    handle_ = {};
    handle_.Instance = kInstances[route.slot];
#if !defined(STM32F303x8)
    handle_.Init.Channel = route.channel;
    handle_.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
    handle_.Init.MemBurst = DMA_MBURST_SINGLE;
    handle_.Init.PeriphBurst = DMA_PBURST_SINGLE;
#endif
    handle_.Init.Direction =
        receive ? DMA_PERIPH_TO_MEMORY : DMA_MEMORY_TO_PERIPH;
    handle_.Init.PeriphInc = DMA_PINC_DISABLE;
    handle_.Init.MemInc = DMA_MINC_ENABLE;
    handle_.Init.PeriphDataAlignment =
        word ? DMA_PDATAALIGN_WORD : DMA_PDATAALIGN_BYTE;
    handle_.Init.MemDataAlignment =
        word ? DMA_MDATAALIGN_WORD : DMA_MDATAALIGN_BYTE;
    handle_.Init.Mode = DMA_NORMAL;
    handle_.Init.Priority = word ? DMA_PRIORITY_VERY_HIGH : DMA_PRIORITY_HIGH;
    if (HAL_DMA_Init(&handle_) != HAL_OK) {
      (void)HAL_DMA_DeInit(&handle_);
      handle_ = {};
      InterruptLock lock;
      owners[route.slot] = nullptr;
      slot_ = -1;
      return false;
    }
    HAL_NVIC_ClearPendingIRQ(kIrqs[route.slot]);
    HAL_NVIC_SetPriority(kIrqs[route.slot], interrupt_priority, 0);
    HAL_NVIC_EnableIRQ(kIrqs[route.slot]);
    return true;
  }
  return false;
}

void Dma::Release() {
  if (slot_ < 0) {
    return;
  }
  const auto slot = static_cast<unsigned>(slot_);
  HAL_NVIC_DisableIRQ(kIrqs[slot]);
  (void)HAL_DMA_Abort(&handle_);
  (void)HAL_DMA_DeInit(&handle_);
  HAL_NVIC_ClearPendingIRQ(kIrqs[slot]);
  InterruptLock lock;
  owners[slot] = nullptr;
  slot_ = -1;
  handle_ = {};
}

void Dma::HandleInterrupt(unsigned slot) {
  if (slot < owners.size() && owners[slot] != nullptr) {
    HAL_DMA_IRQHandler(&owners[slot]->handle_);
  }
}

bool Dma::PrepareBuffer(const void* data, size_t size, bool receive) {
  const auto begin = reinterpret_cast<uintptr_t>(data);
  if (data == nullptr || size == 0 ||
      size > std::numeric_limits<uintptr_t>::max() - begin ||
      size > static_cast<size_t>(std::numeric_limits<int32_t>::max() - 32)) {
    return false;
  }
#if UINTPTR_MAX == UINT32_MAX
  if (!IsAccessibleAddress(begin, size)) {
    return false;
  }
#endif
#if defined(STM32F767xx)
  if ((SCB->CCR & SCB_CCR_DC_Msk) != 0U) {
    if (receive && ((begin % 32U) != 0U || (size % 32U) != 0U)) {
      return false;
    }
    const auto aligned = begin & ~uintptr_t{31};
    const auto length =
        static_cast<int32_t>((begin + size + 31U - aligned) & ~uintptr_t{31});
    auto* address = reinterpret_cast<uint32_t*>(aligned);
    if (receive) {
      SCB_CleanInvalidateDCache_by_Addr(address, length);
    } else {
      SCB_CleanDCache_by_Addr(address, length);
    }
  }
#else
  (void)receive;
#endif
  __DMB();
  return true;
}

void Dma::CompleteReceive(void* data, size_t size) {
#if defined(STM32F767xx)
  if ((SCB->CCR & SCB_CCR_DC_Msk) != 0U && data != nullptr && size != 0) {
    SCB_InvalidateDCache_by_Addr(static_cast<uint32_t*>(data),
                                 static_cast<int32_t>(size));
  }
#else
  (void)data;
  (void)size;
#endif
  __DMB();
}

}  // namespace nkrc::nbed::detail

extern "C" {
#if defined(STM32F303x8)
#define NBED_DMA_IRQ(number)                              \
  void DMA1_Channel##number##_IRQHandler() {              \
    nkrc::nbed::detail::Dma::HandleInterrupt(number - 1); \
  }
NBED_DMA_IRQ(1)
NBED_DMA_IRQ(2)
NBED_DMA_IRQ(3)
NBED_DMA_IRQ(4)
NBED_DMA_IRQ(5)
NBED_DMA_IRQ(6)
NBED_DMA_IRQ(7)
#else
#define NBED_DMA_IRQ(controller, stream, slot)           \
  void DMA##controller##_Stream##stream##_IRQHandler() { \
    nkrc::nbed::detail::Dma::HandleInterrupt(slot);      \
  }
NBED_DMA_IRQ(1, 0, 0)
NBED_DMA_IRQ(1, 1, 1)
NBED_DMA_IRQ(1, 2, 2)
NBED_DMA_IRQ(1, 3, 3)
NBED_DMA_IRQ(1, 4, 4)
NBED_DMA_IRQ(1, 5, 5)
NBED_DMA_IRQ(1, 6, 6)
NBED_DMA_IRQ(1, 7, 7)
NBED_DMA_IRQ(2, 0, 8)
NBED_DMA_IRQ(2, 1, 9)
NBED_DMA_IRQ(2, 2, 10)
NBED_DMA_IRQ(2, 3, 11)
NBED_DMA_IRQ(2, 4, 12)
NBED_DMA_IRQ(2, 5, 13)
NBED_DMA_IRQ(2, 6, 14)
NBED_DMA_IRQ(2, 7, 15)
#endif
#undef NBED_DMA_IRQ
}
