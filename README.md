# Unified Shell Project
============================================================================

This repository contains two related shell implementations: a full-featured
Unix shell for Linux, and a compact port for ESP32 microcontrollers.

## Project Structure

```
unified-shell/           <- This root directory
|
|-- unified-shell/       # Linux Shell (ushell)
|   |-- src/             # Source code
|   |-- include/         # Headers
|   |-- tests/           # Test scripts
|   |-- docs/            # Documentation
|   `-- Makefile         # Build system
|
|-- esp32-shell/         # ESP32 Shell Port
|   |-- src/             # Adapted source code
|   |-- docs/            # ESP32-specific docs
|   |-- platformio.ini   # PlatformIO config
|   `-- flash.sh         # Build/flash script
|
|-- Plan.md              # Original development plan
|-- AI_Interaction.md    # Development history/context
`-- ESP32_Prompts.md     # ESP32 porting guide
```

## The Two Shells

### unified-shell (Linux)
A full-featured Unix-like shell for Linux systems.

**Key Features:**
- Grammar-based parser (BNFC-generated)
- Fork/exec process model with pipelines
- Job control (background jobs, fg/bg, signals)
- Tab completion and command history
- Variable expansion and arithmetic evaluation
- 10 integrated file tools (ls, cat, cp, mv, rm, etc.)
- Vi-like edi text editor
- APT-like package manager
- Multi-threaded builtin execution

**Build:** `cd unified-shell && make && ./ushell`

### esp32-shell (ESP32-S3)
A compact shell port for ESP32 microcontrollers.

**Key Features:**
- Runs on ESP32-S3-DevKitC-1 (8MB Flash, 8MB PSRAM)
- USB serial console interface (115200 baud)
- SPIFFS filesystem for persistent storage
- GPIO control (read/write/mode)
- System monitoring (info, free, uptime, fsinfo)
- File creation via cat >file / cat >>file
- Environment variables with expansion
- Subset of unified-shell commands

**Build:** `cd esp32-shell && ./flash.sh upload monitor`

## Comparison Table

| Feature              | unified-shell (Linux) | esp32-shell (ESP32) |
|----------------------|-----------------------|---------------------|
| Parser               | BNFC grammar          | Simple tokenizer    |
| Process Model        | Fork/exec             | Single-threaded     |
| Pipelines            | Yes (|)               | No                  |
| I/O Redirection      | Yes (< > >>)          | No                  |
| Job Control          | Yes (bg/fg/jobs)      | No                  |
| Tab Completion       | Yes                   | No                  |
| Command History      | Yes (persistent)      | No                  |
| Glob Expansion       | Yes (*, ?, [])        | No                  |
| Arithmetic           | Yes ($((expr)))       | No                  |
| Conditionals         | Yes (if/then/fi)      | No                  |
| File Tools           | 10 tools              | 6 tools             |
| Text Editor (edi)    | Yes                   | No                  |
| File Writing (cat)   | Yes (echo/redirect)   | Yes (cat >file)     |
| GPIO Control         | No                    | Yes                 |
| Filesystem           | Linux VFS             | SPIFFS              |
| Memory               | Dynamic (malloc)      | Static allocation   |

## Documentation

- [unified-shell/README.md](unified-shell/README.md) - Linux shell documentation
- [unified-shell/docs/USER_GUIDE.md](unified-shell/docs/USER_GUIDE.md) - User guide
- [unified-shell/docs/DEVELOPER_GUIDE.md](unified-shell/docs/DEVELOPER_GUIDE.md) - Developer guide
- [esp32-shell/README.md](esp32-shell/README.md) - ESP32 shell documentation
- [esp32-shell/docs/USER_GUIDE.md](esp32-shell/docs/USER_GUIDE.md) - ESP32 user guide
- [esp32-shell/docs/DEVELOPER_GUIDE.md](esp32-shell/docs/DEVELOPER_GUIDE.md) - ESP32 developer guide

## Development Notes

- Plan.md contains the original plan to build the unified shell
- AI_Interaction.md documents the development history and context
- ESP32_Prompts.md contains the step-by-step ESP32 porting guide