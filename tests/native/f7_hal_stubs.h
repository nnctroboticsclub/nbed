#pragma once

#include <array>

#include "hal_stubs.h"

inline constexpr uint32_t HSE_VALUE = 8'000'000;
inline constexpr uint32_t RCC_SYSCLK_DIV1 = 1, RCC_SYSCLK_DIV2 = 2,
                          RCC_SYSCLK_DIV4 = 4, RCC_SYSCLK_DIV8 = 8,
                          RCC_SYSCLK_DIV16 = 16, RCC_SYSCLK_DIV64 = 64,
                          RCC_SYSCLK_DIV128 = 128, RCC_SYSCLK_DIV256 = 256,
                          RCC_SYSCLK_DIV512 = 512;
inline constexpr uint32_t RCC_HCLK_DIV1 = 1, RCC_HCLK_DIV2 = 2,
                          RCC_HCLK_DIV4 = 4, RCC_HCLK_DIV8 = 8,
                          RCC_HCLK_DIV16 = 16;
inline constexpr uint32_t FLASH_LATENCY_7 = 7;
inline constexpr uint32_t PWR_REGULATOR_VOLTAGE_SCALE1 = 1;
inline constexpr uint32_t RCC_OSCILLATORTYPE_HSE = 1,
                          RCC_OSCILLATORTYPE_HSI = 2, RCC_HSE_OFF = 0,
                          RCC_HSE_BYPASS = 1, RCC_HSE_ON = 2, RCC_HSI_ON = 1,
                          RCC_HSICALIBRATION_DEFAULT = 16, RCC_PLL_ON = 1,
                          RCC_PLLSOURCE_HSE = 1, RCC_PLLSOURCE_HSI = 2,
                          RCC_PLLP_DIV2 = 2, RCC_PLLP_DIV4 = 4,
                          RCC_PLLP_DIV6 = 6, RCC_PLLP_DIV8 = 8,
                          RCC_CLOCKTYPE_HCLK = 1, RCC_CLOCKTYPE_SYSCLK = 2,
                          RCC_CLOCKTYPE_PCLK1 = 4, RCC_CLOCKTYPE_PCLK2 = 8,
                          RCC_SYSCLKSOURCE_PLLCLK = 2;
#define IS_RCC_PLLM_VALUE(m) ((m) >= 2 && (m) <= 63)
#define IS_RCC_PLLP_VALUE(p) ((p) == 2 || (p) == 4 || (p) == 6 || (p) == 8)
#define IS_RCC_PLLQ_VALUE(q) ((q) >= 2 && (q) <= 15)
#define __HAL_RCC_PWR_CLK_ENABLE() ((void)0)
#define __HAL_PWR_VOLTAGESCALING_CONFIG(scale) ((void)(scale))
inline constexpr uint32_t RCC_TIMPRES_DESACTIVATED = 0;
inline uint32_t timer_prescaler = 1;
#define __HAL_RCC_TIMCLKPRESCALER(value) (timer_prescaler = (value))

struct RCC_OscInitTypeDef {
  uint32_t OscillatorType{}, HSEState{}, HSIState{}, HSICalibrationValue{};

  struct {
    uint32_t PLLState{}, PLLSource{}, PLLM{}, PLLN{}, PLLP{}, PLLQ{}, PLLR{};
  } PLL;
};

struct RCC_ClkInitTypeDef {
  uint32_t ClockType{}, SYSCLKSource{}, AHBCLKDivider{}, APB1CLKDivider{},
      APB2CLKDivider{};
};

inline RCC_OscInitTypeDef last_osc;
inline RCC_ClkInitTypeDef last_clock;
inline unsigned osc_calls = 0, overdrive_calls = 0;
inline uint32_t last_flash_latency = 0, sysclk_hz = 16'000'000;
inline HAL_StatusTypeDef osc_status = HAL_OK, overdrive_status = HAL_OK,
                         clock_status = HAL_OK;
inline std::vector<uint32_t> delay_calls;

inline HAL_StatusTypeDef HAL_RCC_OscConfig(RCC_OscInitTypeDef* config) {
  ++osc_calls;
  last_osc = *config;
  return osc_status;
}

inline HAL_StatusTypeDef HAL_PWREx_EnableOverDrive() {
  ++overdrive_calls;
  return overdrive_status;
}

inline HAL_StatusTypeDef HAL_RCC_ClockConfig(RCC_ClkInitTypeDef* config,
                                             uint32_t latency) {
  last_clock = *config;
  last_flash_latency = latency;
  if (clock_status == HAL_OK) {
    const uint32_t source =
        last_osc.PLL.PLLSource == RCC_PLLSOURCE_HSE ? HSE_VALUE : 16'000'000;
    sysclk_hz = uint64_t{source} * last_osc.PLL.PLLN / last_osc.PLL.PLLM /
                last_osc.PLL.PLLP;
    SystemCoreClock = sysclk_hz / config->AHBCLKDivider;
    pclk1_hz = SystemCoreClock / config->APB1CLKDivider;
    pclk2_hz = SystemCoreClock / config->APB2CLKDivider;
  }
  return clock_status;
}

inline uint32_t HAL_RCC_GetSysClockFreq() { return sysclk_hz; }

inline uint32_t HAL_RCC_GetHCLKFreq() { return SystemCoreClock; }

inline void HAL_Delay(uint32_t ms) { delay_calls.push_back(ms); }

inline void HAL_IncTick() { ++hal_tick; }
