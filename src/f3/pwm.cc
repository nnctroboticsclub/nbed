#if defined(STM32F303x8)

#include "f3/pwm.h"

#include <algorithm>
#include <array>
#include <cmath>

#include "detail/pwm_timing.h"
#include "detail/timer_hardware.h"
#include "detail/timer_numbers.h"
#include "f3/pin_mapper.h"
#include "f3/pin_routes.h"
#include "f3/timer_reservation.h"
#include "stm32f3xx_hal_gpio.h"

namespace nkrc::nbed::stm32::f3 {

#include "detail/pwm_impl.h"

}  // namespace nkrc::nbed::stm32::f3

#endif
