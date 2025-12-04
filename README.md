# Unified Shell - Feature-Complete Linux Shell

A comprehensive Unix-like shell implementation that combines advanced parsing, custom command-line tools, and modern shell features into a single, cohesive system.

## Project Overview

This project unifies three separate shell-related implementations into a single, feature-rich shell:

1. **Grammar-based Parser** - BNFC-generated parser with variable assignment, arithmetic evaluation, and conditionals
2. **Custom Tools** - Suite of filesystem utilities and a vi-like text editor
3. **Process Management** - Full fork/exec implementation with pipelines and I/O redirection

### Key Features

#### Core Shell Features
- DONE **REPL (Read-Eval-Print Loop)** - Interactive command-line interface
- DONE **Command Execution** - Fork/exec process management
- DONE **Pipeline Support** - Multi-stage command pipelines (`cmd1 | cmd2 | cmd3`)
- DONE **I/O Redirection** - Input (`<`), output (`>`), and append (`>>`)
- DONE **Background Jobs** - Async command execution with `&`

#### Variable System
- DONE **Variable Assignment** - `var=value` syntax
- DONE **Variable Expansion** - `$var` and `${var}` syntax
- DONE **Arithmetic Evaluation** - `$((expression))` with +, -, *, /, %
- DONE **System Environment** - Access to system environment variables

#### Control Flow
- DONE **Conditionals** - `if command then commands fi`
- DONE **If/Else** - `if command then commands else commands fi`
- DONE **Exit Status** - Commands return proper exit codes

#### Built-in Commands
- `cd` - Change directory
- `pwd` - Print working directory
- `echo` - Display text and variables
- `export` - Set environment variables
- `set` - Assign shell variables
- `exit` - Exit the shell
- `help` - Display command help
- `version` - Show version information

#### Integrated Filesystem Tools
All tools run as built-ins (no fork overhead):
- `myls` - List directory contents (with -l, -a flags)
- `mycat` - Display file contents
- `mycp` - Copy files and directories (-r, -i flags)
- `mymv` - Move/rename files (-i flag)
- `myrm` - Remove files and directories (-r, -i flags)
- `mymkdir` - Create directories (-p flag)
- `myrmdir` - Remove empty directories
- `mytouch` - Create files or update timestamps
- `mystat` - Display detailed file information
- `myfd` - Advanced file finder with pattern matching

#### Text Editor
- `edi` - Vi-like modal text editor
  - NORMAL, INSERT, and COMMAND modes
  - File I/O with `:w`, `:q`, `:wq`, `:q!`
  - hjkl navigation
  - Raw terminal control (no ncurses dependency)

#### Advanced Features
- DONE **Wildcard Expansion** - `*`, `?`, `[abc]`, `[a-z]` glob patterns
- DONE **Hidden File Support** - `.` prefix handling
- DONE **Escaped Characters** - `\*`, `\?` literal matching
- DONE **Memory Safety** - Valgrind-clean implementation
- DONE **Error Handling** - Clear error messages and status codes

---

## Quick Start

### Prerequisites
```bash
# Required tools
gcc --version      # GCC compiler
make --version     # GNU Make
bnfc --version     # BNFC 2.9.5+ (if using grammar parser)
flex --version     # Flex 2.6+
bison --version    # Bison 3.8+
```

### Build & Run
```bash
# Clone/navigate to project directory
cd unified-shell

# Build
make

# Run interactive shell
./ushell

# Run with script
./ushell < script.sh

# Run with command
echo "echo hello world" | ./ushell
```

### Quick Test
```bash
# Start shell
./ushell

# Try basic commands
unified-shell> echo Hello World
Hello World

# Variables
unified-shell> export name=Alice
unified-shell> echo Hello, $name
Hello, Alice

# Arithmetic
unified-shell> export x=10
unified-shell> export y=$((x * 2 + 5))
unified-shell> echo $y
25

# Conditionals
unified-shell> if /bin/true then echo Success fi
Success

# Pipelines
unified-shell> echo -e "apple\nbanana\ncherry" | grep a | sort
apple
banana

# File operations
unified-shell> echo "test content" > test.txt
unified-shell> mycat test.txt
test content

# Wildcards
unified-shell> echo *.txt
test.txt output.txt result.txt

# Exit
unified-shell> exit
```

---

## Project Structure

```
unified-shell/
├── src/
│   ├── main.c                 # REPL and main entry point
│   ├── parser/                # BNFC-generated parser
│   │   ├── Grammar.cf         # BNF grammar definition
│   │   ├── Parser.c/h         # Generated parser
│   │   ├── Lexer.c            # Generated lexer
│   │   └── Absyn.c/h          # Abstract syntax tree
│   ├── evaluator/             # Command evaluation
│   │   ├── executor.c         # Fork/exec logic
│   │   ├── environment.c      # Variable storage
│   │   ├── conditional.c      # If/then/else evaluation
│   │   └── arithmetic.c       # Arithmetic expression parsing
│   ├── builtins/              # Built-in commands
│   │   └── builtins.c         # cd, pwd, echo, export, etc.
│   ├── tools/                 # Integrated utilities
│   │   ├── tool_dispatch.c    # Tool routing
│   │   ├── myls.c             # Directory listing
│   │   ├── mycat.c            # File display
│   │   ├── mycp.c, mymv.c     # File operations
│   │   ├── myrm.c             # File deletion
│   │   ├── mymkdir.c, myrmdir.c  # Directory operations
│   │   ├── mytouch.c          # File creation
│   │   ├── mystat.c           # File info
│   │   ├── myfd.c             # File finder
│   │   └── editor.c           # Text editor
│   ├── glob/                  # Wildcard expansion
│   │   └── glob.c             # Pattern matching
│   └── utils/                 # Helper functions
│       └── expansion.c        # Variable expansion
├── include/                   # Header files
├── tests/                     # Test suite
│   ├── test_runner.sh         # Main test script
│   ├── unit/                  # Unit tests
│   └── integration/           # Integration tests
├── docs/                      # Documentation
│   ├── USER_GUIDE.md          # User documentation
│   └── DEVELOPER_GUIDE.md     # Developer documentation
├── examples/                  # Example scripts
├── Makefile                   # Build configuration
└── README.md                  # This file
```

---

## Testing

### Run Test Suite
```bash
cd tests
./test_runner.sh
```

### Individual Test Categories
```bash
# Unit tests
cd tests/unit
./test_env.sh         # Environment/variables
./test_expansion.sh   # Variable expansion
./test_glob.sh        # Wildcard expansion
./test_arithmetic.sh  # Arithmetic evaluation

# Integration tests
cd tests/integration
./test_pipelines.sh      # Pipeline execution
./test_conditionals.sh   # If/then/else
./test_tools.sh          # All built-in tools
./test_redirection.sh    # I/O redirection
```

### Memory Testing
```bash
# Run with valgrind
echo "echo test; exit" | valgrind --leak-check=full ./ushell

# Or use test script
cd tests
./valgrind_test.sh
```

---

## 📖 Usage Examples

### Variables and Arithmetic
```bash
# Variable assignment
export x=10
export name="Alice"

# Variable expansion
echo $x
echo Hello, $name

# Arithmetic
export result=$((5 + 3))
export area=$((width * height))
echo $result

# Complex expressions
export a=10
export b=20
export sum=$(($a + $b))
export product=$(($a * $b))
```

### Conditionals
```bash
# Simple if/then
if /bin/true then echo Success fi

# If/else
if /bin/false then echo Yes else echo No fi

# With variables
export status=0
if test $status -eq 0 then echo OK else echo FAIL fi

# Command-based conditions
if myls README.md then echo "File exists" fi
```

### Pipelines and Redirection
```bash
# Simple pipeline
echo hello | cat

# Multi-stage pipeline
myls | grep .txt | sort

# Input redirection
mycat < input.txt

# Output redirection
echo "output" > file.txt

# Append redirection
echo "more" >> file.txt

# Combined
mycat < input.txt | grep pattern > output.txt
```

### Wildcards
```bash
# Match all .txt files
echo *.txt

# Match single character
echo file?.txt

# Character class
echo file[123].txt

# Range
echo [a-z]*.txt

# Negation
echo file[!0-9].txt

# Use with commands
mycat *.txt
myrm temp*
myls dir[0-9]
```

### Job Control
```bash
# Run command in background
sleep 10 &
[1] 12345

# Start multiple background jobs
find / -name "*.txt" > results.txt 2>&1 &
tar czf archive.tar.gz largedir/ &

# List all jobs
jobs
# [1]   Running                 sleep 10 &
# [2]-  Running                 find / -name "*.txt" > results.txt 2>&1 &
# [3]+  Running                 tar czf archive.tar.gz largedir/ &

# Bring job to foreground
fg %1

# Resume stopped job in background
bg %2

# Stop foreground job with Ctrl+Z
sleep 60
^Z
[1]+  Stopped                 sleep 60

# Resume it in background
bg %1
[1]+ sleep 60 &

# Signal handling
# - Ctrl+C: Terminates foreground job only (shell is immune)
# - Ctrl+Z: Stops foreground job only (shell continues)
```

### File Operations
```bash
# List directory
myls
myls -la /tmp

# Display file
mycat file.txt

# Copy files
mycp source.txt dest.txt
mycp -r dir1 dir2

# Move/rename
mymv old.txt new.txt

# Delete
myrm file.txt
myrm -r directory

# Create directory
mymkdir newdir
mymkdir -p path/to/nested/dir

# Create file
mytouch newfile.txt

# File info
mystat file.txt

# Find files
myfd *.c
myfd -r pattern
```

### Text Editor
```bash
# Edit existing file
edi file.txt

# Create new file
edi newfile.txt

# Editor commands (in COMMAND mode):
# :w         - Save
# :w file    - Save as
# :q         - Quit
# :wq        - Save and quit
# :q!        - Quit without saving
```

---

## Architecture

### Component Overview

#### 1. **Parser** (src/parser/)
- BNFC-generated LL parser from Grammar.cf
- Produces abstract syntax tree (AST)
- Handles syntax errors gracefully
- Supports: commands, variables, conditionals, pipelines, redirection

#### 2. **Evaluator** (src/evaluator/)
- Traverses AST and executes commands
- Manages variable environment
- Handles arithmetic evaluation
- Implements conditional logic
- Performs glob expansion

#### 3. **Executor** (src/evaluator/executor.c)
- Fork/exec process management
- Pipeline construction with pipe()
- I/O redirection with dup2()
- Wait for child processes
- Capture exit codes

#### 4. **Built-ins** (src/builtins/)
- Commands that must run in parent process (cd, export)
- Executed before fork
- Direct access to shell environment

#### 5. **Tools** (src/tools/)
- Filesystem utilities integrated as built-ins
- Function dispatch instead of exec
- Faster execution (no fork overhead)
- Consistent error handling

#### 6. **Glob Expansion** (src/glob/)
- Pattern matching: *, ?, [abc], [a-z], [!abc]
- Directory scanning
- Recursive matching (optional)
- Sort results alphabetically

### Data Flow
```
User Input
    ↓
[Lexer/Parser] → AST
    ↓
[Variable Expansion] → Expand $var
    ↓
[Glob Expansion] → Expand *.txt
    ↓
[Built-in Check] → Run in parent if built-in
    ↓
[Tool Check] → Dispatch to tool function
    ↓
[Fork/Exec] → Execute external command
    ↓
[Wait/Collect] → Capture exit status
    ↓
Output/Status
```

---

## Building from Source

### Manual Build
```bash
# Generate parser (if using BNFC)
cd src/parser
bnfc -c --makefile Grammar.cf
make

# Compile shell
cd ../..
gcc -Wall -Wextra -g -Iinclude \
    src/main.c \
    src/parser/*.c \
    src/evaluator/*.c \
    src/builtins/*.c \
    src/tools/*.c \
    src/glob/*.c \
    src/utils/*.c \
    -o ushell
```

### Makefile Targets
```bash
make          # Build shell
make clean    # Remove build artifacts
make test     # Run test suite
make valgrind # Run with memory checker
make install  # Install to system (requires sudo)
```

---

## 🤝 Contributing

Contributions welcome! Please:

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Add tests for new features
5. Ensure all tests pass
6. Submit a pull request

See `docs/DEVELOPER_GUIDE.md` for detailed contribution guidelines.

---

## Documentation

- **User Guide**: `docs/USER_GUIDE.md` - Comprehensive usage documentation
- **Developer Guide**: `docs/DEVELOPER_GUIDE.md` - Architecture and code documentation
- **Plan**: `Plan.md` - Implementation roadmap
- **Prompts**: `Prompts.md` - Step-by-step implementation guide with tests
- **AI Interaction Log**: `AI_Interaction_ShellBuild.md` - Development history

---

## Project Status

### Implemented Features (DONE)
- DONE Basic REPL
- DONE Command execution (fork/exec)
- DONE Variable assignment and expansion
- DONE Arithmetic evaluation
- DONE Conditionals (if/then/else/fi)
- DONE Pipelines (multi-stage)
- DONE I/O redirection (<, >, >>)
- DONE Built-in commands (cd, pwd, echo, export, exit)
- DONE All filesystem tools integrated
- DONE Text editor integrated
- DONE Wildcard/glob expansion
- DONE Memory safety (valgrind clean)
- DONE Comprehensive test suite
- DONE **Job Control** - Background jobs (&), job management (jobs, fg, bg), signal handling (Ctrl+C, Ctrl+Z)

### Future Enhancements (NOT-DONE)
- NOT-DONE Command history (readline integration)
- NOT-DONE Tab completion
- NOT-DONE Aliases
- NOT-DONE Functions
- NOT-DONE Loops (for, while)
- NOT-DONE Subshell execution (command substitution with backticks)
- NOT-DONE Brace expansion ({a,b,c})
- NOT-DONE Tilde expansion (~)

---

## Known Issues

None at this time. See GitHub Issues for bug reports and feature requests.

---

## 📄 License

This project is released for educational purposes. See LICENSE file for details.

---

## 🙏 Acknowledgments

This project combines and extends work from:
- **ShellByGrammar** - Grammar-based shell parser with BNFC
- **StandAloneTools** - Custom filesystem utilities and text editor
- **shell.c** - Original fork/exec shell implementation

Special thanks to the BNFC project for the excellent parser generator.

---

## 📧 Contact

For questions, issues, or contributions, please open an issue on GitHub or contact the maintainer.

---

## Version

**Version**: 1.0.0  
**Last Updated**: November 19, 2025  
**Status**: Production Ready

---

## Quick Reference Card

```bash
# Variables
export var=value          # Set variable
echo $var                 # Expand variable
result=$((expr))          # Arithmetic

# Control Flow
if cmd then cmds fi       # Conditional
if cmd then cmds else cmds fi  # If/else

# Pipelines & Redirection
cmd1 | cmd2               # Pipeline
cmd < file                # Input redirect
cmd > file                # Output redirect
cmd >> file               # Append redirect

# Wildcards
*                         # Match any characters
?                         # Match single character
[abc]                     # Match a, b, or c
[a-z]                     # Match range
[!abc]                    # Match anything except a, b, c

# Built-in Commands
cd, pwd, echo, export, set, exit, help, version

# File Tools
myls, mycat, mycp, mymv, myrm, mymkdir, myrmdir, mytouch, mystat, myfd

# Editor
edi file.txt              # Edit file
# :w, :q, :wq, :q!        # Command mode
```

---

**Happy Shell Scripting!**
