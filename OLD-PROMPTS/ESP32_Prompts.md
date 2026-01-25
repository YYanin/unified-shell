# ESP32 Shell Port - Implementation Prompts

This document contains step-by-step prompts to port the Unified Shell (ushell) to run on an ESP32 microcontroller. The port involves three major goals:
1. Remove AI integration (pure C code, no Python dependencies)
2. Rename tool commands from my<command> to standard names
3. Adapt the shell for ESP32 embedded environment

## Overview

### Goal 1: Remove AI Integration
- Remove all Python-based AI helper code
- Remove OpenAI API key dependencies
- Keep heuristic command matching (pure C implementation)
- Remove @ prefix command handling that calls external scripts
- Clean up aiIntegr directory

### Goal 2: Rename Tool Commands
Current naming uses my<command> prefix. Change to standard Unix names:
- myls -> ls
- mycat -> cat
- mycp -> cp
- mymv -> mv
- myrm -> rm
- mymkdir -> mkdir
- myrmdir -> rmdir
- mytouch -> touch
- mystat -> stat
- myfd -> find

### Goal 3: ESP32 Adaptation
The ESP32 has significant constraints compared to a Linux system:
- Limited RAM (~320KB usable)
- No traditional filesystem (SPIFFS/LittleFS/FAT on flash)
- No fork/exec (single process, no OS)
- No traditional process signals
- Limited terminal capabilities
- FreeRTOS-based task system instead of POSIX threads
- Different I/O model (UART/USB serial)

Architecture changes needed:
- Replace fork/exec with direct function calls
- Replace POSIX threads with FreeRTOS tasks
- Implement virtual filesystem abstraction
- Adapt terminal I/O for serial communication
- Remove features not applicable to embedded (job control, pipes to external programs)
- Reduce memory footprint

---

## Phase 1: Remove AI Integration

### Prompt 1.1: Remove AI Helper Files and References

#### Prompt
```
Remove all AI integration from the shell, keeping it pure C with no external dependencies:

1. Delete the aiIntegr directory entirely:
   - aiIntegr/ushell_ai.py (Python AI helper)
   - aiIntegr/ushell_ai_venv.sh (Virtual environment setup)
   - aiIntegr/requirements.txt (Python dependencies)
   - aiIntegr/commands.json (Command catalog for AI)
   - aiIntegr/README.md (AI documentation)
   - aiIntegr/__pycache__/ (Python cache)

2. Remove AI-related environment variable handling from main.c:
   - OPENAI_API_KEY references
   - USHELL_LLM_MODEL references
   - USHELL_AI_HELPER references
   - USHELL_AI_DEBUG references
   - USHELL_AI_CONTEXT references

3. Remove the @ prefix command handler:
   - Find where @ commands are parsed and dispatched
   - Remove the code that calls external AI helper
   - Remove any pipe/fork code for AI subprocess

4. Update the help command:
   - Remove references to AI features
   - Remove @ prefix documentation

5. Clean up any remaining AI references:
   - Search for "ai", "AI", "openai", "llm" in all source files
   - Remove dead code and comments

6. Update documentation:
   - Remove AI Integration section from README.md
   - Remove AI references from USER_GUIDE.md
   - Remove AI references from DEVELOPER_GUIDE.md
```

#### Manual Tests
```bash
# Verify aiIntegr directory is removed
ls -la unified-shell/aiIntegr
# Expected: No such file or directory

# Verify no Python dependencies
grep -r "python" unified-shell/src/
# Expected: No matches

# Verify no AI environment variables
grep -r "OPENAI\|AI_HELPER\|LLM_MODEL" unified-shell/src/
# Expected: No matches

# Build and test
cd unified-shell
make clean && make
./ushell
# Type: @list files
# Expected: Error message or command not found (not AI call)

# Type: help
# Expected: No AI references in help output
```

---

### Prompt 1.2: Keep Heuristic Command Suggestions (Optional)

#### Prompt
```
If heuristic command suggestions exist in pure C, keep them. Otherwise skip this prompt.

1. Review any heuristic matching code:
   - Check for command similarity matching
   - Check for typo correction
   - Check for partial command completion

2. If heuristic code exists in C:
   - Keep it functional
   - Ensure no external dependencies
   - Test that suggestions work

3. If heuristic code requires Python:
   - Remove it entirely
   - Add a TODO comment for future C implementation

4. Document what was kept or removed in AI_Interaction.md
```

#### Manual Tests
```bash
./ushell
# Type: pdw (typo for pwd)
# Expected: Either "command not found" or "did you mean: pwd"

# Type: echoo hello (typo for echo)
# Expected: Either "command not found" or "did you mean: echo"
```

---

## Phase 2: Rename Tool Commands

### Prompt 2.1: Rename Source Files

#### Prompt
```
Rename all tool source files from my<command>.c to <command>.c:

1. Rename files in src/tools/:
   - myls.c -> ls.c
   - mycat.c -> cat.c
   - mycp.c -> cp.c
   - mymv.c -> mv.c
   - myrm.c -> rm.c
   - mymkdir.c -> mkdir.c
   - myrmdir.c -> rmdir.c
   - mytouch.c -> touch.c
   - mystat.c -> stat.c
   - myfd.c -> find.c

2. Update function names inside each file:
   - tool_myls() -> tool_ls()
   - tool_mycat() -> tool_cat()
   - etc.

3. Update header declarations in include/tools.h:
   - Change all function declarations to new names

4. Update tool_dispatch.c:
   - Change command name strings: "myls" -> "ls"
   - Change function references to new names
   - Update the tools[] array
```

#### Manual Tests
```bash
# Verify files renamed
ls unified-shell/src/tools/
# Expected: ls.c cat.c cp.c mv.c rm.c mkdir.c rmdir.c touch.c stat.c find.c tool_dispatch.c

# Verify compilation
make clean && make
# Expected: No errors

# Test renamed commands
./ushell
# Type: ls
# Expected: Lists directory (was myls)

# Type: cat README.md
# Expected: Displays file contents (was mycat)
```

---

### Prompt 2.2: Update All References

#### Prompt
```
Update all code references to use new command names:

1. Update builtins.c:
   - Change any references to my* commands
   - Update command dispatch table if present

2. Update help system (help.c):
   - Change help entries from "myls" to "ls"
   - Update usage examples
   - Update descriptions

3. Update completion.c:
   - Change command names in completion lists
   - Update any hardcoded command arrays

4. Update parser files if needed:
   - Check Lexer.c for hardcoded commands
   - Check any grammar files

5. Update documentation:
   - README.md: Change all my* to standard names
   - USER_GUIDE.md: Update examples
   - DEVELOPER_GUIDE.md: Update references

6. Update test files:
   - tests.sh: Change command references
   - tests/test_help.sh: Update command names
   - tests/test_threading.sh: Update if applicable

7. Update Makefile:
   - Change source file references
   - Update object file names
```

#### Manual Tests
```bash
# Verify all commands work with new names
./ushell
ls
cat README.md
cp test.txt test2.txt
mv test2.txt test3.txt
rm test3.txt
mkdir testdir
rmdir testdir
touch newfile.txt
stat newfile.txt
find "*.c" src/
rm newfile.txt

# Verify help shows new names
ls --help
cat --help
# Expected: Help shows "ls" not "myls"

# Verify completion
# Type: l<TAB>
# Expected: Shows "ls" in completions
```

---

### Prompt 2.3: Update Help Entries for Tools

#### Prompt
```
Update all help entries in src/help/help.c for renamed tools:

1. Change help entry names:
   - "myls" -> "ls"
   - "mycat" -> "cat"
   - "mycp" -> "cp"
   - "mymv" -> "mv"
   - "myrm" -> "rm"
   - "mymkdir" -> "mkdir"
   - "myrmdir" -> "rmdir"
   - "mytouch" -> "touch"
   - "mystat" -> "stat"
   - "myfd" -> "find"

2. Update help text content:
   - Change any internal references to my* names
   - Update examples to use new names
   - Update related command references

3. Verify get_help_entry() works with new names

4. Update the "commands" built-in output:
   - List tools with new names
```

#### Manual Tests
```bash
./ushell
# Type: help ls
# Expected: Shows help for ls (not myls)

# Type: help find
# Expected: Shows help for find (not myfd)

# Type: commands
# Expected: Lists ls, cat, cp, mv, rm, mkdir, rmdir, touch, stat, find
```

---

## Phase 3: ESP32 Adaptation - Preparation

### Prompt 3.1: Create ESP32 Project Structure

#### Prompt
```
Create an ESP-IDF compatible project structure for the ESP32 shell:

1. Create new directory: esp32-shell/ (parallel to unified-shell)
   esp32-shell/
   ├── CMakeLists.txt          # ESP-IDF project CMake
   ├── sdkconfig.defaults      # Default ESP32 configuration
   ├── partitions.csv          # Partition table for filesystem
   ├── main/
   │   ├── CMakeLists.txt      # Component CMake
   │   ├── main.c              # ESP32 entry point
   │   ├── esp_shell.h         # ESP32-specific shell header
   │   └── esp_shell.c         # ESP32-specific shell wrapper
   ├── components/
   │   ├── shell_core/         # Core shell logic (adapted)
   │   ├── shell_builtins/     # Built-in commands
   │   ├── shell_tools/        # Tool commands
   │   ├── shell_parser/       # Parser (simplified)
   │   └── shell_vfs/          # Virtual filesystem abstraction
   └── README.md               # ESP32 shell documentation

2. Create root CMakeLists.txt:
   cmake_minimum_required(VERSION 3.16)
   include($ENV{IDF_PATH}/tools/cmake/project.cmake)
   project(esp32_shell)

3. Create sdkconfig.defaults:
   - Enable SPIFFS or LittleFS
   - Configure UART for console
   - Set appropriate stack sizes
   - Enable FreeRTOS features

4. Create partition table (partitions.csv):
   - Include storage partition for filesystem
   - Allocate appropriate sizes

5. Document ESP32-specific requirements in README.md
```

#### Manual Tests
```bash
# Verify project structure
ls -la esp32-shell/
# Expected: CMakeLists.txt, main/, components/, etc.

# Verify ESP-IDF compatibility (requires ESP-IDF installed)
cd esp32-shell
idf.py set-target esp32
idf.py menuconfig
# Expected: Opens ESP-IDF configuration menu
```

---

### Prompt 3.2: Create Platform Abstraction Layer

#### Prompt
```
Create a platform abstraction layer to support both Linux and ESP32:

1. Create include/platform.h:
   - Define platform detection macros
   - Abstract common operations
   
   #ifndef PLATFORM_H
   #define PLATFORM_H
   
   #ifdef ESP_PLATFORM
       // ESP32-specific includes
       #include "freertos/FreeRTOS.h"
       #include "freertos/task.h"
       #include "esp_system.h"
       #include "esp_vfs.h"
       
       #define PLATFORM_ESP32 1
       #define PLATFORM_LINUX 0
   #else
       // Linux-specific includes
       #include <unistd.h>
       #include <sys/wait.h>
       #include <pthread.h>
       
       #define PLATFORM_ESP32 0
       #define PLATFORM_LINUX 1
   #endif
   
   // Platform-independent function declarations
   void platform_init(void);
   void platform_cleanup(void);
   int platform_read_char(void);
   void platform_write_char(char c);
   void platform_write_string(const char *s);
   void platform_sleep_ms(int ms);
   unsigned long platform_get_time_ms(void);
   
   #endif

2. Create src/platform/platform_linux.c:
   - Implement platform functions for Linux
   - Wrap standard library calls

3. Create src/platform/platform_esp32.c:
   - Implement platform functions for ESP32
   - Use ESP-IDF APIs

4. Update Makefile for conditional compilation:
   - Add PLATFORM variable
   - Include appropriate platform source

5. Update CMakeLists.txt for ESP32:
   - Exclude Linux-specific files
   - Include ESP32-specific files
```

#### Manual Tests
```bash
# Linux build test
cd unified-shell
make PLATFORM=linux
./ushell
# Expected: Works as before

# ESP32 build test (requires ESP-IDF)
cd esp32-shell
idf.py build
# Expected: Compiles for ESP32
```

---

### Prompt 3.3: Implement Virtual Filesystem Abstraction

#### Prompt
```
Create a virtual filesystem abstraction layer for ESP32:

1. Create include/shell_vfs.h:
   #ifndef SHELL_VFS_H
   #define SHELL_VFS_H
   
   #include <stddef.h>
   
   // File handle type
   typedef struct vfs_file vfs_file_t;
   
   // Directory entry
   typedef struct {
       char name[256];
       int is_dir;
       size_t size;
       unsigned long mtime;
   } vfs_dirent_t;
   
   // VFS operations
   int vfs_init(void);
   void vfs_cleanup(void);
   
   // File operations
   vfs_file_t* vfs_open(const char *path, const char *mode);
   int vfs_close(vfs_file_t *file);
   size_t vfs_read(void *buf, size_t size, vfs_file_t *file);
   size_t vfs_write(const void *buf, size_t size, vfs_file_t *file);
   int vfs_seek(vfs_file_t *file, long offset, int whence);
   long vfs_tell(vfs_file_t *file);
   
   // Directory operations
   int vfs_mkdir(const char *path);
   int vfs_rmdir(const char *path);
   int vfs_opendir(const char *path, void **handle);
   int vfs_readdir(void *handle, vfs_dirent_t *entry);
   int vfs_closedir(void *handle);
   
   // File management
   int vfs_remove(const char *path);
   int vfs_rename(const char *oldpath, const char *newpath);
   int vfs_stat(const char *path, vfs_dirent_t *entry);
   int vfs_exists(const char *path);
   
   // Path operations
   int vfs_getcwd(char *buf, size_t size);
   int vfs_chdir(const char *path);
   
   #endif

2. Create src/vfs/vfs_linux.c:
   - Implement VFS using standard POSIX calls
   - Wrap fopen, fread, opendir, stat, etc.

3. Create src/vfs/vfs_esp32.c:
   - Implement VFS using ESP-IDF VFS API
   - Support SPIFFS or LittleFS
   - Handle ESP32 filesystem limitations

4. Update all tool commands to use VFS:
   - ls.c: Use vfs_opendir, vfs_readdir
   - cat.c: Use vfs_open, vfs_read
   - cp.c: Use vfs_open, vfs_read, vfs_write
   - etc.

5. Update built-ins to use VFS:
   - cd: Use vfs_chdir
   - pwd: Use vfs_getcwd
```

#### Manual Tests
```bash
# Linux VFS test
./ushell
pwd
cd /tmp
ls
touch test.txt
cat test.txt
rm test.txt
# Expected: All commands work via VFS layer

# ESP32 VFS test (on device)
# Connect to ESP32 serial console
# Type: ls
# Expected: Lists files on SPIFFS/LittleFS
```

---

## Phase 4: ESP32 Adaptation - Core Changes

### Prompt 4.1: Remove Fork/Exec - Direct Execution Only

#### Prompt
```
Replace fork/exec model with direct function calls for ESP32:

1. Create include/executor_esp32.h:
   - Define direct execution model
   - No process creation
   - All commands run in same context

2. Modify src/evaluator/executor.c:
   - Add #ifdef PLATFORM_ESP32 guards
   - Replace fork/exec with direct function calls
   - Remove waitpid and process management
   - Keep built-in and tool dispatch logic

3. Remove pipeline support for external commands:
   - ESP32 cannot pipe to external programs
   - Keep built-in to built-in piping if memory allows
   - Or simplify to single command execution

4. Remove background job support:
   - No fork means no true background processes
   - Optionally: Use FreeRTOS tasks for pseudo-background
   - Or remove entirely for simplicity

5. Update command execution flow:
   - Parse command
   - Dispatch to built-in or tool function
   - Return result directly
   - No child process handling

6. Handle I/O redirection:
   - Implement simple stdin/stdout redirection
   - Use VFS for file-based redirection
```

#### Manual Tests
```bash
# On ESP32 (simulated or real)
# Type: pwd
# Expected: Prints current directory (no fork)

# Type: echo hello
# Expected: Prints hello (direct execution)

# Type: ls | cat
# Expected: Either works with internal piping or shows "pipes not supported"
```

---

### Prompt 4.2: Replace POSIX Threads with FreeRTOS Tasks

#### Prompt
```
Replace pthread implementation with FreeRTOS tasks for ESP32:

1. Create include/threading_esp32.h:
   #ifndef THREADING_ESP32_H
   #define THREADING_ESP32_H
   
   #ifdef ESP_PLATFORM
   #include "freertos/FreeRTOS.h"
   #include "freertos/task.h"
   #include "freertos/semphr.h"
   
   // Mutex wrapper
   typedef SemaphoreHandle_t shell_mutex_t;
   #define shell_mutex_init(m)    (*(m) = xSemaphoreCreateMutex())
   #define shell_mutex_lock(m)    xSemaphoreTake(*(m), portMAX_DELAY)
   #define shell_mutex_unlock(m)  xSemaphoreGive(*(m))
   #define shell_mutex_destroy(m) vSemaphoreDelete(*(m))
   
   #else
   #include <pthread.h>
   
   typedef pthread_mutex_t shell_mutex_t;
   #define shell_mutex_init(m)    pthread_mutex_init(m, NULL)
   #define shell_mutex_lock(m)    pthread_mutex_lock(m)
   #define shell_mutex_unlock(m)  pthread_mutex_unlock(m)
   #define shell_mutex_destroy(m) pthread_mutex_destroy(m)
   
   #endif
   #endif

2. Update src/threading/threading.c:
   - Use shell_mutex_t instead of pthread_mutex_t
   - Use wrapper macros for mutex operations
   - Optionally disable thread pool on ESP32 (run commands synchronously)

3. Update history.c, jobs.c, terminal.c:
   - Replace pthread_mutex_t with shell_mutex_t
   - Use wrapper macros

4. Consider disabling threading on ESP32:
   - Single-threaded execution may be simpler
   - Memory constraints may require it
   - Add SHELL_THREADING_ENABLED config option
```

#### Manual Tests
```bash
# On ESP32
# Execute commands that use mutexes
history
# Expected: No crashes, mutex operations work

# On Linux (verify no regression)
export USHELL_THREAD_BUILTINS=1
./ushell
pwd
echo test
history
# Expected: Threading still works on Linux
```

---

### Prompt 4.3: Adapt Terminal I/O for Serial Console

#### Prompt
```
Modify terminal handling for ESP32 UART/USB serial console:

1. Create src/utils/terminal_esp32.c:
   - Use ESP-IDF UART driver or USB CDC
   - Implement character-by-character input
   - Handle serial-specific escape sequences

2. Adapt line editing:
   - Arrow keys may not work over serial
   - Implement simple line buffer input
   - Keep basic backspace support
   - Consider removing cursor movement (or keep simple version)

3. Handle terminal width:
   - Serial terminals have variable width
   - Use default width (80) or configurable
   - Don't rely on ioctl for terminal size

4. Implement serial output:
   - Use UART TX for output
   - Handle newline conversion (\n -> \r\n if needed)
   - Buffer output for efficiency

5. Update setup_terminal() and restore_terminal():
   - ESP32: Configure UART parameters
   - Linux: Keep existing termios handling

6. Consider echo handling:
   - Serial terminals may echo automatically
   - Implement local echo toggle
```

#### Manual Tests
```bash
# Connect to ESP32 via serial terminal (minicom, screen, etc.)
screen /dev/ttyUSB0 115200

# Type commands
pwd
# Expected: Prints directory

echo hello
# Expected: Prints hello

# Test backspace
echoo<BACKSPACE>
echo hello
# Expected: Corrected command works

# Test line editing (if supported)
# Use arrow keys
# Expected: Either cursor moves or no response (graceful handling)
```

---

### Prompt 4.4: Simplify Parser for Embedded Use

#### Prompt
```
Simplify the parser for ESP32 memory constraints:

1. Evaluate current parser memory usage:
   - BNFC-generated parser may be large
   - Check AST memory allocation patterns
   - Profile memory usage during parsing

2. Options for parser adaptation:
   Option A: Keep BNFC parser with optimizations
   - Reduce buffer sizes
   - Limit parse tree depth
   - Add memory bounds checking
   
   Option B: Create simplified parser
   - Hand-written recursive descent parser
   - Smaller memory footprint
   - Support essential features only

3. Essential features to keep:
   - Simple command execution: cmd arg1 arg2
   - Variable expansion: $VAR
   - I/O redirection: > < >>
   - Quoting: "string" 'string'
   - Comments: # comment

4. Features to simplify or remove:
   - Complex pipelines (keep simple pipes if memory allows)
   - Arithmetic evaluation (keep simple or remove)
   - Glob expansion (keep simple patterns or remove)
   - Conditionals (keep simple if or remove)

5. Update grammar if using simplified parser:
   - Create esp32_grammar.y or similar
   - Document feature differences

6. Add memory limit checks:
   - Maximum command length
   - Maximum argument count
   - Maximum variable count
```

#### Manual Tests
```bash
# On ESP32
# Test basic parsing
echo hello world
# Expected: Works

# Test variable expansion
set NAME=ESP32
echo Hello $NAME
# Expected: Hello ESP32

# Test redirection
echo test > /spiffs/test.txt
cat /spiffs/test.txt
# Expected: test

# Test quoting
echo "hello world"
# Expected: hello world

# Test memory limits
# Type very long command (500+ chars)
# Expected: Error message about command too long, no crash
```

---

### Prompt 4.5: Remove or Adapt Complex Features

#### Prompt
```
Disable or adapt features that don't apply to ESP32:

1. Remove Job Control:
   - No fork means no background processes
   - Remove jobs, fg, bg commands
   - Remove & background operator
   - Or: Implement pseudo-background with FreeRTOS tasks

2. Remove/Adapt Signals:
   - ESP32 doesn't have POSIX signals
   - Remove signal handlers
   - Implement Ctrl+C via serial interrupt (if possible)
   - Or use simple polling for interrupt

3. Adapt History:
   - Keep history in RAM (limited size)
   - Optionally persist to flash (careful with write cycles)
   - Reduce history size (e.g., 20 entries vs 500)

4. Adapt APT Package Manager:
   - Remove entirely (no network packages on ESP32)
   - Or: Implement simple file-based "packages"
   - Consider OTA update functionality instead

5. Simplify Environment Variables:
   - Fixed size environment (e.g., 32 variables max)
   - Shorter values (128 chars max)
   - Store in RAM only

6. Remove EDI editor or simplify:
   - Full vi-like editor may be too large
   - Keep simple line editor if needed
   - Or remove entirely

7. Update help and commands lists:
   - Remove entries for disabled features
   - Update documentation
```

#### Manual Tests
```bash
# On ESP32
# Verify removed features show appropriate error
jobs
# Expected: "jobs: command not available on ESP32" or similar

fg
# Expected: Error or not found

apt
# Expected: Error or not found

# Verify kept features work
pwd
cd
ls
echo
history
help
version
exit
```

---

## Phase 5: ESP32 Specific Features

### Prompt 5.1: Add ESP32-Specific Commands

#### Prompt
```
Add commands specific to ESP32 functionality:

1. Create ESP32 system commands:
   - reboot: Restart the ESP32
   - free: Show free heap memory
   - uptime: Show time since boot
   - info: Show ESP32 chip info (model, revision, MAC)
   - wifi: WiFi configuration (if applicable)
   - gpio: GPIO pin control (optional)

2. Implement in src/builtins/esp32_builtins.c:
   
   int builtin_reboot(char **argv, Env *env) {
       printf("Rebooting...\n");
       esp_restart();
       return 0;  // Never reached
   }
   
   int builtin_free(char **argv, Env *env) {
       printf("Free heap: %lu bytes\n", esp_get_free_heap_size());
       printf("Minimum free heap: %lu bytes\n", esp_get_minimum_free_heap_size());
       return 0;
   }
   
   int builtin_uptime(char **argv, Env *env) {
       int64_t uptime_ms = esp_timer_get_time() / 1000;
       int seconds = (uptime_ms / 1000) % 60;
       int minutes = (uptime_ms / 60000) % 60;
       int hours = (uptime_ms / 3600000) % 24;
       int days = uptime_ms / 86400000;
       printf("Uptime: %d days, %02d:%02d:%02d\n", days, hours, minutes, seconds);
       return 0;
   }
   
   int builtin_info(char **argv, Env *env) {
       esp_chip_info_t chip_info;
       esp_chip_info(&chip_info);
       printf("ESP32 Chip Info:\n");
       printf("  Model: %s\n", chip_info.model == CHIP_ESP32 ? "ESP32" : "Unknown");
       printf("  Cores: %d\n", chip_info.cores);
       printf("  Revision: %d\n", chip_info.revision);
       printf("  Flash: %dMB %s\n", 
              spi_flash_get_chip_size() / (1024 * 1024),
              (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");
       return 0;
   }

3. Add help entries for ESP32 commands

4. Register commands in builtin dispatch table

5. Conditionally compile with #ifdef ESP_PLATFORM
```

#### Manual Tests
```bash
# On ESP32
# Type: free
# Expected: Shows heap memory info

# Type: uptime
# Expected: Shows time since boot

# Type: info
# Expected: Shows ESP32 chip details

# Type: reboot
# Expected: Device restarts
```

---

### Prompt 5.2: Implement Flash Filesystem Support

#### Prompt
```
Set up SPIFFS or LittleFS for persistent storage on ESP32:

1. Configure partition for filesystem:
   - Update partitions.csv:
     # Name,   Type, SubType, Offset,  Size, Flags
     nvs,      data, nvs,     0x9000,  0x6000,
     phy_init, data, phy,     0xf000,  0x1000,
     factory,  app,  factory, 0x10000, 1M,
     storage,  data, spiffs,  ,        1M,

2. Initialize filesystem in main():
   #include "esp_spiffs.h"
   
   esp_vfs_spiffs_conf_t conf = {
       .base_path = "/spiffs",
       .partition_label = NULL,
       .max_files = 5,
       .format_if_mount_failed = true
   };
   esp_vfs_spiffs_register(&conf);

3. Set default working directory:
   - Start in /spiffs
   - This is the only writable location

4. Handle filesystem limitations:
   - SPIFFS: No directories, flat namespace
   - LittleFS: Supports directories
   - Choose based on needs

5. Implement filesystem commands:
   - format: Reformat the filesystem
   - fsinfo: Show filesystem usage

6. Update VFS implementation for chosen filesystem
```

#### Manual Tests
```bash
# On ESP32
# Type: pwd
# Expected: /spiffs (or configured mount point)

# Type: ls
# Expected: Lists files in flash storage

# Type: touch test.txt
# Type: echo "hello" > test.txt
# Type: cat test.txt
# Expected: hello

# Type: fsinfo
# Expected: Shows used/free space

# Reboot and verify persistence
# Type: reboot
# (reconnect)
# Type: cat test.txt
# Expected: hello (file persisted)
```

---

### Prompt 5.3: Configure Memory Optimization

#### Prompt
```
Optimize memory usage for ESP32 constraints:

1. Reduce buffer sizes:
   - Command line buffer: 256 bytes (was 1024+)
   - History entries: 20 (was 500+)
   - Environment variables: 32 max
   - Max filename length: 64 bytes
   - Max path length: 128 bytes

2. Use static allocation where possible:
   - Pre-allocated command buffer
   - Fixed-size history array
   - Static environment storage

3. Create memory configuration header (include/shell_config.h):
   #ifndef SHELL_CONFIG_H
   #define SHELL_CONFIG_H
   
   #ifdef ESP_PLATFORM
       #define MAX_CMD_LENGTH      256
       #define MAX_ARGS            16
       #define MAX_HISTORY         20
       #define MAX_ENV_VARS        32
       #define MAX_VAR_NAME        32
       #define MAX_VAR_VALUE       128
       #define MAX_PATH_LENGTH     128
       #define MAX_FILENAME        64
       #define SHELL_STACK_SIZE    8192
   #else
       #define MAX_CMD_LENGTH      4096
       #define MAX_ARGS            256
       #define MAX_HISTORY         500
       #define MAX_ENV_VARS        256
       #define MAX_VAR_NAME        256
       #define MAX_VAR_VALUE       4096
       #define MAX_PATH_LENGTH     4096
       #define MAX_FILENAME        256
       #define SHELL_STACK_SIZE    65536
   #endif
   
   #endif

4. Update all code to use config constants:
   - Replace hardcoded values
   - Add bounds checking
   - Handle truncation gracefully

5. Monitor heap usage:
   - Print free heap at startup
   - Add low memory warning
   - Implement graceful degradation
```

#### Manual Tests
```bash
# On ESP32
# Type: free
# Note: Free heap amount

# Execute several commands
pwd
ls
echo test
history

# Type: free
# Note: Heap usage (should be stable, no leaks)

# Test memory limits
# Create command with 257+ characters
# Expected: Error "command too long", no crash

# Set many variables
set V1=val1
set V2=val2
... (up to limit)
# Expected: Error when limit reached, no crash
```

---

### Prompt 5.4: Create ESP32 Build Configuration

#### Prompt
```
Create complete ESP-IDF build configuration:

1. Create main/CMakeLists.txt:
   idf_component_register(
       SRCS "main.c" "esp_shell.c"
       INCLUDE_DIRS "."
       REQUIRES console vfs spiffs nvs_flash
   )

2. Create components/shell_core/CMakeLists.txt:
   idf_component_register(
       SRCS 
           "evaluator/environment.c"
           "evaluator/executor.c"
           "evaluator/arithmetic.c"
           "evaluator/conditional.c"
           "utils/expansion.c"
           "utils/history.c"
           "utils/terminal_esp32.c"
           "utils/completion.c"
           "platform/platform_esp32.c"
           "vfs/vfs_esp32.c"
       INCLUDE_DIRS 
           "include"
       REQUIRES 
           shell_builtins 
           shell_tools 
           shell_parser
   )

3. Create sdkconfig.defaults with optimized settings:
   # Console configuration
   CONFIG_ESP_CONSOLE_UART_DEFAULT=y
   CONFIG_ESP_CONSOLE_UART_BAUDRATE=115200
   
   # SPIFFS configuration
   CONFIG_SPIFFS_MAX_PARTITIONS=3
   CONFIG_SPIFFS_CACHE=y
   CONFIG_SPIFFS_CACHE_WR=y
   
   # Memory optimization
   CONFIG_FREERTOS_UNICORE=y
   CONFIG_ESP32_DEFAULT_CPU_FREQ_160=y
   CONFIG_COMPILER_OPTIMIZATION_SIZE=y
   
   # Stack sizes
   CONFIG_MAIN_TASK_STACK_SIZE=8192
   CONFIG_FREERTOS_IDLE_TASK_STACKSIZE=1024

4. Create README.md for ESP32 shell:
   - Build instructions
   - Flash instructions
   - Usage guide
   - Feature list
   - Limitations

5. Create flash script (flash.sh):
   #!/bin/bash
   idf.py -p /dev/ttyUSB0 flash monitor
```

#### Manual Tests
```bash
# Build for ESP32
cd esp32-shell
idf.py set-target esp32
idf.py build
# Expected: Successful build

# Flash to device
idf.py -p /dev/ttyUSB0 flash
# Expected: Successful flash

# Monitor serial output
idf.py -p /dev/ttyUSB0 monitor
# Expected: Shell prompt appears

# Test basic commands
pwd
ls
echo hello
# Expected: All work
```

---

## Phase 6: Testing and Documentation

### Prompt 6.1: Create ESP32 Test Suite

#### Prompt
```
Create test suite for ESP32 shell:

1. Create tests/test_esp32.sh (for host simulation):
   - Tests that can run on Linux with ESP32 defines
   - Memory limit tests
   - Feature subset tests

2. Create tests/test_esp32_device.py:
   - Python script to test via serial
   - Send commands, verify output
   - Automated regression tests

3. Test categories:
   a. Basic commands:
      - pwd, cd, ls, echo
      - Variable set/get
      - File operations
   
   b. Memory limits:
      - Long command handling
      - Many variables
      - Large file operations
   
   c. ESP32-specific:
      - reboot (manual verification)
      - free, uptime, info
      - Filesystem operations
   
   d. Error handling:
      - Invalid commands
      - File not found
      - Permission errors (if applicable)

4. Create test documentation:
   - Test plan document
   - Expected results
   - Known limitations
```

#### Manual Tests
```bash
# Run host tests
./tests/test_esp32.sh
# Expected: All tests pass

# Run device tests (requires connected ESP32)
python3 tests/test_esp32_device.py /dev/ttyUSB0
# Expected: Automated tests pass
```

---

### Prompt 6.2: Update All Documentation

#### Prompt
```
Update documentation for ESP32 shell:

1. Create esp32-shell/README.md:
   - Project overview
   - Features (with limitations noted)
   - Build requirements (ESP-IDF version)
   - Build and flash instructions
   - Usage guide
   - Troubleshooting

2. Update unified-shell/README.md:
   - Add section about ESP32 variant
   - Link to esp32-shell directory
   - Note feature differences

3. Create esp32-shell/docs/USER_GUIDE.md:
   - Getting started
   - Available commands
   - Filesystem usage
   - ESP32-specific commands
   - Limitations

4. Create esp32-shell/docs/DEVELOPER_GUIDE.md:
   - Architecture overview
   - Adding new commands
   - Memory considerations
   - Building and testing

5. Update AI_Interaction.md:
   - Document ESP32 port work
   - List all changes made
   - Note decisions and tradeoffs
```

---

## Success Criteria

### AI Integration Removal Complete When:
1. DONE: aiIntegr directory deleted
2. DONE: No Python code or dependencies
3. DONE: No AI environment variables
4. DONE: No @ command handling
5. DONE: Documentation updated

### Tool Renaming Complete When:
1. DONE: All my<command>.c files renamed
2. DONE: All function names updated
3. DONE: Help entries updated
4. DONE: Completion updated
5. DONE: Documentation updated
6. DONE: Tests updated and passing

### ESP32 Adaptation Complete When:
1. DONE: Platform abstraction layer created
2. DONE: VFS abstraction implemented
3. DONE: Fork/exec replaced with direct execution
4. DONE: Threads replaced with FreeRTOS (or disabled)
5. DONE: Terminal I/O adapted for serial
6. DONE: Parser simplified if needed
7. DONE: Complex features removed/adapted
8. DONE: ESP32-specific commands added
9. DONE: Memory optimized
10. DONE: Builds and runs on ESP32
11. DONE: Tests passing
12. DONE: Documentation complete

---

## Implementation Order

Recommended sequence:

**Phase 1: AI Removal (Prompts 1.1-1.2)**
- Quick cleanup, no architectural changes
- Can be done first to simplify codebase

**Phase 2: Tool Renaming (Prompts 2.1-2.3)**
- Straightforward renaming
- Do while code is still familiar

**Phase 3: ESP32 Preparation (Prompts 3.1-3.3)**
- Create project structure
- Add abstraction layers
- Keep Linux build working

**Phase 4: ESP32 Core Changes (Prompts 4.1-4.5)**
- Major architectural changes
- Test incrementally
- May need iteration

**Phase 5: ESP32 Features (Prompts 5.1-5.4)**
- Add ESP32-specific functionality
- Polish and optimize

**Phase 6: Testing and Docs (Prompts 6.1-6.2)**
- Comprehensive testing
- Final documentation

---

## Notes and Constraints

Following constraints from AgentConstraints.md:
- Document all interactions in AI_Interaction.md using echo
- Add detailed comments to all code
- NO emojis, use ASCII art or DONE/NOT DONE markers
- This ESP32_Prompts.md file was specifically requested

ESP32-specific constraints:
- Must work with ESP-IDF v4.4+ or v5.x
- Target: ESP32 (original), ESP32-S2, ESP32-S3, ESP32-C3
- Minimum 4MB flash recommended
- RAM usage must stay under 200KB (leave room for user applications)
- Must maintain Linux build compatibility during development
- Serial console is primary interface (115200 baud default)

---

## Hardware Requirements

Minimum:
- ESP32 module with 4MB flash
- USB-to-Serial adapter or built-in USB
- Serial terminal software

Recommended:
- ESP32-DevKitC or similar development board
- 8MB+ flash for larger filesystem
- PSRAM for extended memory (ESP32-WROVER)

---

## Testing Commands Summary

Quick reference for manual testing:

```bash
# Build for Linux (with renamed commands, no AI)
cd unified-shell
make clean && make
./ushell
ls
cat README.md
help ls

# Build for ESP32
cd esp32-shell
idf.py set-target esp32
idf.py build
idf.py -p /dev/ttyUSB0 flash monitor

# On ESP32 console
pwd
ls
echo hello
free
uptime
info
help
exit
```
