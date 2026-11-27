#pragma once

#include <cstdint>

namespace nkrc::nbed::interface {

/// @brief マイクロ秒単位の経過時間を測るインターフェース。
class Timer {
 public:
  /// @brief 派生クラスの初期化を準備する。
  Timer() = default;
  /// @brief 派生クラスのオブジェクトを破棄する。
  virtual ~Timer() = default;
  /// @brief オブジェクトのコピー構築は禁止。
  Timer(const Timer&) = delete;
  /// @brief オブジェクトのコピー代入は禁止。
  Timer& operator=(const Timer&) = delete;
  /// @brief オブジェクトのムーブ構築は禁止。
  Timer(Timer&&) = delete;
  /// @brief オブジェクトのムーブ代入は禁止。
  Timer& operator=(Timer&&) = delete;

  /// @brief 経過時間の計測を開始または再開する。計測中なら何もしない。
  virtual void Start() = 0;
  /// @brief 経過時間を保持して計測を一時停止する。
  virtual void Stop() = 0;
  /// @brief 経過時間を 0 に戻す。計測中ならその時点から引き続き計測する。
  virtual void Reset() = 0;

  /// @brief 現在、経過時間を計測しているか調べる。
  /// @return 計測中なら true。
  [[nodiscard]] virtual bool IsRunning() const = 0;
  /// @brief 計測を開始してからの合計時間を読む。
  /// @return 停止中を除いた時間（µs）。
  [[nodiscard]] virtual uint64_t ElapsedMicroseconds() const = 0;
};

}  // namespace nkrc::nbed::interface
