#pragma once

#include <cstdint>

namespace nkrc::nbed::interface {

enum class DmaStatus : uint8_t { kIdle, kBusy, kComplete, kError, kAborted };

enum class DmaRequest : uint8_t {
  kTim1Update,
  kTim2Update,
  kTim3Update,
  kTim15Update,
  kTim16Update,
  kTim17Update,
  kUsart1Tx,
  kUsart1Rx,
  kUsart2Tx,
  kUsart2Rx,
  kUsart3Tx,
  kUsart3Rx,
  kUart4Tx,
  kUart4Rx,
  kUsart6Tx,
  kUsart6Rx
};

}  // namespace nkrc::nbed::interface
