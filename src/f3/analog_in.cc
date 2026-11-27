#if defined(STM32F303x8)

#include "f3/analog_in.h"

#include <array>
#include <cmath>

#include "f3/pin_mapper.h"
#include "f3/pin_routes.h"
#include "stm32f3xx_hal_adc.h"
#include "stm32f3xx_hal_gpio.h"
#include "stm32f3xx_hal_rcc.h"

namespace nkrc::nbed::stm32::f3 {
namespace {

struct AdcState {
  ADC_HandleTypeDef handle{};
  bool initialized{false};
  uint32_t last_channel{0xffffffffU};
};

AdcState& StateFor(uint8_t number) {
  static std::array<AdcState, 2> states{};
  return states[number - 1U];
}

const PinRoute* FindAdcRoute(AdcPeripheral peripheral, interface::PinName pin) {
  if (peripheral != AdcPeripheral::kAdcAuto) {
    return detail::FindPinRoute(kAdcPins, static_cast<uint8_t>(peripheral),
                                PinSignal::kAnalog, pin);
  }
  // Preserve the original default for each pin. Shared pins select ADC1.
  for (const auto& route : kAdcPins) {
    if (route.pin == pin) {
      return &route;
    }
  }
  return nullptr;
}

bool InitializeAdc(uint8_t number) {
  auto& state = StateFor(number);
  if (state.initialized) {
    return true;
  }
  auto& adc = state.handle;
  __HAL_RCC_ADC12_CLK_ENABLE();
  adc.Instance = number == 1 ? ADC1 : ADC2;
  adc.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;
  adc.Init.Resolution = ADC_RESOLUTION_12B;
  adc.Init.ScanConvMode = ADC_SCAN_DISABLE;
  adc.Init.ContinuousConvMode = DISABLE;
  adc.Init.DiscontinuousConvMode = DISABLE;
  adc.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  adc.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  adc.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  adc.Init.NbrOfConversion = 1;
  adc.Init.DMAContinuousRequests = DISABLE;
  adc.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
  adc.Init.LowPowerAutoWait = DISABLE;
  adc.Init.Overrun = ADC_OVR_DATA_OVERWRITTEN;
  if (HAL_ADC_Init(&adc) != HAL_OK ||
      HAL_ADCEx_Calibration_Start(&adc, ADC_SINGLE_ENDED) != HAL_OK) {
    (void)HAL_ADC_DeInit(&adc);
    return false;
  }
  state.initialized = true;
  return true;
}

}  // namespace

AnalogIn::AnalogIn(interface::PinName pin, float reference_voltage)
    : AnalogIn(AdcPeripheral::kAdcAuto, pin, reference_voltage) {}

AnalogIn::AnalogIn(AdcPeripheral peripheral, interface::PinName pin,
                   float reference_voltage)
    : peripheral_(peripheral),
      pin_(pin),
      reference_voltage_(reference_voltage) {}

bool AnalogIn::Initialize() {
  if (adc_number_ != 0U) {
    return true;
  }
  const auto* route = FindAdcRoute(peripheral_, pin_);
  if (route == nullptr || !PinMapper::IsValidPin(pin_) ||
      !std::isfinite(reference_voltage_) || reference_voltage_ <= 0.0F ||
      !InitializeAdc(route->peripheral)) {
    return false;
  }
  GPIO_InitTypeDef gpio{};
  gpio.Mode = GPIO_MODE_ANALOG;
  gpio.Pull = GPIO_NOPULL;
  if (!PinMapper::ConfigureGpio(pin_, gpio)) {
    return false;
  }
  adc_number_ = route->peripheral;
  channel_ = route->value;
  return true;
}

float AnalogIn::ReadRatio() {
  if (adc_number_ == 0U) {
    return 0.0F;
  }
  auto& state = StateFor(adc_number_);
  auto& adc = state.handle;
  ADC_ChannelConfTypeDef channel{};
  channel.Channel = channel_;
  channel.Rank = ADC_REGULAR_RANK_1;
  channel.SingleDiff = ADC_SINGLE_ENDED;
  channel.OffsetNumber = ADC_OFFSET_NONE;
  channel.SamplingTime = ADC_SAMPLETIME_19CYCLES_5;
  if (state.last_channel != channel.Channel) {
    if (HAL_ADC_ConfigChannel(&adc, &channel) != HAL_OK) {
      return 0.0F;
    }
    state.last_channel = channel.Channel;
  }
  if (HAL_ADC_Start(&adc) != HAL_OK) {
    return 0.0F;
  }
  if (HAL_ADC_PollForConversion(&adc, 100) != HAL_OK) {
    (void)HAL_ADC_Stop(&adc);
    return 0.0F;
  }
  const uint32_t value = HAL_ADC_GetValue(&adc);
  (void)HAL_ADC_Stop(&adc);
  return static_cast<float>(value) / 4095.0F;
}

float AnalogIn::ReadVoltage() { return ReadRatio() * reference_voltage_; }

}  // namespace nkrc::nbed::stm32::f3

#endif
