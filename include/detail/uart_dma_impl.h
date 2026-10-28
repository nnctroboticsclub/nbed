// Included inside the selected backend namespace in uart.cc.
interface::DmaRequest Uart::DmaRequestFor(bool receive) const {
  using interface::DmaRequest;
  switch (peripheral_) {
#if defined(STM32F303x8)
    case UartPeripheral::kUsart1:
      return receive ? DmaRequest::kUsart1Rx : DmaRequest::kUsart1Tx;
#else
    case UartPeripheral::kUsart3:
      return receive ? DmaRequest::kUsart3Rx : DmaRequest::kUsart3Tx;
    case UartPeripheral::kUart4:
      return receive ? DmaRequest::kUart4Rx : DmaRequest::kUart4Tx;
#endif
#if defined(STM32F767xx)
    case UartPeripheral::kUsart6:
      return receive ? DmaRequest::kUsart6Rx : DmaRequest::kUsart6Tx;
#endif
    default:
      return receive ? DmaRequest::kUsart2Rx : DmaRequest::kUsart2Tx;
  }
}

bool Uart::SendDma(interface::UartTxMessage message) {
  using interface::DmaStatus;
  if (!initialized_ || tx_dma_status_ == DmaStatus::kBusy || message.empty() ||
      message.size() > 65535 ||
      !detail::Dma::PrepareBuffer(message.data(), message.size(), false)) {
    return false;
  }
  if (!tx_dma_.IsInitialized() &&
      !tx_dma_.Initialize(DmaRequestFor(false), false, false,
                          interrupt_priority_)) {
    return false;
  }
  __HAL_LINKDMA(&handle_, hdmatx, tx_dma_.GetHandle());
  tx_dma_status_ = DmaStatus::kBusy;
  if (HAL_UART_Transmit_DMA(&handle_, message.data(),
                            static_cast<uint16_t>(message.size())) != HAL_OK) {
    tx_dma_status_ = DmaStatus::kError;
    tx_dma_.Release();
    handle_.hdmatx = nullptr;
    return false;
  }
  return true;
}

bool Uart::ReceiveDma(interface::UartRxMessage message) {
  using interface::DmaStatus;
  if (!initialized_ || rx_interrupt_enabled_ ||
      rx_dma_status_ == DmaStatus::kBusy || message.empty() ||
      message.size() > 65535 ||
      !detail::Dma::PrepareBuffer(message.data(), message.size(), true)) {
    return false;
  }
  if (!rx_dma_.IsInitialized() &&
      !rx_dma_.Initialize(DmaRequestFor(true), true, false,
                          interrupt_priority_)) {
    return false;
  }
  __HAL_LINKDMA(&handle_, hdmarx, rx_dma_.GetHandle());
  rx_dma_buffer_ = message;
  rx_dma_status_ = DmaStatus::kBusy;
  if (HAL_UART_Receive_DMA(&handle_, message.data(),
                           static_cast<uint16_t>(message.size())) != HAL_OK) {
    rx_dma_status_ = DmaStatus::kError;
    rx_dma_.Release();
    handle_.hdmarx = nullptr;
    rx_dma_buffer_ = {};
    return false;
  }
  return true;
}

interface::DmaStatus Uart::GetTxDmaStatus() const {
  const auto status = tx_dma_status_;
  __DMB();
  return status;
}

interface::DmaStatus Uart::GetRxDmaStatus() const {
  const auto status = rx_dma_status_;
  __DMB();
  return status;
}

Uart* Uart::FindHandle(UART_HandleTypeDef* handle) {
  if (handle == nullptr) {
    return nullptr;
  }
  // Validate both the peripheral and the exact owner handle.
  for (unsigned index = 0; index <= static_cast<unsigned>(
#if defined(STM32F303x8)
                                        UartPeripheral::kUsart2
#elif defined(STM32F767xx)
                                        UartPeripheral::kUsart6
#else
                                        UartPeripheral::kUart4
#endif
                                    );
       ++index) {
    Uart* owner = InstanceFor(static_cast<UartPeripheral>(index));
    if (owner != nullptr && &owner->handle_ == handle && owner->initialized_) {
      return owner;
    }
  }
  return nullptr;
}

void Uart::HandleTxComplete(UART_HandleTypeDef* handle) {
  Uart* owner = FindHandle(handle);
  if (owner != nullptr &&
      owner->tx_dma_status_ == interface::DmaStatus::kBusy) {
    __DMB();
    owner->tx_dma_status_ = interface::DmaStatus::kComplete;
  }
}

void Uart::HandleError(UART_HandleTypeDef* handle) {
  Uart* owner = FindHandle(handle);
  if (owner == nullptr) {
    return;
  }
  // A UART line error concerns reception. A TX DMA fault is reported by HAL
  // with HAL_UART_ERROR_DMA. Stop the affected transfer before releasing data.
  if ((handle->ErrorCode & HAL_UART_ERROR_DMA) != 0U &&
      owner->tx_dma_status_ == interface::DmaStatus::kBusy) {
    (void)HAL_UART_AbortTransmit(handle);
    owner->tx_dma_status_ = interface::DmaStatus::kError;
  }
  if (owner->rx_dma_status_ == interface::DmaStatus::kBusy) {
    (void)HAL_UART_AbortReceive(handle);
    detail::Dma::CompleteReceive(owner->rx_dma_buffer_.data(),
                                 owner->rx_dma_buffer_.size());
    owner->rx_dma_buffer_ = {};
    owner->rx_dma_status_ = interface::DmaStatus::kError;
  }
}

void Uart::StopDma() {
  using interface::DmaStatus;
  if (tx_dma_.IsInitialized()) {
    if (tx_dma_status_ == DmaStatus::kBusy) {
      (void)HAL_UART_AbortTransmit(&handle_);
    }
    tx_dma_.Release();
    handle_.hdmatx = nullptr;
    if (tx_dma_status_ == DmaStatus::kBusy) {
      tx_dma_status_ = DmaStatus::kAborted;
    }
  }
  if (rx_dma_.IsInitialized()) {
    if (rx_dma_status_ == DmaStatus::kBusy) {
      (void)HAL_UART_AbortReceive(&handle_);
      detail::Dma::CompleteReceive(rx_dma_buffer_.data(),
                                   rx_dma_buffer_.size());
    }
    rx_dma_.Release();
    handle_.hdmarx = nullptr;
    rx_dma_buffer_ = {};
    if (rx_dma_status_ == DmaStatus::kBusy) {
      rx_dma_status_ = DmaStatus::kAborted;
    }
  }
}
