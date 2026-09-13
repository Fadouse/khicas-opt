#!/usr/bin/env python3
"""Run the calculator-first USB protocol against a natively installed UsbPoc.g3a."""

import argparse
import hashlib
import json
import os
from pathlib import Path
import select
import subprocess
import sys
import time

ROOT = Path(__file__).resolve().parents[3]


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--rom",
        type=Path,
        required=True,
        help="Flash exported after native USB installation",
    )
    parser.add_argument("--run-dir", type=Path, required=True)
    parser.add_argument(
        "--launch-keys",
        nargs="+",
        required=True,
        help="Menu navigation from the Run-Matrix icon to USB PoC",
    )
    args = parser.parse_args()
    run_dir = args.run_dir.resolve()
    run_dir.mkdir(parents=True, exist_ok=True)
    report = {"status": "running", "flash_sha256": digest(args.rom), "checks": []}
    process = subprocess.Popen(
        [
            sys.executable,
            "-u",
            str(ROOT / "tools/vm/qemu_vm.py"),
            "--rom",
            str(args.rom.resolve()),
            "--run-dir",
            str(run_dir),
        ],
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
    )
    log = (run_dir / "protocol.log").open("wb")

    def prompt():
        data = bytearray()
        deadline = time.monotonic() + 30
        while not data.endswith(b"cg50> "):
            if time.monotonic() >= deadline:
                raise TimeoutError("CLI command timed out")
            if select.select([process.stdout], [], [], 1)[0]:
                chunk = os.read(process.stdout.fileno(), 65536)
                if not chunk:
                    raise RuntimeError("VM exited: " + data.decode(errors="replace"))
                log.write(chunk)
                log.flush()
                data.extend(chunk)
        return data.decode()[:-6].strip()

    def command(text):
        log.write((text + "\n").encode())
        process.stdin.write((text + "\n").encode())
        process.stdin.flush()
        result = prompt()
        if "error:" in result:
            raise RuntimeError(text + ": " + result)
        return result

    def check(name, actual, expected):
        report["checks"].append(
            {
                "name": name,
                "actual": actual,
                "expected": expected,
                "passed": actual == expected,
            }
        )
        if actual != expected:
            raise AssertionError(f"{name}: {actual!r} != {expected!r}")
        print("PASS:", name, flush=True)

    def exchange(sequence):
        check(
            f"calculator-first exchange {sequence}",
            json.loads(command("usb reply")),
            {
                "calculator_to_host": f"send{sequence}\n",
                "host_to_calculator": f"recv{sequence}\n",
            },
        )

    try:
        prompt()
        command("run 8")
        for key in ("F6", "F6", "F6", "F1", "F1", "F6"):
            command("key " + key)
        command("run 3")
        for key in ("EXE", "MENU", "1", "MENU"):
            command("key " + key)
        for key in args.launch_keys:
            command("key " + key + " 0.06 0.5")
        command("key EXE")
        command("screen " + str(run_dir / "start.ppm"))
        command("usb attach")
        enumeration = json.loads(command("usb enumerate vendor"))
        check(
            "vendor interface",
            enumeration["configuration"],
            "0902200001010080320904000002ff0000000705010240000007058202400000",
        )
        exchange(1)
        command("screen " + str(run_dir / "reply.ppm"))
        command("key EXE")
        check(
            "next request requires device action",
            command("usb receive 64"),
            b"send2\n".hex(),
        )
        # An incorrect reply must not release the next request, even with EXE.
        for name, reply in (
            ("wrong sequence", b"recv9\n"),
            ("embedded NUL suffix", b"recv2\n\0suffix"),
            ("maximum packet", b"x" * 64),
        ):
            command("usb send " + reply.hex())
            command("key EXE")
            check("reject " + name, command("usb token in 2"), "NAK")
        command("usb send " + b"recv2\n".hex())
        command("key EXE")
        exchange(3)
        command("usb detach")
        command("usb attach")
        command("usb enumerate vendor")
        exchange(1)
        command("usb detach")
        command("key EXIT")
        command("key MENU")
        command("screen " + str(run_dir / "exit.ppm"))
        command("usb attach")
        command("key F1")
        storage = json.loads(command("usb enumerate"))
        check(
            "OS storage restored",
            [storage["interface"], storage["vid"], storage["pid"]],
            ["storage", "07cf", "6103"],
        )
        command("usb detach")
        report["status"] = "passed"
    except Exception as error:
        report["status"] = "failed"
        report["error"] = str(error)
        raise
    finally:
        if process.poll() is None:
            process.stdin.write(b"quit\n")
            process.stdin.flush()
            try:
                process.wait(timeout=5)
            except subprocess.TimeoutExpired:
                process.kill()
                process.wait()
        log.close()
        report["transcript_sha256"] = digest(run_dir / "protocol.log")
        (run_dir / "protocol.json").write_text(json.dumps(report, indent=2) + "\n")


if __name__ == "__main__":
    main()
