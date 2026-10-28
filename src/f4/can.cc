#if defined(STM32F446xx)

#include "f4/can.h"

#include "f4/pin_mapper.h"
#include "stm32f4xx.h"
#include "stm32f4xx_hal_cortex.h"
#include "stm32f4xx_hal_gpio.h"
#include "stm32f4xx_hal_gpio_ex.h"
#include "stm32f4xx_hal_rcc.h"

namespace nkrc::nbed::stm32::f4 {
namespace {

Can*& Can1Instance() {
  static Can* instance = nullptr;
  return instance;
}

Can*& Can2Instance() {
  static Can* instance = nullptr;
  return instance;
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

bool IsCan1Pins(interface::PinName rx, interface::PinName tx) {
  return (rx == interface::PinName::kPA11 || rx == interface::PinName::kPB8) &&
         (tx == interface::PinName::kPA12 || tx == interface::PinName::kPB9);
}

bool IsCan2Pins(interface::PinName rx, interface::PinName tx) {
  return (rx == interface::PinName::kPB5 || rx == interface::PinName::kPB12) &&
         (tx == interface::PinName::kPB6 || tx == interface::PinName::kPB13);
}

void DisableGpioClock(GPIO_TypeDef* port) {
  // GPIO clocks may be shared by other nbed peripherals, so intentionally do
  // not disable them during CAN teardown.
  (void)port;
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
      !OwnPeripheral()) {
    return false;
  }

  handle_.Instance = config_.peripheral == CanPeripheral::kCan1 ? CAN1 : CAN2;
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
  initialized_ = true;

  // デフォルトは全フレーム受信（標準・拡張とも）。SetRxFilter()で上書き可能。
  CAN_FilterTypeDef accept_all{};
  accept_all.FilterBank = DefaultFilterBank();
  accept_all.FilterMode = CAN_FILTERMODE_IDMASK;
  accept_all.FilterScale = CAN_FILTERSCALE_32BIT;
  accept_all.FilterIdHigh = 0;
  accept_all.FilterIdLow = 0;
  accept_all.FilterMaskIdHigh = 0;  // マスク0 = 全ビットdon't care
  accept_all.FilterMaskIdLow = 0;
  accept_all.FilterFIFOAssignment = CAN_RX_FIFO0;
  accept_all.FilterActivation = ENABLE;
  accept_all.SlaveStartFilterBank = 14;
  if (HAL_CAN_ConfigFilter(&handle_, &accept_all) != HAL_OK) {
    (void)HAL_CAN_DeInit(&handle_);
    initialized_ = false;
    ReleasePeripheral();
    return false;
  }

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
    (void)HAL_CAN_Stop(&handle_);
  }
  (void)HAL_CAN_DeInit(&handle_);
  started_ = false;
  initialized_ = false;
  ReleasePeripheral();
}

bool Can::Send(const interface::CanMessage& message) {
  const uint32_t max_id =
      message.format == interface::CanFormat::kStandard ? 0x7ffU : 0x1fffffffU;
  if (!started_ || message.size > 8 || message.id > max_id) {
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
  message.id = header.IDE == CAN_ID_STD ? header.StdId : header.ExtId;
  message.size = static_cast<uint8_t>(header.DLC);
  message.format = header.IDE == CAN_ID_STD ? interface::CanFormat::kStandard
                                            : interface::CanFormat::kExtended;
  message.type = header.RTR == CAN_RTR_DATA ? interface::CanFrameType::kData
                                            : interface::CanFrameType::kRemote;
  return true;
}

bool Can::SetRxFilter(const CanFilter& filter) {
  if (!initialized_ || filter.fifo != CAN_RX_FIFO0 ||
      filter.id > (filter.format == interface::CanFormat::kStandard
                       ? 0x7ffU
                       : 0x1fffffffU) ||
      filter.mask > (filter.format == interface::CanFormat::kStandard
                         ? 0x7ffU
                         : 0x1fffffffU)) {
    return false;
  }
  const uint8_t bank = filter.bank == 0xff ? DefaultFilterBank() : filter.bank;
  if (bank >= 28 ||
      (config_.peripheral == CanPeripheral::kCan1 && bank >= 14) ||
      (config_.peripheral == CanPeripheral::kCan2 && bank < 14)) {
    return false;
  }

  CAN_FilterTypeDef hal_filter{};
  hal_filter.FilterBank = bank;
  hal_filter.FilterMode = CAN_FILTERMODE_IDMASK;
  hal_filter.FilterScale = CAN_FILTERSCALE_32BIT;
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
  hal_filter.FilterFIFOAssignment = filter.fifo;
  hal_filter.FilterActivation = ENABLE;
  hal_filter.SlaveStartFilterBank = 14;
  return HAL_CAN_ConfigFilter(&handle_, &hal_filter) == HAL_OK;
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
  if (PinMapper::GetGpioPort(can->config_.rx) == nullptr ||
      PinMapper::GetGpioPort(can->config_.tx) == nullptr) {
    return;
  }
  if (can->config_.peripheral == CanPeripheral::kCan1) {
    __HAL_RCC_CAN1_CLK_ENABLE();
  } else {
    // CAN2 shares the bxCAN filter block and its clock dependency with CAN1.
    __HAL_RCC_CAN1_CLK_ENABLE();
    __HAL_RCC_CAN2_CLK_ENABLE();
  }
  GPIO_InitTypeDef gpio{};
  gpio.Mode = GPIO_MODE_AF_PP;
  gpio.Pull = GPIO_NOPULL;
  gpio.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  gpio.Alternate = GPIO_AF9_CAN1;
  if (!PinMapper::ConfigureGpio(can->config_.rx, gpio) ||
      !PinMapper::ConfigureGpio(can->config_.tx, gpio)) {
    return;
  }

  const IRQn_Type irq = can->config_.peripheral == CanPeripheral::kCan1
                            ? CAN1_RX0_IRQn
                            : CAN2_RX0_IRQn;
  HAL_NVIC_SetPriority(irq, can->config_.interrupt_priority, 0);
  HAL_NVIC_EnableIRQ(irq);
}

void Can::DeconfigureMsp(CAN_HandleTypeDef* handle) {
  Can* const can = FromHandle(handle);
  if (can == nullptr) {
    return;
  }
  for (const auto pin : {can->config_.rx, can->config_.tx}) {
    GPIO_TypeDef* const port = PinMapper::GetGpioPort(pin);
    if (port != nullptr) {
      HAL_GPIO_DeInit(port, PinMapper::GetGpioPin(pin));
      DisableGpioClock(port);
    }
  }
  const IRQn_Type irq = can->config_.peripheral == CanPeripheral::kCan1
                            ? CAN1_RX0_IRQn
                            : CAN2_RX0_IRQn;
  HAL_NVIC_DisableIRQ(irq);
  if (can->config_.peripheral == CanPeripheral::kCan2) {
    __HAL_RCC_CAN2_CLK_DISABLE();
  } else if (Can2Instance() == nullptr || !Can2Instance()->initialized_) {
    __HAL_RCC_CAN1_CLK_DISABLE();
  }
}

bool Can::IsValidPinPair() const {
  if (!PinMapper::IsValidPinPair(config_.rx, config_.tx)) {
    return false;
  }
  return (config_.peripheral == CanPeripheral::kCan1 &&
          IsCan1Pins(config_.rx, config_.tx)) ||
         (config_.peripheral == CanPeripheral::kCan2 &&
          IsCan2Pins(config_.rx, config_.tx));
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
  CAN_RxHeaderTypeDef header{};
  while (HAL_CAN_GetRxFifoFillLevel(&handle_, CAN_RX_FIFO0) > 0) {
    interface::CanMessage message{};
    if (HAL_CAN_GetRxMessage(&handle_, CAN_RX_FIFO0, &header,
                             message.data.data()) != HAL_OK) {
      break;
    }
    message.id = header.IDE == CAN_ID_STD ? header.StdId : header.ExtId;
    message.size = static_cast<uint8_t>(header.DLC);
    message.format = header.IDE == CAN_ID_STD ? interface::CanFormat::kStandard
                                              : interface::CanFormat::kExtended;
    message.type = header.RTR == CAN_RTR_DATA
                       ? interface::CanFrameType::kData
                       : interface::CanFrameType::kRemote;
    if (rx_callback_) {
      rx_callback_(message);
    }
  }
}

uint8_t Can::DefaultFilterBank() const {
  return config_.peripheral == CanPeripheral::kCan1 ? 0 : 14;
}

Can*& Can::InstanceFor(CanPeripheral peripheral) {
  return peripheral == CanPeripheral::kCan1 ? Can1Instance() : Can2Instance();
}

Can* Can::FromHandle(CAN_HandleTypeDef* handle) {
  if (handle == nullptr) {
    return nullptr;
  }
  if (handle->Instance == CAN1) {
    return Can1Instance();
  }
  if (handle->Instance == CAN2) {
    return Can2Instance();
  }
  return nullptr;
}

extern "C" {

void HAL_CAN_MspInit(CAN_HandleTypeDef* hcan) { Can::ConfigureMsp(hcan); }

void HAL_CAN_MspDeInit(CAN_HandleTypeDef* hcan) { Can::DeconfigureMsp(hcan); }

void CAN1_RX0_IRQHandler() { Can::HandleInterrupt(CanPeripheral::kCan1); }

void CAN2_RX0_IRQHandler() { Can::HandleInterrupt(CanPeripheral::kCan2); }

void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef* hcan) {
  Can::HandleRxPending(hcan);
}

}  // extern "C"

}  // namespace nkrc::nbed::stm32::f4

#endif
