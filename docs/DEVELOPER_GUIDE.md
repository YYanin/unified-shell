# Unified Shell (ushell) - Developer Guide

## Table of Contents
1. [Architecture Overview](#architecture-overview)
2. [Interactive System Architecture](#interactive-system-architecture)
3. [Code Structure](#code-structure)
4. [Build System](#build-system)
5. [Parser & Grammar](#parser--grammar)
6. [Evaluation Pipeline](#evaluation-pipeline)
7. [Job Control System](#job-control-system)
8. [Memory Management](#memory-management)
9. [Adding Features](#adding-features)
10. [Testing Guidelines](#testing-guidelines)
11. [Code Style Guide](#code-style-guide)
12. [Debugging](#debugging)
13. [Performance Considerations](#performance-considerations)

---

## Architecture Overview

### High-Level Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                         User Input                          │
└─────────────────────┬───────────────────────────────────────┘
                      ↓
┌─────────────────────────────────────────────────────────────┐
│                    Lexer (Grammar.l)                        │
│  - Tokenization                                             │
│  - Pattern matching                                         │
└─────────────────────┬───────────────────────────────────────┘
                      ↓
┌─────────────────────────────────────────────────────────────┐
│                   Parser (Grammar.y)                        │
│  - Syntax validation                                        │
│  - AST construction                                         │
└─────────────────────┬───────────────────────────────────────┘
                      ↓
┌─────────────────────────────────────────────────────────────┐
│              Abstract Syntax Tree (Absyn.c)                 │
│  - Hierarchical command structure                           │
│  - Type-safe representation                                 │
└─────────────────────┬───────────────────────────────────────┘
                      ↓
┌─────────────────────────────────────────────────────────────┐
│                Evaluator (evaluator.c)                      │
│  - Tree traversal                                           │
│  - Command dispatch                                         │
│  - Control flow                                             │
└─────────┬───────────────────────────────────────────────────┘
          ↓
    ┌─────┴──────┐
    ↓            ↓
┌─────────┐  ┌──────────────┐
│ Built-  │  │   External   │
│  ins    │  │   Commands   │
└─────────┘  └──────────────┘
    ↓              ↓
┌─────────┐  ┌──────────────┐
│ Direct  │  │ Fork/Exec    │
│ Calls   │  │ Executor     │
└─────────┘  └──────────────┘
```

### Component Interaction

```
main.c
  ├─→ REPL loop
  ├─→ Interactive Input (terminal.c)
  ├─→ History Management (history.c)
  ├─→ Tab Completion (completion.c)
  ├─→ Parser (pProgram)
  ├─→ Evaluator (eval_program)
  └─→ Cleanup (atexit)

Interactive System
  ├─→ Terminal (terminal.c)
  │     ├─→ Raw mode setup
  │     ├─→ ANSI escape sequences
  │     ├─→ Multi-line wrapping
  │     └─→ Cursor positioning
  ├─→ History (history.c)
  │     ├─→ Persistent storage (~/.ushell_history)
  │     ├─→ Navigation (up/down arrows)
  │     └─→ Duplicate filtering
  └─→ Completion (completion.c)
        ├─→ Command completion
        ├─→ Filename completion
        └─→ Full-line construction

Evaluator
  ├─→ Variable Expansion
  ├─→ Arithmetic Evaluation
  ├─→ Glob Expansion
  ├─→ Built-in Check
  └─→ External Execution

Executor
  ├─→ Process Creation (fork)
  ├─→ Pipeline Setup (pipe)
  ├─→ I/O Redirection (dup2)
  └─→ Wait/Collect Status

APT Package Manager
  ├─→ apt_builtin (dispatcher)
  ├─→ repo (index management)
  ├─→ install (extraction, PATH setup)
  ├─→ remove (recursive deletion, verification)
  └─→ depends (resolution, circular detection)
```

---

## Package Management Architecture

### APT System Overview

The APT subsystem implements a complete package management system with dependency resolution.

```
                    ┌──────────────────────┐
                    │   builtin_apt()      │
                    │   (dispatcher)       │
                    └──────────┬───────────┘
                               │
        ┌──────────────────────┼──────────────────────┐
        │                      │                      │
    ┌───▼────┐           ┌────▼─────┐          ┌────▼─────┐
    │ init   │           │ install  │          │ remove   │
    │ update │           │ verify   │          │ search   │
    │ list   │           │ show     │          │          │
    └───┬────┘           └────┬─────┘          └────┬─────┘
        │                     │                      │
        │         ┌───────────┴──────────┐          │
        │         │                      │          │
    ┌───▼─────────▼────┐        ┌───────▼──────────▼────┐
    │   repo.c         │        │    depends.c          │
    │ ─────────────    │        │ ─────────────         │
    │ - apt_init()     │        │ - dependency check    │
    │ - load_index()   │        │ - resolution          │
    │ - save_index()   │        │ - circular detection  │
    │ - find_package() │        │ - auto-install        │
    └──────────────────┘        └───────────────────────┘
                                            │
                                    ┌───────┴────────┐
                              ┌─────▼─────┐    ┌────▼─────┐
                              │ install.c │    │ remove.c │
                              │────────── │    │───────── │
                              │ - extract │    │ - verify │
                              │ - setup   │    │ - delete │
                              └───────────┘    └──────────┘
```

### APT Data Structures

#### Package Structure
```c
typedef struct {
    char name[APT_NAME_LEN];           // Package name (64 bytes)
    char version[APT_VERSION_LEN];     // Version string (16 bytes)
    char description[APT_DESC_LEN];    // Description (256 bytes)
    char filename[APT_FILENAME_LEN];   // Archive filename (128 bytes)
    char depends[APT_DEPS_LEN];        // Comma-separated deps (256 bytes)
    int installed;                     // Installation status (0 or 1)
} Package;
```

#### Package Index
```c
typedef struct {
    Package packages[APT_MAX_PACKAGES]; // Array of packages (256 max)
    int count;                          // Number of packages
} PackageIndex;
```

#### Configuration
```c
typedef struct {
    char base_dir[APT_PATH_LEN];       // ~/.ushell/
    char packages_dir[APT_PATH_LEN];   // ~/.ushell/packages/
    char repo_dir[APT_PATH_LEN];       // ~/.ushell/repo/
    char available_dir[APT_PATH_LEN];  // ~/.ushell/repo/available/
    char cache_dir[APT_PATH_LEN];      // ~/.ushell/repo/cache/
    char index_file[APT_PATH_LEN];     // ~/.ushell/repo/index.txt
    char config_file[APT_PATH_LEN];    // ~/.ushell/apt.conf
    int initialized;                    // Initialization flag
} AptConfig;
```

### APT Workflow Diagrams

#### Installation Workflow

```
apt install pkgname
        │
        ▼
    Find package in index
        │
        ▼
    Check if already installed ─────► Already installed → Exit
        │ (not installed)
        ▼
    Check dependencies
        │
        ├─► Missing deps → Error (suggest --auto-install)
        │
        ▼ (all satisfied)
    Create package directory
        │
        ▼
    Extract tar.gz archive
        │
        ▼
    Create METADATA file
        │
        ▼
    Set executable permissions
        │
        ▼
    Update package index
        │
        ▼
    Display success message
```

#### Dependency Resolution Workflow

```
apt install pkgname --auto-install
        │
        ▼
    apt_resolve_dependencies(pkgname)
        │
        ▼
    Get dependencies from METADATA
        │
        ▼
    Check for circular dependencies ───► Circular → Error & Exit
        │ (no circular)
        ▼
    Recursively resolve each dependency
        │
        ▼
    Build installation order list
        │  (dependencies before dependents)
        ▼
    apt_install_dependencies()
        │
        ▼
    Install packages in order
        │
        ├─► Package 1 (dependency)
        ├─► Package 2 (dependency)
        └─► Package N (requested)
```

#### Removal Workflow

```
apt remove pkgname [--force]
        │
        ▼
    Find package in index
        │
        ▼
    Check if installed ─────► Not installed → Exit
        │ (installed)
        ▼
    Check for dependents
        │
        ├─► Has dependents + no --force → Warning (continue)
        ├─► Has dependents + --force → Skip warning
        │
        ▼
    Remove directory recursively
        │
        ▼
    Update package index
        │
        ▼
    Display success message
```

### APT File Formats

#### Repository Index (index.txt)

```
PackageName: basepkg
Version: 1.0
Description: Base package with no dependencies
Filename: basepkg-1.0.tar.gz

PackageName: dependpkg
Version: 1.0
Description: Package that depends on basepkg
Depends: basepkg
Filename: dependpkg-1.0.tar.gz
```

#### Package METADATA

```
Name: basepkg
Version: 1.0
Description: Base package with no dependencies
InstallDate: 2025-11-25 21:34:56
Filename: basepkg-1.0.tar.gz
```

### APT Key Algorithms

#### Circular Dependency Detection

Uses depth-limited DFS with path tracking:

```c
int apt_check_circular_dependency(const char *pkgname, 
                                   char visited[][APT_NAME_LEN],
                                   int visited_count,
                                   int depth)
{
    // Depth limit prevents infinite loops
    if (depth > MAX_DEPENDENCY_DEPTH) return 1;
    
    // Check if package already in visited path
    for (int i = 0; i < visited_count; i++) {
        if (strcmp(visited[i], pkgname) == 0) {
            return 1; // Circular dependency detected
        }
    }
    
    // Add to visited path
    strcpy(visited[visited_count], pkgname);
    
    // Recursively check all dependencies
    char **deps = apt_get_dependencies(pkgname);
    for (int i = 0; deps[i] != NULL; i++) {
        if (apt_check_circular_dependency(deps[i], visited, 
                                          visited_count + 1, 
                                          depth + 1)) {
            return 1;
        }
    }
    
    return 0; // No circular dependency
}
```

#### Dependency Resolution Order

Topological sorting ensures dependencies install before dependents:

```c
int apt_resolve_dependencies(const char *pkgname,
                             char resolved[][APT_NAME_LEN],
                             int *resolved_count)
{
    // Skip if already resolved
    for (int i = 0; i < *resolved_count; i++) {
        if (strcmp(resolved[i], pkgname) == 0) return 0;
    }
    
    // Get dependencies for this package
    char **deps = apt_get_dependencies(pkgname);
    
    // Recursively resolve each dependency first
    for (int i = 0; deps[i] != NULL; i++) {
        apt_resolve_dependencies(deps[i], resolved, resolved_count);
    }
    
    // Add this package after its dependencies
    strcpy(resolved[*resolved_count], pkgname);
    (*resolved_count)++;
    
    return 0;
}
```

### APT Memory Management

- **Package structures**: Static allocation in global package index
- **Dependency lists**: Dynamic allocation with malloc, freed after use
- **String buffers**: Stack allocation for paths and commands
- **Archive extraction**: Uses system tar command (no memory management)

### APT Error Handling

All APT functions return:
- `0` on success
- `-1` on error (with stderr message)

Error messages follow format:
```
apt <subcommand>: <error description>
<helpful suggestion or next step>
```

Examples:
```
apt install: package 'foo' not found in repository
Run 'apt search foo' to find similar packages.

apt install: missing dependencies: basepkg, libutils
Install dependencies first:
  apt install basepkg
  apt install libutils
Or use --auto-install flag to install dependencies automatically.
```

### APT Performance Considerations

- **Index loading**: O(n) scan of index.txt on each `apt update`
- **Package lookup**: O(n) linear search through package array
- **Dependency resolution**: O(d * p) where d=depth, p=packages per level
- **Circular detection**: O(d * n) where d=MAX_DEPTH, n=packages
- **Installation**: Dominated by tar extraction time

### APT Future Enhancements

Potential improvements:
- Hash table for O(1) package lookup
- Binary package format (faster than tar.gz extraction)
- Remote repository support with HTTP/HTTPS
- Package download and caching
- Version comparison and upgrades
- Transaction rollback on failure
- Package signing and GPG verification
- Conflict detection (two packages with same file)
- Package provides/replaces mechanism

---

## Interactive System Architecture

The shell provides readline-like functionality through three cooperating modules:

### Terminal Module (src/utils/terminal.c)

**Purpose**: Low-level terminal control and raw mode input handling.

**Key Components**:

1. **Raw Mode Setup**
   ```c
   void setup_raw_mode(void)
   ```
   - Disables canonical mode (line buffering)
   - Disables echo (prevents double display)
   - Enables character-by-character input
   - Configures termios flags for interactive editing

2. **Line Redrawing Algorithm**
   ```c
   static void redraw_line(const char *prompt, const char *line, int cursor_pos)
   ```
   - Detects terminal width via `ioctl(TIOCGWINSZ)`
   - Calculates row occupation for multi-line wrapping
   - Handles scroll when content expands
   - Uses ANSI escapes for cursor control:
     - `\033[nA`: Move cursor up n rows
     - `\033[nC`: Move cursor right n columns
     - `\033[J`: Clear from cursor to end of screen
     - `\r`: Return to column 0

3. **Terminal Width Detection**
   ```c
   struct winsize w;
   ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
   int width = (w.ws_col > 0) ? w.ws_col : 80;
   ```
   - Dynamically adapts to terminal resizing
   - Falls back to 80 columns if detection fails

4. **Multi-line Wrapping Logic**
   ```c
   int old_rows = old_total / width;
   int new_rows = new_total / width;
   
   // Handle scroll when growing into new row
   if (new_rows > old_rows) {
       char scroll_seq[] = "\n\x1b[A";
       write(STDOUT_FILENO, scroll_seq, 4);
   }
   ```

**Design Rationale**:
- Direct terminal I/O avoids stdio buffering issues
- Row calculation prevents prompt duplication
- Scroll handling maintains terminal state consistency

### History Module (src/utils/history.c)

**Purpose**: Persistent command history with navigation support.

**Key Components**:

1. **Storage Format**
   - File: `~/.ushell_history`
   - Format: One command per line
   - Max entries: `HISTORY_MAX` (default 1000)
   - Ring buffer in memory

2. **Duplicate Filtering**
   ```c
   void add_history(const char *cmd)
   ```
   - Rejects empty commands
   - Prevents consecutive duplicates
   - Preserves unique command sequence

3. **Navigation State**
   ```c
   const char *history_get_prev(void)
   const char *history_get_next(void)
   ```
   - Maintains current position in history
   - Returns NULL at boundaries
   - Supports full traversal

4. **Persistence**
   ```c
   void save_history(void)
   ```
   - Writes on exit (atexit handler)
   - Truncates to max entries
   - Preserves order

**Design Rationale**:
- File-based storage persists across sessions
- Ring buffer optimizes memory usage
- Duplicate filtering improves usability

### Completion Module (src/utils/completion.c)

**Purpose**: Command and filename tab completion.

**Key Components**:

1. **Completion Generation**
   ```c
   char **completion_generate(const char *text, int *count)
   ```
   - Identifies last word in input
   - Searches built-ins and PATH for commands
   - Scans current directory for filenames
   - Returns array of full-line completions

2. **Full-Line Construction**
   ```c
   // For input "cat te<TAB>"
   prefix_len = last_word - text;  // "cat "
   full_completion = malloc(prefix_len + match_len + 1);
   memcpy(full_completion, text, prefix_len);
   strcpy(full_completion + prefix_len, filename);
   // Result: "cat test.txt"
   ```

3. **Command Completion**
   - Checks built-in command table first
   - Scans directories in PATH
   - Matches by prefix

4. **Filename Completion**
   - Uses `opendir()`/`readdir()` for current directory
   - Filters by prefix match
   - Includes directories and files

**Design Rationale**:
- Full-line returns preserve command context
- Dual-mode (command/file) matches user expectations
- Built-ins checked first for performance

### Integration with Main Loop

```c
// In src/main.c
char *input = terminal_readline(prompt);

// terminal_readline() internally:
// 1. Enters raw mode
// 2. Reads character-by-character
// 3. Handles special keys:
//    - Arrow keys (escape sequences)
//    - Tab (invokes completion)
//    - Backspace/Delete
//    - Ctrl+C (cancel)
//    - Ctrl+D (EOF)
// 4. Calls redraw_line() after each change
// 5. Returns to cooked mode
// 6. Adds to history
```

### Testing Interactive Features

**Manual Testing**:
```bash
# Test multi-line wrapping
echo "very long command that wraps to multiple lines..."

# Test history navigation
echo first
echo second
# Press UP: shows "echo second"
# Press UP: shows "echo first"
# Press DOWN: shows "echo second"

# Test tab completion
ec<TAB>          # Completes to "echo"
cat te<TAB>      # Completes to "cat test.txt"
my<TAB><TAB>     # Shows all "my*" commands

# Test line editing
echo Hello<LEFT><LEFT><LEFT><BACKSPACE>i
# Result: "echo Hillo"
```

**Automated Testing**:
- Terminal module tested via expect scripts
- History module unit tests in test suite
- Completion tested with directory fixtures

---

## Code Structure

### Directory Layout

```
unified-shell/
├── src/
│   ├── main.c                    # REPL and main entry point
│   ├── parser/                   # BNFC-generated parser
│   │   ├── Grammar.cf            # BNF grammar specification
│   │   ├── Grammar.l             # Flex lexer (generated)
│   │   ├── Grammar.y             # Bison parser (generated)
│   │   ├── Absyn.c/h            # Abstract syntax tree
│   │   ├── Lexer.c              # Lexer implementation
│   │   ├── Parser.c/h           # Parser implementation
│   │   └── Printer.c/h          # AST printer (debugging)
│   ├── evaluator/               # Command evaluation
│   │   ├── evaluator.c          # Main evaluation logic
│   │   ├── executor.c           # Process execution
│   │   ├── expander.c           # Variable expansion
│   │   ├── arithmetic.c         # Arithmetic evaluation
│   │   └── env.c                # Environment management
│   ├── builtins/                # Built-in commands (13 total)
│   │   ├── builtins.c           # Core built-ins (cd, pwd, echo, etc.)
│   │   └── builtin_edi.c        # Integrated text editor
│   ├── apt/                     # Package management system
│   │   ├── apt_builtin.c        # APT command dispatcher (490 lines)
│   │   ├── repo.c               # Repository & index management (720 lines)
│   │   ├── install.c            # Package installation engine (530 lines)
│   │   ├── remove.c             # Package removal & verification (430 lines)
│   │   └── depends.c            # Dependency resolution (430 lines)
│   ├── tools/                   # Integrated utilities (10 tools)
│   │   ├── myls.c, mycat.c, mycp.c, mymv.c, myrm.c
│   │   ├── mymkdir.c, myrmdir.c, mytouch.c
│   │   ├── mystat.c, myfd.c
│   │   └── tools.c              # Tool registration
│   ├── glob/                    # Pattern matching
│   │   └── glob.c               # Wildcard expansion
│   └── utils/                   # Interactive & utility modules
│       ├── terminal.c           # Raw mode, readline, ANSI escapes
│       ├── history.c            # Command history persistence
│       ├── completion.c         # Tab completion (cmd + file)
│       ├── string_utils.c       # String manipulation
│       └── memory_utils.c       # Memory helpers
├── include/                     # Public headers
│   ├── shell.h                  # Main shell interface
│   ├── evaluator.h, executor.h, expander.h
│   ├── arithmetic.h, env.h
│   ├── builtins.h, tools.h
│   ├── glob.h
│   └── utils.h
└── tests/                       # Test suites
    ├── syntax/                  # Parser tests
    ├── semantic/                # Execution tests
    ├── integration/             # End-to-end tests
    └── valgrind_test.sh        # Memory tests
```

### Key Files

#### src/main.c (150 lines)
- **Purpose**: Main REPL loop, initialization, cleanup
- **Key Functions**:
  - `main()`: Entry point, REPL loop
  - `get_prompt()`: Dynamic prompt generation
  - `sigint_handler()`: Ctrl+C handling
  - `cleanup_shell()`: Exit cleanup

#### src/parser/Grammar.cf (grammar specification)
- **Purpose**: BNF grammar for the shell language
- **Defines**: Tokens, syntax rules, precedence
- **Generate With**: `bnfc -c Grammar.cf`

#### src/evaluator/evaluator.c (600+ lines)
- **Purpose**: Main evaluation logic
- **Key Functions**:
  - `eval_program()`: Entry point for evaluation
  - `eval_command()`: Single command evaluation
  - `eval_pipeline()`: Pipeline setup
  - `eval_conditional()`: If statement handling

#### src/evaluator/executor.c (520+ lines)
- **Purpose**: Process creation and management
- **Key Functions**:
  - `execute_command()`: Fork/exec wrapper
  - `setup_pipeline()`: Pipe creation and coordination
  - `handle_redirection()`: File descriptor manipulation

#### src/evaluator/expander.c (200+ lines)
- **Purpose**: Variable and glob expansion
- **Key Functions**:
  - `expand_variables()`: $VAR expansion
  - `expand_arithmetic()`: $((expr)) evaluation
  - `expand_globs()`: Wildcard expansion

#### src/builtins/builtins.c (280+ lines)
- **Purpose**: Built-in command implementations (13 total)
- **Commands**: cd, pwd, echo, export, set, unset, env, help, version, exit, history, edi, apt

#### src/apt/apt_builtin.c (490 lines)
- **Purpose**: APT package manager command dispatcher
- **Key Functions**:
  - `builtin_apt()`: Main entry point, subcommand dispatcher
  - `apt_cmd_init()`: Initialize package system
  - `apt_cmd_update()`: Reload package index
  - `apt_cmd_list()`: List packages with filtering
  - `apt_cmd_search()`: Search by keyword
  - `apt_cmd_show()`: Display package details
  - `apt_cmd_install()`: Install packages with flag parsing
  - `apt_cmd_remove()`: Remove packages with flag parsing
  - `apt_cmd_help()`: Display APT help

#### src/apt/repo.c (720 lines)
- **Purpose**: Repository and package index management
- **Key Functions**:
  - `apt_init()`: Create directory structure
  - `apt_load_index()`: Parse index.txt file
  - `apt_save_index()`: Write index to disk
  - `apt_find_package()`: Lookup package by name
  - `apt_get_package_count()`: Count packages
  - `apt_update_installed_status()`: Sync index with filesystem
  - `apt_is_installed()`: Check package installation status
  - `apt_check_dependents()`: Find packages depending on target

#### src/apt/install.c (530 lines)
- **Purpose**: Package installation engine
- **Key Functions**:
  - `apt_install_package()`: Main installation workflow
  - `apt_extract_package()`: Extract tar.gz archive
  - `apt_create_metadata()`: Generate METADATA file
  - `apt_make_executables_accessible()`: Set permissions
  - `apt_setup_path()`: Add packages to PATH
  - `apt_check_dependencies_for_package()`: Wrapper for dependency check

#### src/apt/remove.c (430 lines)
- **Purpose**: Package removal and verification
- **Key Functions**:
  - `apt_remove_package()`: Remove package (wrapper)
  - `apt_remove_package_with_force()`: Main removal with force option
  - `apt_remove_directory_recursive()`: Recursive directory deletion
  - `apt_verify_package()`: Integrity checking
  - Package verification checks:
    - Package directory exists
    - METADATA file present and valid
    - bin/ directory present
    - Executables have correct permissions

#### src/apt/depends.c (430 lines)
- **Purpose**: Dependency resolution system
- **Key Functions**:
  - `apt_get_dependencies()`: Parse dependency list from METADATA
  - `apt_check_dependencies()`: Verify all dependencies installed
  - `apt_resolve_dependencies()`: Recursive resolution with ordering
  - `apt_install_dependencies()`: Install all dependencies in order
  - `apt_check_circular_dependency()`: Depth-limited DFS for circular detection
- **Features**:
  - Circular dependency detection (MAX_DEPENDENCY_DEPTH = 10)
  - Correct installation order (dependencies before dependents)
  - Clear error messages with helpful suggestions
  - Support for --auto-install flag

#### src/utils/terminal.c (547 lines)
- **Purpose**: Interactive terminal input with readline-like features
- **Key Functions**:
  - `terminal_readline()`: Main input loop with editing
  - `setup_raw_mode()`: Terminal configuration
  - `redraw_line()`: Multi-line display with wrapping
  - `handle_special_key()`: Arrow keys, tab, backspace

#### src/utils/history.c (~200 lines)
- **Purpose**: Persistent command history
- **Key Functions**:
  - `add_history()`: Store command with duplicate filtering
  - `history_get_prev()`: Navigate backward
  - `history_get_next()`: Navigate forward
  - `save_history()`: Persist to ~/.ushell_history

#### src/utils/completion.c (~250 lines)
- **Purpose**: Tab completion for commands and files
- **Key Functions**:
  - `completion_generate()`: Generate matches
  - `find_commands()`: Search built-ins and PATH
  - `find_files()`: Scan current directory
  - Returns full-line completions (preserves prefix)

#### src/tools/ (10 files, ~1500 lines total)
- **Purpose**: Integrated filesystem utilities
- **Tools**: myls, mycat, mycp, mymv, myrm, mymkdir, myrmdir, mytouch, mystat, myfd

---

## Build System

### Makefile Structure

```makefile
# Variables
CC = gcc
CFLAGS = -Wall -Wextra -g -Iinclude -pthread
PARSER_DIR = src/parser
SOURCES = $(all source files)
OBJECTS = $(SOURCES:.c=.o)

# Main targets
all: ushell tools

ushell: $(OBJECTS)
    $(CC) $(OBJECTS) -o ushell

tools: (individual tool targets)

clean:
    rm -f $(OBJECTS) ushell tools

# Dependencies
-include $(OBJECTS:.o=.d)
```

### Build Process

1. **Parser Generation** (if Grammar.cf changes):
   ```bash
   cd src/parser
   bnfc -c Grammar.cf
   make
   ```

2. **Compile All**:
   ```bash
   make clean
   make
   ```

3. **Compile Individual Module**:
   ```bash
   gcc -c -Wall -Wextra -g -Iinclude src/evaluator/evaluator.c
   ```

### Build Flags

- `-Wall -Wextra`: Enable all warnings
- `-g`: Debug symbols for GDB/Valgrind
- `-Iinclude`: Header search path
- `-pthread`: Thread support (future use)

---

## Parser & Grammar

### Grammar Definition (Grammar.cf)

```bnfc
-- Comments
comment "#" ;
comment "/*" "*/" ;

-- Program structure
Prog. Program ::= [Command] ;

-- Commands
CmdSimple. Command ::= SimpleCommand ;
CmdPipe.   Command ::= Command "|" SimpleCommand ;
CmdRedir.  Command ::= Command RedirOp String ;
CmdIf.     Command ::= "if" Command "then" [Command] "fi" ;

-- Simple commands with arguments
SimpleCmd. SimpleCommand ::= String [Arg] ;

-- Arguments (with expansion)
ArgString.  Arg ::= String ;
ArgVar.     Arg ::= "$" Ident ;
ArgArith.   Arg ::= "$((" ArithExpr "))" ;

-- Arithmetic expressions
ArithAdd. ArithExpr ::= ArithExpr "+" ArithExpr ;
ArithMul. ArithExpr ::= ArithExpr "*" ArithExpr ;
-- etc.

-- Lists
terminator Command ";" ;
separator Arg "" ;
```

### Parser Usage

```c
#include "Parser.h"

// Parse input string
Program ast = pProgram(parse_string(input));

// Check for parse errors
if (!ast) {
    fprintf(stderr, "Parse error\n");
    return;
}

// Evaluate
eval_program(ast, env);

// Cleanup
free_program(ast);
```

### AST Structure

```c
// Example AST nodes (in Absyn.h)
typedef struct Program {
    ListCommand commands;
} *Program;

typedef struct CmdSimple {
    SimpleCommand cmd;
} *Command;

typedef struct SimpleCmd {
    String name;
    ListArg args;
} *SimpleCommand;
```

---

## Evaluation Pipeline

### Evaluation Flow

1. **Parse**: Input → AST
2. **Expand**: Variables, arithmetic, globs
3. **Dispatch**: Built-in or external
4. **Execute**: Run command
5. **Collect**: Wait for status

### Variable Expansion

```c
// In expander.c
char* expand_variables(const char *str, Env *env) {
    // Find $VAR or ${VAR}
    // Look up in environment
    // Replace with value
    // Handle nested expansions
    return expanded;
}
```

### Arithmetic Evaluation

```c
// In arithmetic.c
int eval_arithmetic(ArithExpr expr, Env *env) {
    switch (expr->kind) {
        case ArithAdd:
            return eval_arithmetic(expr->left, env) + 
                   eval_arithmetic(expr->right, env);
        case ArithVar:
            return atoi(env_get(env, expr->name));
        // etc.
    }
}
```

### Command Execution

```c
// In executor.c
int execute_command(char **argv, Env *env) {
    // Check built-ins first
    if (is_builtin(argv[0])) {
        return execute_builtin(argv, env);
    }
    
    // Check integrated tools
    if (is_tool(argv[0])) {
        return execute_tool(argv, env);
    }
    
    // External command - fork/exec
    pid_t pid = fork();
    if (pid == 0) {
        execvp(argv[0], argv);
        // Handle exec failure
    }
    
    // Parent waits
    int status;
    waitpid(pid, &status, 0);
    return WEXITSTATUS(status);
}
```

### Pipeline Setup

```c
// In executor.c
int execute_pipeline(Command *commands, int count) {
    int pipes[count-1][2];
    pid_t pids[count];
    
    // Create pipes
    for (int i = 0; i < count-1; i++) {
        pipe(pipes[i]);
    }
    
    // Fork and exec each command
    for (int i = 0; i < count; i++) {
        pids[i] = fork();
        if (pids[i] == 0) {
            // Setup stdin from previous pipe
            if (i > 0) {
                dup2(pipes[i-1][0], STDIN_FILENO);
            }
            // Setup stdout to next pipe
            if (i < count-1) {
                dup2(pipes[i][1], STDOUT_FILENO);
            }
            // Close all pipes
            // Exec command
        }
    }
    
    // Close all pipes in parent
    // Wait for all children
    // Return last exit status
}
```

---

## Job Control System

The shell implements comprehensive job control for managing background and stopped processes.

### Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                      Job Control Flow                       │
└─────────────────────────────────────────────────────────────┘

User Command
     ↓
Parse & Detect '&' operator
     ↓
Fork & Execute
     ↓
   ┌─────┴─────────┐
   ↓               ↓
Background     Foreground
   ↓               ↓
jobs_add()    Set foreground_job_pid
   ↓               ↓
Return to     Wait with WUNTRACED
  Shell           ↓
   ↓         Detect Ctrl+Z/Ctrl+C
   ↓               ↓
SIGCHLD      Update job status
Handler           ↓
   ↓         jobs_add() if stopped
   ↓               ↓
jobs_update() Clear foreground_job_pid
   ↓               ↓
jobs_cleanup() Return to shell
```

### Core Components

#### 1. Job Data Structure (include/jobs.h)

```c
typedef enum {
    JOB_RUNNING,
    JOB_STOPPED,
    JOB_DONE
} JobStatus;

typedef struct {
    int job_id;          // Sequential job ID [1, 2, 3...]
    pid_t pid;           // Process ID
    char *command;       // Full command string
    JobStatus status;    // Current status
    int background;      // 1 if background, 0 if foreground
} Job;

typedef struct {
    Job jobs[MAX_JOBS];  // Array of jobs
    int count;           // Current number of jobs
    int next_job_id;     // Next job ID to assign
} JobList;
```

**Design decisions:**
- Fixed-size array (MAX_JOBS=64) for simplicity
- Sequential job IDs that can be reused
- Command string copied with strdup()
- Background flag tracks original execution mode

#### 2. Signal Handling (src/jobs/signals.c)

**Global state:**
```c
volatile sig_atomic_t child_exited = 0;  // SIGCHLD flag
pid_t foreground_job_pid = 0;             // Current foreground job
```

**Signal handlers:**

**SIGCHLD Handler:**
```c
void sigchld_handler(int sig) {
    int saved_errno = errno;
    child_exited = 1;  // Set flag for main loop
    errno = saved_errno;
}
```
- Minimal, async-signal-safe
- Only sets flag, actual work done in main loop
- Saves/restores errno

**SIGTSTP Handler (Ctrl+Z):**
```c
void sigtstp_handler(int sig) {
    int saved_errno = errno;
    if (foreground_job_pid > 0) {
        kill(-foreground_job_pid, SIGTSTP);  // Forward to process group
    }
    // Ignore if no foreground job (don't stop shell)
    errno = saved_errno;
}
```
- Forwards signal to foreground job only
- Uses negative PID to send to process group
- Shell ignores signal when at prompt

**SIGINT Handler (Ctrl+C):**
```c
void sigint_handler(int sig) {
    int saved_errno = errno;
    if (foreground_job_pid > 0) {
        kill(-foreground_job_pid, SIGINT);  // Forward to process group
    } else {
        write(STDOUT_FILENO, "\n", 1);  // Newline at prompt
    }
    errno = saved_errno;
}
```
- Forwards to foreground job if present
- Prints newline if at shell prompt
- Uses write() (async-signal-safe)

**Setup Function:**
```c
void setup_signal_handlers(void) {
    struct sigaction sa;
    
    // SIGINT
    sa.sa_handler = sigint_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sigaction(SIGINT, &sa, NULL);
    
    // SIGTSTP
    sa.sa_handler = sigtstp_handler;
    sigaction(SIGTSTP, &sa, NULL);
    
    // SIGCHLD
    sa.sa_handler = sigchld_handler;
    sa.sa_flags = SA_RESTART;  // Note: no SA_NOCLDSTOP
    sigaction(SIGCHLD, &sa, NULL);
    
    // Ignore terminal output/input signals
    signal(SIGTTOU, SIG_IGN);
    signal(SIGTTIN, SIG_IGN);
}
```

#### 3. Job Management API (src/jobs/jobs.c)

**Core functions:**

```c
void jobs_init(void);                    // Initialize job list
int jobs_add(pid_t pid, const char *cmd, int bg);  // Add new job
Job *jobs_get(int job_id);               // Get job by ID
Job *jobs_get_by_index(int index);       // Get job by array index
int jobs_count(void);                    // Count active jobs
void jobs_update_status(void);           // Reap zombies, update status
int jobs_cleanup(void);                  // Remove DONE jobs
void jobs_remove(int job_id);            // Remove specific job
const char *job_status_to_string(JobStatus s);  // Status to string
```

**Key implementation details:**

**jobs_add():**
- Assigns sequential job_id
- Copies command string with strdup()
- Returns job_id on success

**jobs_update_status():**
- Called when child_exited flag is set
- Uses waitpid(-1, &status, WNOHANG) to reap all zombies
- Updates job status based on WIFEXITED, WIFSIGNALED
- Marks completed jobs as JOB_DONE

**jobs_cleanup():**
- Removes all JOB_DONE jobs
- Prints "[N]+ Done cmd" notifications
- Frees command strings
- Returns count of jobs removed

**jobs_get_by_index():**
- Needed because job_id != array index
- Iterates array, skips invalid entries
- Prevents gaps from causing segfaults

#### 4. Executor Integration (src/evaluator/executor.c)

**Background job detection:**
```c
// In parse_pipeline()
int background = 0;
if (line ends with '&') {
    background = 1;
    strip '&' from line
}
// Set background flag on all commands
```

**Background job handling:**
```c
if (is_background) {
    // Don't wait for process
    pid_t job_pid = pids[count - 1];  // Track last process
    
    // Reconstruct command line
    char cmd_line[MAX_CMD_LEN];
    // ... build "cmd1 | cmd2 | cmd3 &"
    
    // Add to job list
    int job_id = jobs_add(job_pid, cmd_line, 1);
    printf("[%d] %d\n", job_id, job_pid);
    
    return 0;  // Return immediately
}
```

**Foreground job handling:**
```c
else {
    // Set foreground job for signal handling
    foreground_job_pid = pids[count - 1];
    
    // Wait with WUNTRACED to detect Ctrl+Z
    for (int i = 0; i < count; i++) {
        waitpid(pids[i], &status, WUNTRACED);
        
        if (WIFSTOPPED(status)) {
            // Job was stopped - add to job list
            jobs_add(pid, cmd_line, 0);
            job->status = JOB_STOPPED;
            printf("[%d]+  Stopped  %s\n", job_id, cmd);
            break;
        }
    }
    
    // Clear foreground job
    foreground_job_pid = 0;
}
```

#### 5. Built-in Commands

**jobs command (src/builtins/builtins.c):**
```c
int builtin_jobs(char **argv, Env *env) {
    // Parse options: -l, -p, -r, -s
    // Iterate jobs using jobs_get_by_index()
    // Format output based on options
    // Show job markers: + (most recent), - (second)
}
```

**fg command:**
```c
int builtin_fg(char **argv, Env *env) {
    // Parse job specification: %n or default to recent
    // Look up job with jobs_get()
    // Send SIGCONT if stopped
    // Set foreground_job_pid
    // Give terminal control: tcsetpgrp(STDIN_FILENO, pid)
    // Wait with WUNTRACED
    // Detect WIFSTOPPED for Ctrl+Z
    // Restore terminal: tcsetpgrp(STDIN_FILENO, getpgrp())
    // Clear foreground_job_pid
}
```

**bg command:**
```c
int builtin_bg(char **argv, Env *env) {
    // Parse job specification
    // Find stopped job
    // Send SIGCONT to resume
    // Update status to RUNNING
    // Mark as background
    // Print confirmation
}
```

### Main Loop Integration (src/main.c)

```c
while (1) {
    // Check if child exited
    if (child_exited) {
        child_exited = 0;
        jobs_update_status();  // Reap zombies
        jobs_cleanup();        // Remove done jobs
    }
    
    // Get input
    // Parse command
    // Execute command
}
```

### Process Groups

**Why process groups matter:**
- Signals must reach all processes in a pipeline
- `kill(-pid, signal)` sends to entire process group
- Each job becomes its own process group leader
- Pipeline: all commands in same process group

**Terminal control:**
- Foreground job "owns" the terminal
- `tcsetpgrp(fd, pgid)` gives terminal to process group
- Shell restores terminal control when job stops/completes
- Background jobs don't own terminal

### Signal Isolation Strategy

```
At Shell Prompt:
  Ctrl+C → sigint_handler → print newline
  Ctrl+Z → sigtstp_handler → ignore
  
Foreground Job Running:
  Ctrl+C → sigint_handler → kill(-fg_pid, SIGINT)
  Ctrl+Z → sigtstp_handler → kill(-fg_pid, SIGTSTP)
  
Background Job Running:
  Ctrl+C → sigint_handler → (job not affected)
  Ctrl+Z → sigtstp_handler → (job not affected)
```

**Implementation:**
- `foreground_job_pid` guards signal delivery
- 0 = shell in foreground, ignore signals
- non-zero = forward signals to that job

### Zombie Prevention

**Problem:** Child processes become zombies if not reaped

**Solution:**
1. SIGCHLD handler sets flag
2. Main loop checks flag
3. Call `jobs_update_status()`
4. Use `waitpid(-1, &status, WNOHANG)` to reap all
5. Update job status or remove

**Why in main loop, not signal handler:**
- Signal handlers must be minimal
- `jobs_update_status()` is not async-signal-safe
- Main loop is safe context for complex work

### Memory Management

**Job command strings:**
- Copied with `strdup()` in `jobs_add()`
- Freed in `jobs_remove()` and `jobs_cleanup()`
- Must free before removing job from array

**Cleanup on exit:**
- `atexit()` registers cleanup
- Iterates all jobs
- Frees all command strings
- No memory leaks

### Testing Strategy

**Automated tests (tests/job_control/):**
- test_background.sh: Background job creation
- test_jobs_cmd.sh: jobs command options
- test_fg_bg.sh: fg/bg error handling
- test_signals.sh: Zombie prevention
- test_integration.sh: Complete workflows

**Manual testing required:**
- Interactive Ctrl+Z behavior
- Interactive Ctrl+C behavior
- Terminal control with fg command
- Visual job completion notifications

**Valgrind testing:**
```bash
echo -e "sleep 1 &\nsleep 2\njobs\nexit" | \
  valgrind --leak-check=full ./ushell
```

### Common Pitfalls

1. **Iterating by job_id instead of index:**
   - Job IDs have gaps after removal
   - Use `jobs_get_by_index()` for iteration

2. **Not clearing foreground_job_pid:**
   - Must clear after job stops/completes
   - Otherwise next Ctrl+C goes to wrong process

3. **Terminal control errors:**
   - `tcsetpgrp()` fails if stdin not a terminal
   - Make errors non-fatal for pipe testing

4. **Race conditions in signal handlers:**
   - Keep handlers minimal
   - Only set flags
   - Do real work in main loop

5. **Memory leaks in command strings:**
   - Always free command before removing job
   - Check for leaks with valgrind

### Extending Job Control

**To add job name matching:**
1. Add string matching in fg/bg
2. Search job commands for substring
3. Return first match

**To add job groups:**
1. Track related jobs
2. Add group operations
3. Update jobs output format

**To add job priorities:**
1. Add nice/priority field to Job struct
2. Implement setpriority() calls
3. Add builtin to change priority

---

## Memory Management

### Allocation Strategy

1. **Parser**: Tree-based allocation
   - Each AST node allocated separately
   - Freed recursively via generated functions

2. **Environment**: Hash table
   - Keys and values strdup()'d
   - Freed on unset or cleanup

3. **Expansion**: Dynamic buffers
   - Temporary strings allocated
   - Freed after command execution

4. **Executor**: Stack/temporary
   - Argument arrays on stack where possible
   - Allocated arrays freed before return

### Cleanup Functions

```c
// Parser cleanup (generated)
void free_Program(Program p);
void free_Command(Command c);
void free_SimpleCommand(SimpleCommand s);

// Environment cleanup
void env_destroy(Env *env) {
    for (each entry) {
        free(entry->key);
        free(entry->value);
        free(entry);
    }
    free(env->table);
    free(env);
}

// Exit cleanup
void cleanup_shell(void) {
    if (shell_env) {
        env_destroy(shell_env);
    }
}
```

### Memory Leak Prevention

1. **Always pair malloc/free**
2. **Use atexit() for cleanup**
3. **Free AST after evaluation**
4. **Clean up temporary expansions**
5. **Test with valgrind regularly**

### Valgrind Testing

```bash
# Run with valgrind
valgrind --leak-check=full --show-leak-kinds=all ./ushell

# Expected output:
# HEAP SUMMARY:
#     in use at exit: 0 bytes in 0 blocks
#   total heap usage: N allocs, N frees, X bytes allocated
#
# LEAK SUMMARY:
#     definitely lost: 0 bytes in 0 blocks
```

---

## Adding Features

### Adding a New Built-in Command

1. **Declare function** in `include/builtins.h`:
   ```c
   int builtin_mycommand(char **argv, Env *env);
   ```

2. **Implement function** in `src/builtins/builtins.c`:
   ```c
   int builtin_mycommand(char **argv, Env *env) {
       // Implementation
       return 0; // Success
   }
   ```

3. **Register in builtins table** in `builtins.c`:
   ```c
   static Builtin builtins[] = {
       // ...existing built-ins...
       {"mycommand", builtin_mycommand},
       {NULL, NULL}
   };
   ```

4. **Add to help** in `builtin_help()`:
   ```c
   printf("  mycommand [args]   Description\n");
   ```

5. **Add tests** in `tests/integration/`:
   ```bash
   # test_mycommand.txt
   mycommand arg1 arg2
   ```

### Adding a New Tool

1. **Create source file** `src/tools/mynew.c`:
   ```c
   #include "tools.h"
   
   int mynew_main(int argc, char **argv, Env *env) {
       // Implementation
       return 0;
   }
   ```

2. **Declare in** `include/tools.h`:
   ```c
   int mynew_main(int argc, char **argv, Env *env);
   ```

3. **Register in** `src/tools/tools.c`:
   ```c
   Tool tools[] = {
       // ...existing tools...
       {"mynew", mynew_main},
       {NULL, NULL}
   };
   ```

4. **Add to Makefile**:
   ```makefile
   TOOL_SOURCES += src/tools/mynew.c
   ```

5. **Update documentation** in help command and USER_GUIDE.md

### Adding Grammar Features

1. **Update** `src/parser/Grammar.cf`:
   ```bnfc
   -- Add new production rule
   CmdMyFeature. Command ::= "myfeature" String ;
   ```

2. **Regenerate parser**:
   ```bash
   cd src/parser
   bnfc -c Grammar.cf
   make
   ```

3. **Update** `Absyn.c/h` if needed (usually automatic)

4. **Add evaluation** in `src/evaluator/evaluator.c`:
   ```c
   case is_CmdMyFeature:
       return eval_myfeature(cmd, env);
   ```

5. **Implement evaluator function**:
   ```c
   int eval_myfeature(Command *cmd, Env *env) {
       // Implementation
       return 0;
   }
   ```

### Adding Tests

1. **Create test file** in appropriate directory:
   - `tests/syntax/` for parser tests
   - `tests/semantic/` for execution tests
   - `tests/integration/` for complex scenarios

2. **Add to test script**:
   ```bash
   # In run_*_tests.sh
   run_test "My Feature" test_myfeature.txt
   ```

3. **Create expected output** if needed:
   ```bash
   # tests/expected/myfeature.txt
   Expected output here
   ```

---

## Testing Guidelines

### Test Organization

```
tests/
├── syntax/              # Parser tests (20)
│   ├── run_syntax_tests.sh
│   └── *.txt
├── semantic/            # Execution tests (10)
│   ├── run_sem_tests.sh
│   ├── expected/
│   └── *.txt
├── integration/         # Complex tests (14)
│   ├── run_integration_tests.sh
│   └── *.txt
└── valgrind_test.sh    # Memory tests (10)
```

### Writing Test Cases

#### Syntax Tests
Test parser correctness:
```bash
# tests/syntax/test_pipe.txt
echo hello | mycat
myls | myfd .c | mycat
```

#### Semantic Tests
Test execution correctness with expected output:
```bash
# tests/semantic/arithmetic.txt
echo $((5 + 3))

# tests/semantic/expected/arithmetic.txt
8
```

#### Integration Tests
Test complex interactions:
```bash
# tests/integration/test_complex.txt
set X=10
set Y=20
if echo test then
    echo Sum: $((X + Y))
fi
myls *.txt > files.txt
mycat files.txt
myrm files.txt
```

### Running Tests

```bash
# All tests
make test

# Individual suites
cd tests/syntax && ./run_syntax_tests.sh
cd tests/semantic && ./run_sem_tests.sh
cd tests/integration && ./run_integration_tests.sh

# Memory tests
./tests/valgrind_test.sh
```

### Testing Interactive Features

Interactive features require manual testing due to terminal I/O:

#### Terminal Module Testing
```bash
# Test multi-line wrapping
./ushell
> echo "very long command that should wrap to multiple lines in the terminal window..."

# Test cursor movement
> echo test<LEFT><LEFT><BACKSPACE>s
# Result: "echo test" → "echo tst"

# Test Ctrl+C cancellation
> some command^C
> # Should return to clean prompt
```

#### History Testing
```bash
# Test persistence
./ushell
> echo first
> echo second
> exit

./ushell
> <UP>         # Should show "echo second"
> <UP>         # Should show "echo first"

# Verify file
cat ~/.ushell_history
```

#### Completion Testing
```bash
# Test command completion
> ec<TAB>      # Should complete to "echo"

# Test filename completion
> touch testfile.txt
> cat te<TAB>  # Should complete to "cat testfile.txt"

# Test multiple matches
> my<TAB><TAB> # Should show all "my*" commands
```

#### Automated Interactive Testing
For CI/CD, use `expect` scripts:
```tcl
#!/usr/bin/expect
spawn ./ushell
expect "> "
send "echo test\r"
expect "test"
expect "> "
send "exit\r"
expect eof
```

### Test Requirements

DONE **Every new feature must have tests**  
DONE **Tests must pass before committing**  
DONE **Memory tests must show no leaks**  
DONE **Edge cases must be tested**  
DONE **Interactive features tested manually**

---

## Code Style Guide

### Naming Conventions

```c
// Functions: lowercase_with_underscores
int execute_command(char **argv);
void free_environment(Env *env);

// Types: PascalCase
typedef struct Environment Env;
typedef struct Command Command;

// Constants: UPPERCASE_WITH_UNDERSCORES
#define MAX_LINE 1024
#define MAX_ARGS 64

// Variables: lowercase_with_underscores
int exit_status;
char *current_dir;
```

### Indentation

- **4 spaces** (no tabs)
- K&R style braces
- One statement per line

```c
// Good
if (condition) {
    do_something();
    do_another();
}

// Bad
if(condition){
do_something();do_another();
}
```

### Comments

```c
// Single-line comments for brief explanations
int count = 0;  // Track iterations

/*
 * Multi-line comments for complex explanations:
 * - Describe the algorithm
 * - Note any assumptions
 * - Reference external docs
 */
int complex_function(int arg) {
    // Implementation
}

/**
 * Function documentation (Doxygen style)
 * @param argv Command arguments
 * @param env Shell environment
 * @return Exit status (0 = success)
 */
int builtin_cd(char **argv, Env *env);
```

### Function Organization

```c
// 1. Includes
#include <stdio.h>
#include "shell.h"

// 2. Defines
#define BUFFER_SIZE 1024

// 3. Types
typedef struct Data Data;

// 4. Forward declarations
static int helper_function(int arg);

// 5. Global variables (minimize)
static Env *global_env = NULL;

// 6. Functions
int main_function(void) {
    // Implementation
}

static int helper_function(int arg) {
    // Implementation
}
```

### Error Handling

```c
// Check return values
int fd = open(filename, O_RDONLY);
if (fd < 0) {
    perror("open");
    return -1;
}

// Check malloc
char *buffer = malloc(size);
if (!buffer) {
    fprintf(stderr, "Out of memory\n");
    return NULL;
}

// Use errno for system calls
if (execvp(argv[0], argv) < 0) {
    if (errno == ENOENT) {
        fprintf(stderr, "Command not found: %s\n", argv[0]);
    }
}
```

---

## Debugging

### GDB Usage

```bash
# Compile with debug symbols
make clean && make

# Start GDB
gdb ./ushell

# Common commands
(gdb) break main
(gdb) run
(gdb) next
(gdb) step
(gdb) print variable
(gdb) backtrace
(gdb) continue
```

### Valgrind Usage

```bash
# Memory leak detection
valgrind --leak-check=full ./ushell

# With suppressions
valgrind --leak-check=full --suppressions=ushell.supp ./ushell

# Generate suppression file
valgrind --gen-suppressions=all ./ushell
```

### Debug Macros

```c
#ifdef DEBUG
#define DEBUG_PRINT(fmt, ...) \
    fprintf(stderr, "DEBUG: " fmt "\n", ##__VA_ARGS__)
#else
#define DEBUG_PRINT(fmt, ...) /* nothing */
#endif

// Usage
DEBUG_PRINT("Variable value: %d", x);
```

### Common Issues

1. **Segmentation Fault**: Use GDB to find line
2. **Memory Leak**: Use valgrind to find allocation
3. **Pipe Broken**: Check file descriptor management
4. **Zombie Process**: Ensure all children are waited for

---

## Performance Considerations

### Optimization Strategies

1. **Minimize Allocations**
   - Use stack buffers when size is known
   - Reuse buffers where possible

2. **Efficient String Operations**
   - Use strncpy instead of strcpy
   - Pre-calculate string lengths

3. **Process Management**
   - Minimize fork() calls
   - Use built-ins instead of external commands

4. **I/O Buffering**
   - Use buffered I/O (stdio)
   - Batch write operations

### Profiling

```bash
# Compile with profiling
gcc -pg -o ushell $(SOURCES)

# Run to generate profile data
./ushell < test_script.txt

# Analyze
gprof ushell gmon.out > analysis.txt
```

### Benchmarking

```bash
# Time command execution
time ./ushell < benchmark.txt

# Compare with bash
time bash < benchmark.txt
```

---

## Summary

### Development Workflow

1. **Plan**: Design feature
2. **Implement**: Write code
3. **Test**: Add tests
4. **Debug**: Fix issues
5. **Document**: Update docs
6. **Review**: Check style
7. **Commit**: Version control

### Key Principles

DONE **Write tests first** (TDD when possible)  
DONE **Keep functions small** (< 50 lines)  
DONE **Handle all errors** (no silent failures)  
DONE **Free all allocations** (no leaks)  
DONE **Document complex logic** (help future you)  
DONE **Follow style guide** (consistency)

### Resources

- **POSIX**: System call reference
- **GNU libc**: Function documentation
- **BNFC**: Parser generator docs
- **Valgrind**: Memory debugging
- **GDB**: Interactive debugging

---

**End of Developer Guide**

For user documentation, see [USER_GUIDE.md](USER_GUIDE.md).  
For examples, see the [examples/](../examples/) directory.
