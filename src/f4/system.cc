#if defined(STM32F446xx)

#include "f4/system.h"

#include <sys/stat.h>
#include <sys/times.h>

#include <cerrno>
#include <cstddef>
#include <cstdint>

#include "f4/clock.h"
#include "f4/time.h"
#include "f4/uart.h"
#include "logger.h"
#include "stm32f4xx_hal.h"

#ifndef NBED_NO_DEFAULT_SERIAL
#include "interface/pin_names.h"
#endif  // NBED_NO_DEFAULT_SERIAL

namespace nkrc::nbed::stm32::f4 {

void Error::ErrorHandler() {
  __disable_irq();
  Logger::Error("SystemError", "An error has occurred.");
  while (true) {}
}

void System::Initialize() { Initialize(Clock::Configuration{}); }

void System::Initialize(const Clock::Configuration& configuration) {
  HAL_Init();
  if (!ClockManager::Initialize(configuration)) {
    ErrorHandler();
  }
  Time::Initialize();

#ifndef NBED_NO_DEFAULT_SERIAL
  static Uart uart(UartPeripheral::kUsart2, interface::PinName::kPA3,
                   interface::PinName::kPA2, 921600);
  if (uart.Initialize()) {
    uart.SetConsole();
    Logger::Info("System",
                 "Default serial console initialized on USART2 (PA3/PA2)");
  }
#endif  // NBED_NO_DEFAULT_SERIAL
}

void System::ErrorHandler() {
  static Error error;
  error.ErrorHandler();
}

extern "C" {

// ===== Hal Msp =====
void HAL_MspInit(void) {
  __HAL_RCC_SYSCFG_CLK_ENABLE();
  __HAL_RCC_PWR_CLK_ENABLE();
  HAL_NVIC_SetPriorityGrouping(NVIC_PRIORITYGROUP_0);
}

// ===== System Interrupt Handler =====
void NMI_Handler(void) {
  while (true) {}
}

void HardFault_Handler(void) {
  while (true) {}
}

void MemManage_Handler(void) {
  while (true) {}
}

void BusFault_Handler(void) {
  while (true) {}
}

void UsageFault_Handler(void) {
  while (true) {}
}

void SVC_Handler(void) {}

void DebugMon_Handler(void) {}

void PendSV_Handler(void) {}

// ===== Error Handler =====
void Error_Handler(void) { System::ErrorHandler(); }

// ===== System Calls =====
__attribute__((weak)) int __io_putchar(int ch) {
  (void)ch;
  return -1;
}

__attribute__((weak)) int __io_getchar(void) { return -1; }

void initialise_monitor_handles() {}

int _getpid() { return 1; }

int _kill(int pid, int sig) {
  (void)pid;
  (void)sig;
  errno = EINVAL;
  return -1;
}

void _exit(int status) {
  _kill(status, -1);
  while (true) {
    __WFI();
  }
}

__attribute__((weak)) int _write(int file, const char* ptr, int len) {
  (void)file;
  for (int i = 0; i < len; i++) {
    if (__io_putchar(*ptr++) < 0) {
      errno = EIO;
      return -1;
    }
  }
  return len;
}

int _read(int file, char* ptr, int len) {
  (void)file;
  for (int i = 0; i < len; i++) {
    const int ch = __io_getchar();
    if (ch < 0) {
      errno = EIO;
      return i == 0 ? -1 : i;
    }
    *ptr++ = static_cast<char>(ch);
  }
  return len;
}

int _close(int file) {
  (void)file;
  return -1;
}

int _fstat(int file, struct stat* st) {
  (void)file;
  st->st_mode = S_IFCHR;
  return 0;
}

int _isatty(int file) {
  (void)file;
  return 1;
}

int _lseek(int file, int ptr, int dir) {
  (void)file;
  (void)ptr;
  (void)dir;
  return 0;
}

int _open(const char* path, int flags, ...) {
  (void)path;
  (void)flags;
  return -1;
}

// The newlib _wait ABI requires an int* status parameter.
// NOLINTNEXTLINE(readability-non-const-parameter)
int _wait(int* status) {
  (void)status;
  errno = ECHILD;
  return -1;
}

int _unlink(const char* name) {
  (void)name;
  errno = ENOENT;
  return -1;
}

int _times(struct tms* buf) {
  (void)buf;
  return -1;
}

int _stat(const char* file, struct stat* st) {
  (void)file;
  st->st_mode = S_IFCHR;
  return 0;
}

int _link(const char* oldpath, const char* newpath) {
  (void)oldpath;
  (void)newpath;
  errno = EMLINK;
  return -1;
}

int _fork() {
  errno = EAGAIN;
  return -1;
}

int _execve(const char* name, const char** argv, const char** env) {
  (void)name;
  (void)argv;
  (void)env;
  errno = ENOMEM;
  return -1;
}

// ===== System Memory Calls =====
void* _sbrk(ptrdiff_t incr) {
  // Defined in linker script
  extern uint8_t _end;
  extern uint8_t _estack;
  extern uint8_t _Min_Stack_Size;

  // Keep the linker address width explicit.
  // NOLINTNEXTLINE(modernize-use-auto)
  static uintptr_t heap_end = reinterpret_cast<uintptr_t>(&_end);

  const uintptr_t stack_limit = reinterpret_cast<uintptr_t>(&_estack) -
                                reinterpret_cast<uintptr_t>(&_Min_Stack_Size);

  // Keep the linker address width explicit.
  // NOLINTNEXTLINE(modernize-use-auto)
  const uintptr_t heap_start = reinterpret_cast<uintptr_t>(&_end);
  uintptr_t next_heap_end = heap_end;

  if (incr >= 0) {
    const uintptr_t increase = static_cast<uintptr_t>(incr);
    if (heap_end > stack_limit || increase > stack_limit - heap_end) {
      errno = ENOMEM;
      return reinterpret_cast<void*>(static_cast<intptr_t>(-1));
    }
    next_heap_end += increase;
  } else {
    // Avoid negating PTRDIFF_MIN, which cannot be represented as a
    // positive ptrdiff_t.
    const uintptr_t decrease =
        static_cast<uintptr_t>(-(incr + 1)) + static_cast<uintptr_t>(1);
    if (heap_end < heap_start || decrease > heap_end - heap_start) {
      errno = EINVAL;
      return reinterpret_cast<void*>(static_cast<intptr_t>(-1));
    }
    next_heap_end -= decrease;
  }

  void* const previous_heap_end = reinterpret_cast<void*>(heap_end);
  heap_end = next_heap_end;
  return previous_heap_end;
}

}  // extern "C"

}  // namespace nkrc::nbed::stm32::f4

#endif
