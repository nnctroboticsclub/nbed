#pragma once

#include <cstdint>
#include <span>

#include "interface/dma.h"

namespace nkrc::nbed::interface {

/// @brief 周波数とパルス幅を指定して PWM を出力するインターフェース。
class Pwm {
 public:
  /// @brief 派生クラスの初期化を準備する。
  Pwm() = default;
  /// @brief 派生クラスのオブジェクトを破棄する。
  virtual ~Pwm() = default;
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
  virtual bool Initialize() = 0;

  /// @brief PWM のアクティブ時間をマイクロ秒で指定する。
  /// @param pulse_width_us パルス幅（µs）。1 周期を超える値は出力可能範囲に収める。
  /// @return 設定できれば true。未初期化や DMA 使用中は false。
  virtual bool SetPulseWidthUs(uint32_t pulse_width_us) = 0;
  /// @brief 周波数を変更する。初期化前は初期値を設定する。
  /// @param frequency_hz 周波数（Hz）。0 より大きい値。
  /// @return 設定できれば true。同じタイマで複数チャンネルを使用中、または DMA 使用中は false。
  virtual bool SetFrequencyHz(uint32_t frequency_hz) = 0;
  /// @brief PWM のデューティ比を設定する。
  /// @param duty_cycle アクティブ時間の割合。0.25 は 25%。0.0～1.0 に収める。
  /// @return 設定できれば true。未初期化、NaN、DMA 使用中は false。
  virtual bool SetDutyCycle(float duty_cycle) = 0;

  /// @brief 比較値の配列を 1 周期ごとに DMA で送り、パルス幅を変える。
  /// @param data 3～65,535 個の CCR 比較値。各値は周期ティック数より小さく、末尾 2 個は 0。
  /// @return DMA が始まれば true。タイマや DMA の競合、未対応の出力設定では false。
  /// @note 配列は転送完了または中断まで保持する。
  virtual bool SendDataDma(std::span<const uint32_t> data) = 0;
  /// @brief PWM の DMA 転送状態を読む。
  /// @return 未開始、転送中、完了、エラー、中断のいずれか。
  [[nodiscard]] virtual DmaStatus GetDmaStatus() const = 0;
  /// @brief 分周後のタイマで 1 周期にかかる刻み数を返す。
  /// @return 周期のティック数（ARR + 1）。未初期化では 0。
  [[nodiscard]] virtual uint64_t GetPeriodTicks() const = 0;
  /// @brief DMA 転送を止め、DMA の使用権を解放する。
  virtual void StopDma() = 0;

  /// @brief PWM の周波数を読む。
  /// @return 初期化後は分周後の周波数（整数 Hz）。初期化前は指定値。
  [[nodiscard]] virtual uint32_t GetFrequencyHz() const = 0;

  /// @brief PWM と DMA を停止し、タイマチャンネルを解放する。
  virtual void Stop() = 0;
};

}  // namespace nkrc::nbed::interface
