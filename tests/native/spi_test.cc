#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <vector>

#ifdef STM32F303x8
#include "f3/spi.h"
namespace device = nkrc::nbed::stm32::f3;
#elif defined(STM32F767xx)
#include "f7/spi.h"
namespace device = nkrc::nbed::stm32::f7;
#else
#include "f4/spi.h"
namespace device = nkrc::nbed::stm32::f4;
#endif
#include "detail/spi_master.h"
#include "hal_stubs.h"

using namespace device;
using nkrc::nbed::detail::ComputeSpiTiming;
using nkrc::nbed::interface::PinName;
using nkrc::nbed::interface::SpiBitOrder;
using nkrc::nbed::interface::SpiMode;

static_assert(!ComputeSpiTiming(0, 1, 45'000'000));
static_assert(!ComputeSpiTiming(64'000'000, 0, 24'000'000));
static_assert(!ComputeSpiTiming(64'000'000, 24'000'001, 24'000'000));
static_assert(!ComputeSpiTiming(64'000'000, 249'999, 24'000'000));
static_assert(ComputeSpiTiming(64'000'000, 250'000, 24'000'000)->prescaler ==
              SPI_BAUDRATEPRESCALER_256);
static_assert(ComputeSpiTiming(64'000'000, 1'000'000, 24'000'000)->actual_hz ==
              1'000'000);
static_assert(ComputeSpiTiming(64'000'001, 1'000'000, 24'000'000)->prescaler ==
              SPI_BAUDRATEPRESCALER_128);
static_assert(ComputeSpiTiming(90'000'000, 45'000'000, 45'000'000)->prescaler ==
              SPI_BAUDRATEPRESCALER_2);

int main() {
  std::array<uint8_t, 3> tx{0x00, 0x5a, 0xff}, rx{};
  {
    Spi spi(SpiPeripheral::kSpi1, PinName::kPA5, PinName::kPA6, PinName::kPA7);
    assert(spi.GetFrequencyHz() == 0);
    assert(!spi.Send(tx) && !spi.Receive(rx) && !spi.Transfer(tx, rx));
    const auto init_calls = spi_init_calls;
    assert(spi.Initialize() && spi.Initialize());
    assert(spi_init_calls == init_calls + 1);
    assert(last_spi_init.Instance == SPI1);
    assert(SPI1->enabled);
    assert(last_spi_init.Init.Mode == SPI_MODE_MASTER);
    assert(last_spi_init.Init.Direction == SPI_DIRECTION_2LINES);
    assert(last_spi_init.Init.DataSize == SPI_DATASIZE_8BIT);
    assert(last_spi_init.Init.NSS == SPI_NSS_SOFT);
    assert(spi.GetFrequencyHz() ==
           ComputeSpiTiming(HAL_RCC_GetPCLK2Freq(), 1'000'000, 45'000'000)
               ->actual_hz);
    Spi duplicate(SpiPeripheral::kSpi1, PinName::kPB3, PinName::kPB4,
                  PinName::kPB5);
    assert(!duplicate.Initialize());
    assert(spi.Transfer(tx, rx, 37) && rx == tx);
    assert(spi_timeout == 37 && spi_size == tx.size());
    assert(spi.Send(tx, 19) && spi_timeout == 19);
    assert((tx == std::array<uint8_t, 3>{0x00, 0x5a, 0xff}));
    assert(spi.Receive(rx, 23) && spi_timeout == 23);
    assert((spi_tx_data == std::vector<uint8_t>{0xff, 0xff, 0xff}));
    assert(spi.Transfer(tx, tx) && spi_tx_data[1] == 0x5a);

    const auto calls = spi_transfer_calls;
    assert(!spi.Send({}) && !spi.Receive({}) && !spi.Transfer({}, {}));
    assert(!spi.Transfer(tx, std::span<uint8_t>(rx).first(2)));
    std::array<uint8_t, 4> overlap{};
    assert(!spi.Transfer(std::span(overlap).first(3),
                         std::span(overlap).subspan(1)));
    assert(!spi.Transfer(std::span(overlap).subspan(1),
                         std::span(overlap).first(3)));
    std::vector<uint8_t> large(65536, 0xa5);
    assert(!spi.Send(large) && !spi.Receive(large) &&
           !spi.Transfer(large, large));
    assert(spi_transfer_calls == calls);
    large.resize(65535);
    assert(spi.Send(large) && spi_size == 65535);
    assert(spi.Transfer(large, large) && spi_size == 65535);
    assert(spi.Receive(large) && spi_size == 65535);
    assert(std::all_of(spi_tx_data.begin(), spi_tx_data.end(),
                       [](uint8_t value) { return value == 0xff; }));
    spi_transfer_status = HAL_ERROR;
    assert(!spi.Send(tx) && !spi.Receive(rx) && !spi.Transfer(tx, rx));
    spi_transfer_status = HAL_OK;
  }
  // Destruction and failed initialization both release peripheral ownership.
  assert(!SPI1->enabled);
  const auto deinit_calls = spi_deinit_calls;
  {
    Spi spi(SpiPeripheral::kSpi1, PinName::kPB3, PinName::kPB4, PinName::kPB5);
    spi_init_status = HAL_ERROR;
    assert(!spi.Initialize() && spi.GetFrequencyHz() == 0);
    assert(spi_deinit_calls == deinit_calls + 1);
    Spi retry(SpiPeripheral::kSpi1, PinName::kPA5, PinName::kPA6,
              PinName::kPA7);
    spi_init_status = HAL_OK;
    assert(retry.Initialize());
    assert(!spi.Initialize());
  }
  for (unsigned mode = 0; mode < 4; ++mode) {
    for (unsigned order = 0; order < 2; ++order) {
      for (const auto sck : {PinName::kPA5, PinName::kPB3}) {
        for (const auto miso : {PinName::kPA6, PinName::kPB4}) {
          for (const auto mosi : {PinName::kPA7, PinName::kPB5}) {
            Spi spi(SpiPeripheral::kSpi1, sck, miso, mosi, 1'000'000,
                    static_cast<SpiMode>(mode),
                    static_cast<SpiBitOrder>(order));
            gpio_inits.clear();
            assert(spi.Initialize());
            assert(gpio_inits.size() == 3);
            for (const auto& entry : gpio_inits) {
              assert(entry.second.Alternate == GPIO_AF5_SPI1);
              assert(entry.second.Mode == GPIO_MODE_AF_PP);
            }
            assert(last_spi_init.Init.CLKPolarity ==
                   (mode & 2 ? SPI_POLARITY_HIGH : SPI_POLARITY_LOW));
            assert(last_spi_init.Init.CLKPhase ==
                   (mode & 1 ? SPI_PHASE_2EDGE : SPI_PHASE_1EDGE));
            assert(last_spi_init.Init.FirstBit ==
                   (order ? SPI_FIRSTBIT_LSB : SPI_FIRSTBIT_MSB));
          }
        }
      }
    }
  }
  for (const auto pin : {PinName::kNC, PinName::kPB6, PinName::kPC5}) {
    Spi bad_sck(SpiPeripheral::kSpi1, pin, PinName::kPA6, PinName::kPA7);
    Spi bad_miso(SpiPeripheral::kSpi1, PinName::kPA5, pin, PinName::kPA7);
    Spi bad_mosi(SpiPeripheral::kSpi1, PinName::kPA5, PinName::kPA6, pin);
    assert(!bad_sck.Initialize() && !bad_miso.Initialize() &&
           !bad_mosi.Initialize());
  }
  for (const auto frequency : {0U, 1U, 54'000'001U}) {
    Spi bad(SpiPeripheral::kSpi1, PinName::kPA5, PinName::kPA6, PinName::kPA7,
            frequency);
    assert(!bad.Initialize());
  }
  Spi bad_peripheral(static_cast<SpiPeripheral>(255), PinName::kPA5,
                     PinName::kPA6, PinName::kPA7);
  assert(!bad_peripheral.Initialize());
  Spi bad_mode(SpiPeripheral::kSpi1, PinName::kPA5, PinName::kPA6,
               PinName::kPA7, 1'000'000, static_cast<SpiMode>(4));
  assert(!bad_mode.Initialize());
  Spi bad_order(SpiPeripheral::kSpi1, PinName::kPA5, PinName::kPA6,
                PinName::kPA7, 1'000'000, SpiMode::kMode0,
                static_cast<SpiBitOrder>(2));
  assert(!bad_order.Initialize());

#ifdef STM32F446xx
  for (const auto sck :
       {PinName::kPA9, PinName::kPB10, PinName::kPB13, PinName::kPC7}) {
    for (const auto miso : {PinName::kPB14, PinName::kPC2}) {
      for (const auto mosi : {PinName::kPB15, PinName::kPC1, PinName::kPC3}) {
        Spi spi(SpiPeripheral::kSpi2, sck, miso, mosi);
        gpio_inits.clear();
        assert(spi.Initialize() && last_spi_init.Instance == SPI2);
        assert(spi.GetFrequencyHz() == 703125);
        assert(gpio_inits.back().second.Alternate ==
               (mosi == PinName::kPC1 ? GPIO_AF7_SPI2 : GPIO_AF5_SPI1));
      }
    }
  }
  for (const auto sck : {PinName::kPB3, PinName::kPC10}) {
    for (const auto miso : {PinName::kPB4, PinName::kPC11}) {
      for (const auto mosi : {PinName::kPB0, PinName::kPB2, PinName::kPB5,
                              PinName::kPC1, PinName::kPC12}) {
        Spi spi(SpiPeripheral::kSpi3, sck, miso, mosi);
        gpio_inits.clear();
        assert(spi.Initialize() && last_spi_init.Instance == SPI3);
        assert(spi.GetFrequencyHz() == 703125);
        assert(gpio_inits.back().second.Alternate ==
               (mosi == PinName::kPB0 || mosi == PinName::kPB2 ? GPIO_AF7_SPI3
                : mosi == PinName::kPC1 ? GPIO_AF5_SPI1
                                        : GPIO_AF6_SPI3));
      }
    }
  }
  Spi first(SpiPeripheral::kSpi1, PinName::kPA5, PinName::kPA6, PinName::kPA7);
  Spi second(SpiPeripheral::kSpi2, PinName::kPB13, PinName::kPB14,
             PinName::kPB15);
  Spi third(SpiPeripheral::kSpi3, PinName::kPC10, PinName::kPC11,
            PinName::kPC12);
  assert(first.Initialize() && second.Initialize() && third.Initialize());
#endif
#ifdef STM32F767xx
  // F7 differs from F446: PC1 SPI2 is AF5; PC7/PB0/PC1 SPI3 are not valid.
  for (const auto sck : {PinName::kPA9, PinName::kPA12, PinName::kPB10,
                         PinName::kPB13, PinName::kPD3}) {
    for (const auto miso : {PinName::kPB14, PinName::kPC2}) {
      for (const auto mosi : {PinName::kPB15, PinName::kPC1, PinName::kPC3}) {
        Spi spi(SpiPeripheral::kSpi2, sck, miso, mosi);
        gpio_inits.clear();
        assert(spi.Initialize() && spi.GetFrequencyHz() == 843750);
        assert(gpio_inits.back().second.Alternate == 5);
      }
    }
  }
  for (const auto sck : {PinName::kPB3, PinName::kPC10}) {
    for (const auto miso : {PinName::kPB4, PinName::kPC11}) {
      for (const auto mosi :
           {PinName::kPB2, PinName::kPB5, PinName::kPC12, PinName::kPD6}) {
        Spi spi(SpiPeripheral::kSpi3, sck, miso, mosi);
        gpio_inits.clear();
        assert(spi.Initialize() && spi.GetFrequencyHz() == 843750);
        const uint32_t af = mosi == PinName::kPB2   ? 7
                            : mosi == PinName::kPD6 ? 5
                                                    : 6;
        assert(gpio_inits.back().second.Alternate == af);
      }
    }
  }
  Spi extra(SpiPeripheral::kSpi1, PinName::kPG11, PinName::kPG9, PinName::kPD7);
  assert(extra.Initialize());
  Spi bad_sck(SpiPeripheral::kSpi2, PinName::kPC7, PinName::kPC2,
              PinName::kPC3);
  Spi bad_mosi(SpiPeripheral::kSpi3, PinName::kPC10, PinName::kPC11,
               PinName::kPC1);
  assert(!bad_sck.Initialize() && !bad_mosi.Initialize());
  using enum PinName;
  using Route = std::pair<PinName, uint32_t>;
  const auto check_routes = [](SpiPeripheral peripheral, SPI_TypeDef* instance,
                               std::initializer_list<Route> scks,
                               std::initializer_list<Route> misos,
                               std::initializer_list<Route> mosis) {
    for (const auto& [sck, sck_af] : scks) {
      for (const auto& [miso, miso_af] : misos) {
        for (const auto& [mosi, mosi_af] : mosis) {
          gpio_inits.clear();
          Spi spi(peripheral, sck, miso, mosi, 50'000'000);
          assert(spi.Initialize() && last_spi_init.Instance == instance);
          assert(spi.GetFrequencyHz() == 27'000'000);
          assert(gpio_inits.size() == 3 &&
                 gpio_inits[0].second.Alternate == sck_af &&
                 gpio_inits[1].second.Alternate == miso_af &&
                 gpio_inits[2].second.Alternate == mosi_af);
        }
      }
    }
  };
  check_routes(SpiPeripheral::kSpi4, SPI4, {{kPE2, 5}, {kPE12, 5}},
               {{kPE5, 5}, {kPE13, 5}}, {{kPE6, 5}, {kPE14, 5}});
  check_routes(SpiPeripheral::kSpi5, SPI5, {{kPF7, 5}}, {{kPF8, 5}},
               {{kPF9, 5}, {kPF11, 5}});
  check_routes(SpiPeripheral::kSpi6, SPI6, {{kPA5, 8}, {kPB3, 8}, {kPG13, 5}},
               {{kPA6, 8}, {kPB4, 8}, {kPG12, 5}},
               {{kPA7, 8}, {kPB5, 8}, {kPG14, 5}});
#endif
}
