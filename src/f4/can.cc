#if defined(STM32F446xx)

#include "f4/can.h"

#include <algorithm>
#include <array>

#include "f4/pin_mapper.h"
#include "f4/pin_routes.h"
#include "stm32f4xx_hal_cortex.h"
#include "stm32f4xx_hal_gpio.h"
#include "stm32f4xx_hal_rcc.h"

namespace nkrc::nbed::stm32::f4 {

#include "detail/can_impl.h"

}  // namespace nkrc::nbed::stm32::f4

#endif
