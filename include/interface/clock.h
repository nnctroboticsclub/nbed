#pragma once

#include <chrono>
#include <cstdint>

namespace nkrc::nbed::interface {

/// @brief システムクロックとミリ秒単位の待機を扱うインターフェース。
class Clock {
 public:
  /// @brief 派生クラスの初期化を準備する。
  Clock() = default;
  /// @brief 派生クラスのオブジェクトを破棄する。
  virtual ~Clock() = default;
  /// @brief オブジェクトのコピー構築は禁止。
  Clock(const Clock&) = delete;
  /// @brief オブジェクトのコピー代入は禁止。
  Clock& operator=(const Clock&) = delete;
  /// @brief オブジェクトのムーブ構築は禁止。
  Clock(Clock&&) = delete;
  /// @brief オブジェクトのムーブ代入は禁止。
  Clock& operator=(Clock&&) = delete;

  /// @brief 既定の構成でシステムクロックを初期化する。
  virtual void Initialize() = 0;

  /// @brief 指定時間だけ呼び出し元を待機させる。
  /// @param milliseconds 待機時間（ミリ秒）。0 は即時復帰する。
  /// @note HAL の時刻更新が動く場所から呼ぶ。割り込みの中では使用しない。
  virtual void Sleep(uint32_t milliseconds) = 0;
  /// @brief 時間型で指定した時間だけ呼び出し元を待機させる。
  /// @param milliseconds 待機時間。0 以下なら即時復帰する。
  /// @note 割り込みの中では使用しない。
  virtual void Sleep(std::chrono::milliseconds milliseconds) = 0;
};

}  // namespace nkrc::nbed::interface
