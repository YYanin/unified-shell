# New Feature Implementation Prompts for Unified Shell

This document contains step-by-step prompts to implement three new major features:
1. Argtable3 integration for command-line parsing with autocomplete and history
2. Simple local package manager (apt)
3. Job control system

Each prompt includes manual test procedures to verify successful completion.

---

## Feature 1: Argtable3 Integration

### Prompt 1.1: Install and Setup Argtable3

#### Prompt
```
Install argtable3 library and integrate it into the unified shell build system:

1. Install argtable3:
   - Ubuntu/Debian: sudo apt-get install libargtable3-dev
   - Or build from source: https://github.com/argtable/argtable3

2. Update Makefile to:
   - Add -largtable3 to LDFLAGS
   - Add argtable3 header path if needed

3. Create include/argtable_defs.h with:
   - Common argtable structure definitions
   - Wrapper functions for argument parsing

4. Create src/utils/arg_parser.c:
   - Initialize argtable structures for shell commands
   - Parse command arguments using argtable3
   - Handle error reporting

5. Test basic compilation with argtable3
```

#### Manual Tests
1. **Library installation check**:
   ```bash
   pkg-config --cflags --libs argtable3
   # Expected: compiler and linker flags
   ```

2. **Compilation test**:
   ```bash
   cd unified-shell
   make clean && make
   # Should compile without errors
   ```

3. **Basic argtable test** (add to main.c temporarily):
   ```c
   #include <argtable3.h>
   struct arg_lit *help = arg_lit0("h", "help", "display help");
   struct arg_end *end = arg_end(20);
   void *argtable[] = {help, end};
   printf("Argtable3 initialized\n");
   arg_freetable(argtable, sizeof(argtable)/sizeof(argtable[0]));
   ```
   Expected: "Argtable3 initialized" without crashes

---

### Prompt 1.2: Replace Command Tokenizer with Argtable3

#### Prompt
```
Replace the existing tokenize_command() function with argtable3-based parsing:

1. Update src/evaluator/executor.c:
   - Remove old tokenize_command() function
   - Create parse_command_argtable(char *line, char ***argv)
   - Use argtable3 to parse command-line arguments
   - Handle quoted strings properly
   - Support escape sequences

2. Handle common command flags:
   - -h, --help for all commands
   - -v, --verbose for debugging
   - Command-specific flags

3. Update execute_command() to use new parser

4. Maintain backward compatibility with existing commands
```

#### Manual Tests
1. **Basic command parsing**:
   ```bash
   ./ushell
   # Type: echo hello world
   # Expected: hello world
   ```

2. **Quoted arguments**:
   ```bash
   # Type: echo "hello world"
   # Expected: hello world (as single argument)
   ```

3. **Command with flags**:
   ```bash
   # Type: ls -la /tmp
   # Expected: detailed directory listing
   ```

4. **Complex parsing**:
   ```bash
   # Type: echo "arg1" arg2 "arg with spaces" arg4
   # Expected: correct argument separation
   ```

---

### Prompt 1.3: Implement Command History with Readline

#### Prompt
```
Integrate GNU readline library for command history and line editing:

1. Install readline:
   - sudo apt-get install libreadline-dev

2. Update Makefile:
   - Add -lreadline to LDFLAGS

3. Replace fgets() in main.c REPL with readline():
   - Use readline(prompt) instead of fgets()
   - Add commands to history with add_history()
   - Load history from ~/.ushell_history on startup
   - Save history on exit

4. Create src/utils/history.c:
   - void history_init(void)
   - void history_load(const char *filename)
   - void history_save(const char *filename)
   - void history_add(const char *line)

5. Implement history built-in command:
   - Display command history
   - Support history expansion (!n, !!, !string)
```

#### Manual Tests
1. **Basic history**:
   ```bash
   ./ushell
   # Type: echo test1
   # Type: echo test2
   # Press Up arrow
   # Expected: "echo test2" appears
   # Press Up arrow again
   # Expected: "echo test1" appears
   ```

2. **History persistence**:
   ```bash
   ./ushell
   # Type: echo save this
   # Type: exit
   ./ushell
   # Press Up arrow
   # Expected: "echo save this" appears
   ```

3. **History command**:
   ```bash
   # Type: history
   # Expected: list of recent commands with numbers
   ```

4. **History expansion**:
   ```bash
   # Type: echo test
   # Type: !!
   # Expected: "echo test" executes again
   ```

5. **Line editing**:
   ```bash
   # Type: echo hello
   # Press Home, type "x ", End
   # Expected: "xecho hello"
   # Use Ctrl+A, Ctrl+E, Ctrl+K, Ctrl+U
   # Expected: standard readline shortcuts work
   ```

---

### Prompt 1.4: Implement Tab Completion

#### Prompt
```
Add tab completion for commands, files, and variables:

1. Implement completion functions in src/utils/completion.c:
   - char** command_completion(const char *text, int start, int end)
   - char* command_generator(const char *text, int state)
   - char** filename_completion(const char *text, int start, int end)
   - char** variable_completion(const char *text, int start, int end)

2. Register completion function with readline:
   - rl_attempted_completion_function = shell_completion
   - rl_completion_entry_function for file/command completion

3. Implement smart completion:
   - First word: complete commands (built-ins + PATH)
   - After first word: complete filenames
   - After $: complete variable names

4. Add custom completion for built-in commands with flags

5. Include header: include/completion.h
```

#### Manual Tests
1. **Command completion**:
   ```bash
   ./ushell
   # Type: ec<TAB>
   # Expected: completes to "echo"
   # Type: my<TAB>
   # Expected: shows myls, mycat, mycp, etc.
   ```

2. **File completion**:
   ```bash
   # Type: cat RE<TAB>
   # Expected: completes to "README.md"
   # Type: ls src/<TAB>
   # Expected: shows subdirectories
   ```

3. **Variable completion**:
   ```bash
   # Type: export testvar=hello
   # Type: echo $te<TAB>
   # Expected: completes to "$testvar"
   ```

4. **Path completion**:
   ```bash
   # Type: cd /tm<TAB>
   # Expected: completes to "/tmp/"
   ```

5. **Built-in completion**:
   ```bash
   # Type: myls -<TAB>
   # Expected: shows -l, -a, -la options
   ```

---

## Feature 2: Simple Package Manager (apt)

### Prompt 2.1: Package Repository Structure

#### Prompt
```
Create local package repository structure and management:

1. Create directory structure:
   ```
   ~/.ushell/
   ├── packages/          # Installed packages
   ├── repo/             # Local repository
   │   ├── available/    # Available packages
   │   ├── cache/        # Downloaded packages
   │   └── index.txt     # Package index
   └── apt.conf          # Configuration
   ```

2. Create src/apt/repo.c and include/apt.h:
   - void apt_init(void) - Initialize package system
   - int apt_load_index(void) - Load package index
   - int apt_save_index(void) - Save package index

3. Package structure:
   ```c
   typedef struct {
       char name[64];
       char version[16];
       char description[256];
       char filename[128];
       char dependencies[256];
       int installed;
   } Package;
   ```

4. Create package index format (index.txt):
   ```
   PackageName: toolname
   Version: 1.0
   Description: A useful tool
   Filename: toolname-1.0.tar.gz
   Depends: libc
   
   PackageName: anothertool
   ...
   ```

5. Initialize repository on first run
```

#### Manual Tests
1. **Directory creation**:
   ```bash
   ./ushell
   # Type: apt init
   # Check: ls ~/.ushell/
   # Expected: packages/, repo/, apt.conf created
   ```

2. **Index file check**:
   ```bash
   cat ~/.ushell/repo/index.txt
   # Expected: empty or sample entries
   ```

3. **Repository initialization**:
   ```bash
   # Type: apt update
   # Expected: "Package index loaded" message
   ```

---

### Prompt 2.2: Implement apt Built-in Command

#### Prompt
```
Create apt command as a built-in with subcommands:

1. Create src/apt/apt_builtin.c:
   - int builtin_apt(char **argv, Env *env)
   - Parse subcommands: install, remove, update, list, search

2. Implement subcommands:
   - apt update: Refresh package index
   - apt list: Show all available packages
   - apt search <term>: Search for packages
   - apt install <pkg>: Install package
   - apt remove <pkg>: Remove package
   - apt show <pkg>: Show package details

3. Add apt to built-in command table in builtins.c

4. Create include/apt.h with function declarations

5. Update Makefile to compile apt sources
```

#### Manual Tests
1. **apt help**:
   ```bash
   ./ushell
   # Type: apt
   # Expected: usage information with subcommands
   ```

2. **apt list**:
   ```bash
   # Type: apt list
   # Expected: list of available packages (empty if none)
   ```

3. **apt search**:
   ```bash
   # Type: apt search tool
   # Expected: packages matching "tool"
   ```

4. **Invalid subcommand**:
   ```bash
   # Type: apt invalid
   # Expected: error message, show valid subcommands
   ```

---

### Prompt 2.3: Package Installation System

#### Prompt
```
Implement package installation functionality:

1. Create src/apt/install.c:
   - int apt_install_package(const char *pkgname)
   - Check if package exists in index
   - Check dependencies
   - Extract package to ~/.ushell/packages/<pkgname>/
   - Register as installed
   - Make executable accessible

2. Package format (simple tar.gz):
   ```
   pkgname-version.tar.gz
   ├── bin/           # Executables
   ├── lib/           # Libraries (if needed)
   ├── METADATA       # Package info
   └── README         # Documentation
   ```

3. Update PATH to include ~/.ushell/packages/bin

4. Create package metadata file:
   ```
   Name: toolname
   Version: 1.0
   Description: Tool description
   InstallDate: 2025-11-24
   ```

5. Handle installation errors gracefully

6. Update Makefile
```

#### Manual Tests
1. **Create test package** (manual setup):
   ```bash
   mkdir -p /tmp/testpkg-1.0/bin
   echo '#!/bin/bash' > /tmp/testpkg-1.0/bin/testcmd
   echo 'echo "Test package works"' >> /tmp/testpkg-1.0/bin/testcmd
   chmod +x /tmp/testpkg-1.0/bin/testcmd
   cd /tmp
   tar czf testpkg-1.0.tar.gz testpkg-1.0/
   mv testpkg-1.0.tar.gz ~/.ushell/repo/available/
   ```

2. **Add to index**:
   ```bash
   echo "PackageName: testpkg" >> ~/.ushell/repo/index.txt
   echo "Version: 1.0" >> ~/.ushell/repo/index.txt
   echo "Description: Test package" >> ~/.ushell/repo/index.txt
   echo "Filename: testpkg-1.0.tar.gz" >> ~/.ushell/repo/index.txt
   echo "" >> ~/.ushell/repo/index.txt
   ```

3. **Install package**:
   ```bash
   ./ushell
   # Type: apt install testpkg
   # Expected: "Installing testpkg..." success message
   ```

4. **Verify installation**:
   ```bash
   # Type: apt list --installed
   # Expected: testpkg appears in list
   # Check: ls ~/.ushell/packages/testpkg/
   # Expected: bin/, METADATA files present
   ```

5. **Run installed command**:
   ```bash
   # Type: testcmd
   # Expected: "Test package works"
   ```

---

### Prompt 2.4: Package Removal and Management

#### Prompt
```
Implement package removal and management features:

1. Create src/apt/remove.c:
   - int apt_remove_package(const char *pkgname)
   - Check if package is installed
   - Check for dependent packages (warn user)
   - Remove package directory
   - Update installed package list

2. Implement apt show command:
   - Display package details
   - Show installation status
   - Show dependencies
   - Show installation date if installed

3. Implement apt list variations:
   - apt list: All available packages
   - apt list --installed: Only installed packages
   - apt list --upgradable: Packages with updates (future)

4. Add package verification:
   - Check package integrity
   - Verify all files present
   - Report missing/corrupted files

5. Update builtins.c with all apt subcommands
```

#### Manual Tests
1. **Show package info**:
   ```bash
   ./ushell
   # Type: apt show testpkg
   # Expected: package name, version, description, status
   ```

2. **List installed packages**:
   ```bash
   # Type: apt list --installed
   # Expected: testpkg (if installed from previous test)
   ```

3. **Remove package**:
   ```bash
   # Type: apt remove testpkg
   # Expected: "Removing testpkg..." success message
   ```

4. **Verify removal**:
   ```bash
   # Type: apt list --installed
   # Expected: testpkg not in list
   # Check: ls ~/.ushell/packages/
   # Expected: testpkg directory removed
   ```

5. **Remove non-existent package**:
   ```bash
   # Type: apt remove nonexistent
   # Expected: error message "Package not found"
   ```

---

### Prompt 2.5: Dependency Management

#### Prompt
```
Add basic dependency resolution for packages:

1. Create src/apt/depends.c:
   - int apt_check_dependencies(const char *pkgname)
   - int apt_resolve_dependencies(const char *pkgname)
   - char** apt_get_dependencies(const char *pkgname)
   - int apt_install_dependencies(const char *pkgname)

2. Dependency format in index.txt:
   ```
   Depends: pkg1, pkg2, pkg3
   ```

3. Installation logic:
   - Check all dependencies
   - Prompt user to install missing dependencies
   - Option: --auto-install to install dependencies automatically
   - Detect circular dependencies

4. Removal logic:
   - Check if other packages depend on this
   - Warn user before removal
   - Option: --force to remove anyway

5. Update apt install/remove to use dependency checking

6. Update Makefile
```

#### Manual Tests
1. **Create dependent packages**:
   ```bash
   # Manually create two packages:
   # basepkg (no dependencies)
   # dependpkg (depends on basepkg)
   # Add both to index with Depends: field
   ```

2. **Install with missing dependency**:
   ```bash
   ./ushell
   # Type: apt install dependpkg
   # Expected: warning about missing basepkg
   #           prompt to install basepkg first
   ```

3. **Install with auto-dependencies**:
   ```bash
   # Type: apt install dependpkg --auto-install
   # Expected: basepkg installed automatically
   #           then dependpkg installed
   ```

4. **Remove with dependents**:
   ```bash
   # Type: apt remove basepkg
   # Expected: warning that dependpkg depends on it
   #           prompt for confirmation
   ```

5. **Force removal**:
   ```bash
   # Type: apt remove basepkg --force
   # Expected: basepkg removed despite dependent
   ```

---

## Feature 3: Job Control

### Prompt 3.1: Job Data Structures and Tracking

#### Prompt
```
Implement job control data structures and tracking system:

1. Create src/jobs/jobs.c and include/jobs.h:
   ```c
   typedef enum {
       JOB_RUNNING,
       JOB_STOPPED,
       JOB_DONE
   } JobStatus;
   
   typedef struct {
       int job_id;
       pid_t pid;
       char *command;
       JobStatus status;
       int background;
   } Job;
   
   typedef struct {
       Job jobs[MAX_JOBS];
       int count;
   } JobList;
   ```

2. Implement functions:
   - void jobs_init(void)
   - int jobs_add(pid_t pid, const char *cmd, int bg)
   - Job* jobs_get(int job_id)
   - void jobs_remove(int job_id)
   - void jobs_update_status(void)
   - void jobs_print_all(void)

3. Add global job list to main.c

4. Update executor.c to register jobs when forking

5. Update Makefile to compile jobs.c
```

#### Manual Tests
1. **Compilation test**:
   ```bash
   make clean && make
   # Should compile without errors
   ```

2. **Job tracking test** (add temporary code to main.c):
   ```c
   jobs_init();
   jobs_add(1234, "test command", 0);
   jobs_print_all();
   // Expected: shows job with id, pid, command
   ```

3. **Job list operations**:
   ```c
   jobs_add(1111, "job1", 0);
   jobs_add(2222, "job2", 1);
   Job *j = jobs_get(1);
   // Expected: returns job1
   jobs_remove(1);
   // Expected: job1 removed from list
   ```

---

### Prompt 3.2: Background Job Execution

#### Prompt
```
Implement background job execution with & operator:

1. Update src/evaluator/executor.c:
   - Detect & at end of command
   - Fork process without waiting (for background jobs)
   - Add to job list
   - Print job number and PID
   - Continue to next prompt immediately

2. Update parse_pipeline() to detect & operator

3. Modify execute_pipeline() to handle background flag:
   ```c
   if (background) {
       // Don't wait for process
       // Add to job list
       printf("[%d] %d\n", job_id, pid);
   } else {
       // Wait as normal
   }
   ```

4. Handle SIGCHLD to detect when background jobs complete:
   - Set up signal handler in main.c
   - Update job status when child exits
   - Print completion message

5. Non-blocking job status checking
```

#### Manual Tests
1. **Basic background job**:
   ```bash
   ./ushell
   # Type: sleep 5 &
   # Expected: [1] <pid> immediately printed
   #           prompt returns immediately
   # After 5 seconds: "[1]+ Done sleep 5"
   ```

2. **Multiple background jobs**:
   ```bash
   # Type: sleep 3 &
   # Type: sleep 2 &
   # Expected: [1] <pid1>
   #           [2] <pid2>
   #           prompt available
   ```

3. **Background with output**:
   ```bash
   # Type: echo "background" &
   # Expected: [1] <pid>
   #           background (printed immediately or async)
   ```

4. **Foreground vs background**:
   ```bash
   # Type: sleep 2
   # Expected: waits 2 seconds, then prompt
   # Type: sleep 2 &
   # Expected: immediate prompt return
   ```

---

### Prompt 3.3: jobs Built-in Command

#### Prompt
```
Implement jobs built-in command to list all jobs:

1. Create int builtin_jobs(char **argv, Env *env) in src/builtins/builtins.c

2. Display format:
   ```
   [1]   Running                 sleep 10 &
   [2]-  Running                 cat &
   [3]+  Stopped                 vim test.txt
   ```
   - [N]: Job number
   - +: Current job
   - -: Previous job
   - Status: Running/Stopped/Done
   - Command: Original command string

3. Support options:
   - jobs -l: Long format with PID
   - jobs -p: Show PIDs only
   - jobs -r: Show running jobs only
   - jobs -s: Show stopped jobs only

4. Update job status before printing (check for completed jobs)

5. Add to built-in command table
```

#### Manual Tests
1. **Empty jobs list**:
   ```bash
   ./ushell
   # Type: jobs
   # Expected: (no output or "No jobs")
   ```

2. **List running jobs**:
   ```bash
   # Type: sleep 10 &
   # Type: sleep 20 &
   # Type: jobs
   # Expected: [1] Running sleep 10 &
   #           [2] Running sleep 20 &
   ```

3. **Long format**:
   ```bash
   # Type: jobs -l
   # Expected: includes PID in output
   #           [1] 12345 Running sleep 10 &
   ```

4. **PID only**:
   ```bash
   # Type: jobs -p
   # Expected: 12345
   #           12346
   ```

5. **Filter by status**:
   ```bash
   # Type: jobs -r
   # Expected: only running jobs
   ```

---

### Prompt 3.4: fg Built-in Command (Foreground)

#### Prompt
```
Implement fg command to bring background jobs to foreground:

1. Create int builtin_fg(char **argv, Env *env) in builtins.c

2. Functionality:
   - fg: Bring most recent background job to foreground
   - fg %n: Bring job n to foreground
   - fg %cmd: Bring job matching cmd to foreground

3. Implementation:
   - Find job in job list
   - Send SIGCONT if stopped
   - Set as foreground process group
   - Wait for job to complete/stop
   - Update job status

4. Handle terminal control:
   - tcsetpgrp() to give terminal to job
   - Restore terminal to shell after

5. Handle Ctrl+Z (SIGTSTP) to stop foreground job

6. Add to built-in command table
```

#### Manual Tests
1. **Bring job to foreground**:
   ```bash
   ./ushell
   # Type: sleep 30 &
   # Expected: [1] <pid>
   # Type: fg
   # Expected: "sleep 30" in foreground
   #           prompt blocked until complete or Ctrl+Z
   ```

2. **Foreground specific job**:
   ```bash
   # Type: sleep 10 &
   # Type: sleep 20 &
   # Type: fg %1
   # Expected: job 1 brought to foreground
   ```

3. **Ctrl+Z stops job**:
   ```bash
   # Type: sleep 30 &
   # Type: fg
   # Press: Ctrl+Z
   # Expected: job stopped, prompt returns
   # Type: jobs
   # Expected: [1]+ Stopped sleep 30
   ```

4. **Resume stopped job**:
   ```bash
   # With stopped job from above:
   # Type: fg
   # Expected: job resumes in foreground
   ```

5. **Error handling**:
   ```bash
   # Type: fg %99
   # Expected: error "No such job"
   ```

---

### Prompt 3.5: bg Built-in Command (Background)

#### Prompt
```
Implement bg command to resume stopped jobs in background:

1. Create int builtin_bg(char **argv, Env *env) in builtins.c

2. Functionality:
   - bg: Resume most recent stopped job in background
   - bg %n: Resume job n in background
   - bg %cmd: Resume job matching cmd in background

3. Implementation:
   - Find stopped job in job list
   - Send SIGCONT to resume
   - Mark as running in background
   - Job continues without blocking prompt

4. Update job status after resuming

5. Print confirmation: [n] jobname &

6. Add to built-in command table
```

#### Manual Tests
1. **Resume stopped job in background**:
   ```bash
   ./ushell
   # Type: sleep 30
   # Press: Ctrl+Z
   # Expected: [1]+ Stopped sleep 30
   # Type: bg
   # Expected: [1]+ sleep 30 &
   # Type: jobs
   # Expected: [1]+ Running sleep 30 &
   ```

2. **Resume specific stopped job**:
   ```bash
   # Type: sleep 10
   # Press: Ctrl+Z (job 1 stopped)
   # Type: sleep 20
   # Press: Ctrl+Z (job 2 stopped)
   # Type: bg %1
   # Expected: [1] sleep 10 & (resumes in background)
   # Type: jobs
   # Expected: [1] Running, [2] Stopped
   ```

3. **Error handling**:
   ```bash
   # Type: bg %99
   # Expected: error "No such job"
   ```

4. **No stopped jobs**:
   ```bash
   # With no stopped jobs:
   # Type: bg
   # Expected: error "No stopped jobs"
   ```

---

### Prompt 3.6: Signal Handling and Job Control Integration

#### Prompt
```
Implement comprehensive signal handling for job control:

1. Update src/jobs/signals.c (new file):
   - void setup_signal_handlers(void)
   - void sigchld_handler(int sig) - Handle child process exit
   - void sigtstp_handler(int sig) - Handle Ctrl+Z
   - void sigint_handler(int sig) - Handle Ctrl+C

2. Signal behavior:
   - SIGCHLD: Update job status, reap zombies
   - SIGTSTP (Ctrl+Z): Stop foreground job, return to prompt
   - SIGINT (Ctrl+C): Terminate foreground job, not shell
   - SIGTTOU/SIGTTIN: Handle terminal control

3. Shell vs foreground process:
   - Shell ignores SIGTSTP and SIGINT
   - Foreground jobs receive these signals
   - Background jobs ignore SIGINT

4. Terminal control:
   - Save/restore terminal attributes
   - Manage process groups
   - Handle terminal foreground control

5. Zombie process cleanup:
   - Reap all completed children
   - Update job list
   - Print completion messages

6. Update main.c to call setup_signal_handlers()

7. Update Makefile
```

#### Manual Tests
1. **Ctrl+C on foreground job**:
   ```bash
   ./ushell
   # Type: sleep 30
   # Press: Ctrl+C
   # Expected: sleep terminated, prompt returns
   #           shell still running
   ```

2. **Ctrl+C on shell**:
   ```bash
   # At empty prompt, press: Ctrl+C
   # Expected: shell continues (not terminated)
   ```

3. **Ctrl+Z stops job**:
   ```bash
   # Type: cat
   # Press: Ctrl+Z
   # Expected: [1]+ Stopped cat
   #           prompt returns
   ```

4. **Background job completion**:
   ```bash
   # Type: sleep 2 &
   # Wait 3 seconds
   # Expected: [1]+ Done sleep 2
   ```

5. **Multiple signal handling**:
   ```bash
   # Type: sleep 10
   # Press: Ctrl+Z (stopped)
   # Type: bg (background)
   # Press: Ctrl+C (should not affect background)
   # Expected: background job continues
   ```

6. **Zombie cleanup**:
   ```bash
   # Type: sleep 1 &
   # Type: sleep 1 &
   # Type: sleep 1 &
   # Wait 2 seconds
   # Type: jobs
   # Expected: all jobs marked Done, no zombies
   # Check: ps aux | grep defunct
   # Expected: no zombie processes
   ```

---

### Prompt 3.7: Job Control Testing and Polish

#### Prompt
```
Create comprehensive tests for job control and polish implementation:

1. Create tests/job_control/:
   - test_background.sh - Background execution tests
   - test_jobs_cmd.sh - jobs command tests
   - test_fg_bg.sh - fg/bg command tests
   - test_signals.sh - Signal handling tests
   - test_integration.sh - Combined scenarios

2. Edge cases to handle:
   - Empty job list
   - Invalid job numbers
   - Job already finished
   - Terminal control conflicts
   - Rapid job creation/deletion

3. Memory management:
   - Free job command strings
   - Clean up job list on exit
   - No memory leaks in signal handlers

4. User experience improvements:
   - Clearer job status messages
   - Better error messages
   - Consistent output formatting
   - Completion notifications

5. Documentation:
   - Update README.md with job control features
   - Add examples to USER_GUIDE.md
   - Document all job-related commands

6. Add help text for jobs, fg, bg commands
```

#### Manual Tests
1. **Comprehensive workflow**:
   ```bash
   ./ushell
   # Type: sleep 100 &        # [1]
   # Type: cat                # Start in foreground
   # Press: Ctrl+Z            # [2] stopped
   # Type: sleep 50 &         # [3]
   # Type: jobs               # Should show all 3
   # Type: fg %2              # Bring cat to foreground
   # Press: Ctrl+C            # Terminate cat
   # Type: jobs               # [1] and [3] still running
   # Type: bg %1              # Redundant but should work
   # Wait for job 3 to finish
   # Type: jobs               # [1] running, [3] done
   ```

2. **Stress test**:
   ```bash
   # Start 10 background jobs:
   for i in {1..10}; do
       # Type: sleep $i &
   done
   # Type: jobs
   # Expected: all 10 jobs listed
   # Wait for completion
   # Expected: completion messages for all
   ```

3. **Error handling**:
   ```bash
   # Type: fg
   # Expected: error if no jobs
   # Type: bg
   # Expected: error if no stopped jobs
   # Type: fg %999
   # Expected: "No such job: 999"
   ```

4. **Memory test**:
   ```bash
   # Run shell through valgrind with job control
   echo -e "sleep 1 &\nwait\njobs\nexit" | \
       valgrind --leak-check=full ./ushell
   # Expected: no memory leaks
   ```

5. **Integration test**:
   ```bash
   cd tests/job_control
   ./test_integration.sh
   # Expected: all tests pass
   ```

---

## Summary

This implementation plan adds three major features to the unified shell:

### Feature 1: Argtable3 Integration (7 prompts)
- Prompts 1.1-1.4: Argtable3 setup, command parsing, history, tab completion
- Benefits: Modern CLI parsing, command history, autocomplete

### Feature 2: Package Manager (5 prompts)
- Prompts 2.1-2.5: Repository structure, apt command, install/remove, dependencies
- Benefits: Extensible tool ecosystem, package management

### Feature 3: Job Control (7 prompts)
- Prompts 3.1-3.7: Job tracking, background execution, jobs/fg/bg commands, signals
- Benefits: Multi-tasking, process management, professional shell features

### Testing Strategy
Each prompt includes:
- Manual test procedures
- Expected outputs
- Error case verification
- Integration testing
- Memory leak checking

### Recommended Execution Order
1. Complete all Feature 1 prompts (Argtable3) first
2. Then complete Feature 2 (Package Manager)
3. Finally complete Feature 3 (Job Control)
4. Run comprehensive integration tests
5. Update all documentation

### Success Criteria
- NOT DONE: All manual tests pass for each prompt
- NOT DONE: No memory leaks (valgrind clean)
- NOT DONE: No compiler warnings
- NOT DONE: Integration with existing shell features
- NOT DONE: Updated documentation
- NOT DONE: Example scripts demonstrating new features

---

## Next Steps

After completing all prompts, verify:
1. All features work independently
2. All features work together
3. No regressions in existing functionality
4. Performance is acceptable
5. User experience is polished
6. Documentation is complete

Then proceed with final release preparations as outlined in original Prompts.md Prompt 20.
