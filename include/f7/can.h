#pragma once
// IWYU pragma: private, include "nbed.h"

#include <cstdint>

#include "interface/can.h"
#include "interface/pin_names.h"
#include "stm32f7xx_hal.h"  // IWYU pragma: export
#include "stm32f7xx_hal_can.h"

namespace nkrc::nbed::stm32::f7 {

/// @brief NUCLEO-F767ZI で使える Classic CAN コントローラー。
enum class CanPeripheral : uint8_t {
  kCan1,  ///< CAN1 を使用する。
  kCan2,  ///< CAN2 を使用する。
  kCan3,  ///< CAN3 を使用する。
};
/// @brief CAN コントローラーの通信モード。
enum class CanMode : uint8_t {
  kNormal,          ///< 通常の CAN バス通信。
  kLoopback,        ///< 送信内容を内部で折り返す。TX ピンにも送信が現れる。
  kSilent,          ///< バスへ応答や送信を出さずに受信する。
  kSilentLoopback,  ///< バスへ出力せず、内部だけで送信から受信へ折り返す。
};

/// @brief CAN のピン、ビットレート、動作モード、割り込み優先度の設定。
struct CanConfig {
  CanPeripheral peripheral{
      CanPeripheral::kCan1};  ///< 使う CAN コントローラー。
  interface::PinName rx{
      interface::PinName::
          kNC};  ///< トランシーバーの RX 出力に接続するマイコン側のピン。
  interface::PinName tx{
      interface::PinName::
          kNC};  ///< トランシーバーの TX 入力に接続するマイコン側のピン。
  uint32_t bitrate{
      1'000'000};  ///< ボーレート（bps）。CAN バス上の全ノードと揃える。
  CanMode mode{CanMode::kNormal};  ///< CAN の通信・テストモード。
  uint32_t interrupt_priority{
      0};  ///< 割り込み優先度（0～15）。小さい値ほど優先度が高い。
};

/// @brief ID とハードウェアフィルターバンクを指定する受信フィルター。
struct CanFilter {
  uint32_t id{0};    ///< 比較基準の CAN ID。
  uint32_t mask{0};  ///< 比較する ID ビットを 1 にする。
  interface::CanFormat format{
      interface::CanFormat::kStandard};  ///< 受信を許可する ID の形式。
  uint8_t bank{
      0xff};  ///< フィルターバンク番号。0xff でコントローラーの既定バンク。
  uint32_t fifo{CAN_RX_FIFO0};  ///< フレームの格納先。CAN_RX_FIFO0 を指定する。
};

/// @brief CAN トランシーバーを介して Classic CAN を送受信する。
class Can final : public interface::Can {
 public:
  /// @brief 通信モードなどを指定して CAN オブジェクトを作る。
  /// @param config ピン、ビットレートと追加設定。
  explicit Can(CanConfig config);
  /// @brief CAN オブジェクトを通常通信モードで作る。
  /// @param peripheral 使う CAN コントローラー。
  /// @param rx 受信用ピン。
  /// @param tx 送信用ピン。
  /// @param bitrate ボーレート（bps）。既定は 1,000,000。
  Can(CanPeripheral peripheral, interface::PinName rx, interface::PinName tx,
      uint32_t bitrate = 1'000'000);
  /// @brief CAN の送受信を停止し、フィルターと使用権を解放する。
  ~Can() override;

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
  bool Initialize() override;
  /// @brief バスへの送受信を開始する。初回は必要に応じて初期化する。
  /// @return 送受信できる状態になれば true。
  bool Start() override;
  /// @brief 送受信を停止し、受信フィルターとコントローラーの使用権を解放する。
  void Stop() override;

  /// @brief 送信用メールボックスにフレームを登録する。
  /// @param message 送る ID、長さ、データとフレーム形式。
  /// @return 送信要求を登録できれば true。バス上の送信完了は保証しない。
  bool Send(const interface::CanMessage& message) override;
  /// @brief 受信 FIFO0 から 1 フレームを取り出す。
  /// @param[out] message 受信フレームの格納先。
  /// @return 1 フレーム取得できれば true。受信データがなければ待たずに false。
  bool Receive(interface::CanMessage& message) override;

  /// @brief 指定したフィルターバンクに ID フィルターを設定する。
  /// @param filter ID、マスク、バンクなどの設定。
  /// @return 初期化済みで指定したバンクにフィルターを設定できれば true。
  /// @note 最初の明示的な設定で初期の全受信設定を解除する。複数のバンクは OR 条件で働く。
  bool SetRxFilter(const CanFilter& filter);
  /// @brief 既定バンクの ID フィルターを設定する。
  /// @param filter ID の比較条件。
  /// @return フィルターの設定に成功すれば true。
  bool SetRxFilter(const interface::CanRxFilter& filter) override;
  /// @brief 標準 ID のマスクフィルターを既定バンクに設定する。
  /// @param id 11 ビットの基準 ID。
  /// @param mask 比較する ID ビットを 1 にした 11 ビットの値。
  /// @return フィルターを設定できれば true。
  bool SetRxFilter(uint32_t id, uint32_t mask) override;

  /// @brief FIFO0 にフレームが届いたときの受信割り込みを有効にする。
  /// @return 通知の有効化に成功した場合に true。
  /// @note 受信コールバックを先に登録する。ポーリングで受信する場合は Receive() を使う。
  bool EnableRxInterrupt() override;
  /// @brief 受信したフレームを処理する関数を登録する。
  /// @param callback 割り込み内で実行する関数。空の関数で解除する。
  /// @note 引数の参照は呼び出し中だけ有効。必要なデータはコピーし、長い処理はメインループで行う。
  void SetRxCallback(const interface::CanRxCallback& callback) override;

  /// @brief CAN コントローラーの送信エラーカウンターを読む。
  /// @return 現在の TEC。未初期化では 0。
  [[nodiscard]] uint8_t GetSendErrorCount() const override;
  /// @brief CAN コントローラーの受信エラーカウンターを読む。
  /// @return 現在の REC。未初期化では 0。
  [[nodiscard]] uint8_t GetReceiveErrorCount() const override;
  /// @brief HAL の詳細機能を使うためのハンドルを取得する。
  /// @return このオブジェクトが所有する CAN ハンドルへの参照。
  /// @note 初期化後に使う。nbed の転送・割り込みと競合する変更に注意する。
  [[nodiscard]] CAN_HandleTypeDef& GetHandle();

  /// @brief 該当するコントローラーの割り込みを HAL に渡す。
  /// @param peripheral 割り込みの発生源。
  /// @note HAL 連携用。通常は IRQ ハンドラーから呼ばれる。
  static void HandleInterrupt(CanPeripheral peripheral);
  /// @brief CAN FIFO0 に届いたフレームを受信コールバックへ渡す。
  /// @param handle HAL コールバックが受け取るハンドル。
  /// @note HAL 連携用。アプリケーションからの通常呼び出しは不要。
  static void HandleRxPending(CAN_HandleTypeDef* handle);
  /// @brief CAN 初期化時にクロック、GPIO、NVIC を設定する。
  /// @param handle HAL コールバックが受け取るハンドル。
  /// @note HAL 連携用。アプリケーションからの通常呼び出しは不要。
  static void ConfigureMsp(CAN_HandleTypeDef* handle);
  /// @brief CAN 停止時に GPIO、IRQ と未使用のクロックを解放する。
  /// @param handle HAL コールバックが受け取るハンドル。
  /// @note HAL 連携用。アプリケーションからの通常呼び出しは不要。
  static void DeconfigureMsp(CAN_HandleTypeDef* handle);

 private:
  [[nodiscard]] bool IsValidPinPair() const;
  [[nodiscard]] bool ConfigureBitTiming();
  [[nodiscard]] bool OwnPeripheral();
  void ReleasePeripheral();
  void OnRxPending();
  [[nodiscard]] bool DisableFilter(uint8_t bank);
  [[nodiscard]] uint8_t DefaultFilterBank() const;
  [[nodiscard]] static Can*& InstanceFor(CanPeripheral peripheral);
  [[nodiscard]] static Can* FromHandle(CAN_HandleTypeDef* handle);

  CanConfig config_;
  CAN_HandleTypeDef handle_{};
  interface::CanRxCallback rx_callback_;
  uint32_t configured_filters_{0};
  bool accepting_all_{false};
  bool initialized_{false};
  bool started_{false};
};

}  // namespace nkrc::nbed::stm32::f7
