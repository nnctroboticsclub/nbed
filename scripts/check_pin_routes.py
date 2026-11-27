"""Compare nbed's GPIO/ADC routes with ST's package and alternate-function data."""

import argparse
from pathlib import Path
import re
from urllib.parse import quote
from urllib.request import urlopen
import xml.etree.ElementTree as ET


ROOT = Path(__file__).resolve().parents[1]
REVISION = "45c04599e2e0a788c5ed9b6218dc5a28a36041f7"
ST_DATA = f"https://raw.githubusercontent.com/STMicroelectronics/STM32_open_pin_data/{REVISION}/"
DEVICES = {
    "f3": "STM32F303K(6-8)Tx",
    "f4": "STM32F446R(C-E)Tx",
    "f7": "STM32F767ZITx",
}
NS = {"st": "http://dummy.com"}
PHYSICAL_PIN = re.compile(r"P[A-H]\d+")
FUNCTION = re.compile(
    r"(SPI|I2C|FMPI2C|CAN|USART|UART|TIM|ADC)(\d*)_([A-Z0-9_]+)$"
)
TABLES = {
    "SPI": ("Spi", {"SCK", "MISO", "MOSI"}),
    "I2C": ("I2c", {"SCL", "SDA"}),
    "FMPI2C": ("I2c", {"SCL", "SDA"}),
    "CAN": ("Can", {"RX", "TX"}),
    "USART": ("Uart", {"RX", "TX", "CTS", "RTS", "RTS_DE", "DE"}),
    "UART": ("Uart", {"RX", "TX", "CTS", "RTS", "RTS_DE", "DE"}),
    "TIM": ("Timer", {"CH1", "CH2", "CH3", "CH4", "CH1N", "CH2N", "CH3N"}),
    "ADC": ("Adc", set()),
}


def read_xml(path, directory):
    if directory is not None:
        return ET.parse(directory / path).getroot()
    with urlopen(ST_DATA + quote(path), timeout=20) as response:
        return ET.fromstring(response.read())


def pin_routes_from_st(device, directory):
    mcu = read_xml(f"mcu/{device}.xml", directory)
    gpio = mcu.find("st:IP[@Name='GPIO']", NS).get("Version")
    modes = read_xml(f"mcu/IP/GPIO-{gpio}_Modes.xml", directory)
    alternate = {}
    for entry in modes.findall("st:GPIO_Pin", NS):
        pin = PHYSICAL_PIN.match(entry.get("Name"))
        if pin is None:
            continue
        for role in entry.findall("st:PinSignal", NS):
            af = role.find("st:SpecificParameter[@Name='GPIO_AF']/st:PossibleValue", NS)
            if af is not None:
                alternate[pin[0], role.get("Name")] = int(re.match(r"GPIO_AF(\d+)_", af.text)[1])

    routes = set()
    for entry in mcu.findall("st:Pin", NS):
        pin = PHYSICAL_PIN.match(entry.get("Name"))
        if pin is None:
            continue
        for role in entry.findall("st:Signal", NS):
            name = role.get("Name")
            match = FUNCTION.fullmatch(name)
            if match is None:
                continue
            kind, number, signal = match.groups()
            group, valid_signals = TABLES[kind]
            if kind == "ADC" and re.fullmatch(r"IN\d+", signal):
                routes.add((group, int(number), "ANALOG", pin[0], int(signal[2:])))
            elif signal in valid_signals:
                # The small F303 has one controller named CAN in CubeMX.
                # nbed assigns a fourth I2C route number to F446's FMPI2C1.
                index = 4 if kind == "FMPI2C" else int(number or "1")
                function = "RTS" if signal in {"RTS_DE", "DE"} else signal
                routes.add((group, index, function, pin[0], alternate[pin[0], name]))
    return routes


def pin_routes_from_nbed(family):
    code = (ROOT / f"include/{family}/pin_routes.h").read_text(encoding="utf-8")
    routes = []
    for group, body in re.findall(r"PinRoute k(\w+)Pins\[\]\s*\{(.*?)\};", code, re.S):
        for number, signal, pin, value in re.findall(
            r"\{\s*(\d+),\s*PinSignal::k(\w+),\s*PinName::k(P[A-H]\d+),\s*(\d+)\s*\}",
            body,
        ):
            routes.append((group, int(number), signal.upper(), pin, int(value)))
    if len(routes) != len(set(routes)):
        raise ValueError(f"{family}: repeated pin route")
    return set(routes)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--data-dir", type=Path,
        help="use a local checkout of STM32_open_pin_data instead of downloading XML",
    )
    args = parser.parse_args()
    failed = False
    for family, device in DEVICES.items():
        expected = pin_routes_from_st(device, args.data_dir)
        actual = pin_routes_from_nbed(family)
        if expected == actual:
            print(f"{device}: {len(expected)} routes match")
            continue
        failed = True
        for title, difference in (("Missing", expected - actual), ("Unexpected", actual - expected)):
            for group, number, signal, pin, value in sorted(difference):
                print(f"{device}: {title} {group}{number}/{signal} {pin} ({value})")
    return int(failed)


if __name__ == "__main__":
    raise SystemExit(main())
