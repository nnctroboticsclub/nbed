#pragma once

#include <algorithm>

#include "hal_stubs.h"

struct CAN_TypeDef {
  uint32_t ESR{};
};

inline CAN_TypeDef can1, can2;
#define CAN (&can1)
#define CAN1 (&can1)
#define CAN2 (&can2)
inline bool can1_clock{}, can2_clock{};
#define __HAL_RCC_CAN1_CLK_ENABLE() (can1_clock = true)
#define __HAL_RCC_CAN2_CLK_ENABLE() (can2_clock = true)
#define __HAL_RCC_CAN1_CLK_DISABLE() (can1_clock = false)
#define __HAL_RCC_CAN2_CLK_DISABLE() (can2_clock = false)

inline constexpr uint32_t GPIO_AF9_CAN = 9, GPIO_AF9_CAN1 = 9;
inline constexpr uint32_t CAN_BTR_TS1_Pos = 16, CAN_BTR_TS2_Pos = 20;
inline constexpr uint32_t CAN_ESR_TEC_Pos = 16, CAN_ESR_REC_Pos = 24;
inline constexpr uint32_t CAN_ESR_TEC = 0xffU << CAN_ESR_TEC_Pos;
inline constexpr uint32_t CAN_ESR_REC = 0xffU << CAN_ESR_REC_Pos;
enum {
  DISABLE = 0,
  ENABLE = 1,
  CAN_MODE_NORMAL = 0,
  CAN_MODE_LOOPBACK,
  CAN_MODE_SILENT,
  CAN_MODE_SILENT_LOOPBACK,
  CAN_SJW_1TQ = 0,
  CAN_RX_FIFO0 = 0,
  CAN_FILTERMODE_IDMASK = 0,
  CAN_FILTERSCALE_32BIT = 1,
  CAN_ID_STD = 0,
  CAN_ID_EXT = 4,
  CAN_RTR_DATA = 0,
  CAN_RTR_REMOTE = 2,
  CAN_IT_RX_FIFO0_MSG_PENDING = 2,
};

struct CAN_InitTypeDef {
  uint32_t Prescaler{}, Mode{}, SyncJumpWidth{}, TimeSeg1{}, TimeSeg2{},
      TimeTriggeredMode{}, AutoBusOff{}, AutoWakeUp{}, AutoRetransmission{},
      ReceiveFifoLocked{}, TransmitFifoPriority{};
};

struct CAN_HandleTypeDef {
  CAN_TypeDef* Instance{};
  CAN_InitTypeDef Init{};
};

struct CAN_FilterTypeDef {
  uint32_t FilterIdHigh{}, FilterIdLow{}, FilterMaskIdHigh{}, FilterMaskIdLow{},
      FilterFIFOAssignment{}, FilterBank{}, FilterMode{}, FilterScale{},
      FilterActivation{}, SlaveStartFilterBank{};
};

struct CAN_TxHeaderTypeDef {
  uint32_t StdId{}, ExtId{}, IDE{}, RTR{}, DLC{}, TransmitGlobalTime{};
};

struct CAN_RxHeaderTypeDef {
  uint32_t StdId{}, ExtId{}, IDE{}, RTR{}, DLC{};
};

extern "C" void HAL_CAN_MspInit(CAN_HandleTypeDef*);
extern "C" void HAL_CAN_MspDeInit(CAN_HandleTypeDef*);
inline HAL_StatusTypeDef can_init_status = HAL_OK, can_filter_status = HAL_OK;
inline unsigned can_init_calls{}, can_deinit_calls{};

inline HAL_StatusTypeDef HAL_CAN_Init(CAN_HandleTypeDef* handle) {
  ++can_init_calls;
  HAL_CAN_MspInit(handle);
  return can_init_status;
}

inline HAL_StatusTypeDef HAL_CAN_DeInit(CAN_HandleTypeDef* handle) {
  ++can_deinit_calls;
  HAL_CAN_MspDeInit(handle);
  return HAL_OK;
}

inline HAL_StatusTypeDef HAL_CAN_ConfigFilter(CAN_HandleTypeDef*,
                                             CAN_FilterTypeDef*) {
  return can_filter_status;
}

inline HAL_StatusTypeDef HAL_CAN_Start(CAN_HandleTypeDef*) { return HAL_OK; }
inline HAL_StatusTypeDef HAL_CAN_Stop(CAN_HandleTypeDef*) { return HAL_OK; }
inline HAL_StatusTypeDef HAL_CAN_ActivateNotification(CAN_HandleTypeDef*,
                                                     uint32_t) {
  return HAL_OK;
}
inline void HAL_CAN_IRQHandler(CAN_HandleTypeDef*) {}
inline uint32_t HAL_CAN_GetRxFifoFillLevel(CAN_HandleTypeDef*, uint32_t) {
  return 0;
}
inline HAL_StatusTypeDef HAL_CAN_GetRxMessage(CAN_HandleTypeDef*, uint32_t,
                                             CAN_RxHeaderTypeDef*, uint8_t*) {
  return HAL_ERROR;
}
inline HAL_StatusTypeDef HAL_CAN_AddTxMessage(CAN_HandleTypeDef*,
                                             CAN_TxHeaderTypeDef*,
                                             const uint8_t*, uint32_t*) {
  return HAL_OK;
}
