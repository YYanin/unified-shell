# Unified Shell Implementation Plan

## Overview
This document outlines the step-by-step plan for merging three separate shell-related projects into a single, fully-featured shell with advanced features.

### Source Projects
1. **ShellByGrammar** - BNFC-based parser with variable/arithmetic evaluation
2. **StandAloneTools** - Custom command-line tools and text editor
3. **shell.c** - Fork/exec shell with pipeline support

### Target Features
- DONE Variable assignment and expansion
- DONE Arithmetic evaluation
- DONE Conditional statements (if/then/else/fi)
- DONE Pipeline execution
- DONE I/O redirection
- DONE Built-in commands (cd, pwd, echo, export, exit)
- DONE Custom filesystem tools (ls, cp, mv, rm, mkdir, rmdir, touch, stat, cat, fd)
- DONE Vi-like text editor (edi)
- DONE Wildcard/glob expansion (*, ?, [])
- DONE **Job Control** - Background jobs (&), job management (jobs, fg, bg), signal handling
- DONE Integration of all components

---

## Phase 1: Project Analysis & Architecture Design

### Objectives
- Analyze existing codebases to understand dependencies and interfaces
- Design unified architecture that accommodates all features
- Create project directory structure
- Define integration strategy

### Tasks

#### 1.1 Code Analysis
- **ShellByGrammar Analysis**
  - Review Grammar.cf to understand AST structure
  - Examine Eval.c for variable and conditional evaluation logic
  - Study calc implementation for arithmetic parsing
  - Identify reusable components (Env management, evaluator)

- **shell.c Analysis**
  - Review fork/exec implementation
  - Study pipeline construction logic
  - Analyze built-in command structure
  - Examine variable expansion mechanism

- **StandAloneTools Analysis**
  - Inventory all tool implementations
  - Review edi editor architecture
  - Identify common patterns and dependencies

#### 1.2 Architecture Design
- **Core Shell Architecture**
  - REPL (Read-Eval-Print Loop)
  - Parser: Choose between BNFC-generated vs manual parsing
  - AST representation
  - Evaluator/Executor
  - Environment management

- **Component Integration Strategy**
  - Built-in commands vs external tools
  - How to integrate BNFC parser with fork/exec logic
  - Variable storage: unified vs separate environments
  - Command dispatch mechanism

- **Directory Structure**
```
unified-shell/
├── src/
│   ├── main.c              # REPL and initialization
│   ├── parser/             # BNFC-generated or manual parser
│   ├── evaluator/          # Command evaluation and execution
│   ├── builtins/           # Built-in commands
│   ├── tools/              # Integrated command-line tools
│   ├── glob/               # Wildcard expansion
│   └── utils/              # Helper functions
├── include/                # Header files
├── tests/                  # Test suite
├── docs/                   # Documentation
├── Makefile
└── README.md
```

#### 1.3 Decision Points
- **Parser Choice**: BNFC vs manual
  - BNFC Pro: Grammar-driven, handles complex syntax
  - BNFC Con: Overhead, requires BNFC toolchain
  - Manual Pro: Lightweight, easier to debug
  - Manual Con: More code to maintain
  - **Recommendation**: Start with BNFC, option to simplify later

- **Tool Integration**: Built-in vs separate executables
  - Built-in Pro: Faster execution, no fork overhead
  - Built-in Con: Larger binary, memory overhead
  - **Recommendation**: Integrate as built-ins with function dispatch

- **Variable System**: Merge both implementations
  - Use ShellByGrammar's Env structure (more robust)
  - Keep shell.c's expansion logic for backward compatibility
  - Add arithmetic evaluation from calc

---

## Phase 2: Foundation - Project Setup & Core Shell

### Objectives
- Create project directory structure
- Implement minimal REPL
- Set up build system
- Establish testing framework

### Tasks

#### 2.1 Project Initialization
- Create directory structure
- Copy relevant source files to new project
- Set up version control (git)

#### 2.2 Minimal REPL
- Implement basic read-eval-print loop
- Simple command tokenization
- Echo commands for verification

#### 2.3 Build System
- Create Makefile with:
  - BNFC grammar compilation (if using BNFC)
  - Source compilation with proper flags
  - Clean and rebuild targets
  - Test target

#### 2.4 Testing Framework
- Create test directory structure
- Implement test runner script
- Add initial smoke tests

---

## Phase 3: Parser Integration

### Objectives
- Integrate BNFC-generated parser from ShellByGrammar
- Ensure all grammar features are supported
- Connect parser to shell evaluator

### Tasks

#### 3.1 Grammar Integration
- Copy Grammar.cf from ShellByGrammar
- Generate parser files with BNFC
- Integrate Absyn.c/h, Parser.c/h, Lexer.c

#### 3.2 Parser Interface
- Create parser wrapper functions
- Handle parse errors gracefully
- Convert input string to AST

#### 3.3 AST Verification
- Test variable assignment parsing
- Test conditional statement parsing
- Test pipeline parsing
- Test arithmetic expression parsing

---

## Phase 4: Variable System & Environment

### Objectives
- Implement unified variable storage
- Support variable assignment and expansion
- Add arithmetic evaluation

### Tasks

#### 4.1 Environment Management
- Integrate Env structure from ShellByGrammar/Eval.c
- Implement env_new, env_get, env_set, env_free
- Add support for system environment variables

#### 4.2 Variable Assignment
- Implement evaluation for CmdAssign AST nodes
- Store variables in environment
- Handle error cases

#### 4.3 Variable Expansion
- Implement $var expansion in command arguments
- Support expansion in echo, conditionals, etc.
- Handle undefined variables gracefully

#### 4.4 Arithmetic Evaluation
- Integrate calc logic from ShellByGrammar/bncf/calc
- Support arithmetic expressions in assignments
- Support arithmetic in conditionals

---

## Phase 5: Command Execution Engine

### Objectives
- Implement fork/exec mechanism from shell.c
- Support pipeline execution
- Handle I/O redirection
- Implement built-in commands

### Tasks

#### 5.1 Basic Execution
- Implement command execution with fork/exec
- Handle command not found errors
- Wait for child process completion

#### 5.2 Pipeline Execution
- Port pipeline logic from shell.c
- Create pipe file descriptors
- Connect stdout/stdin between processes
- Handle multiple commands in pipeline

#### 5.3 I/O Redirection
- Implement input redirection (<)
- Implement output redirection (>)
- Implement append redirection (>>)
- Handle redirection errors

#### 5.4 Built-in Commands
- Implement cd (change directory)
- Implement pwd (print working directory)
- Implement echo (print arguments)
- Implement export (set environment variables)
- Implement exit (terminate shell)
- Ensure built-ins run in parent process context

---

## Phase 6: Conditional Execution

### Objectives
- Implement if/then/else/fi evaluation
- Support command exit codes
- Handle nested conditionals

### Tasks

#### 6.1 Exit Code Handling
- Capture exit codes from executed commands
- Store last exit code for conditionals
- Implement true/false evaluation

#### 6.2 Conditional Evaluation
- Implement IfCmd AST node evaluation
- Implement IfElseCmd AST node evaluation
- Support pipeline conditions
- Test nested conditionals

---

## Phase 7: Tool Integration

### Objectives
- Integrate custom command-line tools from StandAloneTools
- Implement tools as built-in commands
- Ensure tools work with shell features (pipes, redirection)

### Tasks

#### 7.1 Tool Code Integration
- Copy tool implementations from StandAloneTools/tools/
- Refactor tools to use common function signatures
- Create tool dispatch mechanism

#### 7.2 Individual Tool Integration
- **myls** - List directory contents
- **mycat** - Display file contents
- **mycp** - Copy files
- **mymv** - Move/rename files
- **myrm** - Remove files
- **mymkdir** - Create directories
- **myrmdir** - Remove directories
- **mytouch** - Create/update files
- **mystat** - Display file information
- **myfd** - Advanced file finder

#### 7.3 Tool Testing
- Test each tool independently
- Test tools with pipelines
- Test tools with redirection
- Verify error handling

#### 7.4 Editor Integration (Optional)
- Integrate edi text editor
- Add as built-in command
- Ensure terminal handling works within shell

---

## Phase 8: Wildcard/Glob Expansion

### Objectives
- Implement metacharacter expansion (*, ?, [])
- Support glob patterns in commands
- Handle edge cases and errors

### Tasks

#### 8.1 Glob Pattern Matching
- Implement * (match zero or more characters)
- Implement ? (match single character)
- Implement [] (match character set)
- Implement [!] (match negated character set)

#### 8.2 Path Expansion
- Scan directory for matching files
- Sort results alphabetically
- Handle no matches (literal vs error)

#### 8.3 Integration with Parser
- Expand globs after parsing, before execution
- Support globs in command arguments
- Support globs in redirection filenames

#### 8.4 Edge Cases
- Empty glob results
- Hidden files (.*pattern)
- Escaped metacharacters
- Recursive patterns (**/*)

---

## Phase 9: Testing & Validation

### Objectives
- Comprehensive testing of all features
- Performance testing
- Memory leak detection
- Documentation verification

### Tasks

#### 9.1 Unit Testing
- Test variable operations
- Test arithmetic evaluation
- Test conditionals
- Test glob expansion
- Test each built-in command
- Test each tool

#### 9.2 Integration Testing
- Test complex pipelines
- Test nested conditionals with variables
- Test redirection with tools
- Test glob patterns with commands

#### 9.3 Regression Testing
- Port existing tests from ShellByGrammar
- Port tests from shell.c
- Create new tests for integrated features

#### 9.4 Performance Testing
- Measure command execution overhead
- Test with large pipelines
- Test with many variables
- Profile memory usage

#### 9.5 Memory Testing
- Run with valgrind to detect leaks
- Fix any memory issues
- Verify proper cleanup on exit

---

## Phase 10: Documentation & Polish

### Objectives
- Complete user documentation
- Add inline code comments
- Create usage examples
- Prepare for release

### Tasks

#### 10.1 User Documentation
- Update README.md with feature list
- Create usage guide with examples
- Document all built-in commands
- Document all integrated tools
- Create quickstart guide

#### 10.2 Developer Documentation
- Document architecture and design decisions
- Add inline comments to complex code sections
- Document API for extensibility
- Create contribution guidelines

#### 10.3 Examples
- Create example scripts
- Add demo session transcript
- Document common use cases

#### 10.4 Final Polish
- Code cleanup and formatting
- Optimize performance bottlenecks
- Handle edge cases
- Improve error messages
- Add help command

---

## Implementation Timeline

### Estimated Effort
- **Phase 1**: 1-2 days (Analysis & Design)
- **Phase 2**: 1 day (Foundation)
- **Phase 3**: 1 day (Parser Integration)
- **Phase 4**: 2 days (Variables & Arithmetic)
- **Phase 5**: 2-3 days (Execution Engine)
- **Phase 6**: 1 day (Conditionals)
- **Phase 7**: 2-3 days (Tool Integration)
- **Phase 8**: 2 days (Glob Expansion)
- **Phase 9**: 2-3 days (Testing)
- **Phase 10**: 1-2 days (Documentation)

**Total**: ~15-20 days

### Milestones
1. DONE Basic REPL working
2. DONE Parser integrated, commands parsed to AST
3. DONE Variables working (assignment & expansion)
4. DONE Simple commands executing
5. DONE Pipelines working
6. DONE Conditionals working
7. DONE All tools integrated
8. DONE Glob expansion working
9. DONE All tests passing
10. DONE Documentation complete

---

## Risk Mitigation

### Technical Risks
- **BNFC complexity**: Keep parser simple, fallback to manual if needed
- **Memory management**: Use valgrind early and often
- **Terminal handling**: Test edi carefully, may need isolation
- **Glob edge cases**: Study existing implementations (bash, zsh)

### Process Risks
- **Scope creep**: Stick to defined features, track future enhancements separately
- **Integration issues**: Test incrementally, maintain working version
- **Performance**: Profile early if issues arise

---

## Success Criteria

### Functional Requirements
- DONE All features from original projects work
- DONE New glob expansion feature works
- DONE No regression in existing functionality
- DONE All tests pass

### Quality Requirements
- DONE No memory leaks (valgrind clean)
- DONE Compilation with no warnings
- DONE Comprehensive documentation
- DONE User-friendly error messages

### Performance Requirements
- DONE Command execution overhead < 10ms
- DONE Pipeline execution works with 10+ commands
- DONE Supports 100+ variables

---

## Future Enhancements (Post-MVP)

- Background job management (&, jobs, fg, bg)
- Command history and editing (readline integration)
- Tab completion
- Aliases
- Functions
- Advanced scripting (loops, functions)
- Signal handling (Ctrl+C, Ctrl+Z)
- Subshell execution (command substitution)
- More sophisticated arithmetic (floating point)
- Brace expansion ({a,b,c})
- Tilde expansion (~, ~user)
- Networking commands
- Process management commands (ps, kill)
- Threading support (pthread)

---

## Appendix: Component Mapping

### From ShellByGrammar
- **Grammar.cf** → Core parser grammar
- **Eval.c** → Evaluator framework and variable management
- **bncf/calc/** → Arithmetic expression evaluation
- **Env structure** → Variable storage

### From shell.c
- **exec_pipeline()** → Pipeline execution logic
- **exec_command_child()** → Command execution
- **exec_builtin()** → Built-in command dispatch
- **expand_vars()** → Variable expansion
- **parse_and_run()** → Input parsing (if not using BNFC)

### From StandAloneTools
- **tools/*.c** → All tool implementations
- **editor/edi.c** → Text editor

### New Components
- **glob.c** → Wildcard expansion
- **main.c** → Unified REPL
- **Makefile** → Complete build system
