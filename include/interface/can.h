#pragma once

#include <array>
#include <cstdint>
#include <functional>

namespace nkrc::nbed::interface {

/// @brief CAN の識別子の長さ。
enum class CanFormat : uint8_t {
  kStandard,  ///< 標準 ID（11 ビット）。
  kExtended,  ///< 拡張 ID（29 ビット）。
};
/// @brief CAN フレームの種別。
enum class CanFrameType : uint8_t {
  kData,    ///< データを運ぶフレーム。
  kRemote,  ///< データ送信を要求するリモートフレーム。
};

/// @brief 一つの Classic CAN フレーム。
struct CanMessage {
  uint32_t id{0};   ///< CAN ID。format のビット幅の範囲で指定する。
  uint8_t size{0};  ///< データ長コード（0～8）。
  std::array<uint8_t, 8>
      data{};  ///< 送受信するペイロード。size バイト分を使う。
  CanFormat format{CanFormat::kStandard};  ///< ID の形式。
  CanFrameType type{CanFrameType::kData};  ///< フレーム種別。
};

/// @brief 受信割り込み内で 1 フレームを通知する関数型。
using CanRxCallback = std::function<void(const CanMessage&)>;

/// @brief マスクと ID を指定する CAN の受信フィルター。
struct CanRxFilter {
  uint32_t id{0};    ///< 比較基準の ID。
  uint32_t mask{0};  ///< 比較する ID ビットを 1 にする。0 は任意。
  CanFormat format{CanFormat::kStandard};  ///< 受信を許可する ID の形式。
};

/// @brief Classic CAN の送受信、受信フィルター、受信割り込みを扱うインターフェース。
class Can {
 public:
  /// @brief 派生クラスの初期化を準備する。
  Can() = default;
  /// @brief 派生クラスのオブジェクトを破棄する。
  virtual ~Can() = default;
  /// @brief オブジェクトのコピー構築は禁止。
  Can(const Can&) = delete;
  /// @brief オブジェクトのコピー代入は禁止。
  Can& operator=(const Can&) = delete;
  /// @brief オブジェクトのムーブ構築は禁止。
  Can(Can&&) = delete;
  /// @brief オブジェクトのムーブ代入は禁止。
  Can& operator=(Can&&) = delete;

  /// @brief CAN コントローラーを初期化し、全フレームを受信できる状態にする。
  /// @return 初期化済み、または初期化に成功した場合に true。
  virtual bool Initialize() = 0;
  /// @brief バスへの送受信を開始する。初回は必要に応じて初期化する。
  /// @return 送受信できる状態になれば true。
  virtual bool Start() = 0;
  /// @brief 送受信を停止し、受信フィルターとコントローラーの使用権を解放する。
  virtual void Stop() = 0;

  /// @brief 送信用メールボックスにフレームを登録する。
  /// @param message 送る ID、長さ、データとフレーム形式。
  /// @return 送信要求を登録できれば true。バス上の送信完了は保証しない。
  virtual bool Send(const CanMessage& message) = 0;
  /// @brief 受信 FIFO0 から 1 フレームを取り出す。
  /// @param[out] message 受信フレームの格納先。
  /// @return 1 フレーム取得できれば true。受信データがなければ待たずに false。
  virtual bool Receive(CanMessage& message) = 0;

  /// @brief 既定バンクの ID フィルターを設定する。初期化後に呼ぶ。
  /// @param filter 受信する ID のビットパターンと ID の形式。
  /// @return フィルターを設定できれば true。
  virtual bool SetRxFilter(const CanRxFilter& filter) = 0;
  /// @brief 標準 ID のマスクフィルターを既定バンクに設定する。
  /// @param id 11 ビットの基準 ID。
  /// @param mask 比較する ID ビットを 1 にした 11 ビットの値。
  /// @return フィルターを設定できれば true。
  virtual bool SetRxFilter(uint32_t id, uint32_t mask) = 0;

  /// @brief FIFO0 にフレームが届いたときの受信割り込みを有効にする。
  /// @return 通知の有効化に成功した場合に true。
  /// @note 受信コールバックを先に登録する。ポーリングで受信する場合は Receive() を使う。
  virtual bool EnableRxInterrupt() = 0;
  /// @brief 受信したフレームを処理する関数を登録する。
  /// @param callback 割り込み内で実行する関数。空の関数で解除する。
  /// @note 引数の参照は呼び出し中だけ有効。必要なデータはコピーし、長い処理はメインループで行う。
  virtual void SetRxCallback(const CanRxCallback& callback) = 0;

  /// @brief CAN コントローラーの送信エラーカウンターを読む。
  /// @return 現在の TEC。未初期化では 0。
  [[nodiscard]] virtual uint8_t GetSendErrorCount() const = 0;
  /// @brief CAN コントローラーの受信エラーカウンターを読む。
  /// @return 現在の REC。未初期化では 0。
  [[nodiscard]] virtual uint8_t GetReceiveErrorCount() const = 0;
};

}  // namespace nkrc::nbed::interface
