"""Small GDB Remote Serial Protocol client for QEMU's local Unix socket."""

import socket


class RSP:
    def __init__(self, path):
        self.socket = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        self.socket.settimeout(5)
        self.socket.connect(str(path))
        self.request("?")

    def send(self, command):
        payload = command.encode("ascii")
        self.socket.sendall(
            b"$" + payload + b"#" + f"{sum(payload) & 255:02x}".encode()
        )

    def byte(self):
        value = self.socket.recv(1)
        if not value:
            raise RuntimeError("GDB disconnected")
        return value[0]

    def receive(self):
        while True:
            start = self.byte()
            if start == ord("-"):
                raise RuntimeError("GDB rejected packet checksum")
            if start == ord("$"):
                break
        payload = bytearray()
        while (value := self.byte()) != ord("#"):
            payload.append(value)
            if len(payload) > 1_048_576:
                raise RuntimeError("GDB packet too large")
        checksum = bytes((self.byte(), self.byte()))
        if int(checksum, 16) != sum(payload) & 255:
            self.socket.sendall(b"-")
            raise RuntimeError("GDB response checksum mismatch")
        self.socket.sendall(b"+")
        decoded = bytearray()
        i = 0
        while i < len(payload):
            value = payload[i]
            if value == ord("}"):
                i += 1
                decoded.append(payload[i] ^ 0x20)
            elif value == ord("*"):
                i += 1
                decoded.extend([decoded[-1]] * (payload[i] - 29))
            else:
                decoded.append(value)
            i += 1
        result = decoded.decode("ascii")
        if result.startswith("E") and len(result) == 3:
            raise RuntimeError("GDB error " + result)
        return result

    def request(self, command):
        self.send(command)
        return self.receive()

    def resume(self, seconds=None):
        self.send("c")
        self.socket.settimeout(seconds)
        try:
            return self.receive()
        except (TimeoutError, KeyboardInterrupt):
            self.socket.settimeout(5)
            self.socket.sendall(b"\x03")
            return self.receive()
        finally:
            self.socket.settimeout(5)

    def close(self):
        self.request("D")
        self.socket.close()
