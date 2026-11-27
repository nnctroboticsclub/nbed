#if defined(STM32F303x8)

#include "f3/can.h"

#include <algorithm>
#include <array>

#include "f3/pin_mapper.h"
#include "f3/pin_routes.h"
#include "stm32f3xx_hal_cortex.h"
#include "stm32f3xx_hal_gpio.h"
#include "stm32f3xx_hal_rcc.h"

namespace nkrc::nbed::stm32::f3 {

#include "detail/can_impl.h"

}  // namespace nkrc::nbed::stm32::f3

#endif
