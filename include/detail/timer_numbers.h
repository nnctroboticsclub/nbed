#pragma once

#include <array>
#include <cstdint>

namespace nkrc::nbed::detail {

// PwmTimer と EncoderTimer の列挙値に対応する TIM の番号。
#if defined(STM32F303x8)
inline constexpr std::array<uint8_t, 6> kTimerNumbers{1, 2, 3, 15, 16, 17};
#else
inline constexpr std::array<uint8_t, 12> kTimerNumbers{1, 2,  3,  4,  5,  8,
                                                       9, 10, 11, 12, 13, 14};
#endif

/// @brief 公開列挙値の順序を TIM の番号に変換する。範囲外は 0。
constexpr uint8_t TimerNumber(uint8_t index) {
  return index < kTimerNumbers.size() ? kTimerNumbers[index] : 0;
}

/// @brief 32 ビットカウンターを持つ TIM なら true。
constexpr bool Is32BitTimer(uint8_t number) {
  return number == 2U || number == 5U;
}

/// @brief ハードウェアの ARR または CCR に設定できる最大値。
constexpr uint32_t TimerMaxPeriod(uint8_t number) {
  return Is32BitTimer(number) ? 0xffffffffU : 0xffffU;
}

/// @brief TIM が APB2 バスに属するとき true。
constexpr bool TimerOnApb2(uint8_t number) {
#if defined(STM32F303x8)
  return number == 1U || (number >= 15U && number <= 17U);
#else
  return number == 1U || (number >= 8U && number <= 11U);
#endif
}

/// @brief 相補出力とデッドタイム設定を持つタイマなら true。
constexpr bool HasTimerDeadTime(uint8_t number) {
#if defined(STM32F303x8)
  return number == 1U || (number >= 15U && number <= 17U);
#else
  return number == 1U || number == 8U;
#endif
}

}  // namespace nkrc::nbed::detail
