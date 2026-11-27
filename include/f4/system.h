#pragma once
// IWYU pragma: private, include "nbed.h"

#include "f4/clock.h"
#include "interface/error.h"

namespace nkrc::nbed::stm32::f4 {

/// @brief STM32 の既定エラー処理。
class Error final : public interface::Error {
 public:
  /// @brief 割り込みを無効にし、その場で処理を停止する。
  void ErrorHandler() override;
};

/// @brief アプリケーションが使う HAL、クロック、時刻、既定のコンソールを準備する。
class System {
 public:
  /// @brief ボード既定の設定で nbed を初期化する。main() の先頭で 1 回呼ぶ。
  static void Initialize();
  /// @brief 任意のクロック構成で nbed を初期化する。
  /// @param configuration ボードの発振子と目的のクロックに合った構成。
  static void Initialize(const Clock::Configuration& configuration);

  /// @brief 割り込みを無効にし、その場で処理を停止する。
  static void ErrorHandler();
};

}  // namespace nkrc::nbed::stm32::f4
