# Unified Shell (ushell)

A feature-rich Unix-like shell implementation with advanced parsing, custom command-line tools, and modern shell features.

## Project Status

DONE **Version 1.0.0 - Production Ready**

## Overview

The Unified Shell (ushell) is a fully-functional command-line interface that combines:
1. **Grammar-based Parser** - BNFC-generated parser with robust syntax support
2. **Custom Tools** - Suite of 10 integrated filesystem utilities
3. **Process Management** - Full fork/exec with pipelines and I/O redirection
4. **Advanced Features** - Variables, arithmetic, conditionals, and glob expansion

Built from the ground up using C, ushell demonstrates systems programming concepts including:
- Process creation and management (fork, exec, wait)
- Inter-process communication (pipes)
- File I/O and redirection
- Signal handling
- Memory management
- Lexical analysis and parsing

## Features

### Core Shell Features
- DONE **Interactive REPL** - Read-eval-print loop with dynamic prompt
- DONE **Command Execution** - Fork/exec with proper child process management
- DONE **Pipeline Support** - Multi-stage pipelines (cmd1 | cmd2 | cmd3)
- DONE **I/O Redirection** - Input (<), output (>), and append (>>)
- DONE **Signal Handling** - Ctrl+C interrupts commands, Ctrl+D exits gracefully
- DONE **Job Control** - Background jobs (&), job management (jobs, fg, bg), signal forwarding

### Interactive Features
- DONE **Line Editing** - Arrow keys for cursor movement within line
- DONE **Command History** - UP/DOWN arrows navigate previous commands
- DONE **History Persistence** - Commands saved to ~/.ushell_history
- DONE **Tab Completion** - Auto-complete commands and filenames
- DONE **Multi-line Support** - Long commands wrap correctly to next line
- DONE **Backspace Editing** - Delete characters during input
- DONE **Ctrl+C Handling** - Cancel current input without exiting shell
- DONE **Dynamic Terminal Width** - Adapts to terminal resize

### Variables & Expansion
- DONE **Variable Assignment** - `set VAR=value` for shell-local variables
- DONE **Environment Variables** - `export VAR=value` for child processes
- DONE **Variable Expansion** - `$VAR` and `${VAR}` syntax
- DONE **Arithmetic Evaluation** - `$((expression))` with +, -, *, /, %

### Control Flow
- DONE **Conditional Statements** - `if condition then commands fi`
- DONE **Exit Status Testing** - Commands in if conditions use exit codes

### Pattern Matching
- DONE **Glob Expansion** - `*` (any chars), `?` (single char)
- DONE **Character Classes** - `[abc]`, `[a-z]`, `[!abc]`
- DONE **Multi-pattern** - Multiple globs in single command

### Built-in Commands (17)
- DONE `cd [dir]` - Change directory (default: $HOME)
- DONE `pwd` - Print working directory
- DONE `echo [args...]` - Display arguments with variable expansion
- DONE `export VAR=value` - Set and export environment variable
- DONE `set VAR=value` - Set shell-local variable
- DONE `unset VAR` - Remove variable
- DONE `env` - Display all environment variables
- DONE `help [command]` - Display help message (general or command-specific)
- DONE `version` - Show version and build information
- DONE `history` - Show command history
- DONE `edi [file]` - Vi-like text editor (modal: normal/insert/command)
- DONE `apt` - Package manager (init, update, list, search, show, install, remove, verify)
- DONE `jobs [-l|-p|-r|-s]` - List background jobs with various formats
- DONE `fg [%n]` - Bring background/stopped job to foreground
- DONE `bg [%n]` - Resume stopped job in background
- DONE `commands` - List all available built-in commands
- DONE `exit` - Exit the shell

### Package Management System
- DONE **APT-like Package Manager** - Local package repository system
- DONE **Package Installation** - Install packages with dependency resolution
- DONE **Dependency Management** - Automatic dependency resolution and circular detection
- DONE **Package Removal** - Safe removal with dependent package warnings
- DONE **Package Verification** - Integrity checking for installed packages
- DONE **Repository Index** - Package metadata and search functionality
- DONE **Auto-install Flag** - `--auto-install` to automatically install dependencies
- DONE **Force Removal** - `--force` flag to bypass dependent package warnings

### Integrated Tools (10)
All tools accessible directly within the shell:
- DONE `myls [dir]` - List directory contents
- DONE `mycat file...` - Display file contents
- DONE `mycp src dest` - Copy files
- DONE `mymv src dest` - Move/rename files
- DONE `myrm file...` - Remove files
- DONE `mymkdir dir...` - Create directories
- DONE `myrmdir dir...` - Remove empty directories
- DONE `mytouch file...` - Create/update files
- DONE `mystat file...` - Display file status
- DONE `myfd pattern [dir]` - Find files by pattern

### AI Integration (Model Context Protocol)
- DONE **MCP Server** - TCP-based JSON protocol for AI agent interaction
- DONE **Tool Catalog** - 50+ shell commands exposed as structured tools
- DONE **Special AI Tools** - Context retrieval, command search, natural language suggestions
- DONE **Secure Execution** - Multi-layer security (blacklist, sanitization, resource limits)
- DONE **Progress Notifications** - Real-time execution status updates
- DONE **Audit Logging** - Complete command execution history

## Quick Start

### Installation

```bash
# Clone or navigate to the project directory
cd unified-shell

# Build the shell and all tools
make

# Run the shell
./ushell
```

### Threading Support

The shell supports multi-threaded execution of built-in commands for improved performance and responsiveness.

**Enable/Disable Threading:**
```bash
# Enable threading (default: enabled)
export USHELL_THREAD_BUILTINS=1
./ushell

# Disable threading (use traditional fork/exec)
export USHELL_THREAD_BUILTINS=0
./ushell

# Or unset the variable
unset USHELL_THREAD_BUILTINS
./ushell
```

**Configure Thread Pool:**
```bash
# Set custom thread pool size (default: 4 workers)
export USHELL_THREAD_POOL_SIZE=8
./ushell
```

**Benefits:**
- Faster execution of built-in commands
- Lower overhead compared to fork/exec
- Responsive shell during command execution
- Thread-safe access to history, jobs, and environment

### Help System

All built-in commands support the `--help` flag for detailed usage information:

```bash
# Get help for any command
cd --help
pwd --help
echo --help

# Use the help command
help              # List all commands
help cd           # Show help for cd
help export       # Show help for export

# Short form also works
pwd -h
```

### First Commands

```bash
# Try the help command
help

# Check version
version

# Initialize package system
apt init

# Update package index
apt update

# List available packages
apt list

# Install a package with dependencies
apt install mypackage --auto-install

# Set a variable
set NAME=World

# Use the variable
echo Hello $NAME

# Try arithmetic
echo The answer is: $((6 * 7))

# Use a pipeline
myls | myfd .c

# Change directory and check prompt
cd /tmp
pwd

# Exit the shell
exit
```

## AI Integration

The unified shell includes AI-powered command suggestions using natural language queries.

### Quick Start with AI

```bash
# Ask the AI for command suggestions using @ prefix
@list all python files

# The AI suggests a command
AI Suggestion: find . -name "*.py"

# Confirm to execute
Execute this command? (y/n/e): y
```

**Confirmation Options:**
- `y` - Execute the suggested command
- `n` - Cancel, return to prompt  
- `e` - Edit the suggestion before executing

### Configuration

**Heuristic Mode (Free, No Setup):**
```bash
# Set AI helper path
export USHELL_AI_HELPER=$PWD/aiIntegr/ushell_ai_venv.sh

# Start using @ queries immediately
./ushell
@list files
```

**OpenAI Mode (Intelligent, Requires API Key):**
```bash
# Get API key from https://platform.openai.com/api-keys
export OPENAI_API_KEY="sk-your-api-key-here"
export USHELL_LLM_MODEL="gpt-4o-mini"  # Optional, default model
export USHELL_AI_HELPER=$PWD/aiIntegr/ushell_ai_venv.sh

./ushell
@find all files larger than 1MB modified in last week
```

### Environment Variables

| Variable | Description | Default |
|----------|-------------|---------|
| `OPENAI_API_KEY` | OpenAI API key (optional) | None |
| `USHELL_LLM_MODEL` | AI model to use | `gpt-4o-mini` |
| `USHELL_AI_HELPER` | Path to AI helper script | `./aiIntegr/ushell_ai.py` |
| `USHELL_AI_DEBUG` | Enable debug output | `0` |
| `USHELL_AI_CONTEXT` | Share shell state with AI | `1` |

### Example Queries

```bash
@list all c files in src directory
@show files modified today
@count lines in main.c
@create a backup directory
@find executable files
@show disk usage
```

### Features

- **Dual Mode**: Heuristic (free) or OpenAI-powered (intelligent)
- **Safe by Default**: User confirmation required before execution
- **Command Catalog**: AI knows all built-in and tool commands
- **Context-Aware**: Optional shell state for better suggestions
- **Privacy Controls**: Transparent data usage, opt-out available
- **Cost Effective**: ~$0.0001-$0.0003 per query with gpt-4o-mini

For detailed documentation, see [aiIntegr/README.md](aiIntegr/README.md).

## MCP Server (Model Context Protocol)

The unified shell includes an MCP server that enables AI agents and external tools to interact with the shell programmatically via a structured JSON-RPC protocol.

### Quick Start with MCP

**1. Start the shell with MCP enabled:**
```bash
# Enable MCP server on default port (9000)
USHELL_MCP_ENABLED=1 ./ushell

# Or with custom port and audit logging
USHELL_MCP_ENABLED=1 USHELL_MCP_PORT=9001 \
  USHELL_MCP_AUDIT_LOG=/var/log/ushell_audit.log ./ushell
```

**2. Test with a simple client:**
```bash
# In another terminal, test connection
echo '{"id":"1","method":"initialize","params":{}}' | nc localhost 9000

# Or use the example Python client
cd examples
python3 mcp_client_example.py
```

**3. Try the AI agent example:**
```bash
# Run the mock AI agent
python3 examples/mcp_ai_agent_example.py "Find all Python files"
python3 examples/mcp_ai_agent_example.py "What's in my home directory?"
```

### Key Features

- **50+ Tools**: All built-in commands and utilities exposed as MCP tools
- **Special AI Tools**: 
  - `get_shell_context` - Get current shell state (cwd, user, history, env)
  - `search_commands` - Search for commands by keyword
  - `suggest_command` - Translate natural language to shell commands
- **Secure Execution**: Multi-layer security (blacklist, sanitization, resource limits)
- **Real-time Notifications**: Tool execution progress updates
- **Concurrent Operations**: Support for multiple simultaneous tool calls
- **Audit Logging**: Complete execution history for compliance

### Configuration

| Variable | Default | Description |
|----------|---------|-------------|
| `USHELL_MCP_ENABLED` | `0` | Enable MCP server (`1` = enabled) |
| `USHELL_MCP_PORT` | `9000` | TCP port for MCP server |
| `USHELL_MCP_AUDIT_LOG` | (none) | Path to audit log file |
| `USHELL_MCP_MAX_CLIENTS` | `5` | Maximum concurrent clients |
| `USHELL_MCP_TIMEOUT` | `30` | Client timeout in seconds |

### Protocol Example

```json
// Initialize session
{"id": "1", "method": "initialize", "params": {}}

// List available tools
{"id": "2", "method": "list_tools", "params": {}}

// Execute a tool
{"id": "3", "method": "call_tool", "params": {
  "tool": "ls",
  "args": {"path": "/home/user"}
}}

// Get shell context (special AI tool)
{"id": "4", "method": "call_tool", "params": {
  "tool": "get_shell_context",
  "args": {}
}}
```

### Security

The MCP server includes comprehensive security measures:

- **Command Blacklist**: Blocks dangerous commands (sudo, rm, chmod, etc.)
- **Input Sanitization**: Prevents command injection via metacharacter filtering
- **Path Validation**: Blocks directory traversal and system file access
- **Resource Limits**: CPU time (30s), memory (256MB), processes (10), file size (10MB)
- **Audit Logging**: JSON-formatted execution logs for monitoring

### Troubleshooting

**Connection Refused:**
```bash
# Ensure MCP is enabled
USHELL_MCP_ENABLED=1 ./ushell

# Check if port is in use
lsof -i :9000

# Try a different port
USHELL_MCP_PORT=9001 USHELL_MCP_ENABLED=1 ./ushell
```

**Command Blacklisted:**
```bash
# Use safe alternatives instead
# Instead of 'rm', use 'myrm'
# Instead of 'chmod', modify via tool interface
```

**Permission Denied:**
```bash
# Run as non-root user (recommended)
# Set proper file permissions
chmod +x ushell
```

For comprehensive documentation, see [docs/MCP_INTEGRATION.md](docs/MCP_INTEGRATION.md).

## Job Control

The unified shell includes comprehensive job control for managing background and stopped processes.

### Background Jobs

Run commands in the background by appending `&`:

```bash
# Start a long-running process in background
sleep 30 &
# Output: [1] 12345

# Continue using the shell immediately
echo "Shell is responsive"

# Start multiple background jobs
sleep 10 &    # [1]
sleep 20 &    # [2]
sleep 30 &    # [3]
```

### Listing Jobs

Use the `jobs` command to see all background and stopped jobs:

```bash
# Basic format
jobs
# [1]-  Running              sleep 10 &
# [2]+  Running              sleep 20 &

# Long format (with PIDs)
jobs -l
# [1]-  12345  Running              sleep 10 &
# [2]+  12346  Running              sleep 20 &

# Show PIDs only
jobs -p
# 12345
# 12346

# Show only running jobs
jobs -r

# Show only stopped jobs
jobs -s
```

**Job Markers:**
- `+` = Most recent job (default for fg/bg)
- `-` = Second most recent job

### Signal Handling

**Ctrl+Z** - Stop (suspend) foreground process:
```bash
# Start a command
sleep 60

# Press Ctrl+Z
^Z
[1]+  Stopped                 sleep 60

# Job is now stopped, shell prompt returns
```

**Ctrl+C** - Interrupt (terminate) foreground process:
```bash
# Start a command
sleep 60

# Press Ctrl+C
^C

# Command terminated, shell continues
```

**Note:** Ctrl+C and Ctrl+Z only affect foreground jobs, not the shell itself.

### Foreground Command (fg)

Bring a background or stopped job to the foreground:

```bash
# Start a background job
sleep 30 &
[1] 12345

# Bring it to foreground
fg
# or specify job number
fg %1

# Job now runs in foreground (blocking shell)
```

**Usage:**
- `fg` - Foreground most recent job
- `fg %n` - Foreground job number n
- `fg %job` - Also accepts without %

### Background Command (bg)

Resume a stopped job in the background:

```bash
# Start a foreground job
sleep 60

# Stop it with Ctrl+Z
^Z
[1]+  Stopped                 sleep 60

# Resume it in background
bg
[1]+ sleep 60 &

# Job continues, shell prompt available
```

**Usage:**
- `bg` - Resume most recent stopped job
- `bg %n` - Resume stopped job number n

### Complete Workflow Example

```bash
# 1. Start a long process
sleep 100

# 2. Stop it (Ctrl+Z)
^Z
[1]+  Stopped                 sleep 100

# 3. Start another process in background  
sleep 50 &
[2] 12347

# 4. List all jobs
jobs
[1]+  Stopped                 sleep 100
[2]-  Running                 sleep 50 &

# 5. Resume first job in background
bg %1
[1]+ sleep 100 &

# 6. Bring second job to foreground
fg %2

# 7. Interrupt it (Ctrl+C)
^C

# 8. Check remaining jobs
jobs
[1]+  Running                 sleep 100 &
```

### Job Completion

When background jobs complete, you'll see notification:
```bash
sleep 2 &
[1] 12345

# After 2 seconds, on next command or Enter:
[1]+  Done                    sleep 2 &
```

### Advanced Features

**Pipeline as Background Job:**
```bash
# Run entire pipeline in background
myls | myfd .c | mycat &
[1] 12345
```

**Signal Isolation:**
- Shell ignores Ctrl+C and Ctrl+Z when at prompt
- Signals only sent to foreground processes
- Background jobs immune to Ctrl+C
- Process groups handle pipelines correctly

**Zombie Prevention:**
- Automatic cleanup of completed jobs
- No defunct processes accumulate
- SIGCHLD handler reaps zombies

## Package Management (APT)

The unified shell includes a fully-functional package management system modeled after APT/dpkg.

### Overview

The APT subsystem provides:
- **Local package repository** in `~/.ushell/`
- **Dependency resolution** with circular dependency detection
- **Package installation** with automatic extraction and PATH setup
- **Package removal** with dependent package warnings
- **Package verification** to check integrity
- **Package search** and metadata display

### Quick Start with APT

```bash
# Initialize the package system (first time only)
apt init

# Update the package index
apt update

# List all available packages
apt list

# List only installed packages
apt list --installed

# Search for packages
apt search keyword

# Show package details
apt show packagename

# Install a package (checks dependencies)
apt install packagename

# Install with automatic dependency resolution
apt install packagename --auto-install

# Remove a package
apt remove packagename

# Force remove (skip dependent warnings)
apt remove packagename --force

# Verify package integrity
apt verify packagename
```

### APT Directory Structure

```
~/.ushell/
├── packages/           # Installed packages
│   └── pkgname/
│       ├── bin/        # Executable files
│       ├── lib/        # Library files (if any)
│       └── METADATA    # Package metadata
├── repo/
│   ├── available/      # Package archives (.tar.gz)
│   ├── cache/          # Downloaded packages (future)
│   └── index.txt       # Package index
└── apt.conf            # Configuration file
```

### Package Format

Packages are `.tar.gz` archives with this structure:
```
packagename-version/
├── bin/                # Executables
│   └── command1
└── lib/                # Libraries (optional)
    └── library.so
```

After installation, a `METADATA` file is created:
```
Name: packagename
Version: 1.0
Description: Package description
InstallDate: 2025-11-25 12:34:56
Filename: packagename-1.0.tar.gz
Depends: dep1, dep2
```

### Dependency Management

APT supports automatic dependency resolution:

**Dependency Declaration:**
In `~/.ushell/repo/index.txt`:
```
PackageName: mypackage
Version: 1.0
Description: My package
Depends: basepkg, libpkg
Filename: mypackage-1.0.tar.gz
```

**Installing with Dependencies:**
```bash
# Manual: Install dependencies first
apt install basepkg
apt install libpkg
apt install mypackage

# Automatic: Use --auto-install flag
apt install mypackage --auto-install
```

**Circular Dependency Detection:**
APT detects circular dependencies and prevents infinite loops:
```bash
apt install circpkg --auto-install
# Output: Circular dependency detected: circpkg -> dep1 -> circpkg
#         Cannot resolve dependencies due to circular dependency
```

### PATH Integration

Installed package binaries are automatically added to PATH on shell startup:
```bash
# After installing a package with bin/mycommand
./ushell

# The command is now available
mycommand
```

### Package Removal Safety

APT warns about dependent packages before removal:
```bash
apt remove basepkg
# Output: WARNING: The following packages depend on 'basepkg':
#           mypackage, otherpkg
#         
#         Removing 'basepkg' may break these packages.
#         Consider removing dependent packages first, or use --force flag.
```

Use `--force` to bypass warnings:
```bash
apt remove basepkg --force
# Skips dependent checking
```

### APT Commands Reference

| Command | Description | Flags |
|---------|-------------|-------|
| `apt init` | Initialize package system | - |
| `apt update` | Refresh package index | - |
| `apt list` | List packages | `--installed` |
| `apt search <term>` | Search for packages | - |
| `apt show <pkg>` | Show package details | - |
| `apt install <pkg>` | Install a package | `--auto-install` |
| `apt remove <pkg>` | Remove a package | `--force` |
| `apt verify <pkg>` | Verify package integrity | - |
| `apt help` | Show APT help | - |

## Build Instructions

### Requirements
- GCC compiler
- GNU Make
- Standard C library
- POSIX environment (Linux, macOS, Unix)

### Building

```bash
# Full build (shell + all tools)
make

# Build only the shell
make ushell

# Clean build artifacts
make clean

# Rebuild from scratch
make clean && make
```

### Build Output
- **ushell** - Main shell executable (~260KB)
- **Tool binaries** - 10 integrated utilities (in src/tools/)
- **Object files** - Compiled modules (*.o)

### Compilation Flags
- `-Wall -Wextra` - All warnings enabled
- `-g` - Debug symbols included
- `-pthread` - Thread support for future extensions

## Testing

### Test Suites

The project includes comprehensive testing:

1. **Syntax Tests (20 tests)** - Parser validation
   ```bash
   cd tests/syntax
   ./run_syntax_tests.sh
   ```

2. **Semantic Tests (10 tests)** - Execution validation
   ```bash
   cd tests/semantic
   ./run_sem_tests.sh
   ```

3. **Integration Tests (14 tests)** - End-to-end scenarios
   ```bash
   cd tests/integration
   ./run_integration_tests.sh
   ```

4. **Memory Tests (10 tests)** - Valgrind leak detection
   ```bash
   cd tests
   ./valgrind_test.sh
   ```

### Test Results

DONE **44/44 functional tests passing**  
DONE **10/10 memory tests passing (0 leaks)**  
DONE **Valgrind clean** - All memory properly freed

### Manual Testing

```bash
# Start the shell
./ushell

# Test variables
set X=10
set Y=20
echo X=$X, Y=$Y

# Test arithmetic
echo Result: $((X + Y))

# Test conditionals
if myls /tmp then echo Found /tmp fi

# Test pipelines
myls | myfd .c | mycat

# Test glob expansion
echo *.c
myls test*.txt

# Test I/O redirection
echo "test data" > output.txt
mycat < output.txt
echo "more data" >> output.txt
```

## Architecture Overview

### High-Level Design

```
User Input → Lexer → Parser → AST → Evaluator → Executor
                                        ↓
                              ┌─────────┴─────────┐
                              ↓                   ↓
                          Built-ins           External Commands
                              ↓                   ↓
                          (cd, echo, ...)    (fork/exec)
```

### Key Components

1. **Parser (BNFC-generated)**
   - Lexical analysis: `Grammar.l` (Flex)
   - Syntax analysis: `Grammar.y` (Bison)
   - Abstract syntax tree: `Absyn.c/h`

2. **Evaluator**
   - Command evaluation: `evaluator.c`
   - Variable expansion: `expansion.c`
   - Expression evaluation: `arithmetic.c`
   - Conditional execution: `conditional.c`

3. **Executor**
   - Process management: `executor.c`
   - Pipeline handling: Multi-fork coordination
   - I/O redirection: dup2() management

4. **Built-ins**
   - Command table: `builtins.c`
   - Individual implementations: cd, pwd, echo, history, edi, apt, etc.
   - Integrated text editor: `builtin_edi.c` (731 lines)
   - Package manager: `apt_builtin.c` with full subcommand support

5. **Package Management (APT)**
   - Repository system: `apt/repo.c` (package index, metadata)
   - Installation engine: `apt/install.c` (extraction, PATH setup)
   - Removal system: `apt/remove.c` (recursive removal, verification)
   - Dependency resolver: `apt/depends.c` (circular detection, auto-install)
   - Directory structure: `~/.ushell/` (packages, repo, available, cache)
   - Features: dependency resolution, integrity checking, force removal

6. **Tools Integration**
   - Argtable3-based argument parsing
   - 10 filesystem utilities with full option support
   - Shared environment access
   - Tool dispatcher: `tool_dispatch.c`

7. **Glob Engine**
   - Pattern matching: `glob.c`
   - Wildcard expansion: *, ?, [...]
   - Directory traversal

8. **Interactive System**
   - Terminal management: `terminal.c` (raw mode, ANSI escapes)
   - Command history: `history.c` (persistent to ~/.ushell_history)
   - Tab completion: `completion.c` (commands and filenames)
   - Line editing with multi-line wrapping support

### Memory Management

- **Parser**: Tree-based allocation with cleanup functions
- **Environment**: Hash table with dynamic string allocation
- **Executor**: Temporary buffers freed after each command
- **Cleanup**: atexit() handlers ensure no leaks

## Documentation

- **[User Guide](docs/USER_GUIDE.md)** - Comprehensive usage documentation
- **[Developer Guide](docs/DEVELOPER_GUIDE.md)** - Architecture and contribution guide
- **[Example Scripts](examples/)** - Sample code and tutorials
- [Implementation Plan](../Plan.md) - Step-by-step roadmap
- [Implementation Prompts](../Prompts.md) - Development history

## Examples

See the [examples/](examples/) directory for:
- `example_scripts.sh` - Basic command examples
- `tutorial.txt` - Step-by-step tutorial
- `advanced_examples.sh` - Complex use cases

## Directory Structure

```
unified-shell/
├── src/
│   ├── parser/         # BNFC-generated parser (Grammar.cf)
│   ├── evaluator/      # Command evaluation & execution
│   │   ├── environment.c   # Variable storage
│   │   ├── executor.c      # Command/pipeline execution
│   │   ├── conditional.c   # If/then/else logic
│   │   └── arithmetic.c    # Arithmetic evaluation
│   ├── builtins/       # Built-in command implementations
│   │   ├── builtins.c      # Dispatch table
│   │   └── builtin_edi.c   # Integrated text editor
│   ├── apt/            # Package management system
│   │   ├── apt_builtin.c   # APT command dispatcher
│   │   ├── repo.c          # Repository & index management
│   │   ├── install.c       # Package installation engine
│   │   ├── remove.c        # Package removal & verification
│   │   └── depends.c       # Dependency resolution system
│   ├── tools/          # 10 integrated utilities (argtable3-based)
│   ├── glob/           # Wildcard expansion engine
│   ├── utils/          # Helper functions
│   │   ├── expansion.c     # Variable expansion
│   │   ├── terminal.c      # Interactive input handling
│   │   ├── history.c       # Command history
│   │   ├── completion.c    # Tab completion
│   │   └── arg_parser.c    # Argument parsing utilities
│   ├── argtable3/      # Third-party argument parsing library
│   └── main.c          # REPL and main loop
├── include/            # Public header files
├── tests/
│   ├── syntax/         # Parser tests (20)
│   ├── semantic/       # Execution tests (10)
│   ├── integration/    # End-to-end tests (14)
│   └── valgrind_test.sh # Memory leak tests (10)
├── docs/
│   ├── USER_GUIDE.md        # User documentation
│   └── DEVELOPER_GUIDE.md   # Developer documentation
├── examples/           # Sample scripts and tutorials
├── Makefile           # Build system
└── README.md          # This file
```

## Contributing

### Code Style
- K&R style indentation (4 spaces)
- Function names: lowercase_with_underscores
- Constants: UPPERCASE_WITH_UNDERSCORES
- Comprehensive comments for complex logic

### Adding Features
1. Update grammar in `src/parser/Grammar.cf` if needed
2. Regenerate parser: `cd src/parser && make`
3. Add evaluation logic in `src/evaluator/`
4. Add tests in appropriate `tests/` subdirectory
5. Update documentation

### Testing Requirements
- All new features must include tests
- Run full test suite before submitting
- Verify valgrind clean (no memory leaks)
- Test on Linux environment

See [DEVELOPER_GUIDE.md](docs/DEVELOPER_GUIDE.md) for detailed information.

## Known Limitations

- No job control (background processes with &)
- No alias or function definitions
- Single-threaded execution
- Limited to POSIX systems
- Very long lines (>200 chars) may have display artifacts on some terminals

## Future Enhancements

Potential improvements:
- Background job control (& operator)
- Scripting mode (execute from file)
- Configuration file (~/.ushellrc)
- Additional built-ins (alias, functions)
- Signal forwarding to child processes
- More sophisticated terminal width detection

## Project Status

**Current Version**: 1.0.0  
**Build Date**: November 2025  
**Status**: Production Ready

### Implementation Progress
- DONE All 20 prompts completed
- DONE All features implemented
- DONE All tests passing
- DONE Memory leak-free
- DONE Documentation complete

## License

Educational project for UCSC Silicon Valley Linux Systems coursework.

## Acknowledgments

- BNFC (BNF Converter) for parser generation
- GNU tools (GCC, Make, Flex, Bison)
- POSIX standards for system call interfaces

## Contact

For questions or contributions, please refer to the project documentation or contact the course instructor.
