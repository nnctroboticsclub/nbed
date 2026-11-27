#pragma once

namespace nkrc::nbed::interface {

/// @brief アプリケーションが回復できないエラーを処理するインターフェース。
class Error {
 public:
  /// @brief 派生クラスの初期化を準備する。
  Error() = default;
  /// @brief 派生クラスのオブジェクトを破棄する。
  virtual ~Error() = default;
  /// @brief オブジェクトのコピー構築は禁止。
  Error(const Error&) = delete;
  /// @brief オブジェクトのコピー代入は禁止。
  Error& operator=(const Error&) = delete;
  /// @brief オブジェクトのムーブ構築は禁止。
  Error(Error&&) = delete;
  /// @brief オブジェクトのムーブ代入は禁止。
  Error& operator=(Error&&) = delete;

  /// @brief 回復できないエラーを処理する。STM32 の既定処理は割り込みを止めて停止する。
  virtual void ErrorHandler() = 0;
};

}  // namespace nkrc::nbed::interface
