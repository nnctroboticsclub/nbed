#pragma once

#include <array>

struct DMA_Instance {};

inline std::array<DMA_Instance, 23> dma_instances{};
#define DMA1_Channel1 (&dma_instances[0])
#define DMA1_Channel2 (&dma_instances[1])
#define DMA1_Channel3 (&dma_instances[2])
#define DMA1_Channel4 (&dma_instances[3])
#define DMA1_Channel5 (&dma_instances[4])
#define DMA1_Channel6 (&dma_instances[5])
#define DMA1_Channel7 (&dma_instances[6])
#define DMA1_Stream0 (&dma_instances[7])
#define DMA1_Stream1 (&dma_instances[8])
#define DMA1_Stream2 (&dma_instances[9])
#define DMA1_Stream3 (&dma_instances[10])
#define DMA1_Stream4 (&dma_instances[11])
#define DMA1_Stream5 (&dma_instances[12])
#define DMA1_Stream6 (&dma_instances[13])
#define DMA1_Stream7 (&dma_instances[14])
#define DMA2_Stream0 (&dma_instances[15])
#define DMA2_Stream1 (&dma_instances[16])
#define DMA2_Stream2 (&dma_instances[17])
#define DMA2_Stream3 (&dma_instances[18])
#define DMA2_Stream4 (&dma_instances[19])
#define DMA2_Stream5 (&dma_instances[20])
#define DMA2_Stream6 (&dma_instances[21])
#define DMA2_Stream7 (&dma_instances[22])
#define __HAL_RCC_DMA1_CLK_ENABLE() ((void)0)
#define __HAL_RCC_DMA2_CLK_ENABLE() ((void)0)
inline constexpr uint32_t DMA_CHANNEL_3 = 3, DMA_CHANNEL_4 = 4,
                          DMA_CHANNEL_5 = 5, DMA_CHANNEL_6 = 6,
                          DMA_FIFOMODE_DISABLE = 0, DMA_MBURST_SINGLE = 0,
                          DMA_PBURST_SINGLE = 0, DMA_PERIPH_TO_MEMORY = 0,
                          DMA_MEMORY_TO_PERIPH = 1, DMA_PINC_DISABLE = 0,
                          DMA_MINC_ENABLE = 1, DMA_PDATAALIGN_WORD = 4,
                          DMA_PDATAALIGN_BYTE = 1, DMA_MDATAALIGN_WORD = 4,
                          DMA_PDATAALIGN_HALFWORD = 2,
                          DMA_MDATAALIGN_HALFWORD = 2,
                          DMA_MDATAALIGN_BYTE = 1, DMA_NORMAL = 0,
                          DMA_PRIORITY_VERY_HIGH = 3, DMA_PRIORITY_HIGH = 2;

struct DMA_InitTypeDef {
  uint32_t Channel{}, Direction{}, PeriphInc{}, MemInc{}, PeriphDataAlignment{},
      MemDataAlignment{}, Mode{}, Priority{}, FIFOMode{}, MemBurst{},
      PeriphBurst{};
};

struct DMA_HandleTypeDef {
  DMA_Instance* Instance{};
  DMA_InitTypeDef Init{};
  void* Parent{};
  void (*XferCpltCallback)(DMA_HandleTypeDef*){};
  void (*XferErrorCallback)(DMA_HandleTypeDef*){};
};

inline DMA_HandleTypeDef* last_dma_handle{};
inline std::array<DMA_HandleTypeDef*, 23> initialized_dma{};
inline uint32_t dma_source{}, dma_destination{}, dma_length{};
inline HAL_StatusTypeDef dma_init_status = HAL_OK, dma_start_status = HAL_OK,
                         uart_dma_tx_status = HAL_OK,
                         uart_dma_rx_status = HAL_OK;
inline unsigned dma_irq_count{}, dma_abort_count{}, dma_deinit_count{};
inline bool dma_irq_error = false;

inline HAL_StatusTypeDef HAL_DMA_Init(DMA_HandleTypeDef* handle) {
  if (dma_init_status == HAL_OK) {
    initialized_dma.at(handle->Instance - dma_instances.data()) = handle;
  }
  return dma_init_status;
}

inline HAL_StatusTypeDef HAL_DMA_DeInit(DMA_HandleTypeDef* handle) {
  ++dma_deinit_count;
  initialized_dma.at(handle->Instance - dma_instances.data()) = nullptr;
  return HAL_OK;
}

inline HAL_StatusTypeDef HAL_DMA_Abort(DMA_HandleTypeDef*) {
  ++dma_abort_count;
  return HAL_OK;
}

inline HAL_StatusTypeDef HAL_DMA_Start_IT(DMA_HandleTypeDef* handle,
                                          uint32_t src, uint32_t dst,
                                          uint32_t size) {
  last_dma_handle = handle;
  dma_source = src;
  dma_destination = dst;
  dma_length = size;
  return dma_start_status;
}

inline void HAL_DMA_IRQHandler(DMA_HandleTypeDef* handle) {
  ++dma_irq_count;
  if (dma_irq_error) {
    if (handle->XferErrorCallback) {
      handle->XferErrorCallback(handle);
    }
  } else if (handle->XferCpltCallback) {
    handle->XferCpltCallback(handle);
  }
}

#define __HAL_LINKDMA(handle, field, dma) \
  do {                                    \
    (handle)->field = &(dma);             \
    (dma).Parent = (handle);              \
  } while (false)

inline uint16_t uart_dma_tx_size{}, uart_dma_rx_size{};
inline const uint8_t* uart_dma_tx_data{};
inline uint8_t* uart_dma_rx_data{};

inline HAL_StatusTypeDef HAL_UART_Transmit_DMA(UART_HandleTypeDef* handle,
                                               const uint8_t* data, uint16_t size) {
  uart_dma_tx_size = size;
  uart_dma_tx_data = data;
  last_dma_handle = handle->hdmatx;
  return uart_dma_tx_status;
}

inline HAL_StatusTypeDef HAL_UART_Receive_DMA(UART_HandleTypeDef* handle,
                                              uint8_t* data, uint16_t size) {
  uart_dma_rx_size = size;
  uart_dma_rx_data = data;
  last_dma_handle = handle->hdmarx;
  return uart_dma_rx_status;
}

inline HAL_StatusTypeDef HAL_UART_AbortTransmit(UART_HandleTypeDef*) {
  return HAL_OK;
}

inline unsigned uart_abort_rx_count{};

inline HAL_StatusTypeDef HAL_UART_AbortReceive(UART_HandleTypeDef*) {
  ++uart_abort_rx_count;
  return HAL_OK;
}

inline constexpr uint32_t HAL_UART_ERROR_DMA = 16;

inline void __DMB() {}

struct SCB_Type {
  uint32_t CCR{};
};

inline SCB_Type scb;
#define SCB (&scb)
inline constexpr uint32_t SCB_CCR_DC_Msk = 1U << 16;
inline unsigned cache_clean_count{}, cache_clean_invalidate_count{},
    cache_invalidate_count{};
inline int32_t cache_invalidate_bytes{};

inline void SCB_CleanDCache_by_Addr(uint32_t*, int32_t) { ++cache_clean_count; }

inline void SCB_CleanInvalidateDCache_by_Addr(uint32_t*, int32_t) {
  ++cache_clean_invalidate_count;
}

inline void SCB_InvalidateDCache_by_Addr(uint32_t*, int32_t bytes) {
  cache_invalidate_bytes = bytes;
  ++cache_invalidate_count;
}
