#if defined(STM32F446xx)

#include "f4/encoder.h"

#include "detail/timer_hardware.h"
#include "detail/timer_numbers.h"
#include "f4/pin_mapper.h"
#include "f4/pin_routes.h"
#include "f4/timer_reservation.h"
#include "stm32f4xx_hal_gpio.h"

namespace nkrc::nbed::stm32::f4 {

#include "detail/encoder_impl.h"

}  // namespace nkrc::nbed::stm32::f4

#endif
