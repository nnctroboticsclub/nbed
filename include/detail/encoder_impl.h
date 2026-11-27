// 対応する MCU の名前空間内で encoder.cc から読み込む。
Encoder::Encoder(EncoderTimer timer, interface::PinName channel_a,
                 interface::PinName channel_b, uint32_t counts_per_revolution)
    : timer_(timer),
      channel_a_(channel_a),
      channel_b_(channel_b),
      counts_per_revolution_(counts_per_revolution) {}

Encoder::~Encoder() {
  if (!initialized_) {
    return;
  }
  (void)HAL_TIM_Encoder_Stop(&handle_, TIM_CHANNEL_ALL);
  (void)HAL_TIM_Encoder_DeInit(&handle_);
  TimerReservation::ReleaseEncoder(static_cast<uint8_t>(timer_));
  initialized_ = false;
}

bool Encoder::Initialize() {
  if (initialized_) {
    return true;
  }
  const auto index = static_cast<uint8_t>(timer_);
#if defined(STM32F303x8)
  constexpr auto last_timer = EncoderTimer::kTim3;
#else
  constexpr auto last_timer = EncoderTimer::kTim8;
#endif
  if (timer_ > last_timer || counts_per_revolution_ == 0 ||
      !PinMapper::IsValidPinPair(channel_a_, channel_b_)) {
    return false;
  }
  const uint8_t number = detail::TimerNumber(index);
  const auto* a =
      detail::FindPinRoute(kTimerPins, number, PinSignal::kCh1, channel_a_);
  const auto* b =
      detail::FindPinRoute(kTimerPins, number, PinSignal::kCh2, channel_b_);
  if (a == nullptr || b == nullptr || !TimerReservation::ClaimEncoder(index)) {
    return false;
  }
  handle_.Instance = detail::EnableTimer(number);
  handle_.Init.Prescaler = 0;
  handle_.Init.CounterMode = TIM_COUNTERMODE_UP;
  handle_.Init.Period = detail::TimerMaxPeriod(number);
  handle_.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  handle_.Init.RepetitionCounter = 0;
  handle_.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  TIM_Encoder_InitTypeDef config{};
  config.EncoderMode = TIM_ENCODERMODE_TI12;
  config.IC1Polarity = TIM_ICPOLARITY_RISING;
  config.IC1Selection = TIM_ICSELECTION_DIRECTTI;
  config.IC1Prescaler = TIM_ICPSC_DIV1;
  config.IC1Filter = 0;
  config.IC2Polarity = TIM_ICPOLARITY_RISING;
  config.IC2Selection = TIM_ICSELECTION_DIRECTTI;
  config.IC2Prescaler = TIM_ICPSC_DIV1;
  config.IC2Filter = 0;
  if (HAL_TIM_Encoder_Init(&handle_, &config) != HAL_OK ||
      HAL_TIM_Encoder_Start(&handle_, TIM_CHANNEL_ALL) != HAL_OK) {
    (void)HAL_TIM_Encoder_DeInit(&handle_);
    TimerReservation::ReleaseEncoder(static_cast<uint8_t>(timer_));
    return false;
  }
  GPIO_InitTypeDef gpio{};
  gpio.Mode = GPIO_MODE_AF_PP;
  gpio.Pull = GPIO_NOPULL;
  gpio.Speed = GPIO_SPEED_FREQ_LOW;
  gpio.Alternate = a->value;
  (void)PinMapper::ConfigureGpio(a->pin, gpio);
  gpio.Alternate = b->value;
  (void)PinMapper::ConfigureGpio(b->pin, gpio);
  initialized_ = true;
  return true;
}

int Encoder::GetCount() {
  if (!initialized_) {
    return 0;
  }
  const uint32_t count = __HAL_TIM_GET_COUNTER(&handle_);
  return detail::Is32BitTimer(detail::TimerNumber(static_cast<uint8_t>(timer_)))
             ? static_cast<int32_t>(count)
             : static_cast<int16_t>(count);
}

Angle Encoder::GetAngle() {
  if (!initialized_) {
    return Angle::FromDegree(0.0F);
  }
  return Angle::FromDegree(static_cast<float>(GetCount()) * 360.0F /
                           static_cast<float>(counts_per_revolution_));
}

void Encoder::ResetCount() {
  if (initialized_) {
    __HAL_TIM_SET_COUNTER(&handle_, 0);
  }
}
