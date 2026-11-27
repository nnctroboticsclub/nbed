// 対応する MCU の名前空間内で can.cc から読み込む。
namespace {

struct CanHardware {
  CAN_TypeDef* instance;
  IRQn_Type irq;
  uint8_t first_bank;
};

const std::array kHardware{
#if defined(STM32F303x8)
    CanHardware{CAN, CAN_RX0_IRQn, 0},
#else
    CanHardware{CAN1, CAN1_RX0_IRQn, 0},
    CanHardware{CAN2, CAN2_RX0_IRQn, 14},
#if defined(STM32F767xx)
    CanHardware{CAN3, CAN3_RX0_IRQn, 0},
#endif
#endif
};
std::array<Can*, kHardware.size()> owners{};

constexpr uint8_t kBanksPerController = 14;
#if defined(STM32F303x8)
constexpr uint8_t kCan2StartBank = 0;
constexpr uint32_t kGpioSpeed = GPIO_SPEED_FREQ_HIGH;
#else
constexpr uint8_t kCan2StartBank = kBanksPerController;
constexpr uint32_t kGpioSpeed = GPIO_SPEED_FREQ_VERY_HIGH;
#endif

CAN_FilterTypeDef MakeFilter(uint8_t bank, bool enabled) {
  CAN_FilterTypeDef filter{};
  filter.FilterBank = bank;
  filter.FilterMode = CAN_FILTERMODE_IDMASK;
  filter.FilterScale = CAN_FILTERSCALE_32BIT;
  filter.FilterFIFOAssignment = CAN_RX_FIFO0;
  filter.FilterActivation = enabled ? ENABLE : DISABLE;
  filter.SlaveStartFilterBank = kCan2StartBank;
  return filter;
}

uint32_t ToHalMode(CanMode mode) {
  switch (mode) {
    case CanMode::kLoopback:
      return CAN_MODE_LOOPBACK;
    case CanMode::kSilent:
      return CAN_MODE_SILENT;
    case CanMode::kSilentLoopback:
      return CAN_MODE_SILENT_LOOPBACK;
    case CanMode::kNormal:
    default:
      return CAN_MODE_NORMAL;
  }
}

void DecodeHeader(const CAN_RxHeaderTypeDef& header,
                  interface::CanMessage& message) {
  message.id = header.IDE == CAN_ID_STD ? header.StdId : header.ExtId;
  message.size = static_cast<uint8_t>(std::min<uint32_t>(header.DLC, 8U));
  message.format = header.IDE == CAN_ID_STD ? interface::CanFormat::kStandard
                                            : interface::CanFormat::kExtended;
  message.type = header.RTR == CAN_RTR_DATA ? interface::CanFrameType::kData
                                            : interface::CanFrameType::kRemote;
}

}  // namespace

Can::Can(CanConfig config) : config_(config) {}

Can::Can(CanPeripheral peripheral, interface::PinName rx, interface::PinName tx,
         uint32_t bitrate)
    : Can(CanConfig{
          .peripheral = peripheral,
          .rx = rx,
          .tx = tx,
          .bitrate = bitrate,
      }) {}

Can::~Can() { Stop(); }

bool Can::Initialize() {
  if (initialized_) {
    return true;
  }
  if (!IsValidPinPair() || config_.interrupt_priority > 15U ||
      config_.mode > CanMode::kSilentLoopback || !OwnPeripheral()) {
    return false;
  }

  handle_.Instance =
      kHardware[static_cast<unsigned>(config_.peripheral)].instance;
  handle_.Init.Mode = ToHalMode(config_.mode);
  handle_.Init.SyncJumpWidth = CAN_SJW_1TQ;
  handle_.Init.TimeTriggeredMode = DISABLE;
  handle_.Init.AutoBusOff = DISABLE;
  handle_.Init.AutoWakeUp = DISABLE;
  handle_.Init.AutoRetransmission = ENABLE;
  handle_.Init.ReceiveFifoLocked = DISABLE;
  handle_.Init.TransmitFifoPriority = DISABLE;
  if (!ConfigureBitTiming()) {
    ReleasePeripheral();
    return false;
  }
  if (HAL_CAN_Init(&handle_) != HAL_OK) {
    // HAL_CAN_Init() has already run the MSP hook at this point. Deinitialize
    // it so a failed initialization does not leave pins or the RX IRQ claimed.
    (void)HAL_CAN_DeInit(&handle_);
    ReleasePeripheral();
    return false;
  }
  // 最初は標準・拡張 ID ともすべて受け入れる。
  auto accept_all = MakeFilter(DefaultFilterBank(), true);
  if (HAL_CAN_ConfigFilter(&handle_, &accept_all) != HAL_OK) {
    (void)HAL_CAN_DeInit(&handle_);
    ReleasePeripheral();
    return false;
  }
  configured_filters_ = 1UL << DefaultFilterBank();
  accepting_all_ = true;
  initialized_ = true;
  return true;
}

bool Can::Start() {
  if (!initialized_ && !Initialize()) {
    return false;
  }
  if (started_) {
    return true;
  }
  if (HAL_CAN_Start(&handle_) != HAL_OK) {
    return false;
  }
  started_ = true;
  return true;
}

void Can::Stop() {
  if (!initialized_) {
    return;
  }
  if (started_) {
    started_ = false;
    (void)HAL_CAN_Stop(&handle_);
  }
  const auto first_bank = DefaultFilterBank();
  for (uint8_t bank = first_bank; bank < first_bank + kBanksPerController;
       ++bank) {
    if ((configured_filters_ & (1UL << bank)) != 0U) {
      (void)DisableFilter(bank);
    }
  }
  configured_filters_ = 0;
  accepting_all_ = false;
  (void)HAL_CAN_DeInit(&handle_);
  initialized_ = false;
  ReleasePeripheral();
}

bool Can::Send(const interface::CanMessage& message) {
  const uint32_t max_id =
      message.format == interface::CanFormat::kStandard ? 0x7ffU : 0x1fffffffU;
  if (!started_ || message.format > interface::CanFormat::kExtended ||
      message.type > interface::CanFrameType::kRemote ||
      message.size > message.data.size() || message.id > max_id) {
    return false;
  }

  CAN_TxHeaderTypeDef header{};
  header.StdId = message.id;
  header.ExtId = message.id;
  header.IDE = message.format == interface::CanFormat::kStandard ? CAN_ID_STD
                                                                 : CAN_ID_EXT;
  header.RTR = message.type == interface::CanFrameType::kData ? CAN_RTR_DATA
                                                              : CAN_RTR_REMOTE;
  header.DLC = message.size;
  header.TransmitGlobalTime = DISABLE;
  uint32_t mailbox = 0;
  return HAL_CAN_AddTxMessage(&handle_, &header, message.data.data(),
                              &mailbox) == HAL_OK;
}

bool Can::Receive(interface::CanMessage& message) {
  if (!started_) {
    return false;
  }
  CAN_RxHeaderTypeDef header{};
  if (HAL_CAN_GetRxMessage(&handle_, CAN_RX_FIFO0, &header,
                           message.data.data()) != HAL_OK) {
    return false;
  }
  DecodeHeader(header, message);
  return true;
}

bool Can::SetRxFilter(const CanFilter& filter) {
  if (!initialized_ || filter.fifo != CAN_RX_FIFO0 ||
      filter.format > interface::CanFormat::kExtended ||
      filter.id > (filter.format == interface::CanFormat::kStandard
                       ? 0x7ffU
                       : 0x1fffffffU) ||
      filter.mask > (filter.format == interface::CanFormat::kStandard
                         ? 0x7ffU
                         : 0x1fffffffU)) {
    return false;
  }
  const uint8_t bank = filter.bank == 0xff ? DefaultFilterBank() : filter.bank;
  const uint8_t first_bank = DefaultFilterBank();
  if (bank < first_bank || bank >= first_bank + kBanksPerController) {
    return false;
  }

  auto hal_filter = MakeFilter(bank, true);
  if (filter.format == interface::CanFormat::kStandard) {
    hal_filter.FilterIdHigh = filter.id << 5;
    hal_filter.FilterMaskIdHigh = filter.mask << 5;
    // Require IDE=0 as well, so a standard-ID filter never admits an
    // extended frame with matching top bits.
    hal_filter.FilterMaskIdLow = CAN_ID_EXT;
  } else {
    hal_filter.FilterIdHigh = filter.id >> 13;
    hal_filter.FilterIdLow = (filter.id << 3) | CAN_ID_EXT;
    hal_filter.FilterMaskIdHigh = filter.mask >> 13;
    hal_filter.FilterMaskIdLow = (filter.mask << 3) | CAN_ID_EXT;
  }
  if (HAL_CAN_ConfigFilter(&handle_, &hal_filter) != HAL_OK) {
    return false;
  }
  configured_filters_ |= 1UL << bank;
  if (accepting_all_ && bank != first_bank && !DisableFilter(first_bank)) {
    (void)DisableFilter(bank);
    return false;
  }
  accepting_all_ = false;
  return true;
}

bool Can::DisableFilter(uint8_t bank) {
  auto filter = MakeFilter(bank, false);
  if (HAL_CAN_ConfigFilter(&handle_, &filter) != HAL_OK) {
    return false;
  }
  configured_filters_ &= ~(uint32_t{1} << bank);
  return true;
}

bool Can::SetRxFilter(const interface::CanRxFilter& filter) {
  return SetRxFilter(CanFilter{
      .id = filter.id,
      .mask = filter.mask,
      .format = filter.format,
  });
}

bool Can::SetRxFilter(uint32_t id, uint32_t mask) {
  return SetRxFilter(interface::CanRxFilter{.id = id, .mask = mask});
}

bool Can::EnableRxInterrupt() {
  return initialized_ && HAL_CAN_ActivateNotification(
                             &handle_, CAN_IT_RX_FIFO0_MSG_PENDING) == HAL_OK;
}

void Can::SetRxCallback(const interface::CanRxCallback& callback) {
  rx_callback_ = callback;
}

uint8_t Can::GetSendErrorCount() const {
  if (!initialized_ || handle_.Instance == nullptr) {
    return 0;
  }
  return static_cast<uint8_t>((handle_.Instance->ESR & CAN_ESR_TEC) >>
                              CAN_ESR_TEC_Pos);
}

uint8_t Can::GetReceiveErrorCount() const {
  if (!initialized_ || handle_.Instance == nullptr) {
    return 0;
  }
  return static_cast<uint8_t>((handle_.Instance->ESR & CAN_ESR_REC) >>
                              CAN_ESR_REC_Pos);
}

CAN_HandleTypeDef& Can::GetHandle() { return handle_; }

void Can::HandleInterrupt(CanPeripheral peripheral) {
  Can* const can = InstanceFor(peripheral);
  if (can != nullptr && can->initialized_) {
    HAL_CAN_IRQHandler(&can->handle_);
  }
}

void Can::HandleRxPending(CAN_HandleTypeDef* handle) {
  if (Can* const can = FromHandle(handle); can != nullptr) {
    can->OnRxPending();
  }
}

void Can::ConfigureMsp(CAN_HandleTypeDef* handle) {
  Can* const can = FromHandle(handle);
  if (can == nullptr) {
    return;
  }
  const auto index = static_cast<unsigned>(can->config_.peripheral);
  const auto number = static_cast<uint8_t>(index + 1U);
  const auto* rx =
      detail::FindPinRoute(kCanPins, number, PinSignal::kRx, can->config_.rx);
  const auto* tx =
      detail::FindPinRoute(kCanPins, number, PinSignal::kTx, can->config_.tx);
  if (rx == nullptr || tx == nullptr) {
    return;
  }

  switch (can->config_.peripheral) {
    case CanPeripheral::kCan1:
      __HAL_RCC_CAN1_CLK_ENABLE();
      break;
#if !defined(STM32F303x8)
    case CanPeripheral::kCan2:
      // CAN2 のフィルターは CAN1 と共有する。
      __HAL_RCC_CAN1_CLK_ENABLE();
      __HAL_RCC_CAN2_CLK_ENABLE();
      break;
#endif
#if defined(STM32F767xx)
    case CanPeripheral::kCan3:
      __HAL_RCC_CAN3_CLK_ENABLE();
      break;
#endif
  }

  GPIO_InitTypeDef gpio{};
  gpio.Mode = GPIO_MODE_AF_PP;
  gpio.Pull = GPIO_NOPULL;
  gpio.Speed = kGpioSpeed;
  gpio.Alternate = rx->value;
  (void)PinMapper::ConfigureGpio(rx->pin, gpio);
  gpio.Alternate = tx->value;
  (void)PinMapper::ConfigureGpio(tx->pin, gpio);

  const IRQn_Type irq = kHardware[index].irq;
  HAL_NVIC_SetPriority(irq, can->config_.interrupt_priority, 0);
  HAL_NVIC_EnableIRQ(irq);
}

void Can::DeconfigureMsp(CAN_HandleTypeDef* handle) {
  Can* const can = FromHandle(handle);
  if (can == nullptr) {
    return;
  }
  for (const auto pin : {can->config_.rx, can->config_.tx}) {
    HAL_GPIO_DeInit(PinMapper::GetGpioPort(pin), PinMapper::GetGpioPin(pin));
  }
  const auto index = static_cast<unsigned>(can->config_.peripheral);
  HAL_NVIC_DisableIRQ(kHardware[index].irq);
  switch (can->config_.peripheral) {
    case CanPeripheral::kCan1:
#if !defined(STM32F303x8)
      if (owners[1] != nullptr) {
        break;
      }
#endif
      __HAL_RCC_CAN1_CLK_DISABLE();
      break;
#if !defined(STM32F303x8)
    case CanPeripheral::kCan2:
      __HAL_RCC_CAN2_CLK_DISABLE();
      if (owners[0] == nullptr) {
        __HAL_RCC_CAN1_CLK_DISABLE();
      }
      break;
#endif
#if defined(STM32F767xx)
    case CanPeripheral::kCan3:
      __HAL_RCC_CAN3_CLK_DISABLE();
      break;
#endif
  }
}

bool Can::IsValidPinPair() const {
  const auto index = static_cast<unsigned>(config_.peripheral);
  if (index >= kHardware.size() ||
      !PinMapper::IsValidPinPair(config_.rx, config_.tx)) {
    return false;
  }
  const auto number = static_cast<uint8_t>(index + 1U);
  return detail::FindPinRoute(kCanPins, number, PinSignal::kRx, config_.rx) &&
         detail::FindPinRoute(kCanPins, number, PinSignal::kTx, config_.tx);
}

bool Can::ConfigureBitTiming() {
  if (config_.bitrate == 0 || config_.bitrate > 1'000'000U) {
    return false;
  }
  const uint32_t can_clock = HAL_RCC_GetPCLK1Freq();
  for (uint32_t tq = 8; tq <= 25; ++tq) {
    const uint32_t divisor = config_.bitrate * tq;
    if (divisor == 0 || can_clock % divisor != 0) {
      continue;
    }
    const uint32_t prescaler = can_clock / divisor;
    const uint32_t seg2 = std::max<uint32_t>(2, (tq + 4U) / 5U);
    const uint32_t seg1 = tq - 1U - seg2;
    if (prescaler >= 1 && prescaler <= 1024 && seg1 >= 1 && seg1 <= 16 &&
        seg2 >= 1 && seg2 <= 8) {
      handle_.Init.Prescaler = prescaler;
      handle_.Init.TimeSeg1 = (seg1 - 1U) << CAN_BTR_TS1_Pos;  // bit16〜
      handle_.Init.TimeSeg2 = (seg2 - 1U) << CAN_BTR_TS2_Pos;  // bit20〜
      return true;
    }
  }
  return false;
}

bool Can::OwnPeripheral() {
  Can*& instance = InstanceFor(config_.peripheral);
  if (instance != nullptr && instance != this) {
    return false;
  }
  instance = this;
  return true;
}

void Can::ReleasePeripheral() {
  Can*& instance = InstanceFor(config_.peripheral);
  if (instance == this) {
    instance = nullptr;
  }
}

void Can::OnRxPending() {
  while (started_ && HAL_CAN_GetRxFifoFillLevel(&handle_, CAN_RX_FIFO0) > 0) {
    interface::CanMessage message{};
    if (!Receive(message)) {
      break;
    }
    if (rx_callback_) {
      rx_callback_(message);
    }
  }
}

uint8_t Can::DefaultFilterBank() const {
  return kHardware[static_cast<unsigned>(config_.peripheral)].first_bank;
}

Can*& Can::InstanceFor(CanPeripheral peripheral) {
  const auto index = static_cast<unsigned>(peripheral);
  static Can* invalid = nullptr;
  return index < owners.size() ? owners[index] : invalid;
}

Can* Can::FromHandle(CAN_HandleTypeDef* handle) {
  for (auto* owner : owners) {
    if (owner != nullptr && &owner->handle_ == handle) {
      return owner;
    }
  }
  return nullptr;
}

extern "C" {

void HAL_CAN_MspInit(CAN_HandleTypeDef* hcan) { Can::ConfigureMsp(hcan); }

void HAL_CAN_MspDeInit(CAN_HandleTypeDef* hcan) { Can::DeconfigureMsp(hcan); }

#if defined(STM32F303x8)
void CAN_RX0_IRQHandler() { Can::HandleInterrupt(CanPeripheral::kCan1); }
#else
void CAN1_RX0_IRQHandler() { Can::HandleInterrupt(CanPeripheral::kCan1); }

void CAN2_RX0_IRQHandler() { Can::HandleInterrupt(CanPeripheral::kCan2); }

#endif

#if defined(STM32F767xx)
void CAN3_RX0_IRQHandler() { Can::HandleInterrupt(CanPeripheral::kCan3); }
#endif

void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef* hcan) {
  Can::HandleRxPending(hcan);
}

}  // extern "C"
