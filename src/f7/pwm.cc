#if defined(STM32F767xx)

#include "f7/pwm.h"

#include <algorithm>
#include <array>
#include <cmath>

#include "detail/pwm_timing.h"
#include "detail/timer_hardware.h"
#include "detail/timer_numbers.h"
#include "f7/pin_mapper.h"
#include "f7/pin_routes.h"
#include "f7/timer_reservation.h"
#include "stm32f7xx_hal_gpio.h"

namespace nkrc::nbed::stm32::f7 {

#include "detail/pwm_impl.h"

}  // namespace nkrc::nbed::stm32::f7

#endif
