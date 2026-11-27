#pragma once

namespace nkrc::nbed::interface {

/// @brief 外部ピンの電圧を 12 ビット ADC で読み取るインターフェース。
class AnalogIn {
 public:
  /// @brief 派生クラスの初期化を準備する。
  AnalogIn() = default;
  /// @brief 派生クラスのオブジェクトを破棄する。
  virtual ~AnalogIn() = default;
  /// @brief オブジェクトのコピー構築は禁止。
  AnalogIn(const AnalogIn&) = delete;
  /// @brief オブジェクトのコピー代入は禁止。
  AnalogIn& operator=(const AnalogIn&) = delete;
  /// @brief オブジェクトのムーブ構築は禁止。
  AnalogIn(AnalogIn&&) = delete;
  /// @brief オブジェクトのムーブ代入は禁止。
  AnalogIn& operator=(AnalogIn&&) = delete;

  /// @brief ピンと ADC を初期化する。
  /// @return 使用可能になれば true。
  virtual bool Initialize() = 0;

  /// @brief 1 回変換し、入力電圧を基準電圧に対する比率で返す。
  /// @return 0.0～1.0。未初期化または変換失敗時は 0.0。
  [[nodiscard]] virtual float ReadRatio() = 0;
  /// @brief 1 回変換し、比率に設定した基準電圧を掛ける。
  /// @return 電圧（V）。未初期化または変換失敗時は 0.0。
  [[nodiscard]] virtual float ReadVoltage() = 0;
};

}  // namespace nkrc::nbed::interface
