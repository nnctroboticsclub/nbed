#pragma once

namespace nkrc::nbed::interface {

/// @brief デジタル入力ピンの High／Low を読み取るインターフェース。
class DigitalIn {
 public:
  /// @brief 派生クラスの初期化を準備する。
  DigitalIn() = default;
  /// @brief 派生クラスのオブジェクトを破棄する。
  virtual ~DigitalIn() = default;
  /// @brief オブジェクトのコピー構築は禁止。
  DigitalIn(const DigitalIn&) = delete;
  /// @brief オブジェクトのコピー代入は禁止。
  DigitalIn& operator=(const DigitalIn&) = delete;
  /// @brief オブジェクトのムーブ構築は禁止。
  DigitalIn(DigitalIn&&) = delete;
  /// @brief オブジェクトのムーブ代入は禁止。
  DigitalIn& operator=(DigitalIn&&) = delete;

  /// @brief 指定したピンをデジタル入力に設定する。
  /// @return 使用できる状態になれば true。
  virtual bool Initialize() = 0;

  /// @brief 入力ピンの現在のレベルを読む。
  /// @return High なら true。Low または未初期化では false。
  [[nodiscard]] virtual bool Read() const = 0;

  /// @brief 現在ピンに現れている論理レベルに変換する。
  /// @return High なら true。
  [[nodiscard]] explicit operator bool() const { return Read(); }

  /// @brief 入力レベルと指定値を比較する。
  /// @param value 比較する論理値。
  /// @return 一致すれば true。
  [[nodiscard]] bool operator==(bool value) const { return Read() == value; }

  /// @brief 入力レベルと指定値を比較する。
  /// @param value 比較する論理値。
  /// @return 異なれば true。
  [[nodiscard]] bool operator!=(bool value) const { return Read() != value; }

  /// @brief 指定値と入力レベルを比較する。
  /// @param value 比較する論理値。
  /// @param input 入力オブジェクト。
  /// @return 一致すれば true。
  friend bool operator==(bool value, const DigitalIn& input) {
    return input == value;
  }

  /// @brief 指定値と入力レベルを比較する。
  /// @param value 比較する論理値。
  /// @param input 入力オブジェクト。
  /// @return 異なれば true。
  friend bool operator!=(bool value, const DigitalIn& input) {
    return input != value;
  }
};

}  // namespace nkrc::nbed::interface
