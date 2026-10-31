// Included inside the selected backend namespace in uart.cc.
bool Uart::SendWords(interface::UartWordTxMessage message) {
  if (!initialized_ || (config_.hal.Mode & UART_MODE_TX) == 0 ||
      !detail::IsUartWordTransfer(config_.hal) ||
      tx_dma_status_ == interface::DmaStatus::kBusy || message.empty() ||
      message.size() > 65535 ||
      (reinterpret_cast<uintptr_t>(message.data()) & 1U) != 0 ||
      !std::all_of(message.begin(), message.end(),
                   [](uint16_t word) { return word <= 0x1ffU; })) {
    return false;
  }
  return HAL_UART_Transmit(&handle_,
      reinterpret_cast<uint8_t*>(const_cast<uint16_t*>(message.data())),
      static_cast<uint16_t>(message.size()), config_.send_timeout) == HAL_OK;
}

bool Uart::ReceiveWords(interface::UartWordRxMessage message) {
  if (!initialized_ || (config_.hal.Mode & UART_MODE_RX) == 0 ||
      !detail::IsUartWordTransfer(config_.hal) || rx_interrupt_enabled_ ||
      rx_dma_status_ == interface::DmaStatus::kBusy || message.empty() ||
      message.size() > 65535 ||
      (reinterpret_cast<uintptr_t>(message.data()) & 1U) != 0) {
    return false;
  }
  return HAL_UART_Receive(&handle_, reinterpret_cast<uint8_t*>(message.data()),
      static_cast<uint16_t>(message.size()), config_.receive_timeout) == HAL_OK;
}

void Uart::SetRxWordCallback(const interface::UartWordRxCallback& callback) {
  rx_word_callback_ = callback;
}
