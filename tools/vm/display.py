"""fx-CG50 display model: 396x224 RGB565 framebuffer and PNG output.

The fx-CG50 panel is 396x224 pixels driven by a Renesas R61524-compatible
controller whose register interface is reachable at 0xB4000000.  The panel
also has a normal RAM VRAM buffer that the OS blits with DMA.

This module keeps a plain Python framebuffer and knows how to render it
without third-party dependencies, so it works in a headless build environment.
"""

import struct
import zlib

WIDTH = 396
HEIGHT = 224
INNER_WIDTH = 384
INNER_HEIGHT = 216


def rgb565(red, green, blue):
    return ((red >> 3) << 11) | ((green >> 2) << 5) | (blue >> 3)


class Framebuffer:
    def __init__(self, width=WIDTH, height=HEIGHT):
        self.width = width
        self.height = height
        self.pixels = bytearray(width * height * 2)

    def clear(self, color=0xFFFF):
        word = color.to_bytes(2, "big")
        self.pixels[:] = word * (self.width * self.height)

    def set_pixel(self, x, y, color):
        if 0 <= x < self.width and 0 <= y < self.height:
            offset = (y * self.width + x) * 2
            self.pixels[offset:offset + 2] = (color & 0xFFFF).to_bytes(2, "big")

    def get_pixel(self, x, y):
        offset = (y * self.width + x) * 2
        return int.from_bytes(self.pixels[offset:offset + 2], "big")

    def blit(self, source, x=0, y=0, width=None, height=None, source_stride=None):
        """Copy a big-endian RGB565 image into the panel, clipping to bounds."""
        width = self.width - x if width is None else width
        height = self.height - y if height is None else height
        source_stride = width if source_stride is None else source_stride
        for row in range(height):
            target_y = y + row
            if not 0 <= target_y < self.height:
                continue
            take = max(0, min(width, self.width - x))
            if take == 0:
                continue
            start = row * source_stride * 2
            chunk = bytes(source[start:start + take * 2])
            offset = (target_y * self.width + x) * 2
            self.pixels[offset:offset + len(chunk)] = chunk

    # -- output --------------------------------------------------------------
    def render_png(self, path):
        rows = bytearray()
        for y in range(self.height):
            rows.append(0)
            row = self.pixels[y * self.width * 2:(y + 1) * self.width * 2]
            for index in range(0, len(row), 2):
                color = int.from_bytes(row[index:index + 2], "big")
                red = (color >> 11) & 0x1F
                green = (color >> 5) & 0x3F
                blue = color & 0x1F
                rows.append((red << 3) | (red >> 2))
                rows.append((green << 2) | (green >> 4))
                rows.append((blue << 3) | (blue >> 2))
        return _write_png(path, self.width, self.height, bytes(rows))

    def render_ascii(self, scale=1):
        """A crude terminal preview useful while debugging headless."""
        ramp = " .:-=+*#%@"
        lines = []
        for y in range(0, self.height, 2 * scale):
            line = []
            for x in range(0, self.width, 2 * scale):
                color = self.get_pixel(x, y)
                red = (color >> 11) & 0x1F
                green = (color >> 5) & 0x3F
                blue = color & 0x1F
                level = (red * 3 + green * 6 + blue) // 10
                line.append(ramp[min(len(ramp) - 1, level * len(ramp) // 64)])
            lines.append("".join(line))
        return "\n".join(lines)


def _write_png(path, width, height, raw_rows):
    def chunk(tag, payload):
        return (
            struct.pack(">I", len(payload))
            + tag
            + payload
            + struct.pack(">I", zlib.crc32(tag + payload) & 0xFFFFFFFF)
        )

    header = struct.pack(">IIBBBBB", width, height, 8, 2, 0, 0, 0)
    png = (
        b"\x89PNG\r\n\x1a\n"
        + chunk(b"IHDR", header)
        + chunk(b"IDAT", zlib.compress(raw_rows, 9))
        + chunk(b"IEND", b"")
    )
    with open(path, "wb") as stream:
        stream.write(png)
    return path
