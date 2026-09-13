#!/usr/bin/env python3
"""Build and debug the ESP32-S3 KhiCAS USB bridge with pinned ESP-IDF/QEMU."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shlex
import shutil
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[2]
WORK = ROOT / ".build/esp32"
IDF = WORK / "esp-idf"
QEMU = WORK / "qemu"
IDF_COMMIT = "fcae32885b0296b32044cb99ecbdc50d98dddb83"
QEMU_COMMIT = "febae182e132e4055529be423a818225ebddaa3a"
PACKAGES = "python312 cmake ninja pkg-config gperf glib pixman libgcrypt libslirp libusb1 openssl git gcc meson"


def run(command, cwd=ROOT, **kwargs):
    return subprocess.run(list(map(str, command)), cwd=cwd, check=True, **kwargs)


def shell(script):
    script = "set -e\n" + script
    if shutil.which("nix-shell"):
        run(["nix-shell", "-p", *PACKAGES.split(), "--run", script])
    else:
        run(["bash", "-e", "-c", script])


def sdk(command):
    shell(f"export IDF_TOOLS_PATH={shlex.quote(str(WORK / 'tools'))}\n"
          f". {shlex.quote(str(IDF / 'export.sh'))}\n" + shlex.join(list(map(str, command))))


def verify_revision(directory, expected):
    actual = run(["git", "rev-parse", "HEAD"], cwd=directory,
                 capture_output=True, text=True).stdout.strip()
    if actual != expected:
        raise RuntimeError(f"Unexpected source revision in {directory}: {actual}")


def setup():
    WORK.mkdir(parents=True, exist_ok=True)
    if not IDF.exists():
        run(["git", "clone", "--depth", "1", "--branch", "v5.5.1", "--recursive",
             "--shallow-submodules", "https://github.com/espressif/esp-idf.git", IDF])
    verify_revision(IDF, IDF_COMMIT)
    shell(f"export IDF_TOOLS_PATH={shlex.quote(str(WORK / 'tools'))}\n"
          f"{shlex.quote(str(IDF / 'install.sh'))} esp32s3")
    if not QEMU.exists():
        run(["git", "init", QEMU])
        run(["git", "remote", "add", "origin", "https://github.com/espressif/qemu.git"], cwd=QEMU)
        run(["git", "fetch", "--depth", "1", "origin", QEMU_COMMIT], cwd=QEMU)
        run(["git", "checkout", "--detach", "FETCH_HEAD"], cwd=QEMU)
    verify_revision(QEMU, QEMU_COMMIT)


def build_qemu():
    verify_revision(QEMU, QEMU_COMMIT)
    patch = ROOT / "build/esp32/qemu.patch"
    applied = subprocess.run(["git", "apply", "--reverse", "--check", str(patch)], cwd=QEMU,
                             stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL).returncode == 0
    if not applied:
        run(["git", "apply", "--check", patch], cwd=QEMU)
        run(["git", "apply", patch], cwd=QEMU)
    for name in ("cg50-proxy.c", "dwc2-esp32-dma.inc"):
        shutil.copyfile(ROOT / "tools/vm/esp32" / name, QEMU / "hw/usb" / name)
    build = WORK / "qemu-build"
    build.mkdir(exist_ok=True)
    configure = [QEMU / "configure", "--target-list=xtensa-softmmu", "--enable-gcrypt",
                 "--enable-slirp", "--disable-werror", "--disable-docs", "--disable-gtk",
                 "--disable-sdl", "--disable-vnc", "--disable-tools", "--disable-guest-agent"]
    shell(f"cd {shlex.quote(str(build))}\n" + shlex.join(list(map(str, configure))) +
          "\nninja -j 8 qemu-system-xtensa")
    metadata = {"upstream": QEMU_COMMIT, "patch_sha256": digest(patch),
                "overlays": {p.name: digest(p) for p in (ROOT / "tools/vm/esp32").iterdir() if p.is_file()},
                "binary_sha256": digest(build / "qemu-system-xtensa")}
    (WORK / "qemu.json").write_text(json.dumps(metadata, indent=2) + "\n")


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def merge_flash(build, output):
    image = bytearray(b"\xff" * (4 * 1024 * 1024))
    files = json.loads((build / "flasher_args.json").read_text())["flash_files"]
    for offset, name in files.items():
        data = (build / name).read_bytes()
        start = int(offset, 0)
        if start + len(data) > len(image):
            raise ValueError("ESP32 image exceeds configured flash capacity")
        image[start:start + len(data)] = data
    output.write_bytes(image)


def firmware(target):
    verify_revision(IDF, IDF_COMMIT)
    build = WORK / f"firmware-{target}"
    defaults = str(ROOT / "build/esp32/sdkconfig.defaults")
    if target == "qemu":
        defaults += ";" + str(ROOT / "build/esp32/sdkconfig.qemu")
    sdk(["idf.py", "-C", ROOT / "build/esp32", "-B", build,
         "-D", f"SDKCONFIG={WORK / ('sdkconfig.' + target)}",
         "-D", f"SDKCONFIG_DEFAULTS={defaults}", "build"])
    merge_flash(build, build / "flash.bin")
    print(build / "flash.bin")


def launch(args):
    os.umask(0o077)
    directory = args.run_dir.resolve()
    directory.mkdir(parents=True, exist_ok=True)
    qmp, gdb = directory / "qmp.sock", directory / "gdb.sock"
    if len(str(gdb).encode()) >= 104:
        raise ValueError("Use a shorter run directory, for example /tmp/khicas-s3")
    if qmp.exists() or gdb.exists():
        raise ValueError("Debug sockets already exist; stop their VM or use another run directory")
    serial = directory / "serial.log"
    serial.touch(mode=0o600, exist_ok=True)
    serial.chmod(0o600)
    flash = directory / "flash.bin"
    if not flash.exists():
        merge_flash(WORK / "firmware-qemu", flash)
        flash.chmod(0o600)
    lan = f"tap,id=lan,model=open_eth,ifname={args.lan_tap},script=no,downscript=no" if args.lan_tap else \
          f"socket,id=lan,model=open_eth,listen=127.0.0.1:{args.lan_port}"
    command = [WORK / "qemu-build/qemu-system-xtensa", "-M", "esp32s3",
               "-drive", f"file={flash},if=mtd,format=raw", "-display", "none",
               "-serial", f"file:{serial}", "-monitor", "stdio",
               "-qmp", f"unix:{qmp},server=on,wait=off", "-gdb", f"unix:{gdb},server=on,wait=off",
               "-nic", "user,id=wan,model=open_eth", "-nic", lan]
    if args.usb_socket:
        command += ["-device", f"usb-cg50,port=1,socket={args.usb_socket.resolve()}"]
    print(f"Serial: {directory / 'serial.log'}\nGDB: target remote {gdb}", flush=True)
    try:
        run(command)
    finally:
        qmp.unlink(missing_ok=True)
        gdb.unlink(missing_ok=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    subs = parser.add_subparsers(dest="action", required=True)
    subs.add_parser("setup")
    subs.add_parser("build-qemu")
    build = subs.add_parser("firmware")
    build.add_argument("target", choices=("qemu", "hardware"))
    vm = subs.add_parser("run")
    vm.add_argument("--run-dir", type=Path, default=Path("/tmp/khicas-s3"))
    vm.add_argument("--usb-socket", type=Path)
    vm.add_argument("--lan-tap")
    vm.add_argument("--lan-port", type=int, default=5032)
    args = parser.parse_args()
    if args.action == "setup": setup()
    elif args.action == "build-qemu": build_qemu()
    elif args.action == "firmware": firmware(args.target)
    else: launch(args)


if __name__ == "__main__":
    main()
