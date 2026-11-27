#pragma once

#include <cstdint>
#include <functional>
#include <span>

#include "interface/dma.h"

namespace nkrc::nbed::interface {

/// @brief 送信元のバイト配列への参照。
using UartTxMessage = std::span<const uint8_t>;
/// @brief 受信先の書き込み可能なバイト配列への参照。
using UartRxMessage = std::span<uint8_t>;
/// @brief 1 バイトの受信を通知するコールバック型。
using UartRxCallback = std::function<void(UartRxMessage)>;
/// @brief 9 ビット送信元の uint16_t 配列への参照。
using UartWordTxMessage = std::span<const uint16_t>;
/// @brief 9 ビット受信先の書き込み可能な uint16_t 配列への参照。
using UartWordRxMessage = std::span<uint16_t>;
/// @brief 9 ビットの 1 文字受信を通知するコールバック型。
using UartWordRxCallback = std::function<void(UartWordRxMessage)>;

/// @brief 非同期シリアル通信の通常転送・DMA・受信割り込みを扱うインターフェース。
class Uart {
 public:
  /// @brief 派生クラスの初期化を準備する。
  Uart() = default;
  /// @brief 派生クラスのオブジェクトを破棄する。
  virtual ~Uart() = default;
  /// @brief オブジェクトのコピー構築は禁止。
  Uart(const Uart&) = delete;
  /// @brief オブジェクトのコピー代入は禁止。
  Uart& operator=(const Uart&) = delete;
  /// @brief オブジェクトのムーブ構築は禁止。
  Uart(Uart&&) = delete;
  /// @brief オブジェクトのムーブ代入は禁止。
  Uart& operator=(Uart&&) = delete;

  /// @brief UART コントローラー、選んだピン、割り込みを設定する。
  /// @return 1 回目の初期化に成功した場合、または初期化済みの場合に true。
  virtual bool Initialize() = 0;

  /// @brief バイト列をブロッキング送信する。
  /// @param message 送信する 1～65,535 バイト。
  /// @return 転送完了なら true。9 ビット・パリティなしの場合は SendWords() を使う。
  virtual bool Send(UartTxMessage message) = 0;
  /// @brief 指定サイズのバイト列をブロッキング受信する。
  /// @param[out] message 1～65,535 バイト分の受信バッファ。
  /// @return 全データを受信できれば true。
  virtual bool Receive(UartRxMessage message) = 0;

  /// @brief 9 ビット・パリティなしでワード列をブロッキング送信する。
  /// @param message 1 文字を uint16_t で表す 1～65,535 要素の配列。
  /// @return 全要素を送信できれば true。
  virtual bool SendWords(UartWordTxMessage message) {
    (void)message;
    return false;
  }

  /// @brief 9 ビット・パリティなしでワード列をブロッキング受信する。
  /// @param[out] message 1 文字につき uint16_t 1 要素の受信バッファ。
  /// @return 全要素を受信できれば true。
  virtual bool ReceiveWords(UartWordRxMessage message) {
    (void)message;
    return false;
  }

  /// @brief バイト列の DMA 送信を開始する。
  /// @param message 送信バッファ。DMA 完了または中断まで保持し、書き換えない。
  /// @return 送信を開始できれば true。完了は GetTxDmaStatus() で確認する。
  virtual bool SendDma(UartTxMessage message) = 0;
  /// @brief バイト列の DMA 受信を開始する。
  /// @param[out] message DMA 完了または中断まで保持する受信バッファ。
  /// @return 受信を開始できれば true。完了は GetRxDmaStatus() で確認する。
  /// @note 受信割り込みとの併用はできない。F7 のキャッシュ有効時は 32 バイト境界とサイズに注意する。
  virtual bool ReceiveDma(UartRxMessage message) = 0;

  /// @brief 9 ビット・パリティなしでワード列の DMA 送信を開始する。
  /// @param message 完了または中断まで保持する uint16_t の送信バッファ。
  /// @return DMA 送信が始まれば true。
  virtual bool SendWordsDma(UartWordTxMessage message) {
    (void)message;
    return false;
  }

  /// @brief 9 ビット・パリティなしでワード列の DMA 受信を開始する。
  /// @param[out] message 完了または中断まで保持する uint16_t の受信バッファ。
  /// @return DMA 受信が始まれば true。
  virtual bool ReceiveWordsDma(UartWordRxMessage message) {
    (void)message;
    return false;
  }

  /// @brief 送信用 DMA の状態を読む。
  /// @return 未開始、転送中、完了、エラー、中断のいずれか。
  [[nodiscard]] virtual DmaStatus GetTxDmaStatus() const = 0;
  /// @brief 受信用 DMA の状態を読む。
  /// @return 未開始、転送中、完了、エラー、中断のいずれか。
  [[nodiscard]] virtual DmaStatus GetRxDmaStatus() const = 0;
  /// @brief 送信と受信の DMA を中断し、DMA の使用権を解放する。
  virtual void StopDma() = 0;

  /// @brief 1 文字ずつ受信する割り込みを開始する。
  /// @return 受信を開始できれば true。
  /// @note 受信 DMA と併用せず、先に対応する受信コールバックを登録する。
  virtual bool EnableRxInterrupt() = 0;
  /// @brief 1 バイト受信したときに呼ぶ関数を登録する。
  /// @param callback 割り込み内で実行する処理。受信領域は呼び出し中のみ有効。空の関数で解除する。
  virtual void SetRxCallback(const UartRxCallback& callback) = 0;

  /// @brief 9 ビット・パリティなしで 1 ワード受信したときに呼ぶ関数を登録する。
  /// @param callback 割り込み内で実行する処理。受信領域は呼び出し中のみ有効。空の関数で解除する。
  virtual void SetRxWordCallback(const UartWordRxCallback& callback) {
    (void)callback;
  }

  /// @brief 初期化済みのバイト送信 UART を printf の出力先にする。
  virtual void SetConsole() = 0;
};

}  // namespace nkrc::nbed::interface
