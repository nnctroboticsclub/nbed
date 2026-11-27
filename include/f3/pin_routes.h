#pragma once

#include "detail/pin_routes.h"

namespace nkrc::nbed::stm32::f3 {

using detail::PinRoute;
using detail::PinSignal;
using interface::PinName;

// STM32F303K(6-8)Tx のパッケージ別ピン。出典と照合手順は docs/pinout.md を参照。
inline constexpr PinRoute kSpiPins[]{
    {1, PinSignal::kSck, PinName::kPA5, 5},
    {1, PinSignal::kSck, PinName::kPB3, 5},
    {1, PinSignal::kMiso, PinName::kPA6, 5},
    {1, PinSignal::kMiso, PinName::kPB4, 5},
    {1, PinSignal::kMosi, PinName::kPA7, 5},
    {1, PinSignal::kMosi, PinName::kPB5, 5},
};

inline constexpr PinRoute kI2cPins[]{
    {1, PinSignal::kScl, PinName::kPA15, 4},
    {1, PinSignal::kScl, PinName::kPB6, 4},
    {1, PinSignal::kSda, PinName::kPA14, 4},
    {1, PinSignal::kSda, PinName::kPB7, 4},
};

inline constexpr PinRoute kCanPins[]{
    {1, PinSignal::kRx, PinName::kPA11, 9},
    {1, PinSignal::kTx, PinName::kPA12, 9},
};

inline constexpr PinRoute kUartPins[]{
    {1, PinSignal::kRx, PinName::kPA10, 7},
    {1, PinSignal::kRx, PinName::kPB7, 7},
    {1, PinSignal::kTx, PinName::kPA9, 7},
    {1, PinSignal::kTx, PinName::kPB6, 7},
    {1, PinSignal::kCts, PinName::kPA11, 7},
    {1, PinSignal::kRts, PinName::kPA12, 7},
    {2, PinSignal::kRx, PinName::kPA3, 7},
    {2, PinSignal::kRx, PinName::kPA15, 7},
    {2, PinSignal::kRx, PinName::kPB4, 7},
    {2, PinSignal::kTx, PinName::kPA2, 7},
    {2, PinSignal::kTx, PinName::kPA14, 7},
    {2, PinSignal::kTx, PinName::kPB3, 7},
    {2, PinSignal::kCts, PinName::kPA0, 7},
    {2, PinSignal::kRts, PinName::kPA1, 7},
};

inline constexpr PinRoute kTimerPins[]{
    {1, PinSignal::kCh1, PinName::kPA8, 6},
    {1, PinSignal::kCh2, PinName::kPA9, 6},
    {1, PinSignal::kCh3, PinName::kPA10, 6},
    {1, PinSignal::kCh4, PinName::kPA11, 11},
    {1, PinSignal::kCh1N, PinName::kPA7, 6},
    {1, PinSignal::kCh1N, PinName::kPA11, 6},
    {1, PinSignal::kCh2N, PinName::kPA12, 6},
    {1, PinSignal::kCh2N, PinName::kPB0, 6},
    {1, PinSignal::kCh3N, PinName::kPB1, 6},
    {1, PinSignal::kCh3N, PinName::kPF0, 6},
    {2, PinSignal::kCh1, PinName::kPA0, 1},
    {2, PinSignal::kCh1, PinName::kPA5, 1},
    {2, PinSignal::kCh1, PinName::kPA15, 1},
    {2, PinSignal::kCh2, PinName::kPA1, 1},
    {2, PinSignal::kCh2, PinName::kPB3, 1},
    {2, PinSignal::kCh3, PinName::kPA2, 1},
    {2, PinSignal::kCh3, PinName::kPA9, 10},
    {2, PinSignal::kCh4, PinName::kPA3, 1},
    {2, PinSignal::kCh4, PinName::kPA10, 10},
    {3, PinSignal::kCh1, PinName::kPA6, 2},
    {3, PinSignal::kCh1, PinName::kPB4, 2},
    {3, PinSignal::kCh2, PinName::kPA4, 2},
    {3, PinSignal::kCh2, PinName::kPA7, 2},
    {3, PinSignal::kCh2, PinName::kPB5, 2},
    {3, PinSignal::kCh3, PinName::kPB0, 2},
    {3, PinSignal::kCh4, PinName::kPB1, 2},
    {3, PinSignal::kCh4, PinName::kPB7, 10},
    {15, PinSignal::kCh1, PinName::kPA2, 9},
    {15, PinSignal::kCh2, PinName::kPA3, 9},
    {15, PinSignal::kCh1N, PinName::kPA1, 9},
    {16, PinSignal::kCh1, PinName::kPA6, 1},
    {16, PinSignal::kCh1, PinName::kPA12, 1},
    {16, PinSignal::kCh1, PinName::kPB4, 1},
    {16, PinSignal::kCh1N, PinName::kPA13, 1},
    {16, PinSignal::kCh1N, PinName::kPB6, 1},
    {17, PinSignal::kCh1, PinName::kPA7, 1},
    {17, PinSignal::kCh1, PinName::kPB5, 10},
    {17, PinSignal::kCh1N, PinName::kPB7, 1},
};

inline constexpr PinRoute kAdcPins[]{
    {1, PinSignal::kAnalog, PinName::kPA0, 1},
    {1, PinSignal::kAnalog, PinName::kPA1, 2},
    {1, PinSignal::kAnalog, PinName::kPA2, 3},
    {1, PinSignal::kAnalog, PinName::kPA3, 4},
    {1, PinSignal::kAnalog, PinName::kPB0, 11},
    {1, PinSignal::kAnalog, PinName::kPB1, 12},
    {2, PinSignal::kAnalog, PinName::kPA4, 1},
    {2, PinSignal::kAnalog, PinName::kPA5, 2},
    {2, PinSignal::kAnalog, PinName::kPA6, 3},
    {2, PinSignal::kAnalog, PinName::kPA7, 4},
};

}  // namespace nkrc::nbed::stm32::f3
