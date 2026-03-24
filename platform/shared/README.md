# `platform/shared`

Platform-neutral contracts and utilities shared across multiple runtime families.

Current responsibilities:

- shared board-facing contracts under `include/board/*`
- shared lightweight board utilities that are not tied to a single platform runtime

Boundary note:

- code under this package must not depend on `platform/esp/*` or `platform/nrf52/*`
- platform-specific runtime dispatch, display glue, and board selection stay under their platform roots
- concrete board implementations stay under `boards/<name>/*`
