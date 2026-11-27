#pragma once
// IWYU pragma: private, include "nbed.h"

#include <chrono>
#include <cstdint>

#include "interface/clock.h"
#include "stm32f3xx_hal.h"  // IWYU pragma: export
#include "stm32f3xx_hal_rcc.h"

namespace nkrc::nbed::stm32::f3 {

/// @brief STM32 のシステムクロック、バス分周とミリ秒待機を管理する。
class Clock final : public interface::Clock {
 public:
  /// @brief PLL の入力に使う発振源。
  enum class OscillatorSource : uint8_t {
    kHsi,  ///< 内蔵高速 RC 発振器。
    kHse,  ///< 外部高速クロック。
  };

  /// @brief 外部高速クロックの接続方式。
  enum class HseMode : uint8_t {
    kCrystal,  ///< 水晶振動子。
    kBypass,   ///< ST-LINK MCO などの外部クロック入力。
  };

  /// @brief 発振源、PLL、バスクロック、フラッシュ待ち時間の設定。
  struct Configuration {
    OscillatorSource oscillator_source{
        OscillatorSource::kHsi};          ///< PLL に使うクロックの発振源。
    HseMode hse_mode{HseMode::kCrystal};  ///< HSE を使う場合の接続方式。
    uint32_t hse_frequency_hz{
        HSE_VALUE};  ///< HSE の周波数（Hz）。HAL の HSE_VALUE と一致させる。
    uint32_t pll_multiplier{16};  ///< PLL の逓倍比（2～16）。
    uint32_t hse_predivider{1};   ///< HSE 入力の分周比（1～16）。
    uint32_t ahb_divider{
        RCC_SYSCLK_DIV1};  ///< AHB の分周に使う HAL の RCC_SYSCLK_DIVx 定数。
    uint32_t apb1_divider{
        RCC_HCLK_DIV2};  ///< APB1 の分周に使う HAL の RCC_HCLK_DIVx 定数。
    uint32_t apb2_divider{
        RCC_HCLK_DIV1};  ///< APB2 の分周に使う HAL の RCC_HCLK_DIVx 定数。
    uint32_t flash_latency{
        0xffffffffU};  ///< フラッシュの待ちサイクル数。0xffffffff で自動選択。
  };

  /// @brief 既定の構成でシステムクロックを初期化する。
  void Initialize() override;
  /// @brief 指定した発振源と分周でクロックを初期化する。
  /// @param configuration 発振源、PLL、分周比とフラッシュ設定。
  /// @return 構成が有効で HAL の初期化に成功すれば true。
  [[nodiscard]] static bool Initialize(const Configuration& configuration);

  /// @brief 指定時間だけ呼び出し元を待機させる。
  /// @param milliseconds 待機時間（ミリ秒）。0 は即時復帰する。
  /// @note HAL の時刻更新が動く場所から呼ぶ。割り込みの中では使用しない。
  void Sleep(uint32_t milliseconds) override;
  /// @brief 時間型で指定した時間だけ呼び出し元を待機させる。
  /// @param milliseconds 待機時間。0 以下なら即時復帰する。
  /// @note 割り込みの中では使用しない。
  void Sleep(std::chrono::milliseconds milliseconds) override;

  /// @brief SYSCLK の周波数を取得する。
  /// @return クロック周波数（Hz）。
  [[nodiscard]] static uint32_t SystemClockHz();
  /// @brief AHB の HCLK の周波数を取得する。
  /// @return クロック周波数（Hz）。
  [[nodiscard]] static uint32_t HclkHz();
  /// @brief APB1 の PCLK1 の周波数を取得する。
  /// @return クロック周波数（Hz）。
  [[nodiscard]] static uint32_t Pclk1Hz();
  /// @brief APB2 の PCLK2 の周波数を取得する。
  /// @return クロック周波数（Hz）。
  [[nodiscard]] static uint32_t Pclk2Hz();
};

/// @brief アプリケーション全体で共有するクロックを管理する。
class ClockManager {
 public:
  /// @brief 共有クロックは静的メソッドから利用する。
  ClockManager() = delete;

  /// @brief 既定の設定で共有クロックを初期化する。
  /// @return 初期化に成功すれば true。
  [[nodiscard]] static bool Initialize();
  /// @brief 指定の設定で共有クロックを初期化する。
  /// @param configuration クロック構成。
  /// @return 初期化に成功すれば true。
  [[nodiscard]] static bool Initialize(
      const Clock::Configuration& configuration);

  /// @brief 現在の共有クロックを取得する。
  /// @return 共有クロックへの参照。
  static Clock& GetClock();

 private:
  static constinit Clock clock_;
};

}  // namespace nkrc::nbed::stm32::f3
