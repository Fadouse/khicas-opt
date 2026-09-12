# CG50 QEMU development VM

An experimental CG50 board for QEMU 10.1.0, with a command-line debugger and
an optional browser display/keypad. The guest executes its own reset, OS,
menu, graphics and application code. There are no firmware-PC patches or
host substitutes for OS subroutines in this backend.

## Build and run

From the repository root:

```sh
nix-shell tools/vm/shell.nix --run 'python3 tools/vm/build_qemu.py'
python3 tools/vm/qemu_vm.py --rom /absolute/path/to/cg50-flash.bin --ui
```

Without Nix, install GCC, Git, Python 3, Meson, Ninja, pkg-config and the GLib
and pixman development packages, then run `python3 tools/vm/build_qemu.py`.
The build downloads pinned QEMU source and builds only `qemu-system-sh4eb`.
Use `--source PATH` to reuse that exact QEMU checkout. Source and binaries
stay under `.build/vm/` by default; `.build/vm/qemu.json` records their hashes.
The launcher rejects a stale board build. `--qemu PATH` explicitly overrides
that build record.

Supply a locally obtained raw firmware/flash image, not a G3A or an updater
executable. No Casio firmware is included. The board provides 32 MiB of NOR
flash; shorter images are padded with erased bytes and produce a warning.
Such an image is not a complete dump of a physical calculator. The guest
can initialize the missing storage in its private in-memory flash copy.
The input file is never overwritten, and changes are discarded on exit.

The VM starts paused at the reset vector. Run `run 8` to begin boot. With
the tested OS image, first boot opens Message Language. Keep English with
F6, advance through display and power settings with F6, then use F1 to
select the battery type, F1 to confirm, and F6 to finish. The actual guest
screen is authoritative if another firmware follows a different sequence.
In this image, selecting F1 on the language page opens a dialog labelled
"Press: [EXIT]", but the executed wait routine accepts MENU/SET UP. MENU
closes it in the VM. GDB confirmed EXIT reaches the OS as `0x7533`; the
wait routine compares `0x7532`/`0x754d`. This image-specific discrepancy is
not resolved; the board does not remap EXIT or patch the firmware. F6
avoids that optional dialog during setup.

`--ui` prints a local, token-protected URL. Open it for live 384×216 guest
pixels and a keypad. It connects to the same VM as the CLI. No browser is
required for scripts or debugging. The service binds only to 127.0.0.1;
`--port N` selects a port, otherwise one is allocated automatically.

## CLI and dynamic debugging

| Command | Effect |
| --- | --- |
| `run [seconds]`, `pause`, `status` | Continue/stop the guest and inspect actual QEMU state. |
| `regs`, `disas [address]` | Registers and guest disassembly. |
| `step [count]` | Single-step through QEMU's GDB stub. |
| `break 0xADDRESS`, `delete 0xADDRESS` | Set/remove an execution breakpoint without patching ROM. |
| `x /8wx 0x8c000000`, `xp /8wx 0x0c000000` | Virtual/physical memory in QEMU monitor syntax. |
| `write 0x8c7f0000 12345678` | Write hexadecimal bytes to guest virtual memory while paused. |
| `key EXE`, `key F6 0.2` | Press/release a key, then pause; optional hold time in seconds. |
| `hold X on`, `hold X off` | Hold/release a matrix key across later commands. |
| `screen /tmp/screen.ppm` | Capture the actual guest framebuffer. |
| `dump 0 0x2000000 /tmp/flash.bin` | Export physical memory, including guest-initialized flash. |
| `hmp help` | Native QEMU monitor commands. |
| `detach` | Release the built-in GDB client for input or external GDB. |
| `quit` | Stop the VM and close local sockets. |

Keys include F1–F6, EXE, MENU, EXIT, AC, SHIFT, ALPHA, X, arrows, digits,
`+ - * / .`, DEL, LOG, LN, SIN, COS, TAN, SQUARE, FRAC, SD, OPTN, VARS,
NEG, EXP, STO, `^`, `(`, `)` and `,`. Browser keyboard Enter/Home/Escape/End
map to EXE/MENU/EXIT/AC. The guest's own key scanning and debounce logic
handle these events. Key commands resume execution briefly, including when
the VM was paused. They are not instruction-synchronous input scheduling.

Use `detach` after internal stepping/breakpoints before driving keys or
browser controls. For an external SH4-capable GDB, pause and detach the
internal client, then connect to the Unix socket printed at startup:

```text
set architecture sh4
set endian big
target remote /tmp/cg50-session/gdb.sock
```

Set `--run-dir /tmp/cg50-session` for that socket path. Only one GDB client
can be attached at a time. Do not issue concurrent CLI/browser execution
commands while single-stepping. QMP remains available for observation.

`--script PATH` executes one CLI command per line (blank lines and `#`
comments are ignored), stops on a command error, and terminates the VM at
script completion. Example after initial setup:

```text
key EXE
key 2
key +
key 2
key EXE
screen /tmp/calculation.ppm
```

`run N` uses host seconds, not an instruction budget or a hardware timing
measurement. Runtime files include `input.json` (image size/hash),
`qemu.log`, `cpu.log`, and requested screenshots. `--log unimp,guest_errors`
helps inspect MMIO; verbose execution tracing can generate large logs.

## Validation

Run the focused device-model checks against the built binary:

```sh
python3 tools/vm/tests/check-qemu.py --qemu /absolute/path/to/qemu-system-sh4eb
```

These execute QEMU's qtest interface with a tiny synthetic SH ROM. They
check CFI/JEDEC modes, NOR bit programming, busy/erase timing, sector
isolation, buffered-program confirmation, decimal carry, keyboard matrix,
write-one-to-clear/periodic scan flags, and RGB565 channel order. They do
not assert that an arbitrary firmware has booted.

Local firmware acceptance on 2026-09-12 used OS 03.80.0010, 11,927,552 bytes,
SHA-256 `9804a6f69b7446ad854558173691a53ce01783240c4bb2d43f5e5594c4bd1571`.
Observed: native storage initialization and first-boot settings, MAIN MENU,
Run-Matrix `2+2 = 4`, return to MENU and reopening Run-Matrix,
`1.5+2.5 = 4`, live screenshots, QMP browser endpoints, single-step,
execution breakpoint, RAM write/readback, and held-key matrix state.
Guest screenshots were visually inspected. Browser HTML/JavaScript syntax
and HTTP/PNG/control responses were checked; automated browser layout
acceptance has not been run.

## Hardware coverage and limits

The board uses QEMU's SH7785 SH-4A CPU model with native instruction/MMU/TLB
execution. It is not a complete SH7305 implementation. Memory includes
32 MiB NOR, 8 MiB main RAM, local/on-chip RAM, and peripheral windows.
The model implements compatible NOR commands, the firmware-used BCD
operations, a KEYSC matrix, two interrupt sources and an LCD surface read
from the guest VRAM selected by LCD DMA setup.

Clock/power/ADC/RTC and several MMIO registers are approximations. Timer
periods and interrupt priorities are not calibrated to hardware. General
DMA transfers, USB/storage passthrough, a G3A installer, full LCD controller
behavior and complete save/restore/reset of peripheral state are not
implemented. Exporting flash does not save RAM or a complete VM snapshot.
Use a new process for a cold restart. Add-ins, KhiCAS and PoC behavior have
not yet been accepted in this VM. It supports firmware bring-up and UI
experiments; hardware timing, performance and device-sensitive behavior
still require a calculator. It does not establish that every future CAS
test can run equivalently on the host.

## Sources and licenses

- [QEMU v10.1.0](https://github.com/qemu/qemu/tree/v10.1.0), pinned commit
  `f8b2f64e2336a28bf0d50b6ef8a7d8c013e9bcf3`. The board overlay
  `qemu/cg50.c` is GPL-2.0-or-later and builds against QEMU's existing SH4
  target without CPU-core patches.
- Peripheral addresses and BCD behavior were cross-checked with
  [hexbinoct/casio-cg50](https://github.com/hexbinoct/casio-cg50/tree/3664a84d0955e783c42f375ab87f42e1d177190a),
  commit `3664a84d0955e783c42f375ab87f42e1d177190a`.
  Its MIT notice is retained in `qemu/LICENSE.reference`.
- `qemu_vm.py` and `rsp.py` provide CLI/QMP/GDB control; `panel.py` and
  `panel.html` provide the browser view, using the PNG writer in `display.py`.
