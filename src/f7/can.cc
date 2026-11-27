#if defined(STM32F767xx)

#include "f7/can.h"

#include <algorithm>
#include <array>

#include "f7/pin_mapper.h"
#include "f7/pin_routes.h"
#include "stm32f7xx_hal_cortex.h"
#include "stm32f7xx_hal_gpio.h"
#include "stm32f7xx_hal_rcc.h"

namespace nkrc::nbed::stm32::f7 {

#include "detail/can_impl.h"

}  // namespace nkrc::nbed::stm32::f7

#endif
