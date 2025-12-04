# Implementation Prompts for Unified Shell

This document contains step-by-step prompts to execute the implementation plan. Each prompt includes manual test procedures to verify successful completion.

---

## Prompt 1: Project Initialization & Directory Structure

### Prompt
```
Create a new directory called 'unified-shell' in the current workspace. Set up the following directory structure:

unified-shell/
├── src/
│   ├── parser/
│   ├── evaluator/
│   ├── builtins/
│   ├── tools/
│   ├── glob/
│   └── utils/
├── include/
├── tests/
│   ├── unit/
│   └── integration/
├── docs/
└── examples/

Create a basic .gitignore file to exclude build artifacts (*.o, *.a, executables).
Create an empty Makefile skeleton.
Create a basic README.md with project title and placeholder sections.
```

### Manual Tests
1. **Directory structure verification**:
   ```bash
   cd unified-shell
   find . -type d | sort
   # Verify all directories are present
   ```

2. **File verification**:
   ```bash
   ls -la
   # Verify .gitignore, Makefile, README.md exist
   ```

3. **Git initialization** (optional):
   ```bash
   git init
   git add .
   git status
   # Verify clean initialization
   ```

---

## Prompt 2: Minimal REPL Implementation

### Prompt
```
Create src/main.c with a minimal REPL (Read-Eval-Print Loop) that:
1. Prints a prompt "unified-shell> "
2. Reads a line of input using fgets
3. Strips the trailing newline
4. Echoes the input back
5. Exits on "exit" command or EOF (Ctrl+D)

Include necessary headers: stdio.h, stdlib.h, string.h

Create include/shell.h with basic constant definitions:
- MAX_LINE 1024
- PROMPT "unified-shell> "

Update the Makefile to compile main.c into an executable called 'ushell':
- Use gcc with -Wall -Wextra -g flags
- Include -I include for header files
- Add clean target
```

### Manual Tests
1. **Compilation test**:
   ```bash
   cd unified-shell
   make
   # Should compile without errors or warnings
   ```

2. **Basic REPL test**:
   ```bash
   ./ushell
   # Type: hello world
   # Expected output: hello world
   # Type: test command
   # Expected output: test command
   ```

3. **Exit test**:
   ```bash
   ./ushell
   # Type: exit
   # Shell should terminate
   ```

4. **EOF test**:
   ```bash
   echo "test" | ./ushell
   # Should run and exit cleanly
   ```

---

## Prompt 3: Parser Integration - Copy BNFC Files

### Prompt
```
Copy the following files from shellByGrammar/bncf/shell/ to unified-shell/src/parser/:
- Grammar.cf
- Absyn.c, Absyn.h
- Parser.c, Parser.h
- Lexer.c
- Printer.c, Printer.h
- Skeleton.c, Skeleton.h
- Buffer.c, Buffer.h

Update the Makefile to:
1. Add PARSER_SRC variable with all parser .c files
2. Add parser compilation rules
3. Link parser objects with main.o
4. Add BNFC generation rule (optional, for grammar modifications):
   ```makefile
   parser: src/parser/Grammar.cf
       cd src/parser && bnfc -c --makefile Grammar.cf
   ```

Create include/parser.h as a wrapper that includes parser headers and declares:
```c
Program pProgram(FILE *inp);
void showProgram(Program p);
```
```

### Manual Tests
1. **File copy verification**:
   ```bash
   ls -la src/parser/
   # Verify all parser files are present
   ```

2. **Compilation test**:
   ```bash
   make clean
   make
   # Should compile without errors
   ```

3. **Parser test** (create test file):
   ```bash
   echo "echo hello" > test.txt
   # Modify main.c temporarily to call pProgram() on a file
   # This is a smoke test to ensure parser links correctly
   ```

---

## Prompt 4: Environment & Variable System

### Prompt
```
Create src/evaluator/environment.c and include/environment.h implementing:

1. Data structures (in header):
   ```c
   #define MAX_VARS 100
   #define VAR_NAME_MAX 64
   #define VAR_VALUE_MAX 256

   typedef struct {
       char *name;
       char *value;
   } Binding;

   typedef struct {
       Binding bindings[MAX_VARS];
       int count;
   } Env;
   ```

2. Functions:
   - Env* env_new(void) - Create new environment
   - void env_free(Env *env) - Free environment memory
   - char* env_get(Env *env, const char *name) - Get variable value
   - void env_set(Env *env, const char *name, const char *value) - Set variable
   - void env_unset(Env *env, const char *name) - Remove variable
   - void env_print(Env *env) - Print all variables (debug)

3. Also check system environment using getenv() if not found in env.

Update Makefile to compile environment.c
Update main.c to create global Env *shell_env in main()
```

### Manual Tests
1. **Compilation test**:
   ```bash
   make clean && make
   # Should compile without errors
   ```

2. **Variable set/get test** (add to main.c temporarily):
   ```c
   env_set(shell_env, "x", "5");
   printf("x = %s\n", env_get(shell_env, "x"));
   env_set(shell_env, "name", "shell");
   env_print(shell_env);
   ```
   Expected output:
   ```
   x = 5
   name = shell
   x = 5
   ```

3. **System env test**:
   ```c
   printf("HOME = %s\n", env_get(shell_env, "HOME"));
   // Should print system HOME directory
   ```

4. **Memory test**:
   ```bash
   valgrind ./ushell
   # Type: exit
   # Should show no memory leaks
   ```

---

## Prompt 5: Variable Expansion

### Prompt
```
Create src/utils/expansion.c and include/expansion.h with:

1. Function: char* expand_variables(const char *input, Env *env)
   - Scans input for $variable tokens
   - Replaces with values from environment
   - Handles $VAR and ${VAR} syntax
   - Returns newly allocated string (caller must free)
   - Handles undefined variables (leave as empty string)

2. Function: void expand_variables_inplace(char *input, Env *env, size_t bufsize)
   - Performs expansion in existing buffer
   - Safer for fixed-size buffers
   - Truncates if result too long

Update main.c REPL to:
1. Call expand_variables_inplace() on input before echoing
2. Print expanded result

Update Makefile to compile expansion.c
```

### Manual Tests
1. **Basic expansion**:
   ```bash
   ./ushell
   # Setup (in code): env_set(shell_env, "x", "5");
   # Type: echo $x
   # Expected: echo 5
   ```

2. **Multiple variables**:
   ```bash
   # Setup: env_set(shell_env, "name", "Alice");
   #        env_set(shell_env, "greeting", "Hello");
   # Type: $greeting $name
   # Expected: Hello Alice
   ```

3. **Undefined variable**:
   ```bash
   # Type: $undefined_var
   # Expected: (empty string or literal $undefined_var based on implementation)
   ```

4. **Mixed text and variables**:
   ```bash
   # Setup: env_set(shell_env, "user", "admin");
   # Type: User: $user, Status: active
   # Expected: User: admin, Status: active
   ```

---

## Prompt 6: Basic Command Execution

### Prompt
```
Create src/evaluator/executor.c and include/executor.h with:

1. Function: int execute_command(char **argv, Env *env)
   - Takes array of command arguments (NULL-terminated)
   - Forks a child process
   - Executes command using execvp()
   - Waits for child completion
   - Returns exit status

2. Function: char** tokenize_command(char *line)
   - Splits line into tokens by whitespace
   - Returns NULL-terminated array of strings
   - Handles quoted strings (basic, space preservation)

Update main.c REPL to:
1. Expand variables
2. Tokenize into argv
3. Execute using execute_command()
4. Free allocated memory

Update Makefile to compile executor.c
```

### Manual Tests
1. **Simple command**:
   ```bash
   ./ushell
   # Type: /bin/echo hello
   # Expected: hello
   ```

2. **Command with arguments**:
   ```bash
   # Type: /bin/ls -la /tmp
   # Expected: directory listing of /tmp
   ```

3. **PATH resolution**:
   ```bash
   # Type: ls
   # Expected: directory listing
   ```

4. **Command not found**:
   ```bash
   # Type: nonexistent_command
   # Expected: error message
   ```

5. **Exit status check** (add to executor.c):
   ```bash
   # Type: /bin/true
   # exit status should be 0
   # Type: /bin/false
   # exit status should be 1
   ```

---

## Prompt 7: Built-in Commands

### Prompt
```
Create src/builtins/builtins.c and include/builtins.h with:

1. Built-in command structure:
   ```c
   typedef int (*builtin_func)(char **argv, Env *env);
   
   typedef struct {
       const char *name;
       builtin_func func;
   } Builtin;
   ```

2. Implement built-in commands:
   - int builtin_cd(char **argv, Env *env)
   - int builtin_pwd(char **argv, Env *env)
   - int builtin_echo(char **argv, Env *env)
   - int builtin_export(char **argv, Env *env)
   - int builtin_exit(char **argv, Env *env)
   - int builtin_set(char **argv, Env *env) - assign variable

3. Function: builtin_func find_builtin(const char *name)
   - Returns function pointer if built-in found, NULL otherwise

4. Update execute_command() to check for built-ins first
   - If built-in, execute in parent process
   - Otherwise, fork/exec

Update Makefile to compile builtins.c
```

### Manual Tests
1. **cd command**:
   ```bash
   ./ushell
   # Type: pwd
   # Remember output
   # Type: cd /tmp
   # Type: pwd
   # Expected: /tmp
   ```

2. **echo command**:
   ```bash
   # Type: echo hello world
   # Expected: hello world
   ```

3. **export command**:
   ```bash
   # Type: export myvar=testvalue
   # Type: echo $myvar
   # Expected: testvalue
   ```

4. **set command** (variable assignment):
   ```bash
   # Type: set x=10
   # Type: echo $x
   # Expected: 10
   ```

5. **exit command**:
   ```bash
   # Type: exit
   # Shell should terminate
   ```

---

## Prompt 8: Pipeline Implementation

### Prompt
```
Update src/evaluator/executor.c to support pipelines:

1. Add structure for pipeline:
   ```c
   typedef struct {
       char **argv;
       char *infile;
       char *outfile;
       int append;
   } Command;
   ```

2. Function: int parse_pipeline(char *line, Command **commands, int *count)
   - Splits line by | character
   - Parses each command segment
   - Detects < > >> for redirection
   - Returns number of commands in pipeline

3. Function: int execute_pipeline(Command *commands, int count, Env *env)
   - Creates pipes between commands
   - Forks processes for each command
   - Sets up stdin/stdout redirection
   - Handles I/O file redirection
   - Waits for all processes

Update main.c to use pipeline execution instead of simple execution.
```

### Manual Tests
1. **Simple pipe**:
   ```bash
   ./ushell
   # Type: echo hello | cat
   # Expected: hello
   ```

2. **Multi-stage pipe**:
   ```bash
   # Type: echo -e "apple\nbanana\ncherry" | grep a | sort
   # Expected: apple
   #           banana
   ```

3. **Input redirection**:
   ```bash
   # Create test file: echo "test content" > /tmp/test.txt
   # Type: cat < /tmp/test.txt
   # Expected: test content
   ```

4. **Output redirection**:
   ```bash
   # Type: echo output test > /tmp/output.txt
   # Type: cat /tmp/output.txt
   # Expected: output test
   ```

5. **Append redirection**:
   ```bash
   # Type: echo line1 > /tmp/append.txt
   # Type: echo line2 >> /tmp/append.txt
   # Type: cat /tmp/append.txt
   # Expected: line1
   #           line2
   ```

6. **Complex pipeline**:
   ```bash
   # Type: ls -la | grep ushell | wc -l
   # Expected: number of matches
   ```

---

## Prompt 9: Conditional Execution (if/then/fi)

### Prompt
```
Create src/evaluator/conditional.c and include/conditional.h:

1. Add global variable: int last_exit_status

2. Function: int parse_conditional(char *line, char **condition, char **then_block, char **else_block)
   - Parses "if <condition> then <commands> fi"
   - Parses "if <condition> then <commands> else <commands> fi"
   - Returns 1 if conditional, 0 if not

3. Function: int execute_conditional(char *condition, char *then_block, char *else_block, Env *env)
   - Executes condition command
   - If exit status == 0, execute then_block
   - Otherwise, execute else_block (if present)
   - Returns final exit status

4. Update main.c to detect and handle conditionals

Note: For full BNFC integration, this would evaluate AST nodes instead of parsing strings.
For simplicity, start with string-based parsing.
```

### Manual Tests
1. **Simple if/then**:
   ```bash
   ./ushell
   # Type: if /bin/true then echo success fi
   # Expected: success
   ```

2. **If/then with false condition**:
   ```bash
   # Type: if /bin/false then echo success fi
   # Expected: (no output)
   ```

3. **If/then/else with true**:
   ```bash
   # Type: if /bin/true then echo yes else echo no fi
   # Expected: yes
   ```

4. **If/then/else with false**:
   ```bash
   # Type: if /bin/false then echo yes else echo no fi
   # Expected: no
   ```

5. **Conditional with variable**:
   ```bash
   # Type: export status=0
   # Type: if test $status -eq 0 then echo ok else echo fail fi
   # Expected: ok
   ```

---

## Prompt 10: Arithmetic Evaluation

### Prompt
```
Create src/evaluator/arithmetic.c and include/arithmetic.h:

1. Function: int eval_arithmetic(const char *expr, Env *env)
   - Parses simple arithmetic expressions: +, -, *, /, %
   - Supports variables ($var)
   - Supports parentheses for precedence
   - Returns integer result

2. Add syntax: $((expression))
   - Expand variables first
   - Evaluate arithmetic
   - Replace with result

3. Update expansion.c to handle $((...)) syntax

Option: Copy and adapt calc parser from shellByGrammar/bncf/calc/ for more robust parsing.
```

### Manual Tests
1. **Simple arithmetic**:
   ```bash
   ./ushell
   # Type: echo $((5 + 3))
   # Expected: 8
   ```

2. **Multiplication**:
   ```bash
   # Type: echo $((4 * 7))
   # Expected: 28
   ```

3. **With variables**:
   ```bash
   # Type: export x=10
   # Type: echo $(($x + 5))
   # Expected: 15
   ```

4. **Complex expression**:
   ```bash
   # Type: export a=3
   # Type: export b=4
   # Type: echo $(($a * $b + 2))
   # Expected: 14
   ```

5. **Variable assignment**:
   ```bash
   # Type: export result=$((100 / 4))
   # Type: echo $result
   # Expected: 25
   ```

---

## Prompt 11: Tool Integration - File Utilities

### Prompt
```
Copy and integrate filesystem tools from StandAloneTools/tools/:

1. Create src/tools/ directory

2. For each tool (myls, mycat, mycp, mymv, myrm, mymkdir, myrmdir, mytouch, mystat):
   - Copy source file to src/tools/
   - Rename main() to tool_<name>_main(int argc, char **argv)
   - Create header declarations in include/tools.h

3. Create src/tools/tool_dispatch.c:
   ```c
   typedef int (*tool_func)(int argc, char **argv);
   
   tool_func find_tool(const char *name);
   ```

4. Update executor.c to check for tools before execvp():
   ```c
   tool_func tool = find_tool(argv[0]);
   if (tool) {
       int argc = 0;
       while (argv[argc]) argc++;
       exit(tool(argc, argv));
   }
   ```

5. Update Makefile to compile all tool files

Note: This integrates tools as built-ins while maintaining their original logic.
```

### Manual Tests
1. **myls test**:
   ```bash
   ./ushell
   # Type: myls
   # Expected: directory listing similar to ls
   ```

2. **mycat test**:
   ```bash
   # Create test file: echo "test content" > /tmp/cat_test.txt
   # Type: mycat /tmp/cat_test.txt
   # Expected: test content
   ```

3. **mymkdir test**:
   ```bash
   # Type: mymkdir /tmp/test_dir
   # Type: myls /tmp | grep test_dir
   # Expected: test_dir
   ```

4. **mytouch test**:
   ```bash
   # Type: mytouch /tmp/newfile.txt
   # Type: myls /tmp/newfile.txt
   # Expected: /tmp/newfile.txt
   ```

5. **mycp test**:
   ```bash
   # Type: echo "copy test" > /tmp/source.txt
   # Type: mycp /tmp/source.txt /tmp/dest.txt
   # Type: mycat /tmp/dest.txt
   # Expected: copy test
   ```

6. **myrm test**:
   ```bash
   # Type: mytouch /tmp/delete_me.txt
   # Type: myrm /tmp/delete_me.txt
   # Type: myls /tmp/delete_me.txt
   # Expected: error (file not found)
   ```

7. **Pipe test with tools**:
   ```bash
   # Type: myls | mycat
   # Expected: directory listing
   ```

---

## Prompt 12: Tool Integration - myfd (File Finder)

### Prompt
```
Integrate myfd from StandAloneTools/tools/myfd.c:

1. Copy myfd.c to src/tools/
2. Refactor main() to tool_myfd_main()
3. Add to tool dispatch table
4. Test pattern matching and directory traversal

myfd features to support:
- Basic pattern matching (glob-style)
- Directory recursion
- File type filtering
- Hidden file handling
```

### Manual Tests
1. **Basic search**:
   ```bash
   ./ushell
   # Type: myfd *.c
   # Expected: list of all .c files in current directory
   ```

2. **Recursive search**:
   ```bash
   # Type: myfd -r test
   # Expected: all files/dirs matching "test" recursively
   ```

3. **Type filtering**:
   ```bash
   # Type: myfd -t f *.txt
   # Expected: only .txt files (not directories)
   ```

4. **Hidden files**:
   ```bash
   # Type: myfd -H .*
   # Expected: hidden files/directories
   ```

5. **Pipe with myfd**:
   ```bash
   # Type: myfd *.c | wc -l
   # Expected: count of .c files
   ```

---

## Prompt 13: Wildcard/Glob Expansion - Part 1

### Prompt
```
Create src/glob/glob.c and include/glob.h for wildcard expansion:

1. Function: char** expand_glob(const char *pattern, int *count)
   - Implements * (match any characters)
   - Implements ? (match single character)
   - Scans current directory for matches
   - Returns array of matching filenames
   - Returns NULL if no matches (or literal pattern)

2. Helper function: int match_pattern(const char *pattern, const char *str)
   - Recursive pattern matching
   - Returns 1 if match, 0 otherwise

3. Integration point: Call before command execution
   - Expand each argument that contains wildcards
   - Replace with expanded list

Update Makefile to compile glob.c
```

### Manual Tests
1. **Star wildcard**:
   ```bash
   ./ushell
   # Type: echo *.c
   # Expected: list of all .c files
   ```

2. **Question mark wildcard**:
   ```bash
   # Type: echo test?.txt
   # Expected: files like test1.txt, testa.txt, etc.
   ```

3. **No matches**:
   ```bash
   # Type: echo *.xyz
   # Expected: *.xyz (literal, or error depending on implementation)
   ```

4. **Multiple patterns**:
   ```bash
   # Type: echo *.c *.h
   # Expected: all .c files followed by all .h files
   ```

5. **With commands**:
   ```bash
   # Type: cat *.txt
   # Expected: concatenated content of all .txt files
   ```

---

## Prompt 14: Wildcard/Glob Expansion - Part 2

### Prompt
```
Extend glob.c to support character classes:

1. Add support for [abc] (match a, b, or c)
2. Add support for [!abc] (match anything except a, b, c)
3. Add support for ranges [a-z], [0-9]
4. Handle escaped characters \*, \?, \[

Update match_pattern() to handle these cases.

Add edge case handling:
- Empty glob results
- Hidden files (.*pattern)
- Recursive globs (**/pattern) [optional, advanced]
```

### Manual Tests
1. **Character class**:
   ```bash
   ./ushell
   # Create files: touch file1.txt file2.txt file3.txt filea.txt
   # Type: echo file[12].txt
   # Expected: file1.txt file2.txt
   ```

2. **Range**:
   ```bash
   # Type: echo file[0-9].txt
   # Expected: file1.txt file2.txt file3.txt
   ```

3. **Negation**:
   ```bash
   # Type: echo file[!a].txt
   # Expected: file1.txt file2.txt file3.txt (not filea.txt)
   ```

4. **Escaped wildcards**:
   ```bash
   # Create file: touch "*.txt"
   # Type: echo \*.txt
   # Expected: *.txt (literal filename)
   ```

5. **Hidden files**:
   ```bash
   # Type: echo .*
   # Expected: .gitignore and other hidden files
   ```

---

## Prompt 15: Editor Integration (Optional)

### Prompt
```
Integrate the edi text editor from StandAloneTools/editor/:

1. Copy edi.c to src/tools/editor.c
2. Refactor main() to tool_edi_main(int argc, char **argv)
3. Add "edi" to tool dispatch
4. Ensure terminal control works correctly within shell

Challenges:
- Terminal raw mode conflicts with shell
- Need to save/restore terminal state
- May need to fork editor as separate process

Consider: Keep editor as external executable instead of integrating.
```

### Manual Tests
1. **Launch editor**:
   ```bash
   ./ushell
   # Type: edi test.txt
   # Expected: editor opens, can edit, save with :w, quit with :q
   ```

2. **Edit and verify**:
   ```bash
   # In edi: add some text, save with :w, quit with :q
   # Type: cat test.txt
   # Expected: content matches what was saved
   ```

3. **Terminal state**:
   ```bash
   # After exiting edi, shell should work normally
   # Type: echo test
   # Expected: test
   ```

**Alternative approach**: Keep edi as separate binary, execute via execvp.

---

## Prompt 16: Comprehensive Testing Suite

### Prompt
```
Create comprehensive test suite in tests/:

1. tests/test_runner.sh - Main test script
   - Runs all tests
   - Reports pass/fail for each
   - Generates summary

2. tests/unit/ - Unit tests for individual components
   - test_env.sh - Environment operations
   - test_expansion.sh - Variable expansion
   - test_glob.sh - Glob expansion
   - test_arithmetic.sh - Arithmetic evaluation

3. tests/integration/ - Integration tests
   - test_pipelines.sh - Pipeline execution
   - test_conditionals.sh - If/then/else
   - test_tools.sh - All built-in tools
   - test_redirection.sh - I/O redirection
   - test_variables.sh - Variable operations

Each test should:
- Set up test environment
- Execute shell commands
- Verify expected output/behavior
- Clean up
- Report PASS/FAIL

Example test structure:
```bash
#!/bin/bash
# Test: Variable assignment and expansion
echo "x=5; echo \$x" | ./ushell > output.txt
if grep -q "5" output.txt; then
    echo "PASS: Variable expansion"
else
    echo "FAIL: Variable expansion"
fi
rm output.txt
```
```

### Manual Tests
1. **Run all tests**:
   ```bash
   cd tests
   ./test_runner.sh
   # Expected: Summary of all test results
   ```

2. **Individual test suite**:
   ```bash
   ./unit/test_env.sh
   # Expected: PASS for all environment tests
   ```

3. **Check coverage**:
   ```bash
   # Verify tests cover:
   # - All built-in commands
   # - All integrated tools
   # - Variable operations
   # - Pipelines
   # - Conditionals
   # - Glob expansion
   # - Error cases
   ```

4. **Regression test**:
   ```bash
   # Port tests from shellByGrammar/bncf/shell/tests/
   cd tests
   ./run_sem_tests.sh
   # Expected: All original tests pass
   ```

---

## Prompt 17: Memory Management & Valgrind Testing

### Prompt
```
Audit all code for memory leaks and add proper cleanup:

1. Review and fix memory management in:
   - environment.c (env_free)
   - expansion.c (free expanded strings)
   - executor.c (free tokenized commands)
   - glob.c (free glob results)
   - All tool integrations

2. Add cleanup function to main.c:
   ```c
   void cleanup_shell(void) {
       env_free(shell_env);
       // Free other global resources
   }
   ```

3. Register cleanup with atexit():
   ```c
   atexit(cleanup_shell);
   ```

4. Create tests/valgrind_test.sh:
   - Runs shell with valgrind
   - Executes various commands
   - Checks for leaks
   - Exits cleanly

Run valgrind and fix all memory issues.
```

### Manual Tests
1. **Basic valgrind test**:
   ```bash
   echo "echo test; exit" | valgrind --leak-check=full ./ushell
   # Expected: 0 bytes lost, no errors
   ```

2. **Variable test**:
   ```bash
   echo "export x=test; echo \$x; exit" | valgrind --leak-check=full ./ushell
   # Expected: 0 bytes lost
   ```

3. **Pipeline test**:
   ```bash
   echo "echo test | cat | cat; exit" | valgrind --leak-check=full ./ushell
   # Expected: 0 bytes lost
   ```

4. **Tool test**:
   ```bash
   echo "myls; mycat README.md; exit" | valgrind --leak-check=full ./ushell
   # Expected: 0 bytes lost
   ```

5. **Comprehensive test**:
   ```bash
   cd tests
   ./valgrind_test.sh
   # Expected: All tests pass with no leaks
   ```

---

## Prompt 18: Error Handling & User Experience

### Prompt
```
Improve error handling and user experience:

1. Add proper error messages for:
   - Command not found
   - File not found (redirection)
   - Permission denied
   - Invalid syntax
   - Too many variables/arguments

2. Add signal handling:
   - Ctrl+C: Interrupt current command, not shell
   - Ctrl+D: EOF, exit shell gracefully
   - Ctrl+Z: Suspend current command (optional)

3. Improve prompts:
   - Show current directory
   - Show username
   - Colorize prompt (optional)

4. Add help command:
   - List built-in commands
   - Show usage for each command

5. Add version command:
   - Display version and build info
```

### Manual Tests
1. **Command not found**:
   ```bash
   ./ushell
   # Type: nonexistent_command
   # Expected: clear error message
   ```

2. **File not found**:
   ```bash
   # Type: cat < nonexistent.txt
   # Expected: error message with filename
   ```

3. **Ctrl+C handling**:
   ```bash
   # Type: sleep 100
   # Press Ctrl+C
   # Expected: command interrupted, shell continues
   ```

4. **Ctrl+D handling**:
   ```bash
   # Press Ctrl+D
   # Expected: shell exits gracefully
   ```

5. **Help command**:
   ```bash
   # Type: help
   # Expected: list of built-in commands with descriptions
   ```

6. **Version command**:
   ```bash
   # Type: version
   # Expected: version number and build date
   ```

---

## Prompt 19: Documentation & Examples

### Prompt
```
Complete comprehensive documentation:

1. Update README.md with:
   - Project overview and features
   - Installation instructions
   - Quick start guide
   - Feature list with examples
   - Build instructions
   - Testing instructions
   - Architecture overview
   - Contributing guidelines

2. Create docs/USER_GUIDE.md:
   - Detailed usage instructions
   - All commands with examples
   - Advanced features (pipelines, conditionals, etc.)
   - Tips and tricks

3. Create docs/DEVELOPER_GUIDE.md:
   - Architecture explanation
   - Code structure
   - How to add new features
   - Testing guidelines
   - Code style guide

4. Create examples/:
   - example_scripts.sh - Sample shell scripts
   - tutorial.txt - Step-by-step tutorial
   - advanced_examples.sh - Complex use cases

5. Add inline comments to all source files:
   - Function documentation
   - Complex algorithm explanations
   - TODO markers for future work
```

### Manual Tests
1. **README completeness**:
   ```bash
   # Verify README includes:
   # - Feature list
   # - Installation steps
   # - Quick start
   # - Examples
   # - Links to other docs
   ```

2. **Follow installation instructions**:
   ```bash
   # In a fresh clone/directory:
   # Follow README.md instructions step-by-step
   # Verify shell builds and runs
   ```

3. **Run example scripts**:
   ```bash
   cd examples
   ../ushell < example_scripts.sh
   # Expected: all examples run successfully
   ```

4. **Tutorial walkthrough**:
   ```bash
   # Follow tutorial.txt step-by-step
   # Verify all commands work as documented
   ```

---

## Prompt 20: Final Polish & Release

### Prompt
```
Final polish and preparation for release:

1. Code cleanup:
   - Format all code consistently (indentation, style)
   - Remove debug printf statements
   - Remove commented-out code
   - Fix any compiler warnings

2. Optimization:
   - Profile performance with gprof
   - Optimize hot paths if needed
   - Reduce memory footprint

3. Version tagging:
   - Set version to 1.0.0 in version.h
   - Update all documentation with version
   - Create git tag

4. Final testing:
   - Run full test suite
   - Test on different systems (if available)
   - Verify valgrind clean
   - Stress test with large files/pipelines

5. Create distribution:
   - Clean build
   - Create tarball
   - Write CHANGELOG.md

6. Optional: Compare with original projects
   - Verify all features from shell.c work
   - Verify all features from ShellByGrammar work
   - Verify all tools from StandAloneTools work
   - Verify new glob features work
```

### Manual Tests
1. **Clean build**:
   ```bash
   make clean
   make
   # Should compile with 0 warnings
   ```

2. **Full test suite**:
   ```bash
   cd tests
   ./test_runner.sh
   # Expected: 100% pass rate
   ```

3. **Valgrind full check**:
   ```bash
   tests/valgrind_test.sh
   # Expected: 0 leaks, 0 errors
   ```

4. **Stress test**:
   ```bash
   # Create large file
   seq 1 10000 > large.txt
   # Test: cat large.txt | grep 5 | wc -l
   # Expected: correct count
   ```

5. **Feature comparison checklist**:
   ```
   ✓ Variables (from shell.c and ShellByGrammar)
   ✓ Arithmetic (from ShellByGrammar calc)
   ✓ Conditionals (from ShellByGrammar)
   ✓ Pipelines (from shell.c)
   ✓ Redirection (from shell.c)
   ✓ Built-ins (from shell.c)
   ✓ All tools (from StandAloneTools)
   ✓ Editor (from StandAloneTools)
   ✓ Glob expansion (new feature)
   ```

6. **User acceptance testing**:
   ```bash
   # Real-world scenarios:
   # - Navigate directories
   # - Manipulate files
   # - Use pipelines
   # - Write simple scripts
   # - Use conditionals
   # - Use variables and arithmetic
   ```

7. **Documentation verification**:
   ```
   ✓ README.md complete
   ✓ USER_GUIDE.md complete
   ✓ DEVELOPER_GUIDE.md complete
   ✓ All examples work
   ✓ CHANGELOG.md created
   ```

---

## Summary

This prompt sequence provides a complete, step-by-step implementation plan with manual testing procedures for each phase. Following these prompts in order will result in a fully functional unified shell with all requested features.

### Key Milestones
- Prompts 1-2: Foundation
- Prompts 3-5: Parsing & Variables
- Prompts 6-7: Basic Execution
- Prompts 8-10: Advanced Features
- Prompts 11-12: Tool Integration
- Prompts 13-14: Glob Expansion
- Prompt 15: Editor (Optional)
- Prompts 16-17: Testing & Quality
- Prompts 18-19: Polish & Documentation
- Prompt 20: Release

Each prompt builds on previous work, and manual tests ensure each phase is completed correctly before moving to the next.
