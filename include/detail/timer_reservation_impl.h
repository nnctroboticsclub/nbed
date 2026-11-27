// 対応する MCU の名前空間内で timer_reservation.cc から読み込む。
namespace {

struct TimerReservationState {
  uint8_t pwm_channel_mask{0};
  bool encoder_claimed{false};
};

#if defined(STM32F303x8)
constexpr std::array<uint8_t, 6> kPwmChannelCounts{4, 4, 4, 2, 1, 1};
constexpr uint8_t kEncoderTimerCount = 3;
#else
constexpr std::array<uint8_t, 12> kPwmChannelCounts{4, 4, 4, 4, 4, 4,
                                                    2, 1, 1, 2, 1, 1};
constexpr uint8_t kEncoderTimerCount = 6;
#endif
static_assert(kPwmChannelCounts.size() == detail::kTimerNumbers.size());

TimerReservationState* StateFor(uint8_t timer) {
  static std::array<TimerReservationState, kPwmChannelCounts.size()> states{};
  return timer < states.size() ? &states.at(timer) : nullptr;
}

uint8_t ChannelBit(uint8_t timer, uint8_t channel) {
  return timer < kPwmChannelCounts.size() && channel < kPwmChannelCounts[timer]
             ? static_cast<uint8_t>(1U << channel)
             : 0U;
}

}  // namespace

bool TimerReservation::ClaimPwm(uint8_t timer, uint8_t channel) {
  TimerReservationState* const state = StateFor(timer);
  const uint8_t channel_bit = ChannelBit(timer, channel);
  if (state == nullptr || channel_bit == 0U || state->encoder_claimed ||
      (state->pwm_channel_mask & channel_bit) != 0U) {
    return false;
  }
  state->pwm_channel_mask |= channel_bit;
  return true;
}

void TimerReservation::ReleasePwm(uint8_t timer, uint8_t channel) {
  TimerReservationState* const state = StateFor(timer);
  if (state != nullptr) {
    state->pwm_channel_mask &=
        static_cast<uint8_t>(~ChannelBit(timer, channel));
  }
}

bool TimerReservation::ClaimEncoder(uint8_t timer) {
  if (timer >= kEncoderTimerCount) {
    return false;
  }
  TimerReservationState* const state = StateFor(timer);
  if (state == nullptr || state->encoder_claimed ||
      state->pwm_channel_mask != 0U) {
    return false;
  }
  state->encoder_claimed = true;
  return true;
}

void TimerReservation::ReleaseEncoder(uint8_t timer) {
  if (timer >= kEncoderTimerCount) {
    return;
  }
  TimerReservationState* const state = StateFor(timer);
  if (state != nullptr) {
    state->encoder_claimed = false;
  }
}
