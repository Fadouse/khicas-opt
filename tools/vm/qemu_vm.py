#!/usr/bin/env python3
"""CLI control of the experimental CG50 QEMU board (local firmware required)."""

import argparse
import cmd
import hashlib
import json
import re
from pathlib import Path
import shlex
import socket
import subprocess
import sys
import time
import threading

from rsp import RSP

ROOT = Path(__file__).resolve().parents[2]


class QMP:
    def __init__(self, path):
        self.socket = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        self.socket.settimeout(10)
        self.socket.connect(str(path))
        self.file = self.socket.makefile("rwb", buffering=0)
        self.events = []
        self.lock = threading.RLock()
        greeting = self.receive()
        if "QMP" not in greeting:
            raise RuntimeError(f"Invalid QMP greeting: {greeting}")
        self.execute("qmp_capabilities")

    def receive(self):
        line = self.file.readline()
        if not line:
            raise RuntimeError("QEMU disconnected")
        return json.loads(line)

    def execute(self, command, arguments=None):
        with self.lock:
            return self._execute(command, arguments)

    def _execute(self, command, arguments=None):
        packet = {"execute": command}
        if arguments is not None:
            packet["arguments"] = arguments
        self.file.write(json.dumps(packet).encode() + b"\n")
        while True:
            result = self.receive()
            if "event" in result:
                self.events.append(result)
                self.events = self.events[-100:]
            elif "error" in result:
                raise RuntimeError(str(result["error"]))
            elif "return" in result:
                return result["return"]

    def hmp(self, command):
        return self.execute("human-monitor-command", {"command-line": command})

    def close(self):
        self.file.close()
        self.socket.close()


class DebugShell(cmd.Cmd):
    prompt = "cg50> "
    intro = "CG50 QEMU debugger. help lists commands; guest starts paused at reset."

    def __init__(self, qmp, run_dir):
        super().__init__()
        self.qmp = qmp
        self.run_dir = run_dir
        self.debugger = None
        self.last_error = None

    def onecmd(self, line):
        self.last_error = None
        try:
            return super().onecmd(line)
        except (ValueError, OSError, RuntimeError) as error:
            self.last_error = error
            print(f"error: {error}", file=sys.stderr)

    def emptyline(self):
        pass

    def default(self, line):
        raise ValueError(f"Unknown command: {line}")

    def gdb(self):
        self.qmp.execute("stop")
        if self.debugger is None:
            self.debugger = RSP(self.run_dir / "gdb.sock")
        return self.debugger

    def do_step(self, arg):
        """step [N]: execute N guest instructions using QEMU's GDB stub."""
        count = int(arg or "1", 0)
        if not 1 <= count <= 10000:
            raise ValueError("step count must be 1..10000")
        debugger = self.gdb()
        for _ in range(count):
            reply = debugger.request("s")
            if not reply.startswith(("S", "T")):
                raise RuntimeError("Unexpected GDB stop response: " + reply)
        self.do_regs("")

    def do_break(self, arg):
        """break ADDRESS: add an execution breakpoint without modifying ROM."""
        address = int(arg, 0)
        if not 0 <= address <= 0xFFFFFFFF:
            raise ValueError("address outside SH4 address space")
        print(self.gdb().request(f"Z1,{address:x},2"))

    def do_delete(self, arg):
        """delete ADDRESS: remove an execution breakpoint."""
        print(self.gdb().request(f"z1,{int(arg, 0):x},2"))

    def do_detach(self, arg):
        """detach: release the internal debugger so external GDB can connect."""
        if self.debugger:
            self.debugger.close()
            self.debugger = None
            self.qmp.execute("stop")

    def do_status(self, arg):
        """status: show actual QEMU run state."""
        print(json.dumps(self.qmp.execute("query-status"), indent=2))

    def do_run(self, arg):
        """run [seconds]: continue, optionally pause after a host-time interval."""
        seconds = float(arg) if arg else None
        if seconds is not None and not 0 < seconds <= 60:
            raise ValueError("interval must be between 0 and 60 seconds")
        if self.debugger:
            print(self.debugger.resume(seconds))
            self.do_regs("")
            return
        self.qmp.execute("cont")
        if seconds is not None:
            time.sleep(seconds)
            self.qmp.execute("stop")
            self.do_regs("")

    def do_pause(self, arg):
        """pause: stop execution at the current instruction boundary."""
        self.qmp.execute("stop")

    def do_regs(self, arg):
        """regs: display CPU registers."""
        print(self.qmp.hmp("info registers"))

    def do_x(self, arg):
        """x /COUNTfmt ADDRESS: inspect virtual memory (QEMU monitor syntax)."""
        print(self.qmp.hmp("x " + arg))

    def do_xp(self, arg):
        """xp /COUNTfmt ADDRESS: inspect physical memory."""
        print(self.qmp.hmp("xp " + arg))

    def do_disas(self, arg):
        """disas ADDRESS: disassemble 20 guest instructions."""
        address = arg or re.search(
            r"pc=(0x[0-9a-f]+)", self.qmp.hmp("info registers")
        ).group(1)
        print(self.qmp.hmp("x /20i " + address))

    def do_screen(self, arg):
        """screen [PATH.ppm]: save the live guest framebuffer via QEMU."""
        path = Path(arg).expanduser().resolve() if arg else self.run_dir / "screen.ppm"
        self.qmp.execute("screendump", {"filename": str(path)})
        print(path)

    @staticmethod
    def key_name(name):
        aliases = {
            "EXE": "ret",
            "MENU": "home",
            "EXIT": "esc",
            "AC": "end",
            "ALPHA": "ctrl",
            "+": "kp_add",
            "*": "kp_multiply",
            "-": "minus",
            "/": "slash",
            ".": "dot",
            "DEL": "backspace",
            "LOG": "l",
            "LN": "n",
            "SIN": "s",
            "COS": "c",
            "TAN": "t",
            "^": "p",
            "SQUARE": "q",
            "FRAC": "f",
            "SD": "d",
            "(": "bracket_left",
            ")": "bracket_right",
            ",": "comma",
            "STO": "equal",
            "OPTN": "o",
            "VARS": "v",
            "NEG": "kp_subtract",
            "EXP": "e",
        }
        return aliases.get(name.upper(), name.lower())

    def send_key(self, name, down):
        self.qmp.execute(
            "input-send-event",
            {
                "events": [
                    {
                        "type": "key",
                        "data": {
                            "down": down,
                            "key": {"type": "qcode", "data": self.key_name(name)},
                        },
                    }
                ]
            },
        )

    def do_hold(self, arg):
        """hold NAME on|off: hold a physical key across later commands (e.g. X + EXE)."""
        name, state = shlex.split(arg)
        if state not in ("on", "off"):
            raise ValueError("hold NAME on|off")
        self.qmp.execute("cont")
        try:
            self.send_key(name, state == "on")
            time.sleep(0.02)
        finally:
            self.qmp.execute("stop")

    def do_write(self, arg):
        """write ADDRESS HEXBYTES: write virtual memory through GDB while paused."""
        address, data = shlex.split(arg)
        address = int(address, 0)
        payload = bytes.fromhex(data)
        if (
            not 0 < len(payload) <= 4096
            or not 0 <= address <= 0xFFFFFFFF - len(payload) + 1
        ):
            raise ValueError(
                "write must contain 1..4096 bytes within the address space"
            )
        print(self.gdb().request(f"M{address:x},{len(payload):x}:{payload.hex()}"))

    def do_key(self, arg):
        """key NAME [seconds]: press/release a matrix key; run then pause the VM.
        Names: F1-F6, EXE, MENU, EXIT, AC, SHIFT, ALPHA, X, arrows, digits, + - * / .
        """
        parts = shlex.split(arg)
        if not 1 <= len(parts) <= 2:
            raise ValueError("key NAME [hold seconds]")
        name = self.key_name(parts[0])
        duration = float(parts[1]) if len(parts) == 2 else 0.06
        if not 0.001 <= duration <= 5:
            raise ValueError("hold seconds must be 0.001..5")
        self.qmp.execute("cont")
        try:
            for down in (True, False):
                self.send_key(name, down)
                time.sleep(duration if down else 0.1)
        finally:
            self.qmp.execute("stop")

    def do_dump(self, arg):
        """dump ADDRESS SIZE PATH: save guest physical memory."""
        address, size, filename = shlex.split(arg)
        path = Path(filename).expanduser().resolve()
        self.qmp.execute(
            "pmemsave",
            {"val": int(address, 0), "size": int(size, 0), "filename": str(path)},
        )
        print(path)

    def do_hmp(self, arg):
        """hmp COMMAND: access the native QEMU monitor (help lists commands)."""
        print(self.qmp.hmp(arg))

    def do_quit(self, arg):
        """quit: terminate this VM without changing the input firmware."""
        self.do_detach("")
        return True

    do_EOF = do_quit


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--rom", type=Path, required=True)
    parser.add_argument(
        "--qemu", type=Path, help="override the locally recorded QEMU build"
    )
    parser.add_argument("--run-dir", type=Path, default=ROOT / ".build/vm/session")
    parser.add_argument(
        "--script", type=Path, help="read debugger commands from a file"
    )
    parser.add_argument(
        "--ui", action="store_true", help="serve a local live display and keypad"
    )
    parser.add_argument(
        "--port",
        type=int,
        default=0,
        help="browser UI port; default chooses a free port",
    )
    parser.add_argument("--log", default="guest_errors", help="QEMU -d flags")
    args = parser.parse_args()
    rom = args.rom.resolve(strict=True)
    if args.qemu:
        qemu = args.qemu.resolve(strict=True)
    else:
        metadata_path = ROOT / ".build/vm/qemu.json"
        if not metadata_path.exists():
            parser.error("build QEMU first with tools/vm/build_qemu.py, or pass --qemu")
        build_info = json.loads(metadata_path.read_text())
        board_hash = hashlib.sha256(
            Path(__file__).with_name("qemu").joinpath("cg50.c").read_bytes()
        ).hexdigest()
        if build_info["board_sha256"] != board_hash:
            parser.error("CG50 board changed since the last build; rebuild QEMU")
        qemu = Path(build_info["binary"]).resolve(strict=True)
    run_dir = args.run_dir.resolve()
    run_dir.mkdir(parents=True, exist_ok=True)
    qmp_path = run_dir / "qmp.sock"
    gdb_path = run_dir / "gdb.sock"
    if len(str(qmp_path).encode()) >= 104 or len(str(gdb_path).encode()) >= 104:
        parser.error(
            "run directory path too long for Unix sockets; use --run-dir /tmp/cg50-session"
        )
    if qmp_path.exists() or gdb_path.exists():
        parser.error(
            "socket already exists; choose a different --run-dir or stop its VM first"
        )
    metadata = {
        "rom": str(rom),
        "size": rom.stat().st_size,
        "sha256": hashlib.sha256(rom.read_bytes()).hexdigest(),
        "qemu": str(qemu),
    }
    (run_dir / "input.json").write_text(json.dumps(metadata, indent=2) + "\n")
    command = [
        str(qemu),
        "-M",
        "cg50",
        "-bios",
        str(rom),
        "-display",
        "none",
        "-serial",
        "none",
        "-monitor",
        "none",
        "-S",
        "-qmp",
        f"unix:{qmp_path},server=on,wait=off",
        "-gdb",
        f"unix:{gdb_path},server=on,wait=off",
        "-d",
        args.log,
        "-D",
        str(run_dir / "cpu.log"),
    ]
    with (run_dir / "qemu.log").open("w") as log:
        process = subprocess.Popen(command, stdout=log, stderr=log)
        connection = None
        panel = None
        try:
            deadline = time.monotonic() + 10
            while not qmp_path.exists():
                if process.poll() is not None or time.monotonic() >= deadline:
                    raise RuntimeError(
                        f"QEMU failed to start; read {run_dir / 'qemu.log'}"
                    )
                time.sleep(0.02)
            connection = QMP(qmp_path)
            print(
                f"Firmware: {rom}\nSHA256: {metadata['sha256']}\nGDB: target remote {gdb_path}"
            )
            shell = DebugShell(connection, run_dir)
            if args.ui:
                from panel import start_panel

                panel, url = start_panel(shell, args.port)
                print("UI: " + url, flush=True)
            if args.script:
                for line in args.script.read_text().splitlines():
                    if line.strip() and not line.lstrip().startswith("#"):
                        print("cg50> " + line)
                        stop = shell.onecmd(line)
                        if shell.last_error:
                            raise RuntimeError(
                                f"Script command failed: {line}"
                            ) from shell.last_error
                        if stop:
                            break
            else:
                shell.cmdloop()
        finally:
            if panel:
                panel.shutdown()
                panel.server_close()
            if connection:
                connection.close()
            if process.poll() is None:
                process.terminate()
                try:
                    process.wait(timeout=5)
                except subprocess.TimeoutExpired:
                    process.kill()
                    process.wait()
            for path in (qmp_path, gdb_path):
                path.unlink(missing_ok=True)


if __name__ == "__main__":
    main()
