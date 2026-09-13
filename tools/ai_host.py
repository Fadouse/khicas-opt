#!/usr/bin/env python3
"""Serve one calculator-initiated AI request while the VM CPU is running."""

import argparse
import json
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path[:0] = [str(ROOT / "src/runtime"), str(ROOT / "tools/vm")]


def main():
    from khicas_ai import client_from_options, options, serve_once
    from usbhost import USBHost

    parser = argparse.ArgumentParser(description=__doc__, add_help=False)
    parser.add_argument("--usb-socket", type=Path, required=True)
    args, rest = parser.parse_known_args()
    client = client_from_options(options(rest))
    usb = USBHost(args.usb_socket)
    try:
        print(json.dumps(serve_once(usb, client), indent=2), flush=True)
    finally:
        usb.close()


if __name__ == "__main__":
    main()
