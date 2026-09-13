"""Transaction-level USB host for the CG50 USBHS model."""

import socket
import struct
import time
from pathlib import Path
import subprocess


class SCSIError(RuntimeError):
    pass


class USBHost:
    def __init__(self, path):
        self.socket = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        self.socket.settimeout(5)
        self.socket.connect(str(path))
        self.stream = self.socket.makefile("rwb", buffering=0)
        self.tag = 0
        self.bulk_in_endpoint = self.bulk_out_endpoint = None
        self.bulk_in_size = self.bulk_out_size = 0

    def command(self, text):
        self.stream.write(text.encode("ascii") + b"\n")
        reply = self.stream.readline(8300)
        if not reply.endswith(b"\n"):
            raise RuntimeError("USB host transport disconnected or malformed response")
        result = reply.decode("ascii").rstrip("\n")
        if result.startswith(("ERROR", "STALL", "DISCONNECTED")):
            raise RuntimeError(result)
        return result

    def transaction(self, text, timeout=3):
        deadline = time.monotonic() + timeout
        while True:
            result = self.command(text)
            if result != "NAK":
                return result
            if time.monotonic() >= deadline:
                raise TimeoutError("USB NAK timeout: " + text[:80])
            time.sleep(0.002)

    def control(self, setup, data=b""):
        if len(setup) != 8:
            raise ValueError("USB setup must contain 8 bytes")
        direction, request, value, index, length = struct.unpack("<BBHHH", setup)
        self.command("setup " + setup.hex())
        result = bytearray()
        if direction & 128:
            while len(result) < length:
                packet = self.transaction("in 0")
                if not packet.startswith("DATA "):
                    raise RuntimeError("Unexpected USB response: " + packet)
                packet = bytes.fromhex(packet[5:])
                result.extend(packet)
                if len(packet) < 64:
                    break
            self.transaction("out 0")
        else:
            if length != len(data):
                raise ValueError("Control OUT data length mismatch")
            for offset in range(0, length, 64):
                self.transaction("out 0 " + data[offset : offset + 64].hex())
            if self.transaction("in 0") != "DATA ":
                raise RuntimeError("Expected zero-length USB status packet")
        return bytes(result)

    def enumerate(self, interface="storage"):
        classes = {"storage": bytes((8, 6, 80)), "vendor": bytes((255, 0, 0))}
        if interface not in classes:
            raise ValueError("Interface must be storage or vendor")
        self.bulk_in_endpoint = self.bulk_out_endpoint = None
        self.command("reset")
        time.sleep(0.2)
        device = self.control(struct.pack("<BBHHH", 128, 6, 0x100, 0, 18))
        if len(device) != 18 or device[1] != 1:
            raise RuntimeError("Invalid USB device descriptor: " + device.hex())
        self.control(struct.pack("<BBHHH", 0, 5, 1, 0, 0))
        header = self.control(struct.pack("<BBHHH", 128, 6, 0x200, 0, 9))
        if len(header) != 9 or header[1] != 2:
            raise RuntimeError("Invalid USB configuration descriptor: " + header.hex())
        length = struct.unpack_from("<H", header, 2)[0]
        if not 9 <= length <= 4096:
            raise RuntimeError("Invalid configuration length")
        configuration = self.control(struct.pack("<BBHHH", 128, 6, 0x200, 0, length))
        if len(configuration) != length:
            raise RuntimeError("Truncated configuration descriptor")
        offset, selected = 0, False
        endpoints = {}
        while offset < length:
            size = configuration[offset]
            if size < 2 or offset + size > length:
                raise RuntimeError("Malformed USB descriptor chain")
            descriptor = configuration[offset : offset + size]
            if descriptor[1] == 4:
                selected = (
                    size >= 9
                    and len(endpoints) != 2
                    and descriptor[3] == 0
                    and descriptor[5:8] == classes[interface]
                )
                if selected:
                    endpoints = {}
            elif (
                descriptor[1] == 5 and selected and size >= 7 and descriptor[3] & 3 == 2
            ):
                packet = struct.unpack_from("<H", descriptor, 4)[0] & 0x7FF
                if not 0 < packet <= 512 or not descriptor[2] & 15:
                    raise RuntimeError("Invalid bulk endpoint descriptor")
                endpoints[bool(descriptor[2] & 128)] = (descriptor[2] & 15, packet)
            offset += size
        if len(endpoints) != 2:
            raise RuntimeError("No supported USB " + interface + " endpoint pair")
        self.bulk_in_endpoint, self.bulk_in_size = endpoints[True]
        self.bulk_out_endpoint, self.bulk_out_size = endpoints[False]
        self.control(struct.pack("<BBHHH", 0, 9, header[5], 0, 0))
        return {
            "interface": interface,
            "device": device.hex(),
            "configuration": configuration.hex(),
            "vid": f"{struct.unpack_from('<H', device, 8)[0]:04x}",
            "pid": f"{struct.unpack_from('<H', device, 10)[0]:04x}",
        }

    def reply(self):
        """Receive the calculator's request before sending a matching response."""
        request = self.bulk_in(64)
        sequence = request[4:-1]
        if (
            not request.startswith(b"send")
            or not request.endswith(b"\n")
            or not sequence.isdigit()
            or not 1 <= len(sequence) <= 10
            or sequence.startswith(b"0")
            or int(sequence) > 0xFFFFFFFF
        ):
            raise ValueError("Unexpected calculator request: " + request.hex())
        response = b"recv" + sequence + b"\n"
        self.bulk_out(response)
        return {
            "calculator_to_host": request.decode("ascii"),
            "host_to_calculator": response.decode("ascii"),
        }

    def close(self):
        self.stream.close()
        self.socket.close()

    def bulk_out(self, data):
        if self.bulk_out_endpoint is None:
            raise RuntimeError("Enumerate the USB device first")
        for offset in range(0, len(data), self.bulk_out_size):
            packet = data[offset : offset + self.bulk_out_size]
            reply = self.transaction(f"out {self.bulk_out_endpoint} " + packet.hex())
            if reply != "OK":
                raise RuntimeError("Unexpected bulk OUT response: " + reply)

    def bulk_in(self, length):
        if self.bulk_in_endpoint is None:
            raise RuntimeError("Enumerate the USB device first")
        result = bytearray()
        while len(result) < length:
            reply = self.transaction(f"in {self.bulk_in_endpoint}")
            if not reply.startswith("DATA "):
                raise RuntimeError("Unexpected bulk IN response: " + reply)
            packet = bytes.fromhex(reply[5:])
            result.extend(packet)
            if len(packet) < self.bulk_in_size:
                break
        if len(result) > length:
            raise RuntimeError("USB device exceeded bulk transfer length")
        return bytes(result)

    def scsi(self, cdb, length=0, data=None):
        if not 1 <= len(cdb) <= 16 or not 0 <= length <= 1024 * 1024:
            raise ValueError("SCSI CDB or transfer length out of bounds")
        if data is not None and len(data) != length:
            raise ValueError("SCSI OUT length mismatch")
        self.tag = (self.tag + 1) & 0xFFFFFFFF
        cbw = struct.pack(
            "<4sIIBBB16s",
            b"USBC",
            self.tag,
            length,
            128 if data is None else 0,
            0,
            len(cdb),
            cdb,
        )
        self.bulk_out(cbw)
        result = b""
        if length:
            if data is None:
                result = self.bulk_in(length)
            else:
                self.bulk_out(data)
        if len(result) == 13 and result[:4] == b"USBS" and length != 13:
            csw, result = result, b""
        else:
            try:
                csw = self.bulk_in(13)
            except TimeoutError as error:
                raise TimeoutError(
                    f"SCSI status timeout after {len(result)}/{length} data bytes; prefix={result[:16].hex()}"
                ) from error
        if len(csw) != 13:
            raise RuntimeError("Malformed SCSI status: " + csw.hex())
        signature, tag, residue, status = struct.unpack("<4sIIB", csw)
        if signature != b"USBS" or tag != self.tag or residue > length:
            raise RuntimeError("Invalid SCSI status: " + csw.hex())
        if status:
            raise SCSIError(f"SCSI command failed: status={status}, residue={residue}")
        if data is None and len(result) != length - residue:
            raise RuntimeError(
                f"SCSI data length disagrees with residue: received={len(result)}, requested={length}, status={csw.hex()}"
            )
        return result

    def capacity(self):
        data = self.scsi(bytes.fromhex("25000000000000000000"), 8)
        if len(data) != 8:
            raise RuntimeError("Invalid READ CAPACITY response")
        last, size = struct.unpack(">II", data)
        if size != 512 or not 0 < last < 131072:
            raise RuntimeError(f"Unsupported storage geometry: {last + 1} x {size}")
        return last + 1, size

    def read_blocks(self, lba, count):
        if not 0 <= lba <= 0xFFFFFFFF or not 1 <= count <= 128:
            raise ValueError("Invalid block range")
        cdb = struct.pack(">BBIBHB", 0x28, 0, lba, 0, count, 0)
        data = self.scsi(cdb, count * 512)
        if len(data) != count * 512:
            raise RuntimeError("Short block read")
        return data

    def write_blocks(self, lba, data):
        count = len(data) // 512
        if len(data) % 512 or not 1 <= count <= 128 or not 0 <= lba <= 0xFFFFFFFF:
            raise ValueError("Invalid block write")
        self.scsi(struct.pack(">BBIBHB", 0x2A, 0, lba, 0, count, 0), len(data), data)

    def image(self, path):
        # Reset/media-change unit attention must be acknowledged by the host.
        try:
            self.scsi(bytes(6))
        except SCSIError:
            sense = self.scsi(bytes.fromhex("030000001200"), 18)
            if len(sense) != 18 or sense[2] & 15 != 6:
                raise RuntimeError("Storage not ready: sense=" + sense.hex())
            self.scsi(bytes(6))
        blocks, size = self.capacity()
        with Path(path).open("wb") as output:
            for lba in range(0, blocks, 8):
                output.write(self.read_blocks(lba, min(8, blocks - lba)))
        return {"blocks": blocks, "block_size": size, "path": str(path)}

    def install(self, paths, image_path):
        paths = [Path(path).resolve(strict=True) for path in paths]
        if not paths or any(
            path.suffix.lower() not in (".g3a", ".ac2") for path in paths
        ):
            raise ValueError("Install requires G3A/AC2 files")
        self.image(image_path)
        original = Path(image_path).read_bytes()
        start = struct.unpack_from("<I", original, 454)[0]
        if original[510:512] != b"\x55\xaa" or not 0 < start < len(original) // 512:
            raise RuntimeError("Invalid virtual disk partition table")
        filesystem = f"{image_path}@@{start * 512}"
        for path in paths:
            subprocess.run(
                ["mcopy", "-o", "-i", filesystem, str(path), "::/"], check=True
            )
        modified = Path(image_path).read_bytes()
        if len(original) != len(modified):
            raise RuntimeError("Filesystem tool changed virtual disk capacity")
        written = 0
        for offset in range(0, len(original), 512):
            block = modified[offset : offset + 512]
            if block != original[offset : offset + 512]:
                self.write_blocks(offset // 512, block)
                if self.read_blocks(offset // 512, 1) != block:
                    raise RuntimeError(
                        f"USB write verification failed at LBA {offset // 512}"
                    )
                written += 1
        return {"files": [str(path) for path in paths], "verified_sectors": written}
