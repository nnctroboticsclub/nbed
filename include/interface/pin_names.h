#pragma once

#include <cstdint>

namespace nkrc::nbed::interface {

#ifdef STM32F303x8
/// @brief NUCLEO-F303K8 の STM32 ピン名と Arduino コネクター名。
enum class PinName : std::uint8_t {
  kPA0 = 0x00,   ///< GPIO PA0。
  kPA1 = 0x01,   ///< GPIO PA1。
  kPA2 = 0x02,   ///< GPIO PA2。
  kPA3 = 0x03,   ///< GPIO PA3。
  kPA4 = 0x04,   ///< GPIO PA4。
  kPA5 = 0x05,   ///< GPIO PA5。
  kPA6 = 0x06,   ///< GPIO PA6。
  kPA7 = 0x07,   ///< GPIO PA7。
  kPA8 = 0x08,   ///< GPIO PA8。
  kPA9 = 0x09,   ///< GPIO PA9。
  kPA10 = 0x0A,  ///< GPIO PA10。
  kPA11 = 0x0B,  ///< GPIO PA11。
  kPA12 = 0x0C,  ///< GPIO PA12。
  kPA13 = 0x0D,  ///< GPIO PA13。
  kPA14 = 0x0E,  ///< GPIO PA14。
  kPA15 = 0x0F,  ///< GPIO PA15。

  kPB0 = 0x10,  ///< GPIO PB0。
  kPB1 = 0x11,  ///< GPIO PB1。
  kPB2 = 0x12,  ///< 互換性のための旧ピン名。F303K8 の LQFP32 では使用不可。
  kPB3 = 0x13,  ///< GPIO PB3。
  kPB4 = 0x14,  ///< GPIO PB4。
  kPB5 = 0x15,  ///< GPIO PB5。
  kPB6 = 0x16,  ///< GPIO PB6。
  kPB7 = 0x17,  ///< GPIO PB7。

  kPC0 = 0x20,   ///< 互換性のための旧ピン名。F303K8 の LQFP32 では使用不可。
  kPC1 = 0x21,   ///< 互換性のための旧ピン名。F303K8 の LQFP32 では使用不可。
  kPC2 = 0x22,   ///< 互換性のための旧ピン名。F303K8 の LQFP32 では使用不可。
  kPC3 = 0x23,   ///< 互換性のための旧ピン名。F303K8 の LQFP32 では使用不可。
  kPC4 = 0x24,   ///< 互換性のための旧ピン名。F303K8 の LQFP32 では使用不可。
  kPC5 = 0x25,   ///< 互換性のための旧ピン名。F303K8 の LQFP32 では使用不可。
  kPC13 = 0x2D,  ///< 互換性のための旧ピン名。F303K8 の LQFP32 では使用不可。

  kPF0 = 0x50,  ///< GPIO PF0。
  kPF1 = 0x51,  ///< GPIO PF1。

  kD0 = kPA10,   ///< D0 コネクターの別名。実ピンは PA10。
  kD1 = kPA9,    ///< D1 コネクターの別名。実ピンは PA9。
  kD2 = kPA12,   ///< D2 コネクターの別名。実ピンは PA12。
  kD3 = kPB0,    ///< D3 コネクターの別名。実ピンは PB0。
  kD4 = kPB7,    ///< D4 コネクターの別名。実ピンは PB7。
  kD5 = kPB6,    ///< D5 コネクターの別名。実ピンは PB6。
  kD6 = kPB1,    ///< D6 コネクターの別名。実ピンは PB1。
  kD7 = kPF0,    ///< D7 コネクターの別名。実ピンは PF0。
  kD8 = kPF1,    ///< D8 コネクターの別名。実ピンは PF1。
  kD9 = kPA8,    ///< D9 コネクターの別名。実ピンは PA8。
  kD10 = kPA11,  ///< D10 コネクターの別名。実ピンは PA11。
  kD11 = kPB5,   ///< D11 コネクターの別名。実ピンは PB5。
  kD12 = kPB4,   ///< D12 コネクターの別名。実ピンは PB4。
  kD13 = kPB3,   ///< D13 コネクターの別名。実ピンは PB3。

  kA0 = kPA0,  ///< A0 コネクターの別名。実ピンは PA0。
  kA1 = kPA1,  ///< A1 コネクターの別名。実ピンは PA1。
  kA2 = kPA3,  ///< A2 コネクターの別名。実ピンは PA3。
  kA3 = kPA4,  ///< A3 コネクターの別名。実ピンは PA4。
  kA4 = kPA5,  ///< A4 コネクターの別名。実ピンは PA5。
  kA5 = kPA6,  ///< A5 コネクターの別名。実ピンは PA6。
  kA6 = kPA7,  ///< A6 コネクターの別名。実ピンは PA7。
  kA7 = kPA2,  ///< A7 コネクターの別名。実ピンは PA2。

  kNC = 0xFF,  ///< ピンを使用しないことを表す値。
};
#endif

#ifdef STM32F446xx
/// @brief NUCLEO-F446RE の STM32 ピン名と Arduino コネクター名。
enum class PinName : std::uint8_t {
  kPA0 = 0x00,   ///< GPIO PA0。
  kPA1 = 0x01,   ///< GPIO PA1。
  kPA2 = 0x02,   ///< GPIO PA2。
  kPA3 = 0x03,   ///< GPIO PA3。
  kPA4 = 0x04,   ///< GPIO PA4。
  kPA5 = 0x05,   ///< GPIO PA5。
  kPA6 = 0x06,   ///< GPIO PA6。
  kPA7 = 0x07,   ///< GPIO PA7。
  kPA8 = 0x08,   ///< GPIO PA8。
  kPA9 = 0x09,   ///< GPIO PA9。
  kPA10 = 0x0A,  ///< GPIO PA10。
  kPA11 = 0x0B,  ///< GPIO PA11。
  kPA12 = 0x0C,  ///< GPIO PA12。
  kPA13 = 0x0D,  ///< GPIO PA13。
  kPA14 = 0x0E,  ///< GPIO PA14。
  kPA15 = 0x0F,  ///< GPIO PA15。

  kPB0 = 0x10,   ///< GPIO PB0。
  kPB1 = 0x11,   ///< GPIO PB1。
  kPB2 = 0x12,   ///< GPIO PB2。
  kPB3 = 0x13,   ///< GPIO PB3。
  kPB4 = 0x14,   ///< GPIO PB4。
  kPB5 = 0x15,   ///< GPIO PB5。
  kPB6 = 0x16,   ///< GPIO PB6。
  kPB7 = 0x17,   ///< GPIO PB7。
  kPB8 = 0x18,   ///< GPIO PB8。
  kPB9 = 0x19,   ///< GPIO PB9。
  kPB10 = 0x1A,  ///< GPIO PB10。
  kPB12 = 0x1C,  ///< GPIO PB12。
  kPB13 = 0x1D,  ///< GPIO PB13。
  kPB14 = 0x1E,  ///< GPIO PB14。
  kPB15 = 0x1F,  ///< GPIO PB15。

  kPC0 = 0x20,   ///< GPIO PC0。
  kPC1 = 0x21,   ///< GPIO PC1。
  kPC2 = 0x22,   ///< GPIO PC2。
  kPC3 = 0x23,   ///< GPIO PC3。
  kPC4 = 0x24,   ///< GPIO PC4。
  kPC5 = 0x25,   ///< GPIO PC5。
  kPC6 = 0x26,   ///< GPIO PC6。
  kPC7 = 0x27,   ///< GPIO PC7。
  kPC8 = 0x28,   ///< GPIO PC8。
  kPC9 = 0x29,   ///< GPIO PC9。
  kPC10 = 0x2A,  ///< GPIO PC10。
  kPC11 = 0x2B,  ///< GPIO PC11。
  kPC12 = 0x2C,  ///< GPIO PC12。
  kPC13 = 0x2D,  ///< GPIO PC13。
  kPC14 = 0x2E,  ///< GPIO PC14。
  kPC15 = 0x2F,  ///< GPIO PC15。

  kPD2 = 0x32,  ///< GPIO PD2。

  kPH0 = 0x70,  ///< GPIO PH0。
  kPH1 = 0x71,  ///< GPIO PH1。

  kD0 = kPA3,   ///< D0 コネクターの別名。実ピンは PA3。
  kD1 = kPA2,   ///< D1 コネクターの別名。実ピンは PA2。
  kD2 = kPA10,  ///< D2 コネクターの別名。実ピンは PA10。
  kD3 = kPB3,   ///< D3 コネクターの別名。実ピンは PB3。
  kD4 = kPB5,   ///< D4 コネクターの別名。実ピンは PB5。
  kD5 = kPB4,   ///< D5 コネクターの別名。実ピンは PB4。
  kD6 = kPB10,  ///< D6 コネクターの別名。実ピンは PB10。
  kD7 = kPA8,   ///< D7 コネクターの別名。実ピンは PA8。
  kD8 = kPA9,   ///< D8 コネクターの別名。実ピンは PA9。
  kD9 = kPC7,   ///< D9 コネクターの別名。実ピンは PC7。
  kD10 = kPB6,  ///< D10 コネクターの別名。実ピンは PB6。
  kD11 = kPA7,  ///< D11 コネクターの別名。実ピンは PA7。
  kD12 = kPA6,  ///< D12 コネクターの別名。実ピンは PA6。
  kD13 = kPA5,  ///< D13 コネクターの別名。実ピンは PA5。
  kD14 = kPB9,  ///< D14 コネクターの別名。実ピンは PB9。
  kD15 = kPB8,  ///< D15 コネクターの別名。実ピンは PB8。

  kA0 = kPA0,  ///< A0 コネクターの別名。実ピンは PA0。
  kA1 = kPA1,  ///< A1 コネクターの別名。実ピンは PA1。
  kA2 = kPA4,  ///< A2 コネクターの別名。実ピンは PA4。
  kA3 = kPB0,  ///< A3 コネクターの別名。実ピンは PB0。
  kA4 = kPC1,  ///< A4 コネクターの別名。実ピンは PC1。
  kA5 = kPC0,  ///< A5 コネクターの別名。実ピンは PC0。

  kNC = 0xFF,  ///< ピンを使用しないことを表す値。
};
#endif

#ifdef STM32F767xx
/// @brief NUCLEO-F767ZI の STM32 ピン名と Arduino／Zio コネクター名。
enum class PinName : std::uint8_t {
  kPA0 = 0x00,   ///< GPIO PA0。
  kPA1 = 0x01,   ///< GPIO PA1。
  kPA2 = 0x02,   ///< GPIO PA2。
  kPA3 = 0x03,   ///< GPIO PA3。
  kPA4 = 0x04,   ///< GPIO PA4。
  kPA5 = 0x05,   ///< GPIO PA5。
  kPA6 = 0x06,   ///< GPIO PA6。
  kPA7 = 0x07,   ///< GPIO PA7。
  kPA8 = 0x08,   ///< GPIO PA8。
  kPA9 = 0x09,   ///< GPIO PA9。
  kPA10 = 0x0A,  ///< GPIO PA10。
  kPA11 = 0x0B,  ///< GPIO PA11。
  kPA12 = 0x0C,  ///< GPIO PA12。
  kPA13 = 0x0D,  ///< GPIO PA13。
  kPA14 = 0x0E,  ///< GPIO PA14。
  kPA15 = 0x0F,  ///< GPIO PA15。
  kPB0 = 0x10,   ///< GPIO PB0。
  kPB1 = 0x11,   ///< GPIO PB1。
  kPB2 = 0x12,   ///< GPIO PB2。
  kPB3 = 0x13,   ///< GPIO PB3。
  kPB4 = 0x14,   ///< GPIO PB4。
  kPB5 = 0x15,   ///< GPIO PB5。
  kPB6 = 0x16,   ///< GPIO PB6。
  kPB7 = 0x17,   ///< GPIO PB7。
  kPB8 = 0x18,   ///< GPIO PB8。
  kPB9 = 0x19,   ///< GPIO PB9。
  kPB10 = 0x1A,  ///< GPIO PB10。
  kPB11 = 0x1B,  ///< GPIO PB11。
  kPB12 = 0x1C,  ///< GPIO PB12。
  kPB13 = 0x1D,  ///< GPIO PB13。
  kPB14 = 0x1E,  ///< GPIO PB14。
  kPB15 = 0x1F,  ///< GPIO PB15。
  kPC0 = 0x20,   ///< GPIO PC0。
  kPC1 = 0x21,   ///< GPIO PC1。
  kPC2 = 0x22,   ///< GPIO PC2。
  kPC3 = 0x23,   ///< GPIO PC3。
  kPC4 = 0x24,   ///< GPIO PC4。
  kPC5 = 0x25,   ///< GPIO PC5。
  kPC6 = 0x26,   ///< GPIO PC6。
  kPC7 = 0x27,   ///< GPIO PC7。
  kPC8 = 0x28,   ///< GPIO PC8。
  kPC9 = 0x29,   ///< GPIO PC9。
  kPC10 = 0x2A,  ///< GPIO PC10。
  kPC11 = 0x2B,  ///< GPIO PC11。
  kPC12 = 0x2C,  ///< GPIO PC12。
  kPC13 = 0x2D,  ///< GPIO PC13。
  kPC14 = 0x2E,  ///< GPIO PC14。
  kPC15 = 0x2F,  ///< GPIO PC15。
  kPD0 = 0x30,   ///< GPIO PD0。
  kPD1 = 0x31,   ///< GPIO PD1。
  kPD2 = 0x32,   ///< GPIO PD2。
  kPD3 = 0x33,   ///< GPIO PD3。
  kPD4 = 0x34,   ///< GPIO PD4。
  kPD5 = 0x35,   ///< GPIO PD5。
  kPD6 = 0x36,   ///< GPIO PD6。
  kPD7 = 0x37,   ///< GPIO PD7。
  kPD8 = 0x38,   ///< GPIO PD8。
  kPD9 = 0x39,   ///< GPIO PD9。
  kPD10 = 0x3A,  ///< GPIO PD10。
  kPD11 = 0x3B,  ///< GPIO PD11。
  kPD12 = 0x3C,  ///< GPIO PD12。
  kPD13 = 0x3D,  ///< GPIO PD13。
  kPD14 = 0x3E,  ///< GPIO PD14。
  kPD15 = 0x3F,  ///< GPIO PD15。
  kPE0 = 0x40,   ///< GPIO PE0。
  kPE1 = 0x41,   ///< GPIO PE1。
  kPE2 = 0x42,   ///< GPIO PE2。
  kPE3 = 0x43,   ///< GPIO PE3。
  kPE4 = 0x44,   ///< GPIO PE4。
  kPE5 = 0x45,   ///< GPIO PE5。
  kPE6 = 0x46,   ///< GPIO PE6。
  kPE7 = 0x47,   ///< GPIO PE7。
  kPE8 = 0x48,   ///< GPIO PE8。
  kPE9 = 0x49,   ///< GPIO PE9。
  kPE10 = 0x4A,  ///< GPIO PE10。
  kPE11 = 0x4B,  ///< GPIO PE11。
  kPE12 = 0x4C,  ///< GPIO PE12。
  kPE13 = 0x4D,  ///< GPIO PE13。
  kPE14 = 0x4E,  ///< GPIO PE14。
  kPE15 = 0x4F,  ///< GPIO PE15。
  kPF0 = 0x50,   ///< GPIO PF0。
  kPF1 = 0x51,   ///< GPIO PF1。
  kPF2 = 0x52,   ///< GPIO PF2。
  kPF3 = 0x53,   ///< GPIO PF3。
  kPF4 = 0x54,   ///< GPIO PF4。
  kPF5 = 0x55,   ///< GPIO PF5。
  kPF6 = 0x56,   ///< GPIO PF6。
  kPF7 = 0x57,   ///< GPIO PF7。
  kPF8 = 0x58,   ///< GPIO PF8。
  kPF9 = 0x59,   ///< GPIO PF9。
  kPF10 = 0x5A,  ///< GPIO PF10。
  kPF11 = 0x5B,  ///< GPIO PF11。
  kPF12 = 0x5C,  ///< GPIO PF12。
  kPF13 = 0x5D,  ///< GPIO PF13。
  kPF14 = 0x5E,  ///< GPIO PF14。
  kPF15 = 0x5F,  ///< GPIO PF15。
  kPG0 = 0x60,   ///< GPIO PG0。
  kPG1 = 0x61,   ///< GPIO PG1。
  kPG2 = 0x62,   ///< GPIO PG2。
  kPG3 = 0x63,   ///< GPIO PG3。
  kPG4 = 0x64,   ///< GPIO PG4。
  kPG5 = 0x65,   ///< GPIO PG5。
  kPG6 = 0x66,   ///< GPIO PG6。
  kPG7 = 0x67,   ///< GPIO PG7。
  kPG8 = 0x68,   ///< GPIO PG8。
  kPG9 = 0x69,   ///< GPIO PG9。
  kPG10 = 0x6A,  ///< GPIO PG10。
  kPG11 = 0x6B,  ///< GPIO PG11。
  kPG12 = 0x6C,  ///< GPIO PG12。
  kPG13 = 0x6D,  ///< GPIO PG13。
  kPG14 = 0x6E,  ///< GPIO PG14。
  kPG15 = 0x6F,  ///< GPIO PG15。
  kPH0 = 0x70,   ///< GPIO PH0。
  kPH1 = 0x71,   ///< GPIO PH1。

  // NUCLEO-F767ZI Arduino/Zio aliases (D11 default route: PA7).
  kD0 = kPG9,    ///< D0 コネクターの別名。実ピンは PG9。
  kD1 = kPG14,   ///< D1 コネクターの別名。実ピンは PG14。
  kD2 = kPF15,   ///< D2 コネクターの別名。実ピンは PF15。
  kD3 = kPE13,   ///< D3 コネクターの別名。実ピンは PE13。
  kD4 = kPF14,   ///< D4 コネクターの別名。実ピンは PF14。
  kD5 = kPE11,   ///< D5 コネクターの別名。実ピンは PE11。
  kD6 = kPE9,    ///< D6 コネクターの別名。実ピンは PE9。
  kD7 = kPF13,   ///< D7 コネクターの別名。実ピンは PF13。
  kD8 = kPF12,   ///< D8 コネクターの別名。実ピンは PF12。
  kD9 = kPD15,   ///< D9 コネクターの別名。実ピンは PD15。
  kD10 = kPD14,  ///< D10 コネクターの別名。実ピンは PD14。
  kD11 = kPA7,   ///< D11 コネクターの別名。実ピンは PA7。
  kD12 = kPA6,   ///< D12 コネクターの別名。実ピンは PA6。
  kD13 = kPA5,   ///< D13 コネクターの別名。実ピンは PA5。
  kD14 = kPB9,   ///< D14 コネクターの別名。実ピンは PB9。
  kD15 = kPB8,   ///< D15 コネクターの別名。実ピンは PB8。

  kA0 = kPA3,   ///< A0 コネクターの別名。実ピンは PA3。
  kA1 = kPC0,   ///< A1 コネクターの別名。実ピンは PC0。
  kA2 = kPC3,   ///< A2 コネクターの別名。実ピンは PC3。
  kA3 = kPF3,   ///< A3 コネクターの別名。実ピンは PF3。
  kA4 = kPF5,   ///< A4 コネクターの別名。実ピンは PF5。
  kA5 = kPF10,  ///< A5 コネクターの別名。実ピンは PF10。

  kNC = 0xFF,  ///< ピンを使用しないことを表す値。
};
#endif

}  // namespace nkrc::nbed::interface
