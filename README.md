# XAVRX N64

A from-scratch Nintendo 64 emulator project targeting the New Nintendo 2DS XL.

## Project status

**Current status: early development.**

This repository currently contains an initial MIPS III/R4300i interpreter core, a big-endian RDRAM bus, host-side unit tests, a 3DS dual-screen interface, and an SD browser that reads and validates `.z64`, `.v64`, and `.n64` cartridge images. ROM execution, RSP/RDP graphics, audio, and a tested CIA release are not implemented yet.

## Build and test on a desktop

```sh
cmake -S . -B build -DXAVRX_BUILD_TESTS=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

## Build the 3DS homebrew executable

Install devkitPro with the 3DS development packages, then run:

```sh
make -f Makefile.3ds
```

This initial target creates a 3DSX homebrew executable for development. CIA packaging and installable release generation are scheduled for the integration/release phase; the requested final distribution format remains CIA only.

## Roadmap

1. Architecture and MIPS core
2. Nintendo 64 hardware
3. Game compatibility
4. 3DS interface and optimization
5. Integration and stable release

## Legal

XAVRX N64 is an independent emulator project. It does not include Nintendo firmware, copyrighted game ROMs, or proprietary BIOS files. Use only game data you are legally entitled to use.
