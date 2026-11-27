#pragma once
// IWYU pragma: private, include "nbed.h"

#include <cstdint>

#include "interface/time.h"

namespace nkrc::nbed::stm32::f7 {

/// @brief HAL のミリ秒カウンターと DWT から起動後の時刻を取得する。
class Time {
 public:
  /// @brief クロックが確定した後に時間計測を初期化する。
  /// @return 時間計測を使用できる場合に true。
  /// @note 通常は System::Initialize() が呼ぶ。
  static bool Initialize();

  /// @brief SysTick が進むたびに時刻計測用の内部状態を更新する。
  /// @note 割り込みハンドラーから HAL_IncTick() の後に呼ばれる。
  static void OnSysTick();

  /// @brief 起動後の HAL 時刻をミリ秒で読み取る。
  /// @return 32 ビットのミリ秒値。約 49.7 日で周回する。
  [[nodiscard]] static uint32_t Milliseconds();
  /// @brief ミリ秒と CPU サイクルから起動後の時刻を求める。
  /// @return 64 ビットのマイクロ秒値。
  [[nodiscard]] static uint64_t Microseconds();
};

/// @brief 開始・停止・再開ができる経過時間計。System::Initialize() の後に使う。
class Timer final : public interface::Timer {
 public:
  /// @brief 経過時間の計測を開始または再開する。計測中なら何もしない。
  void Start() override;
  /// @brief 経過時間を保持して計測を一時停止する。
  void Stop() override;
  /// @brief 経過時間を 0 に戻す。計測中ならその時点から引き続き計測する。
  void Reset() override;

  /// @brief 現在、経過時間を計測しているか調べる。
  /// @return 計測中なら true。
  [[nodiscard]] bool IsRunning() const override;
  /// @brief 計測を開始してからの合計時間を読む。
  /// @return 停止中を除いた時間（µs）。
  [[nodiscard]] uint64_t ElapsedMicroseconds() const override;

 private:
  uint64_t elapsed_us_{0};
  uint64_t started_at_us_{0};
  bool running_{false};
};

}  // namespace nkrc::nbed::stm32::f7
