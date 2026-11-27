#pragma once

#include "units.h"

namespace nkrc::nbed::interface {

/// @brief A 相・B 相から回転量を得るインクリメンタルエンコーダーのインターフェース。
class Encoder {
 public:
  /// @brief 派生クラスの初期化を準備する。
  Encoder() = default;
  /// @brief 派生クラスのオブジェクトを破棄する。
  virtual ~Encoder() = default;
  /// @brief オブジェクトのコピー構築は禁止。
  Encoder(const Encoder&) = delete;
  /// @brief オブジェクトのコピー代入は禁止。
  Encoder& operator=(const Encoder&) = delete;
  /// @brief オブジェクトのムーブ構築は禁止。
  Encoder(Encoder&&) = delete;
  /// @brief オブジェクトのムーブ代入は禁止。
  Encoder& operator=(Encoder&&) = delete;

  /// @brief 2 相入力とカウンターを設定し、計数を始める。
  /// @return 計数できる状態になれば true。
  virtual bool Initialize() = 0;

  /// @brief A 相・B 相の両エッジを数えた現在の値を返す。
  /// @return 符号付きカウント。カウンター幅で周回し、未初期化では 0。
  [[nodiscard]] virtual int GetCount() = 0;
  /// @brief カウントと初期設定の 1 回転当たりカウント数から角度を計算する。
  /// @return 累積角度。未初期化では 0 度。
  [[nodiscard]] virtual Angle GetAngle() = 0;

  /// @brief 現在の位置を原点としてカウントを 0 にする。
  virtual void ResetCount() = 0;
};

}  // namespace nkrc::nbed::interface
