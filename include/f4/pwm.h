#pragma once
// IWYU pragma: private, include "nbed.h"

#include <cstdint>
#include <span>

#include "detail/dma.h"
#include "interface/pin_names.h"
#include "interface/pwm.h"
#include "stm32f4xx_hal.h"  // IWYU pragma: export

namespace nkrc::nbed::stm32::f4 {

/// @brief NUCLEO-F446RE で PWM 出力に使える TIM。
enum class PwmTimer : uint8_t {
  kTim1,   ///< TIM1 を使用する。
  kTim2,   ///< TIM2 を使用する。
  kTim3,   ///< TIM3 を使用する。
  kTim4,   ///< TIM4 を使用する。
  kTim5,   ///< TIM5 を使用する。
  kTim8,   ///< TIM8 を使用する。
  kTim9,   ///< TIM9 を使用する。
  kTim10,  ///< TIM10 を使用する。
  kTim11,  ///< TIM11 を使用する。
  kTim12,  ///< TIM12 を使用する。
  kTim13,  ///< TIM13 を使用する。
  kTim14,  ///< TIM14 を使用する。
};
/// @brief タイマの通常（CHx）および相補（CHxN）出力の選択。
enum class PwmChannel : uint8_t {
  kCh1,   ///< CH1 の通常出力。
  kCh2,   ///< CH2 の通常出力。
  kCh3,   ///< CH3 の通常出力。
  kCh4,   ///< CH4 の通常出力。
  kCh1N,  ///< CH1N の相補出力。
  kCh2N,  ///< CH2N の相補出力。
  kCh3N,  ///< CH3N の相補出力。
};

/// @brief タイマのチャンネルから周期的な PWM 波形を出力する。
class Pwm final : public interface::Pwm {
 public:
  /// @brief 同じ TIM を使う複数チャンネルの内部状態。
  struct TimerState;

  /// @brief PWM の極性、相補出力、デッドタイムの HAL 設定。
  struct Configuration {
    interface::PinName complementary_pin{
        interface::PinName::
            kNC};  ///< 通常出力と同時に使う CHxN ピン。kNC なら通常出力のみ。
    uint32_t polarity{
        TIM_OCPOLARITY_HIGH};  ///< 通常出力の極性。既定はアクティブ High。
    uint32_t complementary_polarity{
        TIM_OCNPOLARITY_HIGH};  ///< 相補出力の極性。既定はアクティブ High。
    uint32_t idle_state{TIM_OCIDLESTATE_RESET};  ///< 通常出力のアイドルレベル。
    uint32_t complementary_idle_state{
        TIM_OCNIDLESTATE_RESET};  ///< 相補出力のアイドルレベル。
    uint32_t dead_time{
        0};  ///< デッドタイムの DTG レジスター値（0～255）。同じタイマの各出力で揃える。
    uint32_t off_state_run_mode{
        TIM_OSSR_DISABLE};  ///< 動作中に主出力を切ったときの出力の扱い（OSSR）。
    uint32_t off_state_idle_mode{
        TIM_OSSI_DISABLE};  ///< アイドル時の出力の扱い（OSSI）。
  };

  /// @brief タイマと出力ピンを指定して PWM を作る。
  /// @param timer 使用するタイマ。同じタイマの各チャンネルは周波数を共有する。
  /// @param channel 通常または相補出力のチャンネル。
  /// @param pin 選んだチャンネルの出力ピン。
  /// @param frequency_hz 周波数（Hz）。既定は 50。
  Pwm(PwmTimer timer, PwmChannel channel, interface::PinName pin,
      uint32_t frequency_hz = 50);
  /// @brief タイマと出力ピンを指定して PWM を作る。
  /// @param timer 使用するタイマ。同じタイマの各チャンネルは周波数を共有する。
  /// @param channel 通常または相補出力のチャンネル。
  /// @param pin 選んだチャンネルの出力ピン。
  /// @param frequency_hz 周波数（Hz）。既定は 50。
  /// @param configuration 極性、相補出力、デッドタイムなどの追加設定。
  Pwm(PwmTimer timer, PwmChannel channel, interface::PinName pin,
      uint32_t frequency_hz, const Configuration& configuration);
  /// @brief PWM と DMA を停止し、チャンネルを解放する。
  ~Pwm() override;
  /// @brief オブジェクトのコピー構築は禁止。
  Pwm(const Pwm&) = delete;
  /// @brief オブジェクトのコピー代入は禁止。
  Pwm& operator=(const Pwm&) = delete;
  /// @brief オブジェクトのムーブ構築は禁止。
  Pwm(Pwm&&) = delete;
  /// @brief オブジェクトのムーブ代入は禁止。
  Pwm& operator=(Pwm&&) = delete;

  /// @brief タイマ、チャンネル、ピンを設定し、出力を始める。
  /// @return 出力できる状態になれば true。
  bool Initialize() override;

  /// @brief PWM のアクティブ時間をマイクロ秒で指定する。
  /// @param pulse_width_us パルス幅（µs）。1 周期を超える値は出力可能範囲に収める。
  /// @return 設定できれば true。未初期化や DMA 使用中は false。
  bool SetPulseWidthUs(uint32_t pulse_width_us) override;
  /// @brief 周波数を変更する。初期化前は初期値を設定する。
  /// @param frequency_hz 周波数（Hz）。0 より大きい値。
  /// @return 設定できれば true。同じタイマで複数チャンネルを使用中、または DMA 使用中は false。
  bool SetFrequencyHz(uint32_t frequency_hz) override;
  /// @brief PWM のデューティ比を設定する。
  /// @param duty_cycle アクティブ時間の割合。0.25 は 25%。0.0～1.0 に収める。
  /// @return 設定できれば true。未初期化、NaN、DMA 使用中は false。
  bool SetDutyCycle(float duty_cycle) override;

  /// @brief PWM の周波数を読む。
  /// @return 初期化後は分周後の周波数（整数 Hz）。初期化前は指定値。
  [[nodiscard]] uint32_t GetFrequencyHz() const override;

  /// @brief 比較値の配列を 1 周期ごとに DMA で送り、パルス幅を変える。
  /// @param data 3～65,535 個の CCR 比較値。各値は周期ティック数より小さく、末尾 2 個は 0。
  /// @return DMA が始まれば true。タイマや DMA の競合、未対応の出力設定では false。
  /// @note 配列は転送完了または中断まで保持する。
  bool SendDataDma(std::span<const uint32_t> data) override;
  /// @brief PWM の DMA 転送状態を読む。
  /// @return 未開始、転送中、完了、エラー、中断のいずれか。
  [[nodiscard]] interface::DmaStatus GetDmaStatus() const override;
  /// @brief 分周後のタイマで 1 周期にかかる刻み数を返す。
  /// @return 周期のティック数（ARR + 1）。未初期化では 0。
  [[nodiscard]] uint64_t GetPeriodTicks() const override;
  /// @brief DMA 転送を止め、DMA の使用権を解放する。
  void StopDma() override;

  /// @brief PWM と DMA を停止し、タイマチャンネルを解放する。
  void Stop() override;

 private:
  [[nodiscard]] TimerState& State() const;
  [[nodiscard]] uint32_t HalChannel() const;
  [[nodiscard]] bool IsValidConfiguration() const;
  [[nodiscard]] uint32_t TimerClockHz() const;

  static void OnDmaComplete(DMA_HandleTypeDef* handle);
  static void OnDmaError(DMA_HandleTypeDef* handle);
  void FinishDma(interface::DmaStatus status);
  detail::Dma dma_;
  volatile interface::DmaStatus dma_status_{interface::DmaStatus::kIdle};

  PwmTimer timer_;
  PwmChannel channel_;
  interface::PinName pin_;
  uint32_t frequency_hz_;
  Configuration configuration_{};
  bool initialized_{false};
};

}  // namespace nkrc::nbed::stm32::f4
