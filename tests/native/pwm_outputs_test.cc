#include <array>
#include <cassert>
#include <cstdint>
#include <limits>

#include "f3_pwm_hal_stubs.h"
#ifdef STM32F303x8
#include "f3/encoder.h"
#include "f3/pwm.h"
namespace device = nkrc::nbed::stm32::f3;
#elif defined(STM32F767xx)
#include "f7/encoder.h"
#include "f7/pwm.h"
namespace device = nkrc::nbed::stm32::f7;
#else
#include "f4/encoder.h"
#include "f4/pwm.h"
namespace device = nkrc::nbed::stm32::f4;
#endif

using namespace device;
using nkrc::nbed::interface::PinName;

static_assert(static_cast<uint8_t>(PwmChannel::kCh1) == 0);
static_assert(static_cast<uint8_t>(PwmChannel::kCh4) == 3);

namespace {
struct Mapping {
  PwmTimer timer;
  PwmChannel channel;
  PinName pin;
  uint32_t alternate;
};

// ST package/AF tables: DS9866 (F303K8), DS10693 (F446RE), DS11532 (F767ZI).
const std::array kMappings{
#ifdef STM32F303x8
    Mapping{PwmTimer::kTim1, PwmChannel::kCh1N, PinName::kPA7, 6},
    Mapping{PwmTimer::kTim1, PwmChannel::kCh1N, PinName::kPA11, 6},
    Mapping{PwmTimer::kTim1, PwmChannel::kCh2N, PinName::kPA12, 6},
    Mapping{PwmTimer::kTim1, PwmChannel::kCh2N, PinName::kPB0, 6},
    Mapping{PwmTimer::kTim1, PwmChannel::kCh3N, PinName::kPB1, 6},
    Mapping{PwmTimer::kTim1, PwmChannel::kCh3N, PinName::kPF0, 6},
    Mapping{PwmTimer::kTim15, PwmChannel::kCh1N, PinName::kPA1, 9},
    Mapping{PwmTimer::kTim16, PwmChannel::kCh1N, PinName::kPA13, 1},
    Mapping{PwmTimer::kTim16, PwmChannel::kCh1N, PinName::kPB6, 1},
    Mapping{PwmTimer::kTim17, PwmChannel::kCh1N, PinName::kPB7, 1},
#else
    Mapping{PwmTimer::kTim1, PwmChannel::kCh1N, PinName::kPA7, 1},
    Mapping{PwmTimer::kTim1, PwmChannel::kCh1N, PinName::kPB13, 1},
    Mapping{PwmTimer::kTim1, PwmChannel::kCh2N, PinName::kPB0, 1},
    Mapping{PwmTimer::kTim1, PwmChannel::kCh2N, PinName::kPB14, 1},
    Mapping{PwmTimer::kTim1, PwmChannel::kCh3N, PinName::kPB1, 1},
    Mapping{PwmTimer::kTim1, PwmChannel::kCh3N, PinName::kPB15, 1},
    Mapping{PwmTimer::kTim8, PwmChannel::kCh1N, PinName::kPA5, 3},
    Mapping{PwmTimer::kTim8, PwmChannel::kCh1N, PinName::kPA7, 3},
    Mapping{PwmTimer::kTim8, PwmChannel::kCh2N, PinName::kPB0, 3},
    Mapping{PwmTimer::kTim8, PwmChannel::kCh2N, PinName::kPB14, 3},
    Mapping{PwmTimer::kTim8, PwmChannel::kCh3N, PinName::kPB1, 3},
    Mapping{PwmTimer::kTim8, PwmChannel::kCh3N, PinName::kPB15, 3},
#ifdef STM32F767xx
    Mapping{PwmTimer::kTim1, PwmChannel::kCh1N, PinName::kPE8, 1},
    Mapping{PwmTimer::kTim1, PwmChannel::kCh2N, PinName::kPE10, 1},
    Mapping{PwmTimer::kTim1, PwmChannel::kCh3N, PinName::kPE12, 1},
#endif
#endif
};

void CheckPins() {
  unsigned valid = 0;
  for (const auto timer : {PwmTimer::kTim1, PwmTimer::kTim2, PwmTimer::kTim3
#ifdef STM32F303x8
                           ,
                           PwmTimer::kTim15, PwmTimer::kTim16, PwmTimer::kTim17
#else
                           ,
                           PwmTimer::kTim8
#endif
       }) {
    for (const auto channel :
         {PwmChannel::kCh1N, PwmChannel::kCh2N, PwmChannel::kCh3N}) {
      for (unsigned pin = 0; pin <= 255; ++pin) {
        const Mapping* expected = nullptr;
        for (const auto& mapping : kMappings) {
          if (mapping.timer == timer && mapping.channel == channel &&
              static_cast<unsigned>(mapping.pin) == pin) {
            expected = &mapping;
          }
        }
        Pwm output(timer, channel, static_cast<PinName>(pin), 1000);
        const auto calls = pwm_init_calls;
        assert(output.Initialize() == (expected != nullptr));
        if (expected != nullptr) {
          ++valid;
          assert(output.Initialize());
          assert(gpio_inits.back().second.Alternate == expected->alternate);
          auto* instance = last_pwm_init.Instance;
          assert(instance->active_channels == 0);
          assert(instance->active_n_channels != 0 &&
                 instance->main_output_enabled);
          assert(output.SetDutyCycle(0.25F));
          const auto index = static_cast<unsigned>(channel) - 4;
          assert(instance->CCR[index] == (instance->ARR + 1) / 4);
        } else {
          assert(pwm_init_calls == calls);
        }
      }
    }
  }
  assert(valid == kMappings.size());
}

void CheckPairAndOwnership() {
  const Pwm::Configuration config{.complementary_pin = PinName::kPA7,
                                  .dead_time = 12};
  Pwm pair(PwmTimer::kTim1, PwmChannel::kCh1, PinName::kPA8, 1000, config);
  assert(pair.Initialize());
  assert(tim1.active_channels == 1 && tim1.active_n_channels == 1);
  assert(last_break_config.DeadTime == 12);
  assert(pair.SetDutyCycle(0.25F));
  assert(pair.SetFrequencyHz(2000));
  assert(tim1.CCR[0] == (tim1.ARR + 1) / 4);
  Pwm n_only(PwmTimer::kTim1, PwmChannel::kCh1N, PinName::kPA7, 2000);
  const auto config_calls = pwm_config_calls;
  assert(!n_only.Initialize() && pwm_config_calls == config_calls);
  Pwm wrong_dead_time(PwmTimer::kTim1, PwmChannel::kCh2, PinName::kPA9, 2000);
  assert(!wrong_dead_time.Initialize());
  Pwm other(PwmTimer::kTim1, PwmChannel::kCh2, PinName::kPA9, 2000,
            {.dead_time = 12});
  assert(other.Initialize());
  assert(!pair.SetFrequencyHz(1000));
  other.Stop();
  assert(tim1.main_output_enabled && tim1.active_n_channels == 1);
  assert(pair.SetFrequencyHz(1000));
  pair.Stop();
  assert(!tim1.main_output_enabled && tim1.active_channels == 0 &&
         tim1.active_n_channels == 0);
  assert(n_only.Initialize());
  assert(!pair.Initialize());
  n_only.Stop();
  assert(pair.Initialize());
}

void CheckConfiguration() {
  const Pwm::Configuration config{
      .polarity = TIM_OCPOLARITY_LOW,
      .complementary_polarity = TIM_OCNPOLARITY_LOW,
      .idle_state = TIM_OCIDLESTATE_SET,
      .complementary_idle_state = TIM_OCNIDLESTATE_SET,
      .dead_time = 128,
      .off_state_run_mode = TIM_OSSR_ENABLE,
      .off_state_idle_mode = TIM_OSSI_ENABLE};
  Pwm output(PwmTimer::kTim1, PwmChannel::kCh1N, PinName::kPA7, 1000, config);
  assert(output.Initialize());
  assert(last_pwm_config.OCPolarity == TIM_OCPOLARITY_LOW);
  assert(last_pwm_config.OCNPolarity == TIM_OCNPOLARITY_LOW);
  assert(last_pwm_config.OCIdleState == TIM_OCIDLESTATE_SET);
  assert(last_pwm_config.OCNIdleState == TIM_OCNIDLESTATE_SET);
  assert(last_break_config.DeadTime == 128);
  assert(last_break_config.OffStateRunMode == TIM_OSSR_ENABLE);
  assert(last_break_config.OffStateIDLEMode == TIM_OSSI_ENABLE);
  Pwm mismatch(PwmTimer::kTim1, PwmChannel::kCh2, PinName::kPA9, 1000,
               {.dead_time = 128});
  assert(!mismatch.Initialize());
  output.Stop();
  Pwm polarity(PwmTimer::kTim2, PwmChannel::kCh1, PinName::kPA0, 1000,
               {.polarity = TIM_OCPOLARITY_LOW});
  assert(polarity.Initialize());
  assert(last_pwm_config.OCPolarity == TIM_OCPOLARITY_LOW);
}

void CheckInvalidConfiguration() {
  const auto reject = [](PwmTimer timer, PwmChannel channel, PinName pin,
                         const Pwm::Configuration& config) {
    const auto calls = pwm_init_calls;
    Pwm output(timer, channel, pin, 1000, config);
    assert(!output.Initialize() && pwm_init_calls == calls);
  };
  reject(PwmTimer::kTim1, PwmChannel::kCh1, PinName::kPA8,
         {.complementary_pin = PinName::kPA9});
  reject(PwmTimer::kTim1, PwmChannel::kCh4, PinName::kPA11,
         {.complementary_pin = PinName::kPA7});
  reject(PwmTimer::kTim1, PwmChannel::kCh1N, PinName::kPA7,
         {.complementary_pin = PinName::kPA8});
  reject(PwmTimer::kTim2, PwmChannel::kCh1, PinName::kPA0, {.dead_time = 1});
  reject(PwmTimer::kTim2, PwmChannel::kCh1, PinName::kPA0,
         {.idle_state = TIM_OCIDLESTATE_SET});
  reject(PwmTimer::kTim1, PwmChannel::kCh1, PinName::kPA8, {.dead_time = 256});
  reject(PwmTimer::kTim1, PwmChannel::kCh1, PinName::kPA8,
         {.polarity = std::numeric_limits<uint32_t>::max()});
  reject(PwmTimer::kTim1, static_cast<PwmChannel>(255), PinName::kPA8, {});
}

void CheckFailures() {
  for (auto* status : {&pwm_init_status, &pwm_config_status, &pwm_break_status,
                       &pwm_start_status, &pwm_n_start_status}) {
    Pwm output(PwmTimer::kTim1, PwmChannel::kCh1, PinName::kPA8, 1000,
               {.complementary_pin = PinName::kPA7});
    *status = HAL_ERROR;
    pwm_partial_start = pwm_n_partial_start = true;
    assert(!output.Initialize());
    assert(tim1.active_channels == 0 && tim1.active_n_channels == 0);
    *status = HAL_OK;
    pwm_partial_start = pwm_n_partial_start = false;
    assert(output.Initialize());
  }
  Pwm other(PwmTimer::kTim1, PwmChannel::kCh2, PinName::kPA9, 1000);
  assert(other.Initialize());
  const auto deinit_calls = pwm_deinit_calls;
  Pwm pair(PwmTimer::kTim1, PwmChannel::kCh1, PinName::kPA8, 1000,
           {.complementary_pin = PinName::kPA7});
  pwm_n_start_status = HAL_ERROR;
  pwm_n_partial_start = true;
  assert(!pair.Initialize());
  assert(pwm_deinit_calls == deinit_calls);
  assert(tim1.active_channels == 2 && tim1.active_n_channels == 0 &&
         tim1.main_output_enabled);
  pwm_n_start_status = HAL_OK;
  pwm_n_partial_start = false;
  assert(pair.Initialize());
  other.Stop();
  assert(tim1.active_channels == 1 && tim1.active_n_channels == 1 &&
         tim1.main_output_enabled);
}
}  // namespace

#ifndef STM32F303x8
void CheckAdditionalTimers() {
  using enum PinName;
  using enum PwmTimer;

  struct Case {
    PwmTimer timer;
    PwmChannel channel;
    PinName pin;
    TIM_TypeDef* instance;
    uint32_t alternate;
    uint32_t clock;
  };

  const uint32_t apb1 = HAL_RCC_GetPCLK1Freq() * 2;
  const uint32_t apb2 = HAL_RCC_GetPCLK2Freq();
  const std::array cases{
      Case{kTim4, PwmChannel::kCh4, kPB9, TIM4, 2, apb1},
      Case{kTim5, PwmChannel::kCh3, kPA2, TIM5, 2, apb1},
      Case{kTim8, PwmChannel::kCh4, kPC9, TIM8, 3, apb2},
      Case{kTim9, PwmChannel::kCh2, kPA3, TIM9, 3, apb2},
      Case{kTim10, PwmChannel::kCh1, kPB8, TIM10, 3, apb2},
      Case{kTim11, PwmChannel::kCh1, kPB9, TIM11, 3, apb2},
      Case{kTim12, PwmChannel::kCh2, kPB15, TIM12, 9, apb1},
      Case{kTim13, PwmChannel::kCh1, kPA6, TIM13, 9, apb1},
      Case{kTim14, PwmChannel::kCh1, kPA7, TIM14, 9, apb1},
#ifdef STM32F767xx
      Case{kTim4, PwmChannel::kCh1, kPD12, TIM4, 2, apb1},
      Case{kTim9, PwmChannel::kCh2, kPE6, TIM9, 3, apb2},
      Case{kTim10, PwmChannel::kCh1, kPF6, TIM10, 3, apb2},
      Case{kTim11, PwmChannel::kCh1, kPF7, TIM11, 3, apb2},
      Case{kTim13, PwmChannel::kCh1, kPF8, TIM13, 9, apb1},
      Case{kTim14, PwmChannel::kCh1, kPF9, TIM14, 9, apb1},
#endif
  };
  for (const auto& route : cases) {
    Pwm pwm(route.timer, route.channel, route.pin, 50);
    assert(pwm.Initialize() && last_pwm_init.Instance == route.instance);
    assert(route.instance->clock_enabled);
    assert(gpio_inits.back().second.Alternate == route.alternate);
    assert(pwm.GetFrequencyHz() ==
           route.clock / (route.instance->PSC + 1U) / pwm.GetPeriodTicks());
    assert(pwm.SetDutyCycle(1.0F) && pwm.SetFrequencyHz(100));
  }

  Pwm pwm(kTim5, PwmChannel::kCh3, kPA2, 100);
  Encoder encoder(EncoderTimer::kTim5, kPA0, kPA1, 400);
  assert(pwm.Initialize() && !encoder.Initialize());
  pwm.Stop();
  assert(encoder.Initialize() && last_encoder_init.Instance == TIM5);
  assert(last_encoder_init.Init.Period == 0xffffffffU && !pwm.Initialize());
  TIM5->CNT = 0xffff'ffffU;
  assert(encoder.GetCount() == -1);
  encoder.ResetCount();
  assert(encoder.GetCount() == 0);

  Encoder encoder4(EncoderTimer::kTim4, kPB6, kPB7, 400);
  Encoder encoder8(EncoderTimer::kTim8, kPC6, kPC7, 400);
  assert(encoder4.Initialize() && last_encoder_init.Instance == TIM4);
  TIM4->CNT = 0xffffU;
  assert(encoder4.GetCount() == -1);
  assert(encoder8.Initialize() && last_encoder_init.Instance == TIM8);
  assert(gpio_inits.back().second.Alternate == 3);
  Encoder unsupported(static_cast<EncoderTimer>(6), kPA2, kPA3, 400);
  assert(!unsupported.Initialize());
}
#endif

int main() {
  CheckPins();
  CheckPairAndOwnership();
  CheckConfiguration();
  CheckInvalidConfiguration();
  CheckFailures();
#ifndef STM32F303x8
  CheckAdditionalTimers();
#endif
}
