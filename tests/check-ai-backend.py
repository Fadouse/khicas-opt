#!/usr/bin/env python3
"""Exercise real local HTTP requests and bounded OpenAI-compatible responses."""

from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
import json
from pathlib import Path
import sys
import threading

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "src/runtime"))
from khicas_ai import AIError, OpenAICompatible, PROMPT


class Handler(BaseHTTPRequestHandler):
    mode = "ok"
    received = []

    def log_message(self, *args):
        pass

    def do_POST(self):
        data = json.loads(self.rfile.read(int(self.headers["Content-Length"])))
        self.received.append(data)
        assert self.path == "/v1/chat/completions"
        assert self.headers["Authorization"] == "Bearer local-test-key"
        if self.mode == "unauthorized":
            self.send_response(401)
            self.end_headers()
            self.wfile.write(b"PRIVATE provider error")
            return
        if self.mode == "redirect":
            self.send_response(302)
            self.send_header("Location", "http://127.0.0.1:1/secret")
            self.end_headers()
            return
        result = {
            "choices": [
                {
                    "finish_reason": "stop",
                    "message": {"content": "1. Use x^3/3.\nResult: 1/3"},
                }
            ]
        }
        if self.mode == "length":
            result["choices"][0]["finish_reason"] = "length"
        if self.mode == "oversize":
            result["choices"][0]["message"]["content"] = "x" * 2049
        if self.mode == "unicode":
            result["choices"][0]["message"]["content"] = "\u222b x dx"
        if self.mode == "control":
            result["choices"][0]["message"]["content"] = "Result: \x7f"
        if self.mode == "invalid_choice":
            result = {"choices": [1]}
        if self.mode == "invalid":
            result = {}
        self.send_response(200)
        self.send_header("Content-Type", "application/json")
        self.end_headers()
        self.wfile.write(json.dumps(result).encode())


def main():
    server = ThreadingHTTPServer(("127.0.0.1", 0), Handler)
    worker = threading.Thread(target=server.serve_forever, daemon=True)
    worker.start()
    try:
        client = OpenAICompatible(
            f"http://127.0.0.1:{server.server_port}/v1", "test-model", "local-test-key"
        )
        assert client.solve("integrate(x^2,x,0,1)") == b"1. Use x^3/3.\nResult: 1/3"
        payload = Handler.received[-1]
        assert payload == {
            "model": "test-model",
            "stream": False,
            "messages": [
                {"role": "system", "content": PROMPT},
                {"role": "user", "content": "integrate(x^2,x,0,1)"},
            ],
        }
        print(
            "PASS: HTTP request carries original expression, configured model and brief-solution prompt"
        )
        for mode in [
            "unauthorized",
            "redirect",
            "length",
            "oversize",
            "unicode",
            "control",
            "invalid",
            "invalid_choice",
        ]:
            Handler.mode = mode
            try:
                client.solve("x")
            except AIError as error:
                if mode == "redirect":
                    assert str(error) == "AI API HTTP 302"
                assert "PRIVATE" not in str(error) and "local-test-key" not in str(
                    error
                )
            else:
                raise AssertionError(mode + " was not rejected")
            print("PASS:", mode)
    finally:
        server.shutdown()
        server.server_close()
        worker.join()


if __name__ == "__main__":
    main()
