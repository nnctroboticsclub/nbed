"""Run driver logic against a tiny host HAL, without attached hardware."""

from pathlib import Path
import os
import shutil
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]


class NativeDriverTests(unittest.TestCase):
  def run_family(self, family, macro, suite="drivers"):
    compiler = shutil.which(os.environ.get("CXX", "g++"))
    if compiler is None:
      self.skipTest("Set CXX to a host C++23 compiler, or install g++")
    with tempfile.TemporaryDirectory(prefix="nbed-native-") as temporary:
      directory = Path(temporary)
      for header in (f"stm32{family}xx.h", f"stm32{family}xx_hal.h",
                     f"stm32{family}xx_hal_gpio.h", f"stm32{family}xx_hal_gpio_ex.h",
                     f"stm32{family}xx_hal_rcc.h", f"stm32{family}xx_hal_uart.h",
                     f"stm32{family}xx_hal_spi.h", f"stm32{family}xx_hal_adc.h",
                     f"stm32{family}xx_hal_can.h", f"stm32{family}xx_hal_cortex.h",
                     f"stm32{family}xx_hal_i2c.h", f"stm32{family}xx_hal_pwr_ex.h"):
        stub = ("can_hal_stubs.h" if suite == "can" else
                "f3_pwm_hal_stubs.h" if suite in ("f3_pwm", "pwm_outputs", "dma") else
                "f7_hal_stubs.h" if suite in ("f7_clock", "f7_peripherals") else
                "hal_stubs.h")
        (directory / header).write_text(f'#include "{stub}"\n', encoding="utf-8")
      executable = directory / f"{family}-test.exe"
      drivers = {"spi": ("spi", "pin_mapper"),
                 "can": ("can", "pin_mapper"),
                 "exti": ("interrupt_in", "gpio", "pin_mapper"),
                 "f3_pwm": ("pwm", "timer_reservation", "pin_mapper"),
                 "pwm_outputs": ("pwm", "timer_reservation", "pin_mapper"),
                 "f7_clock": ("clock", "time"),
                 "f7_peripherals": ("i2c", "analog_in", "pin_mapper")}.get(
                     suite, ("gpio", "pin_mapper", "time", "timer_reservation", "uart"))
      if suite == "dma":
        drivers = ("pwm", "uart", "timer_reservation", "pin_mapper")
      sources = [ROOT / "src" / family / f"{name}.cc" for name in drivers]
      if "pwm" in drivers or "uart" in drivers:
        sources.append(ROOT / "src" / family / "dma.cc")
      command = [compiler, "-std=c++23", "-Wall", "-Wextra", "-Werror", f"-D{macro}",
                 "-I", str(directory), "-I", str(ROOT / "tests/native"),
                 "-I", str(ROOT / "include"),
                 str(ROOT / f"tests/native/{suite}_test.cc"),
                 *map(str, sources), "-o", str(executable)]
      result = subprocess.run(command, capture_output=True, text=True)
      self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
      try:
        result = subprocess.run([str(executable)], capture_output=True, text=True)
      except OSError as error:
        if getattr(error, "winerror", None) == 4551:
          self.skipTest(f"{family}: compiled (including static assertions); "
                        "Windows application control blocked execution (4551)")
        raise
      self.assertEqual(result.returncode, 0, result.stdout + result.stderr)

  def test_f3_dma(self):
    self.run_family("f3", "STM32F303x8", "dma")

  def test_f3_can(self):
    self.run_family("f3", "STM32F303x8", "can")

  def test_f4_can(self):
    self.run_family("f4", "STM32F446xx", "can")

  def test_f7_can(self):
    self.run_family("f7", "STM32F767xx", "can")

  def test_f4_dma(self):
    self.run_family("f4", "STM32F446xx", "dma")

  def test_f7_dma(self):
    self.run_family("f7", "STM32F767xx", "dma")

  def test_f3_drivers(self):
    self.run_family("f3", "STM32F303x8")

  def test_f3_exti(self):
    self.run_family("f3", "STM32F303x8", "exti")

  def test_f4_exti(self):
    self.run_family("f4", "STM32F446xx", "exti")

  def test_f7_exti(self):
    self.run_family("f7", "STM32F767xx", "exti")

  def test_f4_drivers(self):
    self.run_family("f4", "STM32F446xx")

  def test_f3_spi(self):
    self.run_family("f3", "STM32F303x8", "spi")

  def test_f3_pwm(self):
    self.run_family("f3", "STM32F303x8", "f3_pwm")

  def test_f3_pwm_outputs(self):
    self.run_family("f3", "STM32F303x8", "pwm_outputs")

  def test_f4_pwm_outputs(self):
    self.run_family("f4", "STM32F446xx", "pwm_outputs")

  def test_f7_pwm_outputs(self):
    self.run_family("f7", "STM32F767xx", "pwm_outputs")

  def test_f4_spi(self):
    self.run_family("f4", "STM32F446xx", "spi")

  def test_f7_drivers(self):
    self.run_family("f7", "STM32F767xx")

  def test_f7_spi(self):
    self.run_family("f7", "STM32F767xx", "spi")

  def test_f7_clock(self):
    self.run_family("f7", "STM32F767xx", "f7_clock")

  def test_f7_peripherals(self):
    self.run_family("f7", "STM32F767xx", "f7_peripherals")


if __name__ == "__main__":
  unittest.main()
