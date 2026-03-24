# `platform/esp/boards`

Shared ESP board runtime glue, dispatcher support, and reusable ESP-facing code.

Current responsibilities:

- ESP-specific runtime dispatch under `include/platform/esp/boards/*` and `src/board_runtime.cpp`
- ESP display abstractions and panel drivers under `include/display/*` and `src/display/*`
- board-local input drivers such as rotary handling under `include/input/*` and `src/input/*`
- any remaining ESP-only shared board implementations under `src/board/*`

Boundary note:

- code that directly owns a specific board's peripherals, display buses, PMU wiring, LoRa wiring, GPS wiring, touch/keyboard/NFC, display panel setup, or board singleton instances belongs under `boards/<name>/*`
- platform-neutral board contracts now live under `platform/shared/include/board/*`
- code that is intentionally shared across multiple ESP boards but still platform-specific belongs here
