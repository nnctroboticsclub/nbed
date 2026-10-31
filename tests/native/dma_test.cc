#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <vector>

#include "detail/ws2812b_encoding.h"
#include "f3_pwm_hal_stubs.h"
#ifdef STM32F303x8
#include "f3/pwm.h"
#include "f3/uart.h"
namespace device = nkrc::nbed::stm32::f3;
#elif defined(STM32F767xx)
#include "f7/pwm.h"
#include "f7/uart.h"
namespace device = nkrc::nbed::stm32::f7;
#else
#include "f4/pwm.h"
#include "f4/uart.h"
namespace device = nkrc::nbed::stm32::f4;
#endif

using namespace device;
using nkrc::nbed::interface::DmaRequest;
using nkrc::nbed::interface::DmaStatus;
using nkrc::nbed::interface::PinName;
using nkrc::nbed::interface::UartRxMessage;
using enum PinName;
using enum PwmTimer;
using enum PwmChannel;
using enum UartPeripheral;
using nkrc::nbed::detail::Dma;
using enum DmaStatus;

extern "C" {
void HAL_UART_TxCpltCallback(UART_HandleTypeDef*);
void HAL_UART_RxCpltCallback(UART_HandleTypeDef*);
void HAL_UART_ErrorCallback(UART_HandleTypeDef*);
}

#ifdef STM32F303x8
static_assert(Dma::IsAccessibleAddress(0x20000000U, 12288));
static_assert(!Dma::IsAccessibleAddress(0x20003000U, 1));
#elif defined(STM32F767xx)
static_assert(Dma::IsAccessibleAddress(0x20020000U, 393216));
static_assert(!Dma::IsAccessibleAddress(0x20000000U, 32));
#else
static_assert(Dma::IsAccessibleAddress(0x20000000U, 131072));
static_assert(!Dma::IsAccessibleAddress(0x20020000U, 1));
#endif
static_assert(!Dma::IsAccessibleAddress(0x08000000U, 32));
static_assert(!Dma::IsAccessibleAddress(0x20020000U, 0));
static_assert(!Dma::IsAccessibleAddress(0x20080000U, 1));

namespace {
void TestOwnership() {
  Dma first, second;
  primask = 1;
  assert(!first.Initialize(DmaRequest::kTim1Update, false, true, 16));
  assert(first.Initialize(DmaRequest::kTim1Update, false, true));
  assert(primask == 1);
  assert(first.GetHandle().Init.MemDataAlignment == DMA_MDATAALIGN_WORD);
  assert(!second.Initialize(DmaRequest::kTim1Update, false, true));
  assert(!first.Initialize(DmaRequest::kTim2Update));
#ifdef STM32F303x8
  assert(first.GetHandle().Instance == DMA1_Channel5);
  assert(!second.Initialize(DmaRequest::kUsart1Rx, true));
#else
  assert(first.GetHandle().Instance == DMA2_Stream5);
  assert(first.GetHandle().Init.Channel == DMA_CHANNEL_6);
#endif
  first.Release();
  assert(primask == 1);
  dma_init_status = HAL_ERROR;
  assert(!second.Initialize(DmaRequest::kTim1Update));
  dma_init_status = HAL_OK;
  assert(second.Initialize(DmaRequest::kTim1Update));
  second.Release();
  const auto count = dma_irq_count;
  Dma::HandleInterrupt(0);
  Dma::HandleInterrupt(999);
  assert(dma_irq_count == count);
  primask = 0;
  assert(!Dma::PrepareBuffer(nullptr, 10, false));
}

void TestRoutes() {
  struct Mapping {
    DmaRequest request;
    DMA_Instance* instance;
    uint32_t channel;
  };

  const std::array mappings{
#ifdef STM32F303x8
      Mapping{DmaRequest::kTim1Update, DMA1_Channel5, 0},
      Mapping{DmaRequest::kTim2Update, DMA1_Channel2, 0},
      Mapping{DmaRequest::kTim3Update, DMA1_Channel3, 0},
      Mapping{DmaRequest::kTim15Update, DMA1_Channel5, 0},
      Mapping{DmaRequest::kTim16Update, DMA1_Channel3, 0},
      Mapping{DmaRequest::kTim17Update, DMA1_Channel1, 0},
      Mapping{DmaRequest::kUsart1Tx, DMA1_Channel4, 0},
      Mapping{DmaRequest::kUsart1Rx, DMA1_Channel5, 0},
      Mapping{DmaRequest::kUsart2Tx, DMA1_Channel7, 0},
      Mapping{DmaRequest::kUsart2Rx, DMA1_Channel6, 0},
#else
      Mapping{DmaRequest::kTim1Update, DMA2_Stream5, DMA_CHANNEL_6},
      Mapping{DmaRequest::kTim2Update, DMA1_Stream1, DMA_CHANNEL_3},
      Mapping{DmaRequest::kTim3Update, DMA1_Stream2, DMA_CHANNEL_5},
      Mapping{DmaRequest::kUsart1Tx, DMA2_Stream7, DMA_CHANNEL_4},
      Mapping{DmaRequest::kUsart1Rx, DMA2_Stream2, DMA_CHANNEL_4},
      Mapping{DmaRequest::kUsart2Tx, DMA1_Stream6, DMA_CHANNEL_4},
      Mapping{DmaRequest::kUsart2Rx, DMA1_Stream5, DMA_CHANNEL_4},
      Mapping{DmaRequest::kUsart3Tx, DMA1_Stream3, DMA_CHANNEL_4},
      Mapping{DmaRequest::kUsart3Rx, DMA1_Stream1, DMA_CHANNEL_4},
      Mapping{DmaRequest::kUart4Tx, DMA1_Stream4, DMA_CHANNEL_4},
      Mapping{DmaRequest::kUart4Rx, DMA1_Stream2, DMA_CHANNEL_4},
      Mapping{DmaRequest::kUart5Tx, DMA1_Stream7, DMA_CHANNEL_4},
      Mapping{DmaRequest::kUart5Rx, DMA1_Stream0, DMA_CHANNEL_4},
      Mapping{DmaRequest::kUsart6Tx, DMA2_Stream6, DMA_CHANNEL_5},
      Mapping{DmaRequest::kUsart6Rx, DMA2_Stream1, DMA_CHANNEL_5},
#ifdef STM32F767xx
      Mapping{DmaRequest::kUart7Tx, DMA1_Stream1, DMA_CHANNEL_5},
      Mapping{DmaRequest::kUart7Rx, DMA1_Stream3, DMA_CHANNEL_5},
      Mapping{DmaRequest::kUart8Tx, DMA1_Stream0, DMA_CHANNEL_5},
      Mapping{DmaRequest::kUart8Rx, DMA1_Stream6, DMA_CHANNEL_5},
#endif
#endif
  };
  for (const auto& mapping : mappings) {
    Dma owner;
    assert(owner.Initialize(mapping.request));
    assert(owner.GetHandle().Instance == mapping.instance);
    assert(owner.GetHandle().Init.Channel == mapping.channel);
    Dma conflict;
    assert(!conflict.Initialize(mapping.request));
  }
  Dma invalid;
  assert(!invalid.Initialize(static_cast<DmaRequest>(255)));
}

void TestEncoding() {
  using namespace nkrc::nbed::detail;
  const std::array colors{Rgb{0x80, 0x01, 0xFF}};
  std::array<uint32_t, 24 + kWs2812ResetSlots> waveform{};
  assert(EncodeWs2812b(colors, 100, waveform));
  assert(waveform[0] == 28 && waveform[7] == 56);  // Green MSB first.
  assert(waveform[8] == 56 && waveform[9] == 28);  // Red.
  for (size_t i = 16; i < 24; ++i) {
    assert(waveform[i] == 56);
  }
  for (size_t i = 24; i < waveform.size(); ++i) {
    assert(waveform[i] == 0);
  }
  assert(!EncodeWs2812b(colors, 0, waveform));
  assert(!EncodeWs2812b(colors, 100, std::span{waveform}.first(25)));
  for (const uint64_t period : {80, 225, 270}) {
    assert(EncodeWs2812b(colors, period, waveform));
    const double zero_ns = waveform[0] * 1250.0 / period;
    const double one_ns = waveform[7] * 1250.0 / period;
    assert(zero_ns > 330 && zero_ns < 370 && one_ns > 680 && one_ns < 720);
  }
}

void TestPwm() {
  Pwm pwm(kTim1, kCh1, kPA8, 800'000);
  std::array<uint32_t, 7> wave{12, 40, 20, 55, 0, 0, 0};
  assert(!pwm.SendDataDma(wave));
  assert(pwm.Initialize());
  assert(pwm.GetPeriodTicks() != 0);
  assert(!pwm.SendDataDma(std::span{wave}.first(2)));
  auto invalid = wave;
  invalid.back() = 1;
  assert(!pwm.SendDataDma(invalid));
  invalid = wave;
  invalid[0] = static_cast<uint32_t>(pwm.GetPeriodTicks());
  assert(!pwm.SendDataDma(invalid));
  dma_start_status = HAL_ERROR;
  assert(!pwm.SendDataDma(wave));
  assert(pwm.GetDmaStatus() == kError);
  dma_start_status = HAL_OK;
  assert(pwm.SendDataDma(wave));
  assert(pwm.GetDmaStatus() == kBusy);
  assert(dma_length == wave.size());
  assert(dma_source ==
         static_cast<uint32_t>(reinterpret_cast<uintptr_t>(wave.data())));
  assert((TIM1->DIER & TIM_DMA_UPDATE) != 0 && (TIM1->CR1 & TIM_CR1_CEN) != 0);
  assert(!pwm.SendDataDma(wave));
  assert(!pwm.SetFrequencyHz(1000));
  assert(!pwm.SetDutyCycle(0.5F));
  assert(!pwm.SetPulseWidthUs(1));
  Pwm sibling(kTim1, kCh2, kPA9, 800'000);
  assert(!sibling.Initialize());
  // Hardware model: every update latches the previous CCR preload, then
  // DMA writes the next value. Verify the first/last meaningful pulses.
  uint32_t preload = TIM1->CCR[0];
  std::vector<uint32_t> observed{0};
  for (const uint32_t ticks : wave) {
    observed.push_back(preload);
    preload = ticks;
  }
  assert((observed == std::vector<uint32_t>{0, 0, 12, 40, 20, 55, 0, 0}));
  last_dma_handle->XferCpltCallback(last_dma_handle);
  assert(pwm.GetDmaStatus() == kComplete && TIM1->CCR[0] == 0);
  assert((TIM1->DIER & TIM_DMA_UPDATE) == 0);
  assert(pwm.SendDataDma(wave));
  last_dma_handle->XferErrorCallback(last_dma_handle);
  assert(pwm.GetDmaStatus() == kError && TIM1->CCR[0] == 0);
  assert(pwm.SendDataDma(wave));
  pwm.StopDma();
  assert(pwm.GetDmaStatus() == kAborted);
  assert(sibling.Initialize());
  assert(!pwm.SendDataDma(wave));
  sibling.Stop();
  assert(pwm.SendDataDma(wave));
  pwm.Stop();
  assert(pwm.GetDmaStatus() == kAborted);
  assert(pwm.Initialize());
  assert(pwm.SendDataDma(wave));
}

void TestUart() {
  Uart uart(kUsart2, kPA3, kPA2);
  alignas(32) std::array<uint8_t, 32> buffer{};
  assert(!uart.SendDma(buffer));
  assert(uart.Initialize());
  assert(!uart.SendDma({}));
  assert(!uart.ReceiveDma({}));
  uart_dma_tx_status = HAL_ERROR;
  assert(!uart.SendDma(buffer));
  assert(uart.GetTxDmaStatus() == kError);
  uart_dma_tx_status = HAL_OK;
  assert(uart.SendDma(buffer));
  assert(!uart.SendDma(buffer) && !uart.Send(buffer));
  assert(uart.ReceiveDma(buffer));
  assert(!uart.Receive(buffer) && !uart.EnableRxInterrupt());
  assert(!uart.ReceiveDma(buffer));
  UART_HandleTypeDef other{};
  other.Instance = uart.GetHandle().Instance;
  HAL_UART_TxCpltCallback(&other);
  HAL_UART_RxCpltCallback(&other);
  assert(uart.GetTxDmaStatus() == kBusy && uart.GetRxDmaStatus() == kBusy);
  unsigned calls = 0;
  uart.SetRxCallback([&calls](UartRxMessage) { ++calls; });
  HAL_UART_RxCpltCallback(&uart.GetHandle());
  assert(uart.GetRxDmaStatus() == kComplete && calls == 0);
  HAL_UART_TxCpltCallback(&uart.GetHandle());
  assert(uart.GetTxDmaStatus() == kComplete);
  assert(uart.SendDma(buffer));
  assert(uart.ReceiveDma(buffer));
  uart.GetHandle().ErrorCode = HAL_UART_ERROR_DMA;
  HAL_UART_ErrorCallback(&uart.GetHandle());
  assert(uart.GetTxDmaStatus() == kError && uart.GetRxDmaStatus() == kError);
  assert(uart.SendDma(buffer));
  assert(uart.ReceiveDma(buffer));
  uart.StopDma();
  assert(uart.GetTxDmaStatus() == kAborted &&
         uart.GetRxDmaStatus() == kAborted);
  assert(uart.GetHandle().hdmatx == nullptr &&
         uart.GetHandle().hdmarx == nullptr);
  uart_dma_rx_status = HAL_ERROR;
  assert(!uart.ReceiveDma(buffer));
  uart_dma_rx_status = HAL_OK;
  assert(uart.EnableRxInterrupt());
  assert(!uart.ReceiveDma(buffer));
}

void TestRxCompletionLifetime() {
  Uart uart(kUsart2, kPA3, kPA2);
  alignas(32) std::array<uint8_t, 32> buffer{};
#ifdef STM32F767xx
  SCB->CCR |= SCB_CCR_DC_Msk;
#endif
  assert(uart.Initialize());
  assert(uart.ReceiveDma(buffer));
  HAL_UART_RxCpltCallback(&uart.GetHandle());
  assert(uart.GetRxDmaStatus() == kComplete);
  const auto invalidations = cache_invalidate_count;
  const auto aborts = uart_abort_rx_count;
  unsigned calls = 0;
  uart.SetRxCallback([&calls](UartRxMessage) { ++calls; });
  assert(uart.EnableRxInterrupt());
  uart.StopDma();
  // A completed DMA buffer may already have expired or been reused. Cleanup
  // must neither invalidate it again nor abort the subsequent IRQ reception.
  assert(cache_invalidate_count == invalidations);
  assert(uart_abort_rx_count == aborts);
  HAL_UART_RxCpltCallback(&uart.GetHandle());
  assert(calls == 1);
#ifdef STM32F767xx
  SCB->CCR = 0;
#endif
}

void TestConflictsAndCache() {
  Dma timer;
  assert(timer.Initialize(DmaRequest::kTim2Update));
#ifndef STM32F303x8
  Uart uart(kUsart3,
#ifdef STM32F767xx
            kPD9, kPD8
#else
            kPC5, kPB10
#endif
  );
  std::array<uint8_t, 32> buffer{};
  assert(uart.Initialize());
  assert(!uart.ReceiveDma(buffer));  // Same DMA1 stream 1.
  timer.Release();
  assert(uart.ReceiveDma(buffer));
#endif
#ifdef STM32F767xx
  alignas(32) std::array<uint8_t, 64> cache_buffer{};
  SCB->CCR |= SCB_CCR_DC_Msk;
  assert(!Dma::PrepareBuffer(cache_buffer.data() + 1, 32, true));
  assert(!Dma::PrepareBuffer(cache_buffer.data(), 31, true));
  assert(Dma::PrepareBuffer(cache_buffer.data(), 32, true));
  assert(cache_clean_invalidate_count > 0);
  Dma::CompleteReceive(cache_buffer.data(), 32);
  assert(cache_invalidate_count > 0);
  assert(Dma::PrepareBuffer(cache_buffer.data() + 1, 31, false));
  assert(cache_clean_count > 0);
  SCB->CCR = 0;
#endif
}
}  // namespace

int main() {
  TestOwnership();
  TestRoutes();
  TestEncoding();
  TestPwm();
  TestUart();
  TestRxCompletionLifetime();
  TestConflictsAndCache();
  for (const auto* handle : initialized_dma) {
    assert(handle == nullptr);
  }
}
