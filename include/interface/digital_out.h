#pragma once

#include "digital_in.h"

namespace nkrc::nbed::interface {

/// @brief デジタル出力ピンの High／Low を設定するインターフェース。
class DigitalOut {
 public:
  /// @brief 派生クラスの初期化を準備する。
  DigitalOut() = default;
  /// @brief 派生クラスのオブジェクトを破棄する。
  virtual ~DigitalOut() = default;
  /// @brief オブジェクトのコピー構築は禁止。
  DigitalOut(const DigitalOut&) = delete;
  /// @brief オブジェクトのコピー代入は禁止。
  DigitalOut& operator=(const DigitalOut&) = delete;
  /// @brief オブジェクトのムーブ構築は禁止。
  DigitalOut(DigitalOut&&) = delete;
  /// @brief オブジェクトのムーブ代入は禁止。
  DigitalOut& operator=(DigitalOut&&) = delete;

  /// @brief ピンを出力に設定し、指定した初期値を出力する。
  /// @return 使用できる状態になれば true。
  virtual bool Initialize() = 0;

  /// @brief 出力レベルを設定する。未初期化なら何もしない。
  /// @param value true で High、false で Low。
  virtual void Write(bool value) = 0;
  /// @brief ピンで読み取った現在のレベルを返す。
  /// @return High なら true。Low または未初期化では false。
  [[nodiscard]] virtual bool Read() const = 0;

  /// @brief 現在ピンに現れている論理レベルに変換する。
  /// @return High なら true。
  [[nodiscard]] explicit operator bool() const { return Read(); }

  /// @brief 指定した論理値を出力する。
  /// @param value true で High、false で Low。
  /// @return この出力オブジェクト。
  virtual DigitalOut& operator=(bool value) {
    Write(value);
    return *this;
  }

  /// @brief 入力ピンの現在値を読み取り、その値を出力する。
  /// @param input 入力オブジェクト。
  /// @return この出力オブジェクト。
  DigitalOut& operator=(const DigitalIn& input) {
    Write(input.Read());
    return *this;
  }
};

}  // namespace nkrc::nbed::interface
