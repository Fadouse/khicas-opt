"""Local browser display for the QEMU CG50 framebuffer and hardware keypad."""

import json
from http.server import BaseHTTPRequestHandler, HTTPServer
from pathlib import Path
import secrets
import threading
from urllib.parse import parse_qs, urlsplit

from display import _write_png


def ppm_to_png(source, destination):
    with Path(source).open("rb") as stream:
        if stream.readline().strip() != b"P6":
            raise ValueError("Expected QEMU P6 screenshot")
        dimensions = stream.readline()
        while dimensions.startswith(b"#"):
            dimensions = stream.readline()
        width, height = map(int, dimensions.split())
        if stream.readline().strip() != b"255" or not 0 < width * height <= 1_000_000:
            raise ValueError("Invalid QEMU screenshot geometry")
        pixels = stream.read(width * height * 3 + 1)
        if len(pixels) != width * height * 3:
            raise ValueError("Invalid screenshot pixel length")
    rows = b"".join(
        b"\0" + pixels[y * width * 3 : (y + 1) * width * 3] for y in range(height)
    )
    _write_png(destination, width, height, rows)


def start_panel(shell, port):
    token = secrets.token_urlsafe(24)
    html = Path(__file__).with_name("panel.html").read_bytes()

    class Handler(BaseHTTPRequestHandler):
        def log_message(self, *args):
            pass

        def reply(self, status, content, content_type):
            self.send_response(status)
            self.send_header("Content-Type", content_type)
            self.send_header("Content-Length", str(len(content)))
            self.send_header("Cache-Control", "no-store")
            self.end_headers()
            self.wfile.write(content)

        def authenticated(self):
            return parse_qs(urlsplit(self.path).query).get("token") == [token]

        def do_GET(self):
            if not self.authenticated():
                self.reply(403, b"Use the URL printed by the CLI.", "text/plain")
                return
            try:
                path = urlsplit(self.path).path
                if path == "/":
                    self.reply(200, html, "text/html; charset=utf-8")
                elif path == "/frame":
                    ppm = shell.run_dir / "panel.ppm"
                    png = shell.run_dir / "panel.png"
                    shell.qmp.execute("screendump", {"filename": str(ppm)})
                    ppm_to_png(ppm, png)
                    self.reply(200, png.read_bytes(), "image/png")
                elif path == "/status":
                    result = shell.qmp.execute("query-status")
                    self.reply(200, json.dumps(result).encode(), "application/json")
                else:
                    self.reply(404, b"Not found", "text/plain")
            except (ValueError, OSError, RuntimeError) as error:
                self.reply(500, str(error).encode(), "text/plain")

        def do_POST(self):
            if not self.authenticated():
                self.reply(403, b"Forbidden", "text/plain")
                return
            try:
                length = int(self.headers.get("Content-Length", "0"))
                if not 0 < length <= 256:
                    raise ValueError("Invalid request length")
                request = json.loads(self.rfile.read(length))
                action = request.get("action")
                if action == "key":
                    shell.do_key(request["key"])
                elif action in ("run", "pause"):
                    shell.qmp.execute("cont" if action == "run" else "stop")
                else:
                    raise ValueError("Unknown action")
                self.reply(200, b"{}", "application/json")
            except (ValueError, OSError, RuntimeError, KeyError) as error:
                self.reply(400, str(error).encode(), "text/plain")

    server = HTTPServer(("127.0.0.1", port), Handler)
    thread = threading.Thread(target=server.serve_forever, daemon=True)
    thread.start()
    return server, f"http://127.0.0.1:{server.server_port}/?token={token}"
