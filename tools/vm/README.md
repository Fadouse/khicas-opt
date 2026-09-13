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
The optional language confirmation closes with EXIT. A populated storage image
can also show an add-in installation note after setup; acknowledge it with EXE.
A CPU reset currently returns this extracted image to initial setup.

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
| `key EXE`, `key DOWN 0.06 0.5` | Press/release, with optional hold and settling times in seconds. |
| `chord 1 , AC` | Press/release a group of matrix keys together. |
| `hold X on`, `hold X off` | Hold/release a matrix key across later commands. |
| `screen /tmp/screen.ppm` | Capture the actual guest framebuffer. |
| `dump 0 0x2000000 /tmp/flash.bin` | Export physical memory, including guest-initialized flash. |
| `hmp help`, `hmp system_reset` | Monitor commands; reset restarts the CPU at A0000000, retaining memory and peripherals. |
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
script completion. Add `--interactive` to keep the CLI open afterward. Example after initial setup:

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

## Virtual USB host

The CLI host exchanges USB setup and bulk packets with the modeled USBHS
controller. The firmware performs enumeration, SCSI and storage operations;
files are not injected into OS memory. Install requires `mcopy` from mtools
(included in the Nix shell). This is a local Unix-socket host, not a device
mounted by the host operating system.

After the guest has completed initial setup:

```text
usb attach
key F1 0.06 1
usb enumerate
usb scsi 120000002400 36
usb scsi 25000000000000000000 8
usb image /tmp/cg50-disk.img
usb install /absolute/path/khicas50.g3a /absolute/path/khicas50.ac2
usb detach
run 3
```

Inspect the actual connection menu before choosing F1. `usb install` reads the
virtual disk, modifies its FAT partition with mtools, writes changed sectors
through firmware SCSI WRITE(10), and reads back each written sector. Original
package names must match their add-in packaging. A new-file transfer, unlike
an unchanged-file timestamp update, was accepted by the tested firmware's
normal examination-mode exit workflow; follow its restart/EXIT prompt.

`usb state` and `usb token COMMAND` also work while paused for GDB. Other USB
commands run the guest for the transfer and pause afterward. Raw tokens include
`setup HEX8`, `in ENDPOINT` and `out ENDPOINT HEX`. Use `CG50_TRACE_USB=1`
for bounded controller write/FIFO traces in `cpu.log`. `usb reset` resets the
USB bus, not the CPU. Export flash with `dump` before quitting to retain files.

For menu navigation, short presses such as `key DOWN 0.06 0.5` avoid repeat.
Longer default presses can help expression entry, but always check the actual
input shown by the guest before judging a calculation.

## Validation

### Independent USB protocol add-in

Build the small polling driver with the existing CG50 SDK/toolchain directory:

```sh
python3 tools/build_usb_poc.py --tools-dir /absolute/path/to/toolchain
```

The output is `.build/usb-poc/UsbPoc.g3a`, with ELF, linker map and input/output
hashes alongside it. Install it through the native USB storage procedure above,
detach USB, acknowledge the OS completion prompt, and launch **USB PoC** from
MAIN MENU. Then use:

```text
usb attach
usb enumerate vendor
usb reply
screen /tmp/usb-reply.ppm
key EXE
usb reply
usb detach
key EXIT
```

The calculator queues `send1\n` after configuration. The host's `usb reply`
reads that request and sends `recv1\n`; the guest displays `Reply OK; EXE: next`.
EXE advances the sequence only after a valid reply. USB IN transactions are
still polled by the host; the calculator initiates the application message.
`usb receive 64` and `usb send HEX` expose raw bulk payloads for debugging.
The guest handles EP0 descriptors and configuration, writes requests to the
controller's IN FIFO, and reads replies from its OUT FIFO. The QEMU controller
does not generate protocol responses for either side.

This VM-only prototype advertises vendor class FF, test VID/PID FFFF:FFFF,
and two 64-byte bulk endpoints. It is independent of KhiCAS and does not alter
its application entry or settings. Physical USB timing, electrical D+/D−
behavior, assigned VID/PID and complete USB conformance are not validated.
The polling driver assumes that OS USB storage is idle when launched; detach
the virtual host before EXIT. It restores the saved clock/interrupt settings,
but does not preserve an in-progress OS USB transfer.

Export the installed flash using `dump 0 0x2000000 /tmp/usb-flash.bin`.
Run native-firmware acceptance against that image:

```sh
python3 tools/vm/tests/check-usb-protocol.py \
  --rom /tmp/usb-flash.bin --run-dir /tmp/usb-check --launch-keys UP
```

The runner uses the tested OS 03.80 setup sequence and starts menu navigation
at Run-Matrix. Supply navigation keys matching the installed icon arrangement;
`UP` selects USB PoC when it is the only icon in the last row. It checks actual
calculator-first payloads, EXE sequencing, rejection of incorrect/embedded-NUL/
64-byte replies, reconnection, and restoration of native OS storage enumeration.
Results, transcript and guest screenshots stay in the run directory. The
separate native USB install/readback precedes this runner.

### Device models

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
execution and SH7305 identification registers. It is not a complete SH7305
implementation. Memory includes 32 MiB NOR, 8 MiB main RAM, local/on-chip RAM,
and peripheral windows. Implemented paths include NOR programming/erase, BCD,
KEYSC masks and polling/interrupt scans, USBHS control and bulk FIFOs, VBUS
sensing, and a BCD calendar RTC with divider, periodic/carry/alarm flags.

Local acceptance additionally covered native USB enumeration (07cf:6103), a
16,852,480-byte disk read, G3A/AC2 installation with 18,011 changed-sector
readbacks, ordinary KhiCAS launch, exact fractions, integration, file save,
exit/reentry, and session restoration from exported flash. Native UK exam entry,
ordinary add-in restriction, power-off/on persistence and USB new-file exit
were exercised. These are bounded scenario checks, not a full CAS regression.
Package and model hashes and separate reset results are in
[verification.json](../../docs/bench/verification.json).

Clock, power, ADC conversion timing and interrupt priorities remain approximate.
The LCD surface reads guest VRAM selected by LCD DMA setup: full LCD GRAM,
controller-drawn examination borders and correct powered-off display retention
are not implemented. General DMA, USB DMA and complete peripheral snapshot/
reset serialization are not implemented. `system_reset` resets the CPU while
retaining RAM, flash, RTC and device state; it is not a calibrated physical
reset model. A new process reloads flash but does not restore a whole VM snapshot.

Storage image/install operations use 4 KiB read batches. A 16 KiB READ returned
all data but an inconsistent SCSI residue in testing; it remains unresolved and
the host rejects it rather than ignoring the status. Hardware timing, performance,
and other device-sensitive behavior still require calculator comparison. This
VM does not establish that every future CAS test can replace a physical test.

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

- SH7305 IDs, matrix coordinates, RTC and USBHS register definitions were
  cross-checked with [gint](https://git.planet-casio.com/Lephenixnoir/gint)
  commit `badbd0fd2bd8ac796fd55d49b93691741bd8a139`.
