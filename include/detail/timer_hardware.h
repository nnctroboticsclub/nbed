#pragma once

#include <cstdint>

#if defined(STM32F303x8)
#include "stm32f3xx_hal_rcc.h"
#elif defined(STM32F446xx)
#include "stm32f4xx_hal_rcc.h"
#elif defined(STM32F767xx)
#include "stm32f7xx_hal_rcc.h"
#endif

namespace nkrc::nbed::detail {

/// @brief 指定した TIM のクロックを有効にし、レジスターへのポインターを返す。
inline TIM_TypeDef* EnableTimer(uint8_t number) {
  switch (number) {
    case 1:
      __HAL_RCC_TIM1_CLK_ENABLE();
      return TIM1;
    case 2:
      __HAL_RCC_TIM2_CLK_ENABLE();
      return TIM2;
    case 3:
      __HAL_RCC_TIM3_CLK_ENABLE();
      return TIM3;
#if defined(STM32F303x8)
    case 15:
      __HAL_RCC_TIM15_CLK_ENABLE();
      return TIM15;
    case 16:
      __HAL_RCC_TIM16_CLK_ENABLE();
      return TIM16;
    case 17:
      __HAL_RCC_TIM17_CLK_ENABLE();
      return TIM17;
#else
    case 4:
      __HAL_RCC_TIM4_CLK_ENABLE();
      return TIM4;
    case 5:
      __HAL_RCC_TIM5_CLK_ENABLE();
      return TIM5;
    case 8:
      __HAL_RCC_TIM8_CLK_ENABLE();
      return TIM8;
    case 9:
      __HAL_RCC_TIM9_CLK_ENABLE();
      return TIM9;
    case 10:
      __HAL_RCC_TIM10_CLK_ENABLE();
      return TIM10;
    case 11:
      __HAL_RCC_TIM11_CLK_ENABLE();
      return TIM11;
    case 12:
      __HAL_RCC_TIM12_CLK_ENABLE();
      return TIM12;
    case 13:
      __HAL_RCC_TIM13_CLK_ENABLE();
      return TIM13;
    case 14:
      __HAL_RCC_TIM14_CLK_ENABLE();
      return TIM14;
#endif
    default:
      return nullptr;
  }
}

}  // namespace nkrc::nbed::detail
