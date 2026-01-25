# ESP-IDF Migration Prompts - PlatformIO to Native ESP-IDF

This document contains step-by-step prompts to migrate the esp32-shell project from
PlatformIO to native Espressif-IDF (ESP-IDF). The migration ensures compatibility
with future ESP-IDF projects and provides full control over the build system.

## Overview

### Current State (PlatformIO)
The esp32-shell project currently uses PlatformIO with ESP-IDF framework:
- platformio.ini defines build configuration
- PlatformIO manages ESP-IDF installation (~/.platformio/packages/framework-espidf)
- Build commands: `pio run`, `pio run -t upload`, `pio device monitor`
- ESP-IDF version: 5.5.0 (managed by PlatformIO)

### Target State (Native ESP-IDF)
After migration, the project will use native ESP-IDF toolchain:
- CMakeLists.txt and idf.py for build configuration
- User-managed ESP-IDF installation (e.g., ~/esp/esp-idf)
- Build commands: `idf.py build`, `idf.py flash`, `idf.py monitor`
- ESP-IDF version: 5.3.x or 5.4.x (user-installed stable release)

### Key Differences

| Aspect              | PlatformIO                      | Native ESP-IDF              |
|---------------------|---------------------------------|-----------------------------|
| Build command       | pio run                         | idf.py build                |
| Flash command       | pio run -t upload               | idf.py flash                |
| Monitor command     | pio device monitor -b 115200    | idf.py monitor              |
| Combined command    | pio run -t upload -t monitor    | idf.py flash monitor        |
| Configuration       | platformio.ini + sdkconfig      | sdkconfig only              |
| IDF Path            | Managed by PlatformIO           | $IDF_PATH environment var   |
| Project structure   | src/ directory                  | main/ component directory   |
| Component location  | src/ (flat)                     | main/ or components/        |

### Files to Create/Modify

**Create:**
- main/CMakeLists.txt (component registration)
- main/idf_component.yml (optional, for component manager)

**Modify:**
- CMakeLists.txt (update for standalone ESP-IDF)
- Move src/*.c to main/*.c
- Update src/CMakeLists.txt -> main/CMakeLists.txt

**Remove (after migration verified):**
- platformio.ini
- .pio/ directory
- flash.sh (replace with idf.py commands)

**Keep unchanged:**
- sdkconfig.defaults
- partitions.csv
- All source files (.c, .h)

---

## Phase 1: Environment Setup

### Prompt 1.1: Install ESP-IDF

#### Prompt
```
Set up native ESP-IDF development environment on Linux:

1. Choose installation location:
   mkdir -p ~/esp
   cd ~/esp

2. Clone ESP-IDF (stable release):
   git clone -b v5.3.2 --recursive https://github.com/espressif/esp-idf.git
   # Or use latest stable: git clone --recursive https://github.com/espressif/esp-idf.git

3. Install ESP-IDF tools:
   cd ~/esp/esp-idf
   ./install.sh esp32s3
   # This installs toolchain, CMake, Python packages

4. Set up environment variables:
   # Add to ~/.bashrc or ~/.zshrc:
   alias get_idf='. $HOME/esp/esp-idf/export.sh'
   
   # Then run to activate:
   get_idf

5. Verify installation:
   idf.py --version
   # Expected: ESP-IDF v5.3.x or similar
```

#### Manual Tests
```bash
# Verify IDF_PATH is set
echo $IDF_PATH
# Expected: /home/<user>/esp/esp-idf

# Verify idf.py is available
which idf.py
# Expected: /home/<user>/esp/esp-idf/tools/idf.py

# Verify toolchain
xtensa-esp32s3-elf-gcc --version
# Expected: Shows GCC version for Xtensa ESP32-S3

# Test with hello_world example
cd ~/esp/esp-idf/examples/get-started/hello_world
idf.py set-target esp32s3
idf.py build
# Expected: Builds successfully
```

---

### Prompt 1.2: Document ESP-IDF Workflow

#### Prompt
```
Create a quick reference for ESP-IDF commands (for developer convenience):

1. Environment activation:
   # Must run in each new terminal before using idf.py
   . $HOME/esp/esp-idf/export.sh
   # Or use the alias: get_idf

2. Common build commands:
   idf.py set-target esp32s3    # Set target chip (once per project)
   idf.py menuconfig            # Configure project options
   idf.py build                 # Build project
   idf.py flash                 # Flash to device
   idf.py monitor               # Open serial monitor (Ctrl+] to exit)
   idf.py flash monitor         # Flash and monitor combined

3. Cleaning:
   idf.py fullclean             # Remove all build artifacts
   rm -rf build/                # Alternative clean
   rm sdkconfig                 # Reset to defaults

4. Useful options:
   idf.py -p /dev/ttyACM0 flash # Specify port
   idf.py -b 921600 flash       # Faster flash baud rate
   idf.py size                  # Show binary size breakdown
   idf.py size-components       # Show size by component

5. Monitor exit:
   Ctrl+]                       # Exit monitor
   Ctrl+T Ctrl+H                # Show monitor help
```

#### Manual Tests
```bash
# No tests - documentation only
# Verify commands work with hello_world example if desired
```

---

## Phase 2: Project Structure Migration

### Prompt 2.1: Reorganize Directory Structure

#### Prompt
```
Reorganize the esp32-shell project from PlatformIO structure to ESP-IDF structure:

1. Current PlatformIO structure:
   esp32-shell/
   |-- platformio.ini          # PlatformIO config (to be removed)
   |-- CMakeLists.txt          # Project-level CMake
   |-- partitions.csv          # Partition table (keep)
   |-- sdkconfig.defaults      # SDK defaults (keep)
   |-- src/                    # Source directory
       |-- CMakeLists.txt      # Component CMake
       |-- main.c
       |-- esp_shell.c
       |-- esp_shell.h
       |-- ... (other files)

2. Target ESP-IDF structure:
   esp32-shell/
   |-- CMakeLists.txt          # Project-level CMake (updated)
   |-- partitions.csv          # Partition table (unchanged)
   |-- sdkconfig.defaults      # SDK defaults (unchanged)
   |-- main/                   # Main component (renamed from src/)
       |-- CMakeLists.txt      # Component CMake (updated)
       |-- main.c
       |-- esp_shell.c
       |-- esp_shell.h
       |-- ... (all source files)
   |-- README.md               # Documentation (updated)

3. Steps to reorganize:
   cd esp32-shell
   
   # Rename src/ to main/ (ESP-IDF convention)
   mv src main
   
   # Backup and remove PlatformIO files
   mkdir -p backup_pio
   mv platformio.ini backup_pio/
   mv .pio backup_pio/ 2>/dev/null || true
   mv flash.sh backup_pio/

4. The main/ directory is a special ESP-IDF component that is
   automatically included. No need for a components/ directory
   unless adding external components.
```

#### Manual Tests
```bash
# Verify new structure
ls -la esp32-shell/
# Expected: CMakeLists.txt, main/, partitions.csv, sdkconfig.defaults, etc.
# NOT expected: platformio.ini, src/

ls esp32-shell/main/
# Expected: CMakeLists.txt, main.c, esp_shell.c, etc.

# Verify src/ no longer exists
ls esp32-shell/src/
# Expected: No such file or directory
```

---

### Prompt 2.2: Update Project-Level CMakeLists.txt

#### Prompt
```
Update the project-level CMakeLists.txt for native ESP-IDF:

Current file (esp32-shell/CMakeLists.txt):
```cmake
# ESP32 Shell - Main CMakeLists.txt
cmake_minimum_required(VERSION 3.16)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(esp32_shell VERSION 1.0.0)
```

This file is already correct for ESP-IDF! Just verify:

1. The cmake_minimum_required version matches ESP-IDF requirements (3.16+)
2. The include() points to ESP-IDF cmake (uses $ENV{IDF_PATH})
3. The project() call defines the project name

No changes needed if the above conditions are met.

Optional enhancements:
```cmake
# ESP32 Shell - Main CMakeLists.txt
# ============================================================================
# Native ESP-IDF project configuration
# Build with: idf.py build
# Flash with: idf.py flash
# Monitor with: idf.py monitor
# ============================================================================

cmake_minimum_required(VERSION 3.16)

# Include ESP-IDF build system
include($ENV{IDF_PATH}/tools/cmake/project.cmake)

# Project definition
project(esp32_shell VERSION 1.0.0)
```

#### Manual Tests
```bash
# Verify CMakeLists.txt content
cat esp32-shell/CMakeLists.txt
# Expected: Contains cmake_minimum_required, include($ENV{IDF_PATH}...), project()
```

---

### Prompt 2.3: Update Component CMakeLists.txt

#### Prompt
```
Update the component CMakeLists.txt (main/CMakeLists.txt) for native ESP-IDF:

1. Rename src/CMakeLists.txt references if the directory was renamed:
   # The file is now at main/CMakeLists.txt

2. Current content should work, but verify it follows ESP-IDF conventions:

```cmake
# ESP32 Shell - Main Component CMakeLists.txt
# ============================================================================
# This CMakeLists.txt registers the main component with ESP-IDF.
# All source files are compiled as part of this component.
# ============================================================================

idf_component_register(
    SRCS 
        "main.c"
        "esp_shell.c"
        "platform_esp32.c"
        "vfs_esp32.c"
        "executor_esp32.c"
        "terminal_esp32.c"
        "parser_esp32.c"
    INCLUDE_DIRS 
        "."
    REQUIRES 
        driver
        esp_timer
        spiffs
        vfs
)
```

3. Notes on idf_component_register():
   - SRCS: List all .c source files
   - INCLUDE_DIRS: Directories with .h headers (. = current dir)
   - REQUIRES: ESP-IDF components this depends on
   - PRIV_REQUIRES: Private dependencies (not exposed to dependents)

4. If any source files are missing, add them to SRCS list
```

#### Manual Tests
```bash
# List source files and compare with CMakeLists.txt SRCS
ls esp32-shell/main/*.c
# Compare output with SRCS list in CMakeLists.txt

# Verify all files in SRCS exist
cat esp32-shell/main/CMakeLists.txt
```

---

## Phase 3: Build System Migration

### Prompt 3.1: Set Target and Initial Build

#### Prompt
```
Configure the project for ESP32-S3 and perform initial build:

1. Navigate to project directory:
   cd esp32-shell

2. Activate ESP-IDF environment:
   . $HOME/esp/esp-idf/export.sh
   # Or: get_idf

3. Set target chip (creates sdkconfig from defaults):
   idf.py set-target esp32s3
   # This creates:
   # - build/ directory
   # - sdkconfig file (from sdkconfig.defaults)

4. Build the project:
   idf.py build
   # Expected output ends with:
   # Project build complete. To flash, run:
   # idf.py flash

5. If build fails, check:
   - All source files listed in main/CMakeLists.txt SRCS
   - All required components in REQUIRES
   - No PlatformIO-specific includes remaining

6. Check binary size:
   idf.py size
   idf.py size-components
```

#### Manual Tests
```bash
# Verify build output
ls esp32-shell/build/
# Expected: Various build directories and files

ls esp32-shell/build/*.bin
# Expected: esp32_shell.bin, bootloader.bin, partition_table.bin, etc.

# Verify sdkconfig was created
ls esp32-shell/sdkconfig
# Expected: File exists

# Check size
cd esp32-shell && idf.py size
# Expected: Shows RAM and Flash usage similar to:
# Total sizes:
# Used static DRAM: xxx bytes
# Used static IRAM: xxx bytes
# Flash code: xxx bytes
# Flash rodata: xxx bytes
```

---

### Prompt 3.2: Handle Build Errors

#### Prompt
```
Common build errors and how to fix them when migrating from PlatformIO:

1. "Component not found" errors:
   - Check REQUIRES list in main/CMakeLists.txt
   - Add missing components (e.g., esp_timer, driver, spiffs, vfs)
   
2. "Header not found" errors:
   - Check INCLUDE_DIRS in main/CMakeLists.txt
   - Verify header files exist in main/ directory
   - For ESP-IDF headers, add component to REQUIRES

3. "Undefined reference" errors:
   - Source file missing from SRCS list
   - Add the .c file to SRCS in main/CMakeLists.txt

4. "Multiple definition" errors:
   - Same file included twice in SRCS
   - Check for duplicate source files

5. PlatformIO-specific code:
   - Remove any PIO_FRAMEWORK_ESP_IDF checks
   - Replace with standard ESP_PLATFORM checks
   
6. Different header paths:
   PlatformIO:  #include <Arduino.h>  (not used in this project)
   ESP-IDF:     #include "freertos/FreeRTOS.h"
   
   Our project already uses ESP-IDF style includes, so no changes needed.

7. Configuration differences:
   - PlatformIO: platformio.ini [env] settings
   - ESP-IDF: sdkconfig / sdkconfig.defaults / menuconfig
   - Our sdkconfig.defaults should work unchanged
```

#### Manual Tests
```bash
# If build fails, capture the error
cd esp32-shell
idf.py build 2>&1 | tee build_log.txt

# Search for specific errors
grep -i "error:" build_log.txt
grep -i "undefined reference" build_log.txt
grep -i "no such file" build_log.txt
```

---

### Prompt 3.3: Flash and Test

#### Prompt
```
Flash the firmware and verify it works:

1. Connect ESP32-S3 board via USB

2. Identify the serial port:
   ls /dev/ttyACM* /dev/ttyUSB*
   # ESP32-S3-DevKitC-1 typically appears as /dev/ttyACM0

3. Flash the firmware:
   idf.py -p /dev/ttyACM0 flash
   # Or let idf.py auto-detect:
   idf.py flash

4. Open serial monitor:
   idf.py -p /dev/ttyACM0 monitor
   # Exit with Ctrl+]

5. Combined flash and monitor:
   idf.py -p /dev/ttyACM0 flash monitor

6. Test the shell:
   - Press Enter to see prompt
   - Type: help
   - Type: info
   - Type: ls
   - Type: free
```

#### Manual Tests
```bash
# In serial monitor, verify shell prompt appears:
esp32> 

# Test commands:
esp32> help
# Expected: List of available commands

esp32> info
# Expected: Chip info, IDF version, memory info

esp32> ls
# Expected: List /spiffs directory

esp32> free
# Expected: Free heap memory

esp32> fsinfo
# Expected: SPIFFS filesystem info

# Test file operations:
esp32> echo hello >test.txt
# or
esp32> cat >test.txt
# (type text, empty line to end)

esp32> cat test.txt
# Expected: Shows file contents
```

---

## Phase 4: Cleanup and Documentation

### Prompt 4.1: Remove PlatformIO Files

#### Prompt
```
After verifying native ESP-IDF build works, remove PlatformIO files:

1. Remove PlatformIO configuration:
   cd esp32-shell
   rm -f platformio.ini

2. Remove PlatformIO build directory:
   rm -rf .pio/

3. Remove old flash script (replaced by idf.py):
   rm -f flash.sh

4. Remove any sdkconfig files generated by PlatformIO:
   rm -f sdkconfig.esp32s3dev
   # Keep sdkconfig.defaults

5. Remove backup directory if migration successful:
   rm -rf backup_pio/

6. Clean git history (optional):
   # Add to .gitignore:
   echo "build/" >> .gitignore
   echo "sdkconfig" >> .gitignore
   echo "sdkconfig.old" >> .gitignore
   
   # The generated sdkconfig should not be committed
   # Only sdkconfig.defaults should be in version control
```

#### Manual Tests
```bash
# Verify PlatformIO files removed
ls esp32-shell/platformio.ini
# Expected: No such file

ls -la esp32-shell/.pio/
# Expected: No such directory

# Verify project still builds
cd esp32-shell
idf.py build
# Expected: BUILD SUCCESSFUL
```

---

### Prompt 4.2: Update README

#### Prompt
```
Update esp32-shell/README.md for native ESP-IDF workflow:

1. Update Quick Start section:

   Replace:
   ### 2. Build the Project
   ```bash
   cd esp32-shell
   ~/.platformio/penv/bin/pio run
   ~/.platformio/penv/bin/pio run -t upload
   ```

   With:
   ### 2. Install ESP-IDF
   ```bash
   # Clone ESP-IDF (one-time setup)
   mkdir -p ~/esp && cd ~/esp
   git clone -b v5.3.2 --recursive https://github.com/espressif/esp-idf.git
   cd esp-idf && ./install.sh esp32s3
   ```

   ### 3. Build the Project
   ```bash
   # Activate ESP-IDF environment (each terminal session)
   . ~/esp/esp-idf/export.sh
   
   cd esp32-shell
   
   # Set target (first time only)
   idf.py set-target esp32s3
   
   # Build
   idf.py build
   
   # Flash to device
   idf.py flash
   
   # Or build, flash, and monitor in one command
   idf.py flash monitor
   ```

2. Update Serial Monitor section:

   Replace PlatformIO monitor commands with:
   ```bash
   # Open serial monitor
   idf.py monitor
   
   # Exit monitor: Ctrl+]
   ```

3. Update Troubleshooting section:
   - Remove PlatformIO-specific troubleshooting
   - Add ESP-IDF troubleshooting:
     - "IDF_PATH not set" -> Run export.sh
     - "Permission denied /dev/ttyACM0" -> Add user to dialout group

4. Update Software Requirements:
   - Remove: PlatformIO
   - Add: ESP-IDF v5.3.x or later
   - Add: CMake 3.16+
   - Add: Python 3.8+
```

#### Manual Tests
```bash
# Review updated README
cat esp32-shell/README.md

# Verify commands in README work
cd esp32-shell
. ~/esp/esp-idf/export.sh
idf.py build
```

---

### Prompt 4.3: Update flash.sh Replacement

#### Prompt
```
Create a convenience script for common ESP-IDF operations (optional):

1. Create esp32-shell/build.sh:
```bash
#!/bin/bash
# ESP32 Shell - Build Helper Script
# ============================================================================
# Convenience wrapper for common idf.py commands
# Usage: ./build.sh [build|flash|monitor|all|clean|menuconfig|size]
# ============================================================================

# Activate ESP-IDF if not already active
if [ -z "$IDF_PATH" ]; then
    if [ -f "$HOME/esp/esp-idf/export.sh" ]; then
        echo "Activating ESP-IDF environment..."
        . "$HOME/esp/esp-idf/export.sh"
    else
        echo "Error: ESP-IDF not found. Please install ESP-IDF first."
        echo "See README.md for installation instructions."
        exit 1
    fi
fi

# Default action
ACTION="${1:-build}"

case "$ACTION" in
    build)
        echo "Building project..."
        idf.py build
        ;;
    flash)
        echo "Flashing to device..."
        idf.py flash
        ;;
    monitor)
        echo "Opening serial monitor (Ctrl+] to exit)..."
        idf.py monitor
        ;;
    all|fm)
        echo "Flashing and monitoring..."
        idf.py flash monitor
        ;;
    clean)
        echo "Cleaning build..."
        idf.py fullclean
        ;;
    menuconfig)
        echo "Opening configuration menu..."
        idf.py menuconfig
        ;;
    size)
        echo "Showing size info..."
        idf.py size
        idf.py size-components
        ;;
    help|--help|-h)
        echo "Usage: ./build.sh [command]"
        echo ""
        echo "Commands:"
        echo "  build      - Build the project (default)"
        echo "  flash      - Flash to device"
        echo "  monitor    - Open serial monitor"
        echo "  all, fm    - Flash and monitor"
        echo "  clean      - Clean build directory"
        echo "  menuconfig - Open ESP-IDF configuration"
        echo "  size       - Show binary size breakdown"
        ;;
    *)
        echo "Unknown command: $ACTION"
        echo "Run './build.sh help' for usage"
        exit 1
        ;;
esac
```

2. Make executable:
   chmod +x esp32-shell/build.sh

3. Usage:
   ./build.sh           # Build only
   ./build.sh flash     # Flash to device
   ./build.sh all       # Flash and monitor
   ./build.sh clean     # Clean build
```

#### Manual Tests
```bash
# Test the script
cd esp32-shell
./build.sh build
./build.sh size
./build.sh help
```

---

### Prompt 4.4: Update Project Documentation

#### Prompt
```
Update all documentation to reflect ESP-IDF workflow:

1. Update esp32-shell/docs/USER_GUIDE.md:
   - Change build instructions from PlatformIO to ESP-IDF
   - Update any pio commands to idf.py equivalents

2. Update esp32-shell/docs/DEVELOPER_GUIDE.md:
   - Update build system documentation
   - Update component structure explanation
   - Update adding new files instructions

3. Update root README.md (unified-shell project):
   - Update esp32-shell build command:
     From: cd esp32-shell && ./flash.sh upload monitor
     To:   cd esp32-shell && idf.py flash monitor
   
4. Update AI_Interaction.md:
   - Document the migration from PlatformIO to ESP-IDF
   - Note any issues encountered and solutions

5. Create/update .gitignore for ESP-IDF:
   ```
   # ESP-IDF build output
   build/
   sdkconfig
   sdkconfig.old
   
   # Dependencies (managed by idf.py)
   managed_components/
   dependencies.lock
   ```
```

#### Manual Tests
```bash
# Verify documentation is consistent
grep -r "pio run" esp32-shell/
# Expected: No matches (all replaced with idf.py)

grep -r "platformio" esp32-shell/
# Expected: No matches
```

---

## Phase 5: Verification and Testing

### Prompt 5.1: Full Build and Flash Test

#### Prompt
```
Perform complete verification of the migrated project:

1. Clean everything and rebuild from scratch:
   cd esp32-shell
   rm -rf build/ sdkconfig
   
   # Activate ESP-IDF
   . ~/esp/esp-idf/export.sh
   
   # Set target
   idf.py set-target esp32s3
   
   # Build
   idf.py build
   
   # Check size
   idf.py size

2. Flash and test all shell commands:
   idf.py flash monitor

3. Test each command category:
   
   Shell control:
   - help
   - info
   - reboot (then reconnect)
   
   System info:
   - free
   - uptime
   - fsinfo
   
   File operations:
   - ls
   - pwd
   - cd /spiffs
   - touch test.txt
   - cat >test.txt (write some text)
   - cat test.txt
   - rm test.txt
   
   GPIO:
   - gpio mode 2 out
   - gpio write 2 1
   - gpio read 2
   - gpio write 2 0
   
   Environment:
   - set VAR=hello
   - env
   - echo $VAR
   - unset VAR

4. Verify no regressions from PlatformIO build
```

#### Manual Tests
```bash
# All tests from the prompt above should pass
# Document any failures for debugging
```

---

### Prompt 5.2: Compare Binary Sizes

#### Prompt
```
Compare binary sizes between PlatformIO and ESP-IDF builds:

1. If you kept the PlatformIO backup:
   # PlatformIO size (from previous builds)
   # RAM:   8.3% (27256 bytes)
   # Flash: 16.9% (265947 bytes)

2. ESP-IDF size:
   cd esp32-shell
   idf.py size
   
   # Note the values:
   # Used static DRAM: _____ bytes
   # Used Flash: _____ bytes

3. Size differences are expected:
   - ESP-IDF may produce slightly different sizes
   - Optimization levels may differ
   - Component inclusion may vary

4. If size increased significantly (>20%), investigate:
   - Check menuconfig for debugging options
   - Verify CONFIG_COMPILER_OPTIMIZATION_SIZE=y
   - Check for additional components being pulled in
```

#### Manual Tests
```bash
# Get detailed size breakdown
idf.py size-components

# Compare with expected sizes
# Any significant deviation should be investigated
```

---

### Prompt 5.3: Document Migration Results

#### Prompt
```
Document the migration in AI_Interaction.md:

Add entry:
## PlatformIO to ESP-IDF Migration (Date: YYYY-MM-DD)
=====================================================

### Migration Summary
- Migrated esp32-shell from PlatformIO to native ESP-IDF
- ESP-IDF version: 5.3.x
- All shell functionality preserved

### Changes Made
1. Directory structure:
   - Renamed src/ to main/ (ESP-IDF convention)
   - Removed platformio.ini
   - Removed .pio/ build directory
   - Removed flash.sh (replaced by idf.py commands)

2. Build system:
   - Using idf.py instead of pio
   - CMakeLists.txt unchanged (already ESP-IDF compatible)
   - sdkconfig.defaults unchanged

3. Documentation updated:
   - README.md: New build instructions
   - USER_GUIDE.md: Updated for idf.py
   - DEVELOPER_GUIDE.md: Updated build info

### Build Results
- RAM usage: XX.X% (XXXXX bytes)
- Flash usage: XX.X% (XXXXX bytes)

### Benefits of Migration
- Direct control over ESP-IDF configuration
- Access to menuconfig for all options
- Consistent with other ESP-IDF projects
- Easier debugging with ESP-IDF tools
- No PlatformIO dependency
```

#### Manual Tests
```bash
# No tests - documentation only
# Verify AI_Interaction.md was updated
tail -50 AI_Interaction.md
```

---

## Quick Reference: PlatformIO vs ESP-IDF Commands

| Task                    | PlatformIO                              | ESP-IDF                           |
|-------------------------|-----------------------------------------|-----------------------------------|
| Activate environment    | (automatic)                             | . ~/esp/esp-idf/export.sh         |
| Set target              | (in platformio.ini)                     | idf.py set-target esp32s3         |
| Configure               | (platformio.ini + sdkconfig)            | idf.py menuconfig                 |
| Build                   | pio run                                 | idf.py build                      |
| Flash                   | pio run -t upload                       | idf.py flash                      |
| Monitor                 | pio device monitor -b 115200            | idf.py monitor                    |
| Flash + Monitor         | pio run -t upload -t monitor            | idf.py flash monitor              |
| Clean                   | pio run -t clean                        | idf.py fullclean                  |
| Size info               | (in build output)                       | idf.py size                       |
| Port selection          | upload_port in platformio.ini           | idf.py -p /dev/ttyACM0            |

---

## Troubleshooting

### ESP-IDF Environment Not Found
```bash
# Error: idf.py: command not found
# Solution: Activate ESP-IDF environment
. ~/esp/esp-idf/export.sh
```

### Permission Denied on Serial Port
```bash
# Error: Permission denied: '/dev/ttyACM0'
# Solution: Add user to dialout group
sudo usermod -a -G dialout $USER
# Log out and back in for changes to take effect
```

### Build Fails After Migration
```bash
# Clean and rebuild
rm -rf build/ sdkconfig
idf.py set-target esp32s3
idf.py build

# Check for missing components
# Add to REQUIRES in main/CMakeLists.txt
```

### Wrong Target Chip
```bash
# Error: Chip is ESP32-S3, not ESP32
# Solution: Set correct target
rm -rf build/ sdkconfig
idf.py set-target esp32s3
idf.py build
```
