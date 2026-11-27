#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include "interface/interrupt_in.h"
#include "interface/pin_names.h"

#if defined(STM32F303x8)
#include "stm32f3xx_hal.h"
#elif defined(STM32F446xx)
#include "stm32f4xx_hal.h"
#elif defined(STM32F767xx)
#include "stm32f7xx_hal.h"
#endif

namespace nkrc::nbed::detail {

/// @brief EXTI の状態を更新する間だけ割り込みを保留し、終了時に元のマスクを復元する。
class ExtiCriticalSection {
 public:
  /// @brief 現在の割り込みマスクを保存して割り込みを止める。
  ExtiCriticalSection() : previous_mask_(__get_PRIMASK()) { __disable_irq(); }

  /// @brief 保存した割り込みマスクを復元する。
  ~ExtiCriticalSection() { __set_PRIMASK(previous_mask_); }

  /// @brief オブジェクトのコピー構築は禁止。
  ExtiCriticalSection(const ExtiCriticalSection&) = delete;
  /// @brief オブジェクトのコピー代入は禁止。
  ExtiCriticalSection& operator=(const ExtiCriticalSection&) = delete;

 private:
  uint32_t previous_mask_;
};

/// @brief GPIO と EXTI をまとめて扱う割り込み入力の共通実装。
/// @tparam Mapper ボードの GPIO マッピング型。
/// @tparam Pull 内部プル抵抗の型。
template <typename Mapper, typename Pull>
class InterruptInput : public interface::InterruptIn {
 public:
  /// @brief 外部割り込み入力を作る。ピンの設定は Initialize() で行う。
  /// @param pin 入力ピン。
  /// @param edge 検出するエッジ。
  /// @param pull 内部プル抵抗。
  /// @param interrupt_priority 割り込み優先度（0～15）。
  InterruptInput(interface::PinName pin, interface::InterruptEdge edge,
                 Pull pull, uint32_t interrupt_priority)
      : pin_(pin), edge_(edge), pull_(pull), priority_(interrupt_priority) {}

  /// @brief 割り込み入力を停止し、EXTI ラインを解放する。
  /// @brief エッジの検出を停止し、EXTI ラインを解放する。
  ~InterruptInput() override { Stop(); }

  /// @brief オブジェクトのコピー構築は禁止。
  InterruptInput(const InterruptInput&) = delete;
  /// @brief オブジェクトのコピー代入は禁止。
  InterruptInput& operator=(const InterruptInput&) = delete;
  /// @brief オブジェクトのムーブ構築は禁止。
  InterruptInput(InterruptInput&&) = delete;
  /// @brief オブジェクトのムーブ代入は禁止。
  InterruptInput& operator=(InterruptInput&&) = delete;

  /// @brief 入力ピンと EXTI ラインを設定し、エッジの検出を始める。
  /// @return 使用権の確保と初期化に成功すれば true。
  /// @note 同じラインの入力があるか、同じ IRQ 内で優先度が違う場合は false。
  bool Initialize() override {
    const ExtiCriticalSection critical;
    if (initialized_) {
      return true;
    }
    if (!Mapper::IsValidPin(pin_) || priority_ >= (1U << __NVIC_PRIO_BITS) ||
        static_cast<unsigned>(edge_) > 2 || static_cast<unsigned>(pull_) > 2) {
      return false;
    }
    const auto line = Line();
    auto& instances = Instances();
    if (instances.at(line) != nullptr) {
      return false;
    }
    const IRQn_Type irq = Irq(line);
    bool shared = false;
    for (std::size_t index = 0; index < instances.size(); ++index) {
      const auto* other = instances.at(index);
      if (other != nullptr && Irq(index) == irq) {
        if (other->priority_ != priority_) {
          return false;
        }
        shared = true;
      }
    }
    GPIO_InitTypeDef gpio{};
    gpio.Mode = edge_ == interface::InterruptEdge::kRising ? GPIO_MODE_IT_RISING
                : edge_ == interface::InterruptEdge::kFalling
                    ? GPIO_MODE_IT_FALLING
                    : GPIO_MODE_IT_RISING_FALLING;
    gpio.Pull = HalPull();
    __HAL_RCC_SYSCFG_CLK_ENABLE();
    if (!Mapper::ConfigureGpio(pin_, gpio)) {
      return false;
    }
    pin_mask_ = Mapper::GetGpioPin(pin_);
    port_ = Mapper::GetGpioPort(pin_);
    __HAL_GPIO_EXTI_CLEAR_IT(pin_mask_);
    edge_count_ = 0;
    instances.at(line) = this;
    initialized_ = true;
    if (!shared) {
      HAL_NVIC_ClearPendingIRQ(irq);
      HAL_NVIC_SetPriority(irq, priority_, 0);
      HAL_NVIC_EnableIRQ(irq);
    }
    return true;
  }

  void Stop() override {
    const ExtiCriticalSection critical;
    if (!initialized_) {
      return;
    }
    const auto line = Line();
    const IRQn_Type irq = Irq(line);
    // HAL_GPIO_Init(INPUT) alone does not mask EXTI. DeInit clears this line.
    HAL_GPIO_DeInit(port_, pin_mask_);
    GPIO_InitTypeDef gpio{};
    gpio.Mode = GPIO_MODE_INPUT;
    gpio.Pull = HalPull();
    (void)Mapper::ConfigureGpio(pin_, gpio);
    __HAL_GPIO_EXTI_CLEAR_IT(pin_mask_);
    Instances().at(line) = nullptr;
    initialized_ = false;
    for (std::size_t index = 0; index < Instances().size(); ++index) {
      if (Instances().at(index) != nullptr && Irq(index) == irq) {
        return;
      }
    }
    HAL_NVIC_DisableIRQ(irq);
    HAL_NVIC_ClearPendingIRQ(irq);
  }

  /// @brief エッジ検出時に呼ぶ関数を登録する。
  /// @param callback 割り込み内で実行する短い処理。空の関数で解除する。
  void SetCallback(const interface::GpioInterruptCallback& callback) override {
    const ExtiCriticalSection critical;
    callback_ = callback;
  }

  /// @brief 入力ピンの現在のレベルを読む。
  /// @return High なら true。Low または未初期化では false。
  [[nodiscard]] bool Read() const override {
    return initialized_ && HAL_GPIO_ReadPin(port_, pin_mask_) == GPIO_PIN_SET;
  }

  /// @brief 処理したエッジの累計を返す。
  /// @return Initialize() 以降の累計。32 ビットで周回する。
  [[nodiscard]] uint32_t GetEdgeCount() const override { return edge_count_; }

  /// @brief 発生したエッジを記録し、登録された関数を呼ぶ。
  /// @param group_mask この IRQ が担当する EXTI ラインのビットマスク。
  /// @note IRQ ハンドラー用。
  static void HandleInterrupt(uint16_t group_mask) {
    auto& instances = Instances();
    uint16_t owned_mask = 0;
    for (std::size_t line = 0; line < instances.size(); ++line) {
      if (instances.at(line) != nullptr) {
        owned_mask |= static_cast<uint16_t>(1U << line);
      }
    }
    const auto pending =
        static_cast<uint16_t>(__HAL_GPIO_EXTI_GET_IT(group_mask & owned_mask));
    // Snapshot and clear before callbacks; a new edge remains pending.
    __HAL_GPIO_EXTI_CLEAR_IT(pending);
    for (std::size_t line = 0; line < instances.size(); ++line) {
      auto* input = instances.at(line);
      if ((pending & (1U << line)) != 0 && input != nullptr) {
        input->edge_count_ = input->edge_count_ + 1U;
        if (input->callback_) {
          input->callback_();
        }
      }
    }
  }

 private:
  static auto& Instances() {
    static std::array<InterruptInput*, 16> instances{};
    return instances;
  }

  [[nodiscard]] std::size_t Line() const {
    return static_cast<uint8_t>(pin_) & 0x0fU;
  }

  static IRQn_Type Irq(std::size_t line) {
    constexpr std::array<IRQn_Type, 5> irqs{EXTI0_IRQn, EXTI1_IRQn,
#if defined(STM32F303x8)
                                            EXTI2_TSC_IRQn,
#else
                                            EXTI2_IRQn,
#endif
                                            EXTI3_IRQn, EXTI4_IRQn};
    return line < irqs.size() ? irqs.at(line)
           : line < 10        ? EXTI9_5_IRQn
                              : EXTI15_10_IRQn;
  }

  [[nodiscard]] uint32_t HalPull() const {
    return pull_ == Pull::kUp     ? GPIO_PULLUP
           : pull_ == Pull::kDown ? GPIO_PULLDOWN
                                  : GPIO_NOPULL;
  }

  interface::PinName pin_;
  interface::InterruptEdge edge_;
  Pull pull_;
  uint32_t priority_;
  interface::GpioInterruptCallback callback_;
  GPIO_TypeDef* port_{nullptr};
  uint16_t pin_mask_{0};
  volatile uint32_t edge_count_{0};
  bool initialized_{false};
};

}  // namespace nkrc::nbed::detail
