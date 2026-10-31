#pragma once

#include <cstdint>

#if defined(STM32F303x8)
#include "stm32f3xx_hal.h"
#include "stm32f3xx_hal_uart.h"
#elif defined(STM32F446xx)
#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_uart.h"
#elif defined(STM32F767xx)
#include "stm32f7xx_hal.h"
#include "stm32f7xx_hal_uart.h"
#endif

namespace nkrc::nbed::detail {

inline UART_InitTypeDef DefaultUartInit(uint32_t baud_rate = 921600) {
  UART_InitTypeDef init{};
  init.BaudRate = baud_rate;
  init.WordLength = UART_WORDLENGTH_8B;
  init.StopBits = UART_STOPBITS_1;
  init.Parity = UART_PARITY_NONE;
  init.Mode = UART_MODE_TX_RX;
  init.HwFlowCtl = UART_HWCONTROL_NONE;
  init.OverSampling = UART_OVERSAMPLING_16;
#if !defined(STM32F446xx)
  init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
#endif
  return init;
}

inline bool IsUartWordTransfer(const UART_InitTypeDef& init) {
  return init.WordLength == UART_WORDLENGTH_9B &&
         init.Parity == UART_PARITY_NONE;
}

inline bool IsValidUartInit(const UART_InitTypeDef& init,
                            USART_TypeDef* instance) {
  const bool word_length = init.WordLength == UART_WORDLENGTH_8B ||
                           init.WordLength == UART_WORDLENGTH_9B
#if defined(UART_WORDLENGTH_7B)
                           || init.WordLength == UART_WORDLENGTH_7B
#endif
      ;
  const bool stop_bits = init.StopBits == UART_STOPBITS_1 ||
                         init.StopBits == UART_STOPBITS_2
#if defined(UART_STOPBITS_0_5)
                         || init.StopBits == UART_STOPBITS_0_5 ||
                         init.StopBits == UART_STOPBITS_1_5
#endif
      ;
  const bool parity = init.Parity == UART_PARITY_NONE ||
                      init.Parity == UART_PARITY_EVEN ||
                      init.Parity == UART_PARITY_ODD;
  const bool mode = init.Mode == UART_MODE_TX || init.Mode == UART_MODE_RX ||
                    init.Mode == UART_MODE_TX_RX;
  const bool flow = init.HwFlowCtl == UART_HWCONTROL_NONE ||
                    init.HwFlowCtl == UART_HWCONTROL_CTS ||
                    init.HwFlowCtl == UART_HWCONTROL_RTS ||
                    init.HwFlowCtl == UART_HWCONTROL_RTS_CTS;
  if (init.BaudRate == 0 || !IS_UART_BAUDRATE(init.BaudRate) ||
      !word_length || !stop_bits || !parity || !mode ||
      !flow || (init.OverSampling != UART_OVERSAMPLING_8 &&
                init.OverSampling != UART_OVERSAMPLING_16) ||
      (init.HwFlowCtl != UART_HWCONTROL_NONE &&
       !IS_UART_HWFLOW_INSTANCE(instance))) {
    return false;
  }
#if !defined(STM32F446xx)
  if (init.OneBitSampling != UART_ONE_BIT_SAMPLE_DISABLE &&
      init.OneBitSampling != UART_ONE_BIT_SAMPLE_ENABLE) {
    return false;
  }
#endif
  return true;
}

#if !defined(STM32F446xx)
inline bool IsValidUartAdvanced(const UART_AdvFeatureInitTypeDef& init,
                                const UART_InitTypeDef& basic,
                                USART_TypeDef* instance) {
  constexpr uint32_t kFlags = UART_ADVFEATURE_TXINVERT_INIT |
      UART_ADVFEATURE_RXINVERT_INIT | UART_ADVFEATURE_DATAINVERT_INIT |
      UART_ADVFEATURE_SWAP_INIT | UART_ADVFEATURE_RXOVERRUNDISABLE_INIT |
      UART_ADVFEATURE_DMADISABLEONERROR_INIT | UART_ADVFEATURE_AUTOBAUDRATE_INIT |
      UART_ADVFEATURE_MSBFIRST_INIT;
  if ((init.AdvFeatureInit & ~kFlags) != 0) {
    return false;
  }
  const auto valid = [&](uint32_t flag, uint32_t value, uint32_t disabled,
                         uint32_t enabled) {
    // Reject non-default values whose HAL initialization flag was omitted.
    return (init.AdvFeatureInit & flag) != 0
               ? value == disabled || value == enabled
               : value == disabled;
  };
  if (!valid(UART_ADVFEATURE_TXINVERT_INIT, init.TxPinLevelInvert,
             UART_ADVFEATURE_TXINV_DISABLE, UART_ADVFEATURE_TXINV_ENABLE) ||
      !valid(UART_ADVFEATURE_RXINVERT_INIT, init.RxPinLevelInvert,
             UART_ADVFEATURE_RXINV_DISABLE, UART_ADVFEATURE_RXINV_ENABLE) ||
      !valid(UART_ADVFEATURE_DATAINVERT_INIT, init.DataInvert,
             UART_ADVFEATURE_DATAINV_DISABLE, UART_ADVFEATURE_DATAINV_ENABLE) ||
      !valid(UART_ADVFEATURE_SWAP_INIT, init.Swap,
             UART_ADVFEATURE_SWAP_DISABLE, UART_ADVFEATURE_SWAP_ENABLE) ||
      !valid(UART_ADVFEATURE_RXOVERRUNDISABLE_INIT, init.OverrunDisable,
             UART_ADVFEATURE_OVERRUN_ENABLE, UART_ADVFEATURE_OVERRUN_DISABLE) ||
      !valid(UART_ADVFEATURE_DMADISABLEONERROR_INIT, init.DMADisableonRxError,
             UART_ADVFEATURE_DMA_ENABLEONRXERROR,
             UART_ADVFEATURE_DMA_DISABLEONRXERROR) ||
      !valid(UART_ADVFEATURE_AUTOBAUDRATE_INIT, init.AutoBaudRateEnable,
             UART_ADVFEATURE_AUTOBAUDRATE_DISABLE,
             UART_ADVFEATURE_AUTOBAUDRATE_ENABLE) ||
      !valid(UART_ADVFEATURE_MSBFIRST_INIT, init.MSBFirst,
             UART_ADVFEATURE_MSBFIRST_DISABLE,
             UART_ADVFEATURE_MSBFIRST_ENABLE)) {
    return false;
  }
  const bool auto_baud = (init.AdvFeatureInit &
                          UART_ADVFEATURE_AUTOBAUDRATE_INIT) != 0 &&
                         init.AutoBaudRateEnable ==
                             UART_ADVFEATURE_AUTOBAUDRATE_ENABLE;
  if ((init.AdvFeatureInit & UART_ADVFEATURE_AUTOBAUDRATE_INIT) != 0 &&
#if defined(STM32F303x8)
      !IS_UART_AUTOBAUDRATE_DETECTION_INSTANCE(instance)) {
#else
      !IS_USART_AUTOBAUDRATE_DETECTION_INSTANCE(instance)) {
#endif
    return false;
  }
  if (!auto_baud) {
    return init.AutoBaudRateMode == UART_ADVFEATURE_AUTOBAUDRATE_ONSTARTBIT;
  }
  if (init.AutoBaudRateMode != UART_ADVFEATURE_AUTOBAUDRATE_ONSTARTBIT &&
       init.AutoBaudRateMode != UART_ADVFEATURE_AUTOBAUDRATE_ONFALLINGEDGE &&
       init.AutoBaudRateMode != UART_ADVFEATURE_AUTOBAUDRATE_ON0X7FFRAME &&
       init.AutoBaudRateMode != UART_ADVFEATURE_AUTOBAUDRATE_ON0X55FRAME) {
    return false;
  }
#if defined(UART_WORDLENGTH_7B)
  if (basic.WordLength == UART_WORDLENGTH_7B &&
      (init.AutoBaudRateMode == UART_ADVFEATURE_AUTOBAUDRATE_ON0X7FFRAME ||
       init.AutoBaudRateMode == UART_ADVFEATURE_AUTOBAUDRATE_ON0X55FRAME)) {
    return false;
  }
#else
  (void)basic;
#endif
  return true;
}
#endif

}  // namespace nkrc::nbed::detail
