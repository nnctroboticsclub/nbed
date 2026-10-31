// Included by uart_test.cc after Unit and the RX/TX pin expectations.
struct FlowCase {
  UartPeripheral peripheral;
  PinName pin;
  bool cts;
  uint32_t alternate;
};

#include "uart_flow_cases.h"

UartConfig ConfigFor(const Unit& unit) {
  UartConfig config;
  config.peripheral = unit.peripheral;
  config.rx = Pins(unit.peripheral, true).front().pin;
  config.tx = Pins(unit.peripheral, false).front().pin;
  return config;
}

void RejectConfig(const UartConfig& config) {
  const auto calls = uart_init_calls;
  const auto pins = gpio_inits.size();
  Uart uart(config);
  assert(!uart.Initialize());
  assert(calls == uart_init_calls && pins == gpio_inits.size());
}

void TestBasicConfig(const Unit& unit) {
  const std::array lengths{UART_WORDLENGTH_8B, UART_WORDLENGTH_9B,
#if defined(UART_WORDLENGTH_7B)
      UART_WORDLENGTH_7B,
#endif
  };
  const std::array stops{UART_STOPBITS_1, UART_STOPBITS_2,
#if defined(UART_STOPBITS_0_5)
      UART_STOPBITS_0_5, UART_STOPBITS_1_5,
#endif
  };
  for (auto length : lengths) {
    for (auto stop : stops) {
      for (auto parity : {UART_PARITY_NONE, UART_PARITY_EVEN, UART_PARITY_ODD}) {
        for (auto sampling : {UART_OVERSAMPLING_8, UART_OVERSAMPLING_16}) {
          auto config = ConfigFor(unit);
          config.hal.WordLength = length;
          config.hal.StopBits = stop;
          config.hal.Parity = parity;
          config.hal.OverSampling = sampling;
          config.hal.BaudRate = 100000;
          config.send_timeout = 37;
          config.receive_timeout = 51;
          config.gpio_pull = GPIO_PULLUP;
          config.gpio_speed = GPIO_SPEED_FREQ_MEDIUM;
#if defined(STM32F446xx)
          config.one_bit_sampling = true;
#else
          config.hal.OneBitSampling = UART_ONE_BIT_SAMPLE_ENABLE;
#endif
          gpio_inits.clear();
          Uart uart(config);
          assert(uart.Initialize());
          const auto& actual = uart.GetHandle().Init;
          assert(actual.BaudRate == 100000 && actual.WordLength == length);
          assert(actual.StopBits == stop && actual.Parity == parity);
          assert(actual.OverSampling == sampling);
          assert(actual.OneBitSampling == UART_ONE_BIT_SAMPLE_ENABLE);
          assert(gpio_inits[0].second.Pull == GPIO_PULLUP);
          assert(gpio_inits[1].second.Speed == GPIO_SPEED_FREQ_MEDIUM);
          std::array<uint8_t, 2> bytes{};
          std::array<uint16_t, 2> words{};
          if (length == UART_WORDLENGTH_9B && parity == UART_PARITY_NONE) {
            assert(uart.SendWords(words) && uart.ReceiveWords(words));
          } else {
            assert(uart.Send(bytes) && uart.Receive(bytes));
            assert(!uart.SendWords(words) && !uart.ReceiveWords(words));
          }
          assert(uart_tx_timeout == 37 && uart_rx_timeout == 51);
        }
      }
    }
  }
  for (auto mode : {UART_MODE_TX, UART_MODE_RX}) {
    auto config = ConfigFor(unit);
    config.hal.Mode = mode;
    if (mode == UART_MODE_TX) {
      config.rx = kNC;
    } else {
      config.tx = kNC;
    }
    Uart uart(config);
    assert(uart.Initialize());
    std::array<uint8_t, 2> bytes{};
    assert(uart.Send(bytes) == (mode == UART_MODE_TX));
    assert(uart.Receive(bytes) == (mode == UART_MODE_RX));
    assert(uart.SendDma(bytes) == (mode == UART_MODE_TX));
    assert(uart.ReceiveDma(bytes) == (mode == UART_MODE_RX));
    uart.StopDma();
    assert(uart.EnableRxInterrupt() == (mode == UART_MODE_RX));
  }
  for (auto field : {0, 1, 2, 3, 4, 5, 6, 7, 8, 9}) {
    auto config = ConfigFor(unit);
    switch (field) {
      case 0:
        config.hal.BaudRate = 0;
        break;
      case 1:
        config.hal.WordLength = 0xdead;
        break;
      case 2:
        config.hal.StopBits = 0xdead;
        break;
      case 3:
        config.hal.Parity = 0xdead;
        break;
      case 4:
        config.hal.Mode = 0;
        break;
      case 5:
        config.hal.HwFlowCtl = 0xdead;
        break;
      case 6:
        config.hal.OverSampling = 0xdead;
        break;
      case 7:
        config.gpio_pull = 0xdead;
        break;
      case 8:
        config.gpio_speed = 0xdead;
        break;
      case 9:
        config.hal.BaudRate = 0xffffffffU;
        break;
    }
    RejectConfig(config);
  }
}

void TestFlowConfig(const Unit& unit) {
  std::vector<FlowCase> cts, rts;
  for (const auto& pin : kFlowCases) {
    if (pin.peripheral == unit.peripheral) {
      (pin.cts ? cts : rts).push_back(pin);
    }
  }
  for (const auto& input : cts) {
    for (const auto& output : rts) {
      auto config = ConfigFor(unit);
      config.cts = input.pin;
      config.rts = output.pin;
      config.hal.HwFlowCtl = UART_HWCONTROL_RTS_CTS;
      gpio_inits.clear();
      Uart uart(config);
      assert(uart.Initialize() && gpio_inits.size() == 4);
      assert(uart.GetHandle().Init.HwFlowCtl == UART_HWCONTROL_RTS_CTS);
      const auto& rx = gpio_inits[2];
      const auto& tx = gpio_inits[3];
      assert(rx.first == PinMapper::GetGpioPort(input.pin));
      assert(rx.second.Pin == PinMapper::GetGpioPin(input.pin));
      assert(rx.second.Alternate == input.alternate);
      assert(tx.first == PinMapper::GetGpioPort(output.pin));
      assert(tx.second.Pin == PinMapper::GetGpioPin(output.pin));
      assert(tx.second.Alternate == output.alternate);
    }
  }
  for (bool input : {false, true}) {
    const auto& pins = input ? cts : rts;
    for (unsigned value = 0; value < 256; ++value) {
      auto config = ConfigFor(unit);
      config.hal.HwFlowCtl = input ? UART_HWCONTROL_CTS : UART_HWCONTROL_RTS;
      (input ? config.cts : config.rts) = static_cast<PinName>(value);
      const bool expected = std::any_of(pins.begin(), pins.end(),
          [&](const auto& pin) { return pin.pin == static_cast<PinName>(value); });
      Uart uart(config);
      assert(uart.Initialize() == expected);
    }
  }
  auto missing = ConfigFor(unit);
  missing.hal.HwFlowCtl = UART_HWCONTROL_RTS_CTS;
  RejectConfig(missing);
  if (!cts.empty()) {
    auto unused = ConfigFor(unit);
    unused.cts = cts.front().pin;
    RejectConfig(unused);
  }
}

#if !defined(STM32F446xx)
void TestAdvancedConfig(const Unit& unit) {
  {
    auto config = ConfigFor(unit);
    auto& advanced = config.advanced;
    advanced.AdvFeatureInit = UART_ADVFEATURE_TXINVERT_INIT |
        UART_ADVFEATURE_RXINVERT_INIT | UART_ADVFEATURE_DATAINVERT_INIT |
        UART_ADVFEATURE_SWAP_INIT | UART_ADVFEATURE_RXOVERRUNDISABLE_INIT |
        UART_ADVFEATURE_DMADISABLEONERROR_INIT | UART_ADVFEATURE_MSBFIRST_INIT;
    advanced.TxPinLevelInvert = UART_ADVFEATURE_TXINV_ENABLE;
    advanced.RxPinLevelInvert = UART_ADVFEATURE_RXINV_ENABLE;
    advanced.DataInvert = UART_ADVFEATURE_DATAINV_ENABLE;
    advanced.Swap = UART_ADVFEATURE_SWAP_ENABLE;
    advanced.OverrunDisable = UART_ADVFEATURE_OVERRUN_DISABLE;
    advanced.DMADisableonRxError = UART_ADVFEATURE_DMA_DISABLEONRXERROR;
    advanced.MSBFirst = UART_ADVFEATURE_MSBFIRST_ENABLE;
    Uart uart(config);
    assert(uart.Initialize());
    const auto& actual = uart.GetHandle().AdvancedInit;
    assert(actual.AdvFeatureInit == advanced.AdvFeatureInit);
    assert(actual.TxPinLevelInvert == advanced.TxPinLevelInvert);
    assert(actual.RxPinLevelInvert == advanced.RxPinLevelInvert);
    assert(actual.DataInvert == advanced.DataInvert);
    assert(actual.Swap == advanced.Swap);
    assert(actual.OverrunDisable == advanced.OverrunDisable);
    assert(actual.DMADisableonRxError == advanced.DMADisableonRxError);
    assert(actual.MSBFirst == advanced.MSBFirst);
  }
  for (auto mode : {UART_ADVFEATURE_AUTOBAUDRATE_ONSTARTBIT,
                    UART_ADVFEATURE_AUTOBAUDRATE_ONFALLINGEDGE,
                    UART_ADVFEATURE_AUTOBAUDRATE_ON0X7FFRAME,
                    UART_ADVFEATURE_AUTOBAUDRATE_ON0X55FRAME}) {
    auto config = ConfigFor(unit);
    config.advanced.AdvFeatureInit = UART_ADVFEATURE_AUTOBAUDRATE_INIT;
    config.advanced.AutoBaudRateEnable = UART_ADVFEATURE_AUTOBAUDRATE_ENABLE;
    config.advanced.AutoBaudRateMode = mode;
#ifdef STM32F303x8
    const bool capable = unit.number == 1;
#else
    const bool capable = unit.number == 1 || unit.number == 2 ||
                         unit.number == 3 || unit.number == 6;
#endif
    Uart uart(config);
    assert(uart.Initialize() == capable);
    if (capable) {
      assert(uart.GetHandle().AdvancedInit.AutoBaudRateMode == mode);
    }
  }
  for (auto mode : {UART_MODE_TX, UART_MODE_RX}) {
    auto config = ConfigFor(unit);
    config.hal.Mode = mode;
    config.advanced.AdvFeatureInit = UART_ADVFEATURE_SWAP_INIT;
    config.advanced.Swap = UART_ADVFEATURE_SWAP_ENABLE;
    if (mode == UART_MODE_TX) {
      config.tx = kNC;
    } else {
      config.rx = kNC;
    }
    Uart uart(config);
    assert(uart.Initialize());
  }
  for (unsigned field = 0; field < 12; ++field) {
    auto config = ConfigFor(unit);
    auto& init = config.advanced;
    // Nonzero fields with omitted flags, unknown flags and invalid values.
    switch (field) {
      case 0:
        init.AdvFeatureInit = 0x100;
        break;
      case 1:
        init.TxPinLevelInvert = UART_ADVFEATURE_TXINV_ENABLE;
        break;
      case 2:
        init.RxPinLevelInvert = UART_ADVFEATURE_RXINV_ENABLE;
        break;
      case 3:
        init.DataInvert = UART_ADVFEATURE_DATAINV_ENABLE;
        break;
      case 4:
        init.Swap = UART_ADVFEATURE_SWAP_ENABLE;
        break;
      case 5:
        init.OverrunDisable = UART_ADVFEATURE_OVERRUN_DISABLE;
        break;
      case 6:
        init.DMADisableonRxError = UART_ADVFEATURE_DMA_DISABLEONRXERROR;
        break;
      case 7:
        init.MSBFirst = UART_ADVFEATURE_MSBFIRST_ENABLE;
        break;
      case 8:
        init.AutoBaudRateEnable = UART_ADVFEATURE_AUTOBAUDRATE_ENABLE;
        break;
      case 9:
        init.AutoBaudRateMode = UART_ADVFEATURE_AUTOBAUDRATE_ON0X55FRAME;
        break;
      case 10:
        config.hal.OneBitSampling = 0xdead;
        break;
      case 11:
        init.AdvFeatureInit = UART_ADVFEATURE_RXINVERT_INIT;
        init.RxPinLevelInvert = 0xdead;
        break;
    }
    RejectConfig(config);
  }
}
#endif

void TestWordTransfers(const Unit& unit) {
  auto config = ConfigFor(unit);
  config.hal.WordLength = UART_WORDLENGTH_9B;
  config.send_timeout = 17;
  config.receive_timeout = 23;
  alignas(32) std::array<uint16_t, 16> tx{0x1ff, 0x100, 0x55}, rx{};
  Uart uart(config);
  assert(uart.Initialize());
  std::array<uint8_t, 2> bytes{};
  assert(!uart.Send(bytes) && !uart.Receive(bytes));
  assert(!uart.SendDma(bytes) && !uart.ReceiveDma(bytes));
  tx_data.clear();
  assert(uart.SendWords(tx));
  assert(uart_transfer_count == tx.size() && uart_tx_timeout == 17);
  assert(tx_data.size() == sizeof(tx));
  assert(tx_data[0] == 0xff && tx_data[1] == 1);
  uart_receive_words.assign(tx.begin(), tx.end());
  assert(uart.ReceiveWords(rx) && rx == tx);
  assert(uart_transfer_count == rx.size() && uart_rx_timeout == 23);
  uart_receive_words.clear();
  tx[0] = 0x200;
  assert(!uart.SendWords(tx) && !uart.SendWordsDma(tx));
  tx[0] = 0x1ff;
  assert(!uart.SendWords({}) && !uart.ReceiveWords({}));
  assert(!uart.SendWordsDma({}) && !uart.ReceiveWordsDma({}));
  alignas(2) std::array<uint8_t, 4> unaligned{};
  auto* unaligned_words = reinterpret_cast<uint16_t*>(unaligned.data() + 1);
  assert(!uart.SendWords({unaligned_words, 1}));
  assert(!uart.ReceiveWords({unaligned_words, 1}));
  assert(!uart.SendWordsDma({unaligned_words, 1}));
  assert(!uart.ReceiveWordsDma({unaligned_words, 1}));
#ifdef STM32F767xx
  SCB->CCR |= SCB_CCR_DC_Msk;
#endif
  assert(uart.SendWordsDma(tx) && uart.ReceiveWordsDma(rx));
  assert(uart_dma_tx_size == tx.size() && uart_dma_rx_size == rx.size());
  assert(uart.GetHandle().hdmatx->Init.MemDataAlignment == DMA_MDATAALIGN_HALFWORD);
  assert(uart.GetHandle().hdmarx->Init.PeriphDataAlignment == DMA_PDATAALIGN_HALFWORD);
  HAL_UART_TxCpltCallback(&uart.GetHandle());
  HAL_UART_RxCpltCallback(&uart.GetHandle());
  assert(uart.GetTxDmaStatus() == kComplete && uart.GetRxDmaStatus() == kComplete);
#ifdef STM32F767xx
  assert(cache_invalidate_bytes == static_cast<int32_t>(sizeof(rx)));
  SCB->CCR = 0;
#endif
  uart.StopDma();
  unsigned word_calls = 0, byte_calls = 0;
  uart.SetRxCallback([&](auto) { ++byte_calls; });
  uart.SetRxWordCallback([&](auto message) {
    assert(message.size() == 1 && message[0] == 0x1af);
    ++word_calls;
  });
  assert(uart.EnableRxInterrupt());
  *reinterpret_cast<uint16_t*>(uart_it_data) = 0x1af;
  const auto calls = uart_it_calls;
  HAL_UART_RxCpltCallback(&uart.GetHandle());
  assert(word_calls == 1 && byte_calls == 0 && uart_it_calls == calls + 1);
}
