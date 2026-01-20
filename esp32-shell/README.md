# ESP32-S3 Shell
# ============================================================================
# Interactive command-line shell for ESP32-S3 microcontrollers.
# Provides a Unix-like interface over USB serial console.
# ============================================================================

## Hardware Requirements

### Tested Board
- **Board**: ESP32-S3-DevKitC-1
- **Chip**: ESP32-S3 (QFN56, revision v0.2)
- **Features**: WiFi, BLE, Embedded PSRAM 8MB (AP_3v3)
- **Crystal**: 40MHz
- **Flash**: 8MB

### Connections
- USB-C or Micro-USB cable (board dependent)
- The board appears as `/dev/ttyACM0` on Linux

## Software Requirements

- **PlatformIO**: Version 6.x or later (via VS Code extension or CLI)
- **ESP-IDF**: 5.5.0 (installed automatically by PlatformIO)
- **Python**: 3.8+ (for PlatformIO)

## Quick Start

### 1. Install PlatformIO
```bash
# Install PlatformIO CLI (if not using VS Code extension)
pip install platformio

# Or use the version in the PlatformIO virtual environment
~/.platformio/penv/bin/pio --version
```

### 2. Build the Project
```bash
cd esp32-shell

# Build only
~/.platformio/penv/bin/pio run

# Build and upload to connected ESP32-S3
~/.platformio/penv/bin/pio run -t upload
```

### 3. Connect to Serial Console
```bash
# Open serial monitor at 115200 baud
~/.platformio/penv/bin/pio device monitor -b 115200

# Alternative: Use any serial terminal
minicom -D /dev/ttyACM0 -b 115200
screen /dev/ttyACM0 115200
picocom -b 115200 /dev/ttyACM0
```

### 4. Exit Serial Monitor
- **PlatformIO monitor**: Press `Ctrl+]` or `Ctrl+C`
- **minicom**: Press `Ctrl+A` then `X`
- **screen**: Press `Ctrl+A` then `\`
- **picocom**: Press `Ctrl+A` then `Ctrl+X`

## Project Structure

```
esp32-shell/
|-- platformio.ini          # PlatformIO configuration (esp32s3dev)
|-- CMakeLists.txt          # ESP-IDF project CMake
|-- partitions.csv          # Custom partition table with SPIFFS
|-- sdkconfig.defaults      # Default ESP-IDF settings
|-- src/
    |-- main.c              # Application entry point (app_main)
    |-- esp_shell.c         # Shell implementation
    |-- esp_shell.h         # Shell interface header
    |-- platform.h          # Platform abstraction header
    |-- platform_esp32.c    # ESP32 platform implementation
    |-- platform_linux.c    # Linux platform (for testing)
    |-- CMakeLists.txt      # Component build config
```

## Available Commands

### Shell Control
- `help` - Show available commands
- `exit` - Reboot the ESP32 (no OS to exit to)
- `reboot` - Reboot the ESP32

### System Information
- `info` - Show chip info, memory, IDF version
- `free` - Show free heap memory
- `uptime` - Show time since boot

### File Operations (SPIFFS)
- `pwd` - Print working directory
- `cd <dir>` - Change directory
- `ls [dir]` - List directory contents
- `cat <file>` - Display file contents
- `touch <file>` - Create empty file
- `rm <file>` - Remove file
- `echo <text>` - Print text
- `fsinfo` - Show filesystem usage (total/used/free)
- `format --yes` - Format SPIFFS filesystem (WARNING: erases all files)

### Environment Variables
- `set VAR=value` - Set environment variable
- `unset VAR` - Remove environment variable
- `env` - List all environment variables
- `$VAR` or `${VAR}` - Variable expansion in commands

### Text Editor
- `edi [file]` - Vi-like modal text editor

**Editor Modes:**
- **NORMAL** - Navigate and edit (default mode)
- **INSERT** - Type text directly
- **COMMAND** - Execute commands (`:w`, `:q`, etc.)

**Navigation (Normal Mode):**
- `h/j/k/l` - Left/Down/Up/Right
- `0/$` - Beginning/End of line
- `g/G` - First/Last line
- Arrow keys also work

**Editing (Normal Mode):**
- `i` - Enter insert mode at cursor
- `a` - Enter insert mode after cursor
- `o/O` - New line below/above
- `x` - Delete character at cursor
- `dd` - Delete current line

**Commands (press `:` first):**
- `:w` - Save file
- `:q` - Quit (fails if modified)
- `:wq` - Save and quit
- `:q!` - Force quit without saving

**Other Keys:**
- `ESC` - Return to normal mode
- `Backspace` - Delete character (insert mode)
- `Enter` - New line (insert mode)

**Limitations (ESP32):**
- Maximum 50 lines per file
- Maximum 128 characters per line
- Screen size: 80x24
- Uses SPIFFS filesystem at /spiffs

### GPIO Control
- `gpio read <pin>` - Read GPIO pin state (0 or 1)
- `gpio write <pin> <0|1>` - Set GPIO pin output level
- `gpio mode <pin> <in|out>` - Configure GPIO direction

## Troubleshooting

### Wrong chip type error during upload
If you see "This chip is ESP32-S3, not ESP32. Wrong --chip argument?":
- Ensure `platformio.ini` has `board = esp32-s3-devkitc-1`
- Clean build: `rm -rf .pio/build sdkconfig*`
- Rebuild: `~/.platformio/penv/bin/pio run -t upload`

### UART driver errors
If you see "uart_read_bytes: uart driver error":
- The UART driver may not be installed properly
- Check `platform_esp32.c` calls `uart_driver_install()`

### Garbled serial output
- Ensure baud rate is 115200: `pio device monitor -b 115200`
- Press the EN/Reset button on the board to see boot messages

### USB device not detected
```bash
# Check if device is connected
dmesg | tail -20

# Install udev rules for PlatformIO
curl -fsSL https://raw.githubusercontent.com/platformio/platformio-core/develop/platformio/assets/system/99-platformio-udev.rules | sudo tee /etc/udev/rules.d/99-platformio-udev.rules
sudo udevadm control --reload-rules
sudo udevadm trigger
```

### Build fails with missing IDF_PATH
PlatformIO handles ESP-IDF automatically. If issues persist:
```bash
~/.platformio/penv/bin/pio platform install espressif32
```

## Memory Considerations

ESP32-S3 has ~320KB usable RAM. The shell is designed to be memory-efficient:
- Command line buffer: 256 bytes
- History: 10 entries max
- File operations use small buffers

Use `free` command to monitor heap usage.

## SPIFFS Filesystem

The partition table allocates 1MB for SPIFFS at `/spiffs`.
On first boot, SPIFFS is automatically formatted if needed.

To upload files to SPIFFS:
```bash
# Create a 'data' folder with files to upload
mkdir data
echo "Hello ESP32" > data/hello.txt

# Upload to SPIFFS
~/.platformio/penv/bin/pio run -t uploadfs
```

## Platform Abstraction

The platform abstraction layer (platform.h) allows the same code to compile for:
- ESP32 (using FreeRTOS and ESP-IDF)
- Linux (for desktop testing)

## License

MIT License
