#pragma once

#include <array>

#include "hal_stubs.h"

inline constexpr uint32_t GPIO_MODE_ANALOG = 20, GPIO_MODE_AF_OD = 21,
                          GPIO_AF4_I2C1 = 4;
inline constexpr uint32_t DISABLE = 0, ADC_CLOCK_SYNC_PCLK_DIV4 = 4,
                          ADC_RESOLUTION_12B = 12,
                          ADC_EXTERNALTRIGCONVEDGE_NONE = 0,
                          ADC_SOFTWARE_START = 0, ADC_DATAALIGN_RIGHT = 0,
                          ADC_EOC_SINGLE_CONV = 1, ADC_SAMPLETIME_15CYCLES = 15;
inline constexpr uint32_t ADC_CHANNEL_0 = 0, ADC_CHANNEL_1 = 1,
                          ADC_CHANNEL_2 = 2, ADC_CHANNEL_3 = 3,
                          ADC_CHANNEL_4 = 4, ADC_CHANNEL_5 = 5,
                          ADC_CHANNEL_6 = 6, ADC_CHANNEL_7 = 7,
                          ADC_CHANNEL_8 = 8, ADC_CHANNEL_9 = 9,
                          ADC_CHANNEL_10 = 10, ADC_CHANNEL_11 = 11,
                          ADC_CHANNEL_12 = 12, ADC_CHANNEL_13 = 13,
                          ADC_CHANNEL_14 = 14, ADC_CHANNEL_15 = 15;

struct ADC_TypeDef {};

inline ADC_TypeDef adc1, adc2, adc3;
#define ADC1 (&adc1)
#define ADC2 (&adc2)
#define ADC3 (&adc3)
#define __HAL_RCC_ADC2_CLK_ENABLE() ((void)0)
#define __HAL_RCC_ADC12_CLK_ENABLE() ((void)0)
#define __HAL_RCC_ADC1_CLK_ENABLE() ((void)0)
#define __HAL_RCC_ADC3_CLK_ENABLE() ((void)0)

inline constexpr uint32_t ADC_SCAN_DISABLE = 0, ADC_OVR_DATA_OVERWRITTEN = 0,
                          ADC_SINGLE_ENDED = 0, ADC_OFFSET_NONE = 0,
                          ADC_REGULAR_RANK_1 = 1,
                          ADC_SAMPLETIME_19CYCLES_5 = 19;

struct ADC_InitTypeDef {
  uint32_t ClockPrescaler{}, Resolution{}, ScanConvMode{}, ContinuousConvMode{},
      DiscontinuousConvMode{}, ExternalTrigConvEdge{}, ExternalTrigConv{},
      DataAlign{}, NbrOfConversion{}, DMAContinuousRequests{}, EOCSelection{},
      LowPowerAutoWait{}, Overrun{};
};

struct ADC_HandleTypeDef {
  ADC_TypeDef* Instance{};
  ADC_InitTypeDef Init{};
};

struct ADC_ChannelConfTypeDef {
  uint32_t Channel{}, Rank{}, SamplingTime{}, SingleDiff{}, OffsetNumber{};
};

inline HAL_StatusTypeDef adc_init_status = HAL_OK, adc_config_status = HAL_OK,
                         adc_poll_status = HAL_OK;
inline ADC_HandleTypeDef last_adc_init;
inline unsigned adc_stops = 0;
inline std::vector<std::pair<ADC_TypeDef*, uint32_t>> adc_channels;

inline HAL_StatusTypeDef HAL_ADC_Init(ADC_HandleTypeDef* handle) {
  last_adc_init = *handle;
  return adc_init_status;
}

inline HAL_StatusTypeDef HAL_ADC_ConfigChannel(ADC_HandleTypeDef* handle,
                                               ADC_ChannelConfTypeDef* config) {
  adc_channels.emplace_back(handle->Instance, config->Channel);
  return adc_config_status;
}

inline HAL_StatusTypeDef HAL_ADC_DeInit(ADC_HandleTypeDef*) { return HAL_OK; }

inline HAL_StatusTypeDef HAL_ADCEx_Calibration_Start(ADC_HandleTypeDef*,
                                                     uint32_t) {
  return HAL_OK;
}

inline HAL_StatusTypeDef HAL_ADC_Start(ADC_HandleTypeDef*) { return HAL_OK; }

inline HAL_StatusTypeDef HAL_ADC_PollForConversion(ADC_HandleTypeDef*,
                                                   uint32_t) {
  return adc_poll_status;
}

inline uint32_t HAL_ADC_GetValue(ADC_HandleTypeDef*) { return 2048; }

inline HAL_StatusTypeDef HAL_ADC_Stop(ADC_HandleTypeDef*) {
  ++adc_stops;
  return HAL_OK;
}

struct I2C_TypeDef {};

inline I2C_TypeDef i2c1, i2c2, i2c3, i2c4, fmpi2c1;
#define I2C1 (&i2c1)
#define I2C2 (&i2c2)
#define I2C3 (&i2c3)
#define I2C4 (&i2c4)
#define FMPI2C1 (&fmpi2c1)
inline constexpr uint32_t RCC_I2C1CLKSOURCE_HSI = 1, RCC_I2C2CLKSOURCE_HSI = 1,
                          RCC_I2C3CLKSOURCE_HSI = 1, RCC_I2C4CLKSOURCE_HSI = 1,
                          RCC_FMPI2C1CLKSOURCE_HSI = 1;
inline std::array<uint32_t, 4> i2c_clocks{};
#define __HAL_RCC_I2C1_CONFIG(source) (i2c_clocks[0] = (source))
#define __HAL_RCC_I2C2_CONFIG(source) (i2c_clocks[1] = (source))
#define __HAL_RCC_I2C3_CONFIG(source) (i2c_clocks[2] = (source))
#define __HAL_RCC_I2C4_CONFIG(source) (i2c_clocks[3] = (source))
#define __HAL_RCC_FMPI2C1_CONFIG(source) (i2c_clocks[3] = (source))
#define __HAL_RCC_I2C4_CLK_ENABLE() ((void)0)
#define __HAL_RCC_FMPI2C1_CLK_ENABLE() ((void)0)
#define __HAL_RCC_I2C1_CLK_ENABLE() ((void)0)
#define __HAL_RCC_I2C2_CLK_ENABLE() ((void)0)
#define __HAL_RCC_I2C3_CLK_ENABLE() ((void)0)
inline constexpr uint32_t I2C_OA2_NOMASK = 0, I2C_ADDRESSINGMODE_7BIT = 7,
                          I2C_DUALADDRESS_DISABLE = 0,
                          I2C_GENERALCALL_DISABLE = 0,
                          I2C_NOSTRETCH_DISABLE = 0,
                          I2C_ANALOGFILTER_ENABLE = 1, I2C_DUTYCYCLE_2 = 2;

struct I2C_InitTypeDef {
  uint32_t ClockSpeed{}, DutyCycle{}, Timing{}, OwnAddress2Masks{},
      OwnAddress1{}, AddressingMode{}, DualAddressMode{}, OwnAddress2{},
      GeneralCallMode{}, NoStretchMode{};
};

struct I2C_HandleTypeDef {
  I2C_TypeDef* Instance{};
  I2C_InitTypeDef Init{};
};

inline I2C_HandleTypeDef last_i2c_init;
inline HAL_StatusTypeDef i2c_init_status = HAL_OK, i2c_filter_status = HAL_OK,
                         i2c_transfer_status = HAL_OK;
inline unsigned i2c_deinits = 0, i2c_transfers = 0;
inline uint16_t last_i2c_address = 0;

inline HAL_StatusTypeDef HAL_I2C_Init(I2C_HandleTypeDef* handle) {
  last_i2c_init = *handle;
  return i2c_init_status;
}

inline HAL_StatusTypeDef HAL_I2C_DeInit(I2C_HandleTypeDef*) {
  ++i2c_deinits;
  return HAL_OK;
}

inline HAL_StatusTypeDef HAL_I2CEx_ConfigAnalogFilter(I2C_HandleTypeDef*,
                                                      uint32_t) {
  return i2c_filter_status;
}

inline HAL_StatusTypeDef HAL_I2CEx_ConfigDigitalFilter(I2C_HandleTypeDef*,
                                                       uint32_t) {
  return i2c_filter_status;
}

inline HAL_StatusTypeDef HAL_I2C_Master_Transmit(I2C_HandleTypeDef*,
                                                 uint16_t address, uint8_t*,
                                                 uint16_t, uint32_t) {
  ++i2c_transfers;
  last_i2c_address = address;
  return i2c_transfer_status;
}

inline HAL_StatusTypeDef HAL_I2C_Master_Receive(I2C_HandleTypeDef* handle,
                                                uint16_t address, uint8_t* data,
                                                uint16_t size,
                                                uint32_t timeout) {
  return HAL_I2C_Master_Transmit(handle, address, data, size, timeout);
}

struct FMPI2C_HandleTypeDef : I2C_HandleTypeDef {};

inline constexpr uint32_t FMPI2C_ADDRESSINGMODE_7BIT = 7,
                          FMPI2C_DUALADDRESS_DISABLE = 0, FMPI2C_OA2_NOMASK = 0,
                          FMPI2C_GENERALCALL_DISABLE = 0,
                          FMPI2C_NOSTRETCH_DISABLE = 0,
                          FMPI2C_ANALOGFILTER_ENABLE = 1;

inline HAL_StatusTypeDef HAL_FMPI2C_Init(FMPI2C_HandleTypeDef* handle) {
  return HAL_I2C_Init(handle);
}

inline HAL_StatusTypeDef HAL_FMPI2C_DeInit(FMPI2C_HandleTypeDef* handle) {
  return HAL_I2C_DeInit(handle);
}

inline HAL_StatusTypeDef HAL_FMPI2CEx_ConfigAnalogFilter(
    FMPI2C_HandleTypeDef* handle, uint32_t value) {
  return HAL_I2CEx_ConfigAnalogFilter(handle, value);
}

inline HAL_StatusTypeDef HAL_FMPI2CEx_ConfigDigitalFilter(
    FMPI2C_HandleTypeDef* handle, uint32_t value) {
  return HAL_I2CEx_ConfigDigitalFilter(handle, value);
}

inline HAL_StatusTypeDef HAL_FMPI2C_Master_Transmit(
    FMPI2C_HandleTypeDef* handle, uint16_t address, uint8_t* data,
    uint16_t size, uint32_t timeout) {
  return HAL_I2C_Master_Transmit(handle, address, data, size, timeout);
}

inline HAL_StatusTypeDef HAL_FMPI2C_Master_Receive(FMPI2C_HandleTypeDef* handle,
                                                   uint16_t address,
                                                   uint8_t* data, uint16_t size,
                                                   uint32_t timeout) {
  return HAL_I2C_Master_Receive(handle, address, data, size, timeout);
}
