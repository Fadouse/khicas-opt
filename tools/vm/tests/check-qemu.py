#!/usr/bin/env python3
"""Exercise actual QEMU CG50 device models, using a tiny license-free ROM."""

import argparse
from pathlib import Path
import socket
import subprocess
import sys
import tempfile
import time

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from qemu_vm import QMP
from usbhost import USBHost


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--qemu", type=Path, required=True)
    args = parser.parse_args()
    with tempfile.TemporaryDirectory(prefix="cg50-qtest-") as directory:
        root = Path(directory)
        rom = root / "test.rom"
        rom.write_bytes(bytes.fromhex("affe0009"))  # bra $; nop, no Casio code
        with (root / "qemu.log").open("w") as log:
            process = subprocess.Popen(
                [
                    str(args.qemu.resolve()),
                    "-M",
                    "cg50",
                    "-accel",
                    "qtest",
                    "-bios",
                    str(rom),
                    "-display",
                    "none",
                    "-serial",
                    "none",
                    "-monitor",
                    "none",
                    "-qtest",
                    f"unix:{root}/test.sock,server=on,wait=off",
                    "-qmp",
                    f"unix:{root}/qmp.sock,server=on,wait=off",
                    "-chardev",
                    f"socket,id=cg50-usb,path={root}/usb.sock,server=on,wait=off",
                ],
                stdout=log,
                stderr=log,
            )
            test = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
            test.settimeout(5)
            qmp = None
            usb = None
            try:
                deadline = time.monotonic() + 10
                while not (root / "test.sock").exists():
                    if process.poll() is not None or time.monotonic() > deadline:
                        raise RuntimeError((root / "qemu.log").read_text())
                    time.sleep(0.02)
                test.connect(str(root / "test.sock"))
                stream = test.makefile("rwb", buffering=0)
                qmp = QMP(root / "qmp.sock")

                def command(text):
                    stream.write(text.encode() + b"\n")
                    line = stream.readline().decode().strip()
                    assert line.startswith("OK"), (text, line)
                    return line[2:].strip()

                def write(address, value):
                    command(f"writew {address:#x} {value:#x}")

                def read(address):
                    return int(command(f"readw {address:#x}"), 0)

                def unlock():
                    write(0xAAA, 0xAA)
                    write(0x554, 0x55)

                # Add-in loaders distinguish native hardware using these IDs.
                assert int(command("readl 0xff000030"), 0) & 0xFFFFFF00 == 0x10300B00
                assert int(command("readl 0xff000044"), 0) & 0xFFFFFFF0 == 0x2C00

                # Firmware programs both ADC control/status words independently.
                write(0x04610088, 0x6042)
                write(0x0461008A, 0x01CF)
                assert read(0x04610088) == 0x6042
                assert read(0x0461008A) == 0x01CF
                command("writeb 0x04610089 0x41")
                assert read(0x04610088) == 0x6041
                assert int(command("readb 0x04610088"), 0) == 0x60
                assert read(0x0461008A) == 0x01CF

                # Calendar rollover, divider and stopped-clock behavior use virtual time.
                rtc = 0x0413FEC0
                command(f"writeb {rtc + 30:#x} 0")
                write(rtc + 14, 0x2024)
                for offset, value in (
                    (12, 2),
                    (10, 0x28),
                    (6, 0x23),
                    (4, 0x59),
                    (2, 0x59),
                ):
                    command(f"writeb {rtc + offset:#x} {value:#x}")
                command(f"writeb {rtc + 30:#x} 1")
                command("clock_step 1000000000")
                assert int(command(f"readb {rtc + 10:#x}"), 0) == 0x29
                assert int(command(f"readb {rtc + 2:#x}"), 0) == 0
                assert read(rtc + 14) == 0x2024
                assert int(command(f"readb {rtc + 28:#x}"), 0) & 0x80
                command("clock_step 7812500")
                assert int(command(f"readb {rtc:#x}"), 0) == 1
                command(f"writeb {rtc + 30:#x} 0")
                command("clock_step 1000000000")
                assert int(command(f"readb {rtc:#x}"), 0) == 1
                command(f"writeb {rtc + 30:#x} 0x11")
                command("clock_step 3906250")
                assert int(command(f"readb {rtc + 30:#x}"), 0) & 0x80
                command(f"writeb {rtc + 30:#x} 0x11")
                assert int(command(f"readb {rtc + 30:#x}"), 0) & 0x80 == 0

                # Cable sensing is level-sensitive after firmware arms ICR1.
                usb = USBHost(root / "usb.sock")
                write(0x0414001C, 0x3000)
                assert int(command("readb 0x04140024"), 0) & 0x40 == 0
                assert usb.command("attach") == "OK"
                assert int(command("readb 0x04050162"), 0) & 2
                assert int(command("readb 0x04140024"), 0) & 0x40
                command("writeb 0x04140024 0")
                assert int(command("readb 0x04140024"), 0) & 0x40
                assert usb.command("detach") == "OK"
                assert int(command("readb 0x04140024"), 0) & 0x40 == 0
                write(0x0414001C, 0x2000)
                assert int(command("readb 0x04140024"), 0) & 0x40

                # Host tokens cross the register/FIFO interface, without a guest driver.
                usb.command("attach")
                usb.command("reset")
                usb.command("setup 8006000100001200")
                assert read(0x04D80054) == 0x0680
                assert read(0x04D80056) == 0x0100
                assert read(0x04D8005A) == 18
                assert usb.command("in 0") == "NAK"
                write(0x04D80020, 0x0100)
                write(0x04D80014, 0x1201)
                write(0x04D80022, 0x8000)
                write(0x04D80060, 1)
                assert usb.command("in 0") == "DATA 1201"
                assert read(0x04D80040) & 7 == 2
                assert usb.command("out 0") == "OK"
                assert read(0x04D80040) & 7 == 0

                # A two-packet FIFO becomes empty only after its final packet.
                write(0x04D80064, 4)
                write(0x04D80068, 0x4012)
                write(0x04D8006C, 512)
                write(0x04D8002C, 0x0104)
                for value in (0x1122, 0x3344):
                    for _ in range(256):
                        write(0x04D8001C, value)
                write(0x04D80076, 1)
                write(0x04D8004A, 0)
                assert usb.command("in 2") == "DATA " + "1122" * 256
                assert read(0x04D8004A) & 16 == 0
                assert usb.command("in 2") == "DATA " + "3344" * 256
                assert read(0x04D8004A) & 16
                assert int(command("readb 0x04d8002e"), 0) == 0x20
                assert int(command("readb 0x04d8002f"), 0) == 0

                # Query and array modes must switch without modifying executable bytes.
                initial = read(0)
                unlock()
                write(0xAAA, 0x90)
                assert [read(a) for a in (0, 2, 28, 30)] == [1, 0x227E, 0x2222, 0x2201]
                write(0, 0xF0)
                assert read(0) == initial
                write(0xAA, 0x98)
                assert [read(a) for a in (0x20, 0x22, 0x24)] == [
                    ord("Q"),
                    ord("R"),
                    ord("Y"),
                ]
                assert read(0x4E) == 25  # 2^25-byte flash
                write(0, 0xF0)

                # NOR programming may clear bits, never restore zero bits to one.
                target = 0xC80000
                for value in (0x1234, 0xFFFF):
                    unlock()
                    write(0xAAA, 0xA0)
                    write(target, value)
                assert read(target) == 0x1234
                unlock()
                write(0xAAA, 0xA0)
                write(target + 0x20000, 0x5678)

                # Erase must enter busy before completion; adjacent sector survives.
                unlock()
                write(0xAAA, 0x80)
                unlock()
                write(target, 0x30)
                first, second = read(target), read(target)
                assert not (first & 0x80) and first & 8 and (first ^ second) & 0x40
                command("clock_step 3000000")
                assert read(target) == read(target + 0x1FFFE) == 0xFFFF
                assert read(target + 0x20000) == 0x5678

                # Buffer data is invisible until the 0x29 confirmation command.
                unlock()
                write(target, 0x25)
                write(target, 1)
                write(target, 0xABC0)
                write(target + 2, 0x1230)
                assert read(target) == 0xFFFF
                write(target, 0x29)
                assert read(target) == 0xABC0 and read(target + 2) == 0x1230

                # Decimal carry shared across add/sub commands, as on the peripheral.
                command("writel 0x04cb0004 0x99999999")
                command("writel 0x04cb0008 0x00000001")
                write(0x04CB0000, 1)
                assert int(command("readl 0x04cb000c"), 0) == 0
                command("writel 0x04cb0004 0x00000005")
                command("writel 0x04cb0008 0x00000003")
                write(0x04CB0000, 2)
                assert int(command("readl 0x04cb000c"), 0) == 1

                # QMP input reaches real KEYSC matrix words and W1C status flags.
                qmp.execute("cont")
                write(0x044B001A, 0x0FFF)
                write(0x044B001C, 0x00FF)
                write(0x044B0014, 0x4800)
                for down in (True, False):
                    qmp.execute(
                        "input-send-event",
                        {
                            "events": [
                                {
                                    "type": "key",
                                    "data": {
                                        "down": down,
                                        "key": {"type": "qcode", "data": "f1"},
                                    },
                                }
                            ]
                        },
                    )
                    assert read(0x044B0008) == (0x4000 if down else 0)
                    if down:
                        write(0x044B001A, 1)
                        write(0x044B001C, 1)
                        assert read(0x044B0008) == 0
                        write(0x044B001A, 0x0FFF)
                        write(0x044B001C, 0x00FF)
                        assert read(0x044B0008) == 0x4000

                assert read(0x044B0014) & 0xFF == 10
                write(0x044B000C, 0x8000)
                write(0x044B0014, 0x760A)
                assert read(0x044B0014) == 0x7600
                command("clock_step 8000000")
                assert read(0x044B0014) == 0x7602  # periodic release/debounce scan
                write(0x044B0014, 0x7602)
                assert read(0x044B0014) == 0x7600
                write(0x044B0014, 0x00FF)
                command("clock_step 8000000")
                assert read(0x044B0014) == 2  # polling with all interrupts disabled
                write(0x044B000C, 0)
                write(0x044B0014, 2)
                command("clock_step 8000000")
                assert read(0x044B0014) == 0
                for key, address, bit in (
                    ("home", 0x044B0008, 8),
                    ("esc", 0x044B0006, 0x800),
                ):
                    for down in (True, False):
                        qmp.execute(
                            "input-send-event",
                            {
                                "events": [
                                    {
                                        "type": "key",
                                        "data": {
                                            "down": down,
                                            "key": {"type": "qcode", "data": key},
                                        },
                                    }
                                ]
                            },
                        )
                        assert read(address) == (bit if down else 0)

                # RGB565 red, green, blue must retain channel order in the QEMU surface.
                for i, color in enumerate((0xF800, 0x07E0, 0x001F)):
                    write(0x0C000000 + i * 2, color)
                screenshot = root / "screen.ppm"
                qmp.execute("screendump", {"filename": str(screenshot)})
                header, pixels = screenshot.read_bytes().split(b"255\n", 1)
                assert b"384 216" in header
                assert pixels[:9] == bytes.fromhex("ff000000ff000000ff"), pixels[
                    :9
                ].hex()
                print(
                    "PASS: RTC calendar/divider, USB cable levels, setup/control FIFO, multi-packet bulk FIFO, CFI/ID, NOR programming, timed erase, buffered program, BCD carry, KEYSC input, RGB565 surface"
                )
                print(
                    "Device-model checks only; real firmware boot requires separate acceptance."
                )
            finally:
                if usb:
                    usb.close()
                if qmp:
                    qmp.close()
                test.close()
                process.terminate()
                try:
                    process.wait(timeout=5)
                except subprocess.TimeoutExpired:
                    process.kill()
                    process.wait()


if __name__ == "__main__":
    main()
