#pragma once

#include <cstdint>
#include <utility>
#include <vector>

struct GPIO_TypeDef {
  uint32_t IDR{}, ODR{};
};

struct GPIO_InitTypeDef {
  uint32_t Pin{}, Mode{}, Pull{}, Speed{}, Alternate{};
};

inline GPIO_TypeDef gpio_a, gpio_b, gpio_c, gpio_d, gpio_e, gpio_f, gpio_g,
    gpio_h;
#define GPIOA (&gpio_a)
#define GPIOB (&gpio_b)
#define GPIOC (&gpio_c)
#define GPIOD (&gpio_d)
#define GPIOE (&gpio_e)
#define GPIOF (&gpio_f)
#define GPIOG (&gpio_g)
#define GPIOH (&gpio_h)
#define __HAL_RCC_GPIOA_CLK_ENABLE() ((void)0)
#define __HAL_RCC_GPIOB_CLK_ENABLE() ((void)0)
#define __HAL_RCC_GPIOC_CLK_ENABLE() ((void)0)
#define __HAL_RCC_GPIOD_CLK_ENABLE() ((void)0)
#define __HAL_RCC_GPIOE_CLK_ENABLE() ((void)0)
#define __HAL_RCC_GPIOF_CLK_ENABLE() ((void)0)
#define __HAL_RCC_GPIOG_CLK_ENABLE() ((void)0)
#define __HAL_RCC_GPIOH_CLK_ENABLE() ((void)0)

enum GPIO_PinState { GPIO_PIN_RESET, GPIO_PIN_SET };

enum {
  GPIO_NOPULL,
  GPIO_PULLUP,
  GPIO_PULLDOWN,
  GPIO_MODE_INPUT,
  GPIO_MODE_OUTPUT_PP,
  GPIO_MODE_AF_PP,
  GPIO_SPEED_FREQ_LOW,
  GPIO_SPEED_FREQ_HIGH,
  GPIO_SPEED_FREQ_VERY_HIGH,
  GPIO_AF7_USART2,
  GPIO_AF8_UART4,
  GPIO_AF8_USART6
};

inline constexpr uint32_t GPIO_MODE_IT_RISING = 0x101;
inline constexpr uint32_t GPIO_MODE_IT_FALLING = 0x102;
inline constexpr uint32_t GPIO_MODE_IT_RISING_FALLING = 0x103;
inline uint32_t exti_pending{}, exti_mask{}, exti_rising{}, exti_falling{};
inline uint32_t primask{};
#define __NVIC_PRIO_BITS 4
#define __HAL_RCC_SYSCFG_CLK_ENABLE() ((void)0)
#define __HAL_GPIO_EXTI_GET_IT(mask) (exti_pending & (mask))
#define __HAL_GPIO_EXTI_CLEAR_IT(mask) (exti_pending &= ~(mask))
inline uint32_t __get_PRIMASK() { return primask; }
inline void __disable_irq() { primask = 1; }
inline void __set_PRIMASK(uint32_t mask) { primask = mask; }

inline std::vector<std::pair<GPIO_TypeDef*, GPIO_InitTypeDef>> gpio_inits;

inline void HAL_GPIO_Init(GPIO_TypeDef* port, GPIO_InitTypeDef* config) {
  gpio_inits.emplace_back(port, *config);
  if ((config->Mode & 0x100U) != 0) {
    exti_mask |= config->Pin;
    exti_rising = (exti_rising & ~config->Pin) |
                  ((config->Mode & 1U) ? config->Pin : 0);
    exti_falling = (exti_falling & ~config->Pin) |
                   ((config->Mode & 2U) ? config->Pin : 0);
  }
}

inline void HAL_GPIO_DeInit(GPIO_TypeDef*, uint32_t pin) {
  exti_mask &= ~pin;
  exti_rising &= ~pin;
  exti_falling &= ~pin;
}

inline void HAL_GPIO_WritePin(GPIO_TypeDef* port, uint16_t pin,
                              GPIO_PinState state) {
  port->ODR = state == GPIO_PIN_SET ? port->ODR | pin : port->ODR & ~pin;
}

inline GPIO_PinState HAL_GPIO_ReadPin(GPIO_TypeDef* port, uint16_t pin) {
  return port->IDR & pin ? GPIO_PIN_SET : GPIO_PIN_RESET;
}

struct USART_TypeDef {};

inline USART_TypeDef usart1, usart2, usart3, uart4, usart6;
#define USART1 (&usart1)
#define USART2 (&usart2)
#define USART3 (&usart3)
#define UART4 (&uart4)
#define USART6 (&usart6)
#define __HAL_RCC_USART1_CLK_ENABLE() ((void)0)
#define __HAL_RCC_USART2_CLK_ENABLE() ((void)0)
#define __HAL_RCC_USART3_CLK_ENABLE() ((void)0)
#define __HAL_RCC_UART4_CLK_ENABLE() ((void)0)
#define __HAL_RCC_USART6_CLK_ENABLE() ((void)0)
#define __HAL_RCC_USART2_CONFIG(source) ((void)0)
#define __HAL_RCC_USART3_CONFIG(source) ((void)0)
#define __HAL_RCC_UART4_CONFIG(source) ((void)0)
#define __HAL_RCC_USART6_CONFIG(source) ((void)0)

enum IRQn_Type {
  EXTI0_IRQn, EXTI1_IRQn, EXTI2_IRQn, EXTI2_TSC_IRQn, EXTI3_IRQn, EXTI4_IRQn,
  EXTI9_5_IRQn, EXTI15_10_IRQn,
  USART1_IRQn,
  USART2_IRQn,
  USART3_IRQn,
  UART4_IRQn,
  USART6_IRQn
};

struct UART_InitTypeDef {
  uint32_t BaudRate{}, WordLength{}, StopBits{}, Parity{}, Mode{}, HwFlowCtl{},
      OverSampling{}, OneBitSampling{};
};

struct UART_HandleTypeDef {
  USART_TypeDef* Instance{};
  UART_InitTypeDef Init{};

  struct {
    uint32_t AdvFeatureInit{};
  } AdvancedInit;
};

enum {
  UART_WORDLENGTH_8B,
  UART_STOPBITS_1,
  UART_PARITY_NONE,
  UART_MODE_TX_RX,
  UART_HWCONTROL_NONE,
  UART_OVERSAMPLING_16,
  UART_ONE_BIT_SAMPLE_DISABLE,
  UART_ADVFEATURE_NO_INIT
};

enum HAL_StatusTypeDef { HAL_OK, HAL_ERROR };

inline constexpr uint32_t HAL_MAX_DELAY = 0xffffffffU;
inline std::vector<uint16_t> tx_sizes;
inline std::vector<uint8_t> tx_data;
inline int fail_tx_call = -1;

inline HAL_StatusTypeDef HAL_UART_Init(UART_HandleTypeDef*) { return HAL_OK; }

inline HAL_StatusTypeDef HAL_UART_DeInit(UART_HandleTypeDef*) { return HAL_OK; }

inline HAL_StatusTypeDef HAL_UART_Transmit(UART_HandleTypeDef*, uint8_t* data,
                                           uint16_t size, uint32_t) {
  tx_sizes.push_back(size);
  if (static_cast<int>(tx_sizes.size()) == fail_tx_call) {
    return HAL_ERROR;
  }
  tx_data.insert(tx_data.end(), data, data + size);
  return HAL_OK;
}

inline HAL_StatusTypeDef HAL_UART_Receive(UART_HandleTypeDef*, uint8_t*,
                                          uint16_t, uint32_t) {
  return HAL_OK;
}

inline HAL_StatusTypeDef HAL_UART_Receive_IT(UART_HandleTypeDef*, uint8_t*,
                                             uint16_t) {
  return HAL_OK;
}

struct SPI_TypeDef {
  bool enabled{false};
};

inline SPI_TypeDef spi1, spi2, spi3;
#define SPI1 (&spi1)
#define SPI2 (&spi2)
#define SPI3 (&spi3)
#define __HAL_RCC_SPI1_CLK_ENABLE() ((void)0)
#define __HAL_RCC_SPI2_CLK_ENABLE() ((void)0)
#define __HAL_RCC_SPI3_CLK_ENABLE() ((void)0)
#define __HAL_SPI_ENABLE(handle) ((handle)->Instance->enabled = true)

inline constexpr uint32_t GPIO_AF5_SPI1 = 5;
inline constexpr uint32_t GPIO_AF5_SPI3 = 5;
inline constexpr uint32_t GPIO_AF6_SPI3 = 6;
inline constexpr uint32_t GPIO_AF7_SPI2 = 7;
inline constexpr uint32_t GPIO_AF7_SPI3 = 7;

inline constexpr uint32_t SPI_BAUDRATEPRESCALER_2 = 0;
inline constexpr uint32_t SPI_BAUDRATEPRESCALER_4 = 8;
inline constexpr uint32_t SPI_BAUDRATEPRESCALER_8 = 16;
inline constexpr uint32_t SPI_BAUDRATEPRESCALER_16 = 24;
inline constexpr uint32_t SPI_BAUDRATEPRESCALER_32 = 32;
inline constexpr uint32_t SPI_BAUDRATEPRESCALER_64 = 40;
inline constexpr uint32_t SPI_BAUDRATEPRESCALER_128 = 48;
inline constexpr uint32_t SPI_BAUDRATEPRESCALER_256 = 56;

enum {
  SPI_MODE_MASTER,
  SPI_DIRECTION_2LINES,
  SPI_DATASIZE_8BIT,
  SPI_POLARITY_LOW,
  SPI_POLARITY_HIGH,
  SPI_PHASE_1EDGE,
  SPI_PHASE_2EDGE,
  SPI_NSS_SOFT,
  SPI_FIRSTBIT_MSB,
  SPI_FIRSTBIT_LSB,
  SPI_TIMODE_DISABLE,
  SPI_CRCCALCULATION_DISABLE,
  SPI_NSS_PULSE_DISABLE,
  SPI_CRC_LENGTH_DATASIZE
};

struct SPI_InitTypeDef {
  uint32_t Mode{}, Direction{}, DataSize{}, CLKPolarity{}, CLKPhase{}, NSS{},
      BaudRatePrescaler{}, FirstBit{}, TIMode{}, CRCCalculation{},
      CRCPolynomial{}, NSSPMode{}, CRCLength{};
};

struct SPI_HandleTypeDef {
  SPI_TypeDef* Instance{};
  SPI_InitTypeDef Init{};
};

inline SPI_HandleTypeDef last_spi_init;
inline HAL_StatusTypeDef spi_init_status = HAL_OK;
inline HAL_StatusTypeDef spi_transfer_status = HAL_OK;
inline unsigned spi_init_calls = 0, spi_deinit_calls = 0,
                spi_transfer_calls = 0;
inline uint16_t spi_size = 0;
inline uint32_t spi_timeout = 0;
inline std::vector<uint8_t> spi_tx_data;

inline HAL_StatusTypeDef HAL_SPI_Init(SPI_HandleTypeDef* handle) {
  ++spi_init_calls;
  last_spi_init = *handle;
  return spi_init_status;
}

inline HAL_StatusTypeDef HAL_SPI_DeInit(SPI_HandleTypeDef* handle) {
  ++spi_deinit_calls;
  handle->Instance->enabled = false;
  return HAL_OK;
}

inline HAL_StatusTypeDef HAL_SPI_Transmit(SPI_HandleTypeDef*, uint8_t* data,
                                          uint16_t size, uint32_t timeout) {
  ++spi_transfer_calls;
  spi_size = size;
  spi_timeout = timeout;
  spi_tx_data.assign(data, data + size);
  return spi_transfer_status;
}

inline HAL_StatusTypeDef HAL_SPI_TransmitReceive(SPI_HandleTypeDef* handle,
                                                 uint8_t* tx, uint8_t* rx,
                                                 uint16_t size,
                                                 uint32_t timeout) {
  const auto status = HAL_SPI_Transmit(handle, tx, size, timeout);
  if (status == HAL_OK) {
    for (uint16_t index = 0; index < size; ++index) {
      rx[index] = spi_tx_data[index];
    }
  }
  return status;
}

#ifdef STM32F303x8
inline uint32_t pclk1_hz = 32'000'000, pclk2_hz = 64'000'000;
#elif defined(STM32F767xx)
inline uint32_t pclk1_hz = 54'000'000, pclk2_hz = 108'000'000;
#else
inline uint32_t pclk1_hz = 45'000'000, pclk2_hz = 90'000'000;
#endif

inline uint32_t HAL_RCC_GetPCLK1Freq() { return pclk1_hz; }

inline uint32_t HAL_RCC_GetPCLK2Freq() { return pclk2_hz; }

inline void HAL_UART_IRQHandler(UART_HandleTypeDef*) {}

inline bool irq_enabled[32]{};
inline uint32_t irq_priority[32]{}, irq_clears[32]{};
inline void HAL_NVIC_SetPriority(IRQn_Type irq, uint32_t priority, uint32_t) {
  irq_priority[irq] = priority;
}

inline void HAL_NVIC_EnableIRQ(IRQn_Type irq) { irq_enabled[irq] = true; }

inline void HAL_NVIC_DisableIRQ(IRQn_Type irq) { irq_enabled[irq] = false; }
inline void HAL_NVIC_ClearPendingIRQ(IRQn_Type irq) { ++irq_clears[irq]; }

struct CoreDebug_Type {
  uint32_t DEMCR{};
};

struct DWT_Type {
  uint32_t CTRL{}, CYCCNT{}, LAR{};
};

inline CoreDebug_Type core_debug;
inline DWT_Type dwt;
#define CoreDebug (&core_debug)
#define DWT (&dwt)
inline constexpr uint32_t CoreDebug_DEMCR_TRCENA_Msk = 1U;
inline constexpr uint32_t DWT_CTRL_CYCCNTENA_Msk = 1U;
inline uint32_t SystemCoreClock = 64'000'000;
inline uint32_t hal_tick = 0;
inline void (*tick_hook)() = nullptr;

inline uint32_t HAL_GetTick() {
  if (tick_hook != nullptr) {
    auto hook = tick_hook;
    tick_hook = nullptr;
    hook();
  }
  return hal_tick;
}
