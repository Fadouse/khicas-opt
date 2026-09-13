#!/usr/bin/env python3
"""Exercise ESP32 guest TLS/config/NAPT in an isolated LAN namespace."""
import argparse
import base64
import contextlib
import hashlib
import http.client
import http.server
import json
import os
from pathlib import Path
import re
import shutil
import socket
import ssl
import struct
import subprocess
import sys
import threading
import tempfile
import time

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools/vm"))
from qemu_vm import QMP, DebugShell

# Reviewed native English viewer for the fixed HTTPS fixture, excluding status bar.
VIEWER_SHA256 = "0c0fb5c897b08b2ed76982033ddf58a0a7f4428dd468e064b4be6ed1db196980"


def verify_viewer(path):
    header, pixels = path.read_bytes().split(b"\n255\n", 1)
    assert header == b"P6\n384 216" and len(pixels) == 384 * 216 * 3
    assert hashlib.sha256(pixels[24 * 384 * 3:112 * 384 * 3]).hexdigest() == VIEWER_SHA256


def wait_for(predicate, seconds=30):
    deadline = time.monotonic() + seconds
    while time.monotonic() < deadline:
        result = predicate()
        if result:
            return result
        time.sleep(0.1)
    raise TimeoutError("Acceptance condition timed out")


def command(*args):
    subprocess.run(args, check=True, stdout=subprocess.DEVNULL)


def tcp_syns(path):
    data = path.read_bytes()
    if data[:4] != b"\xd4\xc3\xb2\xa1":
        raise ValueError("Expected little-endian QEMU PCAP")
    position = 24
    packets = []
    while position + 16 <= len(data):
        _, _, length, _ = struct.unpack_from("<IIII", data, position)
        position += 16
        frame = data[position:position + length]
        position += length
        if len(frame) < 54 or frame[12:14] != b"\x08\x00" or frame[23] != 6:
            continue
        offset = 14 + (frame[14] & 15) * 4
        if len(frame) < offset + 20 or not frame[offset + 13] & 2:
            continue
        source_port, destination_port, sequence = struct.unpack_from("!HHI", frame, offset)
        packets.append((socket.inet_ntoa(frame[26:30]), socket.inet_ntoa(frame[30:34]),
                        source_port, destination_port, sequence))
    return packets


def ntp_server():
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.bind(("127.0.0.1", 123))
    while True:
        data, peer = sock.recvfrom(512)
        if len(data) < 48:
            continue
        stamp = struct.pack(">II", int(time.time()) + 2208988800, 0)
        reply = bytearray(48)
        reply[:4] = bytes((0x24, 1, 4, 0xEC))
        reply[12:16] = b"LOCL"
        reply[16:24] = stamp
        reply[24:32] = data[40:48]
        reply[32:40] = reply[40:48] = stamp
        sock.sendto(reply, peer)


class Fixture(http.server.BaseHTTPRequestHandler):
    questions = []

    def log_message(self, *args):
        pass

    def do_GET(self):
        data = b"guest-nat-reached-upstream\n"
        self.send_response(200)
        self.send_header("Content-Length", str(len(data)))
        self.end_headers()
        self.wfile.write(data)

    def do_POST(self):
        assert self.path == "/v1/chat/completions"
        assert self.headers.get("Authorization") == "Bearer local-test-key"
        length = int(self.headers["Content-Length"])
        assert 0 < length < 16384
        body = json.loads(self.rfile.read(length))
        assert body["stream"] is False
        assert body["model"] == "fixture-model"
        assert '1-3 very short' in body["messages"][0]["content"]
        question = body["messages"][1]["content"]
        self.questions.append(question)
        data = json.dumps({"choices": [{"finish_reason": "stop", "message": {
            "content": "[MOCK] Antiderivative: x^3/3.\nEvaluate from 0 to 1.\nResult: 1/3"
        }}]}).encode()
        self.send_response(200)
        self.send_header("Content-Type", "application/json")
        self.send_header("Content-Length", str(len(data)))
        self.end_headers()
        self.wfile.write(data)


def calculator_exchange(rom, output, esp_qmp, serial, processes, launch_keys):
    directory = Path(tempfile.mkdtemp(prefix="cg50-esp32-"))
    command = [str(ROOT / ".build/vm/bin/qemu-system-sh4eb"), "-M", "cg50", "-bios", str(rom),
               "-display", "none", "-serial", "none", "-monitor", "none", "-S",
               "-qmp", f"unix:{directory / 'qmp.sock'},server=on,wait=off",
               "-chardev", f"socket,id=cg50-usb,path={directory / 'usb.sock'},server=on,wait=off"]
    with (output / "cg50.log").open("w") as log:
        processes.append(subprocess.Popen(command, stdout=log, stderr=log))
    wait_for(lambda: (directory / "qmp.sock").exists())
    cg_qmp = QMP(directory / "qmp.sock")
    shell = DebugShell(cg_qmp, directory)
    try:
        with (output / "cg50-input.log").open("w") as log, contextlib.redirect_stdout(log):
            lines = (ROOT / "tools/vm/tests/ai.cli").read_text().splitlines()
            start = lines.index("key DOWN 0.06 0.5")
            end = lines.index("run 2", start)
            navigation = ["chord " + key.replace("+", " ") if "+" in key else "key " + key + " 0.06 0.5"
                          for key in launch_keys]
            lines[start:end] = navigation
            for line in lines:
                if line.startswith("key EXIT"): break
                if line.strip() and not line.startswith("#"):
                    print(line, flush=True)
                    shell.onecmd(line)
                    if shell.last_error:
                        raise shell.last_error
            shell.do_screen(str(output / "cg50-question.ppm"))
            shell.do_resume("")
        esp_qmp.execute("device_add", {"driver": "usb-cg50", "id": "calculator", "port": "1",
                                     "socket": str(directory / "usb.sock")})
        wait_for(lambda: "USB exchange acknowledged" in serial.read_text(), 100)
        time.sleep(0.5)
        with (output / "cg50-input.log").open("a") as log, contextlib.redirect_stdout(log):
            shell.do_screen(str(output / "cg50-reply.ppm"))
        verify_viewer(output / "cg50-reply.ppm")
        assert len(Fixture.questions) == 1, Fixture.questions
        assert Fixture.questions[0] in ("integrate(x^2,x,0,1)", "integrate(x^2,x,0,1.0)"), Fixture.questions
        print("PASS CG50 ai() -> S3 USB Host -> HTTPS API fixture -> native CG50 reply", flush=True)
        with (output / "cg50-input.log").open("a") as log, contextlib.redirect_stdout(log):
            for line in ("key EXE 0.04 0.3", "key F1 0.04 0.05", "key 1 0.04 0.01", "key EXE 0.04 0.1", "resume"):
                shell.onecmd(line)
                if shell.last_error:
                    raise shell.last_error
        try:
            wait_for(lambda: serial.read_text().count("USB exchange acknowledged") == 2, 40)
        except TimeoutError:
            (output / "usb-registers.txt").write_text(esp_qmp.hmp("xp /20wx 0x60080400"))
            with (output / "cg50-input.log").open("a") as log, contextlib.redirect_stdout(log):
                shell.do_screen(str(output / "cg50-second-failed.ppm"))
            raise
        time.sleep(0.5)
        with (output / "cg50-input.log").open("a") as log, contextlib.redirect_stdout(log):
            shell.do_screen(str(output / "cg50-second.ppm"))
        verify_viewer(output / "cg50-second.ppm")
        assert len(Fixture.questions) == 2
        print("PASS second USB AI request after detach and native viewer exit", flush=True)
    finally:
        cg_qmp.close()


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--inside", action="store_true")
    parser.add_argument("--cg50-rom", type=Path)
    parser.add_argument("--launch-keys", nargs="+", default=["DOWN"] * 4 + ["RIGHT", "RIGHT", "EXE"])
    parser.add_argument("--output", type=Path, default=ROOT / ".build/esp32/acceptance")
    args = parser.parse_args()
    if not args.inside:
        os.execvp("unshare", ["unshare", "--user", "--map-root-user", "--net",
                             sys.executable, __file__, "--inside", *sys.argv[1:]])
    os.umask(0o077)
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=True)
    command("ip", "link", "set", "lo", "up")
    command("ip", "tuntap", "add", "dev", "bridge-lan", "mode", "tap")
    command("ip", "addr", "add", "192.168.50.2/24", "dev", "bridge-lan")
    command("ip", "link", "set", "bridge-lan", "up")
    command("ip", "route", "add", "10.0.2.0/24", "via", "192.168.50.1")
    command("openssl", "req", "-x509", "-newkey", "ec", "-pkeyopt", "ec_paramgen_curve:P-256",
            "-nodes", "-keyout", str(output / "api.key"), "-out", str(output / "api.pem"),
            "-days", "2", "-subj", "/CN=10.0.2.2", "-addext", "subjectAltName=IP:10.0.2.2")
    server = http.server.ThreadingHTTPServer(("127.0.0.1", 8443), Fixture)
    tls = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
    tls.load_cert_chain(output / "api.pem", output / "api.key")
    server.socket = tls.wrap_socket(server.socket, server_side=True)
    threading.Thread(target=server.serve_forever, daemon=True).start()
    threading.Thread(target=ntp_server, daemon=True).start()
    flash = output / "flash.bin"
    firmware = ROOT / ".build/esp32/firmware-qemu"
    image = bytearray(b"\xff" * (4 * 1024 * 1024))
    for offset, name in json.loads((firmware / "flasher_args.json").read_text())["flash_files"].items():
        data = (firmware / name).read_bytes()
        start = int(offset, 0)
        image[start:start + len(data)] = data
    flash.write_bytes(image)
    serial = output / "serial.log"
    serial.unlink(missing_ok=True)
    processes = []
    qmp_path = Path(f"/tmp/khicas-esp32-{os.getpid()}.sock")
    qmp = None
    try:
        qemu = ROOT / ".build/esp32/qemu-build/qemu-system-xtensa"
        cmd = [str(qemu), "-M", "esp32s3", "-drive", f"file={flash},if=mtd,format=raw",
               "-display", "none", "-serial", f"file:{serial}", "-monitor", "none",
               "-qmp", f"unix:{qmp_path},server=on,wait=off",
               "-nic", "user,id=wan,model=open_eth",
               "-nic", "tap,id=lan,model=open_eth,ifname=bridge-lan,script=no,downscript=no",
               "-object", f"filter-dump,id=wan-pcap,netdev=wan,file={output / 'wan.pcap'}",
               "-object", f"filter-dump,id=lan-pcap,netdev=lan,file={output / 'lan.pcap'}"]
        with (output / "qemu.log").open("w") as log:
            processes.append(subprocess.Popen(cmd, stdout=log, stderr=log))
        wait_for(lambda: serial.exists() and "local setup password" in serial.read_text())
        print("PASS actual ESP32-S3 firmware boot and USB host initialization", flush=True)
        qmp = QMP(qmp_path)
        log = serial.read_text()
        password = re.search(r"local setup password .*: ([0-9a-f]+)", log).group(1)
        fingerprint = re.search(r"HTTPS certificate SHA256: ([0-9a-f]+)", log).group(1)
        # Bootstrap trust using the fingerprint from this VM's local serial console.
        insecure = ssl.SSLContext(ssl.PROTOCOL_TLS_CLIENT)
        insecure.check_hostname = False
        insecure.verify_mode = ssl.CERT_NONE
        with socket.create_connection(("192.168.50.1", 443), timeout=10) as raw:
            with insecure.wrap_socket(raw, server_hostname="192.168.50.1") as connection:
                certificate = connection.getpeercert(binary_form=True)
        assert hashlib.sha256(certificate).hexdigest() == fingerprint
        trusted = ssl.create_default_context(cadata=ssl.DER_cert_to_PEM_cert(certificate))
        auth = "Basic " + base64.b64encode(f"admin:{password}".encode()).decode()

        def request(method, path, body=None, authenticate=True, config_header=True):
            connection = http.client.HTTPSConnection("192.168.50.1", context=trusted, timeout=15)
            headers = {}
            if authenticate:
                headers["Authorization"] = auth
            if body is not None:
                body = json.dumps(body)
                headers["Content-Type"] = "application/json"
            if config_header:
                headers["X-KhiCAS-Config"] = "1"
            connection.request(method, path, body, headers)
            response = connection.getresponse()
            result = response.status, response.read()
            connection.close()
            return result

        assert request("GET", "/", authenticate=False)[0] == 401
        assert request("GET", "/")[0] == 200
        print("PASS AP-side HTTPS certificate validation and authentication", flush=True)
        api_ca = (output / "api.pem").read_text()
        config = {"ssid": "fixture-uplink", "wifi_password": "fixture-password",
                  "api_url": "https://10.0.2.2:8443/v1", "model": "fixture-model",
                  "api_key": "local-test-key", "api_ca": api_ca}
        assert request("POST", "/config", config, config_header=False)[0] == 403
        assert request("POST", "/config", dict(config, api_url="http://example.com/v1"))[0] == 400
        assert request("POST", "/config", config)[0] == 200
        code, saved = request("GET", "/config")
        assert code == 200 and json.loads(saved)["has_key"]
        assert b"local-test-key" not in saved and b"fixture-password" not in saved
        print("PASS configuration validation, save, and secret redaction", flush=True)
        upstream = http.client.HTTPSConnection("10.0.2.2", 8443,
                     context=ssl.create_default_context(cadata=api_ca), timeout=15)
        upstream.request("GET", "/nat")
        response = upstream.getresponse()
        assert response.status == 200 and response.read() == b"guest-nat-reached-upstream\n"
        upstream.close()
        lan_syns = [p for p in tcp_syns(output / "lan.pcap") if p[0] == "192.168.50.2" and p[3] == 8443]
        wan_syns = tcp_syns(output / "wan.pcap")
        assert any(w[0] == "10.0.2.15" and w[1] == p[1] and w[3:] == p[3:]
                   for p in lan_syns for w in wan_syns), (lan_syns, wan_syns)
        print("PASS LAN client TCP/TLS through ESP32 guest NAPT to upstream", flush=True)
        assert request("POST", "/restart")[0] == 200
        wait_for(lambda: serial.read_text().count("local setup password") == 2)
        assert request("GET", "/config")[1] == saved
        print("PASS reboot preserves NVS settings, credentials and TLS identity", flush=True)
        if args.cg50_rom:
            calculator_exchange(args.cg50_rom.resolve(), output, qmp, serial, processes, args.launch_keys)
        (output / "result.json").write_text(json.dumps({
            "boot": True, "https": True, "configuration": True, "guest_nat": True,
            "persistence": True, "radio": "not emulated", "usb_ai": bool(args.cg50_rom),
            "provider": "local HTTPS fixture; no real AI key configured",
            "questions": Fixture.questions,
        }, indent=2) + "\n")
    finally:
        if qmp:
            qmp.close()
        for process in reversed(processes):
            process.terminate()
            try:
                process.wait(timeout=10)
            except subprocess.TimeoutExpired:
                process.kill()
                process.wait()
        qmp_path.unlink(missing_ok=True)
        server.shutdown()


if __name__ == "__main__":
    main()
