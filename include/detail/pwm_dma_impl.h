// Included inside the selected backend namespace in pwm.cc.
// Update DMA gives one full period to write the next preloaded CCR, including
// zero-duty entries. The timer is exclusive while this DMA owner is retained.
bool Pwm::SendDataDma(std::span<const uint32_t> data) {
  using interface::DmaRequest;
  using interface::DmaStatus;
  if (!initialized_ || dma_status_ == DmaStatus::kBusy || data.size() < 3 ||
      data.size() > 65535 || data[data.size() - 1] != 0 ||
      data[data.size() - 2] != 0 || IsComplementary(channel_) ||
      configuration_.complementary_pin != interface::PinName::kNC ||
      configuration_.polarity != TIM_OCPOLARITY_HIGH ||
      configuration_.dead_time != 0 ||
      reinterpret_cast<uintptr_t>(data.data()) % alignof(uint32_t) != 0) {
    return false;
  }
  auto& state = State();
  if (state.active_channels != 1 ||
      (state.dma_owner != nullptr && state.dma_owner != this)) {
    return false;
  }
  const uint64_t period = GetPeriodTicks();
  for (const uint32_t ticks : data) {
    if (ticks >= period) {
      return false;
    }
  }
  if (!detail::Dma::PrepareBuffer(data.data(), data.size_bytes(), false)) {
    return false;
  }
  DmaRequest request;
  switch (timer_) {
    case PwmTimer::kTim1:
      request = DmaRequest::kTim1Update;
      break;
    case PwmTimer::kTim2:
      request = DmaRequest::kTim2Update;
      break;
    case PwmTimer::kTim3:
      request = DmaRequest::kTim3Update;
      break;
#if defined(STM32F303x8)
    case PwmTimer::kTim15:
      request = DmaRequest::kTim15Update;
      break;
    case PwmTimer::kTim16:
      request = DmaRequest::kTim16Update;
      break;
    case PwmTimer::kTim17:
      request = DmaRequest::kTim17Update;
      break;
#endif
    default:
      return false;
  }
  if (!dma_.IsInitialized() && !dma_.Initialize(request, false, true)) {
    return false;
  }
  auto& handle = dma_.GetHandle();
  handle.Parent = this;
  handle.XferCpltCallback = OnDmaComplete;
  handle.XferErrorCallback = OnDmaError;
  state.dma_owner = this;
  __HAL_TIM_DISABLE_DMA(&state.handle, TIM_DMA_UPDATE);
  CLEAR_BIT(state.handle.Instance->CR1, TIM_CR1_CEN);
  __HAL_TIM_SET_COMPARE(&state.handle, HalChannel(), 0);
  (void)HAL_TIM_GenerateEvent(&state.handle, TIM_EVENTSOURCE_UPDATE);
  __HAL_TIM_SET_COUNTER(&state.handle, 0);
  volatile uint32_t* compare = nullptr;
  switch (ChannelIndex(channel_)) {
    case 0:
      compare = &state.handle.Instance->CCR1;
      break;
    case 1:
      compare = &state.handle.Instance->CCR2;
      break;
    case 2:
      compare = &state.handle.Instance->CCR3;
      break;
    default:
      compare = &state.handle.Instance->CCR4;
      break;
  }
  dma_status_ = DmaStatus::kBusy;
  if (HAL_DMA_Start_IT(
          &handle,
          static_cast<uint32_t>(reinterpret_cast<uintptr_t>(data.data())),
          static_cast<uint32_t>(reinterpret_cast<uintptr_t>(compare)),
          static_cast<uint32_t>(data.size())) != HAL_OK) {
    FinishDma(DmaStatus::kError);
    dma_.Release();
    state.dma_owner = nullptr;
    __HAL_TIM_ENABLE(&state.handle);
    return false;
  }
  __HAL_TIM_ENABLE_DMA(&state.handle, TIM_DMA_UPDATE);
  __HAL_TIM_ENABLE(&state.handle);
  return true;
}

interface::DmaStatus Pwm::GetDmaStatus() const {
  const auto status = dma_status_;
  __DMB();
  return status;
}

uint64_t Pwm::GetPeriodTicks() const {
  return initialized_ ? static_cast<uint64_t>(State().handle.Init.Period) + 1U
                      : 0;
}

void Pwm::FinishDma(interface::DmaStatus status) {
  auto& state = State();
  __HAL_TIM_DISABLE_DMA(&state.handle, TIM_DMA_UPDATE);
  __HAL_TIM_SET_COMPARE(&state.handle, HalChannel(), 0);
  // Also latch low on an error/abort. On success the last two zeros ensure
  // the output is already low when DMA completes (CCR has one-period preload).
  if (status != interface::DmaStatus::kComplete) {
    (void)HAL_TIM_GenerateEvent(&state.handle, TIM_EVENTSOURCE_UPDATE);
  }
  __DMB();
  dma_status_ = status;
}

void Pwm::OnDmaComplete(DMA_HandleTypeDef* handle) {
  static_cast<Pwm*>(handle->Parent)->FinishDma(interface::DmaStatus::kComplete);
}

void Pwm::OnDmaError(DMA_HandleTypeDef* handle) {
  auto* owner = static_cast<Pwm*>(handle->Parent);
  __HAL_TIM_DISABLE_DMA(&owner->State().handle, TIM_DMA_UPDATE);
  (void)HAL_DMA_Abort(handle);
  owner->FinishDma(interface::DmaStatus::kError);
}

void Pwm::StopDma() {
  if (!dma_.IsInitialized()) {
    return;
  }
  auto& state = State();
  __HAL_TIM_DISABLE_DMA(&state.handle, TIM_DMA_UPDATE);
  (void)HAL_DMA_Abort(&dma_.GetHandle());
  if (dma_status_ == interface::DmaStatus::kBusy) {
    FinishDma(interface::DmaStatus::kAborted);
  }
  dma_.Release();
  state.dma_owner = nullptr;
}
