#pragma once

#include <cstdint>
#include <span>

#include "interface/pin_names.h"

namespace nkrc::nbed::detail {

/// @brief 代替機能のピンが担当する入出力信号。
enum class PinSignal : uint8_t {
  kSck,
  kMiso,
  kMosi,
  kScl,
  kSda,
  kRx,
  kTx,
  kCts,
  kRts,
  kCh1,
  kCh2,
  kCh3,
  kCh4,
  kCh1N,
  kCh2N,
  kCh3N,
  kAnalog,
};

/// @brief 一つの周辺機能と信号に対して使用できるピンと設定値。
struct PinRoute {
  uint8_t peripheral;      ///< 同種の周辺機能を区別する番号。
  PinSignal signal;        ///< ピンに割り当てる入出力信号。
  interface::PinName pin;  ///< STM32 の物理ピン名。
  uint8_t value;           ///< 代替機能番号（AF）。ADC の場合はチャンネル番号。
};

/// @brief 周辺機能番号、信号、ピンが一致する設定を探す。
/// @param routes 対象パッケージのルート表。
/// @param peripheral 探す周辺機能の番号。
/// @param signal ピンに割り当てる信号。
/// @param pin 物理ピン。
/// @return 一致する設定へのポインター。該当がなければ nullptr。
constexpr const PinRoute* FindPinRoute(std::span<const PinRoute> routes,
                                       uint8_t peripheral, PinSignal signal,
                                       interface::PinName pin) {
  for (const auto& route : routes) {
    if (route.peripheral == peripheral && route.signal == signal &&
        route.pin == pin) {
      return &route;
    }
  }
  return nullptr;
}

}  // namespace nkrc::nbed::detail
