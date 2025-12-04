# Unified Shell (ushell) - User Guide

## Table of Contents
1. [Getting Started](#getting-started)
2. [Interactive Features](#interactive-features)
3. [Basic Commands](#basic-commands)
4. [Variables](#variables)
5. [Arithmetic Operations](#arithmetic-operations)
6. [Control Flow](#control-flow)
7. [Pipelines](#pipelines)
8. [I/O Redirection](#io-redirection)
9. [Glob Expansion](#glob-expansion)
10. [Built-in Commands](#built-in-commands)
11. [Job Control](#job-control)
12. [Package Management (APT)](#package-management-apt)
13. [Integrated Tools](#integrated-tools)
14. [Tips and Tricks](#tips-and-tricks)
15. [Troubleshooting](#troubleshooting)

---

## Getting Started

### Starting the Shell

```bash
# From the unified-shell directory
./ushell
```

You'll see a prompt showing your username and current directory:
```
username:~/path/to/directory>
```

### Exiting the Shell

Three ways to exit:
```bash
# Method 1: exit command
exit

# Method 2: Ctrl+D (EOF)
# Press Ctrl+D

# Method 3: exit with status code
exit 0
```

### Getting Help

```bash
# Display all commands and features
help

# Show version information
version
```

### Interrupting Commands

- **Ctrl+C**: Interrupts the current command or cancels current input, returns to prompt
- **Ctrl+D**: Exits the shell (when no command is running)

---

## Interactive Features

The shell provides advanced interactive features similar to bash/zsh.

### Command Line Editing

Navigate and edit your command line using these keyboard shortcuts:

#### Cursor Movement
- **Left Arrow** (←): Move cursor one character left
- **Right Arrow** (→): Move cursor one character right
- **Ctrl+A**: Jump to beginning of line
- **Ctrl+E**: Jump to end of line

#### Text Editing
- **Backspace**: Delete character before cursor
- **Delete**: Delete character at cursor
- **Ctrl+U**: Clear entire line
- **Ctrl+K**: Clear from cursor to end of line

#### Line Wrapping
- Long commands automatically wrap to multiple lines
- Terminal width is dynamically detected
- Multi-line editing preserves cursor position

### Command History

The shell maintains a persistent command history across sessions.

#### Navigating History
- **Up Arrow** (↑): Previous command in history
- **Down Arrow** (↓): Next command in history
- Navigate through your entire command history with arrow keys
- Edit historical commands before re-executing

#### History Storage
- Commands are saved to `~/.ushell_history`
- History persists across shell sessions
- Empty commands and duplicates are filtered out

#### Using the `history` Command
```bash
# Display all stored commands
history

# View command history with line numbers
history

# Re-run a command by typing it after navigating with arrows
# Example: Press Up Arrow to see last command, then Enter to execute
```

### Tab Completion

Press **Tab** to auto-complete commands and file paths.

#### Command Completion
```bash
# Type first few letters of a command and press Tab
ec<TAB>          # Completes to: echo

# Works with built-in commands
hi<TAB>          # Completes to: history

# Works with integrated tools
my<TAB>          # Shows: mycat mycp myls mymkdir mymv myrm myrmdir mytouch
```

#### Filename Completion
```bash
# Complete filenames in current directory
cat test<TAB>    # Completes to: cat test.txt

# Complete paths
ls /u<TAB>       # Completes to: ls /usr/ or shows options

# Works after commands
grep pattern f<TAB>   # Completes filename after pattern
```

#### Multiple Matches
- If multiple completions are possible, press Tab twice to see all options
- Type more characters to narrow down the match
- Completion preserves the command prefix

### Example Interactive Session

```bash
# Start typing a command
ushell> ec<TAB>
ushell> echo "Hello"

# Navigate history
ushell> <UP ARROW>      # Shows: echo "Hello"
ushell> <LEFT><LEFT>    # Move cursor: echo "He|llo"
ushell> <BACKSPACE>     # Edit: echo "H|llo"  
ushell> i               # Type: echo "Hillo"

# Use tab completion with files
ushell> cat te<TAB>
ushell> cat test.txt

# Cancel input with Ctrl+C
ushell> some long command that I don't want^C
ushell> 
```

---

## Basic Commands

### Running External Commands

```bash
# Simple commands
ls
date
whoami

# Commands with arguments
ls -la /tmp
echo "Hello, World!"
grep pattern file.txt
```

### Command Execution

The shell executes commands by:
1. Searching built-in commands first
2. Checking integrated tools
3. Using PATH to find external executables

---

## Variables

### Setting Variables

#### Shell-Local Variables (set)
Variables only available in the shell, not passed to child processes:

```bash
# Set a variable
set NAME=John
set AGE=25
set PATH_DATA=/usr/local/data

# Use the variable
echo Hello, $NAME
echo You are $AGE years old
```

#### Environment Variables (export)
Variables passed to child processes:

```bash
# Export a variable
export EDITOR=vim
export CUSTOM_VAR=value

# Now child processes can see it
printenv EDITOR
```

### Variable Expansion

#### Basic Expansion
```bash
set X=10
echo $X              # Output: 10
echo Value is $X     # Output: Value is 10
```

#### Braced Expansion
Useful when variable name might be ambiguous:

```bash
set PREFIX=test
echo ${PREFIX}_file.txt    # Output: test_file.txt
echo $PREFIXfile.txt       # Would look for variable "PREFIXfile"
```

### Viewing Variables

```bash
# Show all environment variables
env

# Show specific variable (using echo)
echo $HOME
echo $PATH
echo $USER
```

### Unsetting Variables

```bash
# Remove a variable
set X=10
echo $X              # Output: 10
unset X
echo $X              # Output: (empty)
```

---

## Arithmetic Operations

### Basic Syntax

Use `$((expression))` for arithmetic:

```bash
echo $((5 + 3))           # Output: 8
echo $((10 - 4))          # Output: 6
echo $((6 * 7))           # Output: 42
echo $((20 / 4))          # Output: 5
echo $((17 % 5))          # Output: 2 (modulo)
```

### Operations with Variables

```bash
set X=10
set Y=5

echo $((X + Y))           # Output: 15
echo $((X * Y))           # Output: 50
echo $((X - Y))           # Output: 5
echo $((X / Y))           # Output: 2
```

### Complex Expressions

```bash
echo $((2 + 3 * 4))       # Output: 14 (multiplication first)
echo $(((2 + 3) * 4))     # Output: 20 (parentheses)

set A=5
set B=3
set C=2
echo $((A * B + C))       # Output: 17
```

### Practical Examples

```bash
# Calculate area
set WIDTH=10
set HEIGHT=20
echo Area: $((WIDTH * HEIGHT))

# Convert units
set CELSIUS=25
echo Fahrenheit: $(((CELSIUS * 9 / 5) + 32))

# Calculate percentage
set SCORE=85
set TOTAL=100
echo Percentage: $((SCORE * 100 / TOTAL))%
```

---

## Control Flow

### Conditional Statements

#### Basic If Statement

```bash
if command then
    commands_if_success
fi
```

The condition is any command. If it exits with status 0 (success), the then block executes.

#### Examples

```bash
# Check if file exists
if myls test.txt then
    echo File exists
fi

# Check if directory exists
if myls /tmp then
    echo /tmp directory found
fi

# Check command success
if grep pattern file.txt then
    echo Pattern found
fi
```

#### Using Test Commands

```bash
# Test with true (always succeeds)
if true then
    echo This always runs
fi

# Test with false (always fails)
if false then
    echo This never runs
fi
```

#### Checking Exit Status

```bash
# Run command and check status
if mycat existing_file.txt then
    echo File read successfully
fi

# Command that might fail
if mycp source.txt dest.txt then
    echo Copy successful
fi
```

### Practical Examples

```bash
# Check if can change directory
if cd /tmp then
    pwd
fi

# Create file if doesn't exist
if myls backup.txt then
    echo Backup exists
fi

# With variable expansion
set FILE=test.txt
if mycat $FILE then
    echo Read $FILE successfully
fi
```

---

## Pipelines

### Basic Pipeline

Connect command output to next command's input:

```bash
command1 | command2
```

### Examples

#### Two-Stage Pipelines
```bash
# List files and search for pattern
myls | myfd .c

# Display file and search
mycat file.txt | grep keyword

# Generate data and process
echo "test data" | mycat
```

#### Three-Stage Pipelines
```bash
# List, filter, display
myls /tmp | myfd test | mycat

# Multiple filters
mycat large.txt | grep pattern | mycat
```

### Practical Examples

```bash
# Find all C files
myls src/ | myfd .c

# Search through multiple files
mycat *.txt | grep error

# Process and display
echo "Line 1\nLine 2" | mycat

# Count files (with external wc)
myls | wc -l
```

### Pipeline with Built-ins

```bash
# Echo and pipe
echo "Hello World" | mycat

# Variables in pipelines
set NAME=test
echo $NAME | mycat

# Arithmetic in pipelines
echo $((5 + 5)) | mycat
```

---

## I/O Redirection

### Output Redirection

#### Redirect to File (>)
Overwrites the file:

```bash
# Redirect echo output
echo "Hello" > output.txt

# Redirect command output
myls > file_list.txt

# Redirect with variables
set MSG="Important data"
echo $MSG > data.txt
```

#### Append to File (>>)
Adds to the end of file:

```bash
# Append to file
echo "Line 1" > log.txt
echo "Line 2" >> log.txt
echo "Line 3" >> log.txt

# Append command output
myls >> file_log.txt
```

### Input Redirection

#### Redirect from File (<)
Read command input from file:

```bash
# Read from file
mycat < input.txt

# With grep
grep pattern < data.txt
```

### Combining Redirections

```bash
# Input and output
mycat < input.txt > output.txt

# Input and append
mycat < data.txt >> combined.txt

# Multiple operations
echo "Header" > report.txt
mycat < data.txt >> report.txt
echo "Footer" >> report.txt
```

### Practical Examples

```bash
# Save directory listing
myls /tmp > tmp_contents.txt

# Build a report
echo "System Report" > report.txt
echo "=============" >> report.txt
echo "Date: $(date)" >> report.txt
myls >> report.txt

# Process file and save
mycat data.txt > processed.txt

# Copy file using redirection
mycat < source.txt > destination.txt
```

---

## Glob Expansion

### Wildcard Patterns

#### Star (*) - Match Any Characters
```bash
# Match all .txt files
echo *.txt

# Match with prefix
echo test*.txt

# Match with suffix
echo *_backup.txt

# Match all files
echo *

# Use with commands
myls *.c
mycat test*.txt
myrm old_*.log
```

#### Question Mark (?) - Match Single Character
```bash
# Match file1, file2, etc.
echo file?.txt

# Match two characters
echo test??.txt

# Combine with other patterns
echo data_?.txt
```

### Character Classes

#### Range Classes
```bash
# Match file1.txt through file5.txt
echo file[1-5].txt

# Match a-z
echo [a-z]*.txt

# Match numbers
echo test[0-9].log
```

#### Explicit Sets
```bash
# Match specific characters
echo file[abc].txt        # Matches filea.txt, fileb.txt, filec.txt

# Multiple ranges
echo [a-zA-Z]*.txt

# Numbers
echo test[0123456789].txt
```

#### Negation
```bash
# Match everything except
echo [!a]*.txt           # All .txt files NOT starting with 'a'
echo file[!0-9].txt      # file*.txt where * is not a digit
```

### Practical Examples

```bash
# List all C source files
myls *.c

# Copy all text files
mycp *.txt backup/

# Remove old logs
myrm old_*.log

# Find all test files
myls test*.txt

# Match specific patterns
echo [Tt]est*.txt        # Test*.txt or test*.txt

# Complex patterns
echo *[0-9][0-9].txt     # Files ending with two digits before .txt
```

### Combining Patterns

```bash
# Multiple patterns in one command
echo *.c *.h *.txt

# With pipes
myls *.txt | myfd backup

# With redirection
mycat *.txt > combined.txt
```

---

## Built-in Commands

### cd - Change Directory

```bash
# Change to specific directory
cd /tmp
cd /home/user/documents

# Change to home directory
cd
cd ~

# Relative paths
cd ..
cd ../..
cd ./subdirectory

# With variables
set PROJECTS=/home/user/projects
cd $PROJECTS
```

### pwd - Print Working Directory

```bash
# Show current directory
pwd

# Use in scripts
set CURRENT=$(pwd)
echo Current directory: $CURRENT
```

### echo - Display Text

```bash
# Simple text
echo Hello World

# With variables
set NAME=John
echo Hello, $NAME

# Multiple arguments
echo The answer is $((21 * 2))

# With special characters
echo "Line 1\nLine 2"

# Empty line
echo
```

### export - Environment Variables

```bash
# Export variable
export PATH=/usr/local/bin:$PATH
export EDITOR=vim
export LANG=en_US.UTF-8

# Set and export in one command
export MY_VAR=value
```

### set - Shell Variables

```bash
# Set variable (shell-local)
set X=10
set NAME=TestUser
set DATA_DIR=/var/data

# Multiple variables
set A=1
set B=2
set C=3
```

### unset - Remove Variables

```bash
# Remove variable
set TEMP=value
echo $TEMP
unset TEMP
echo $TEMP             # (empty)

# Remove environment variable
export MY_VAR=test
unset MY_VAR
```

### env - Display Environment

```bash
# Show all environment variables
env

# Use with grep to find specific variable
env | grep PATH
env | grep HOME
```

### help - Command Help

```bash
# Display all help information
help
```

Shows:
- All built-in commands with descriptions
- Integrated tools list
- Feature summary

### version - Version Information

```bash
# Show version and build info
version
```

Shows:
- Version number
- Build date and time
- Features list
- Tool count

### exit - Exit Shell

```bash
# Exit with success
exit

# Exit with specific code
exit 0
exit 1
```

### history - Command History

```bash
# Display all stored commands
history

# Shows command history with timestamps
# History persists across shell sessions
# Stored in ~/.ushell_history
```

Features:
- Shows all commands from current and previous sessions
- Commands are saved automatically
- Empty commands and duplicates filtered
- Navigate history with Up/Down arrow keys

### edi - Text Editor

```bash
# Edit a file
edi filename.txt

# Create new file
edi newfile.c

# Edit with glob
edi *.txt     # Opens first match
```

Features:
- Vi-like text editor built into the shell
- Basic text editing capabilities
- Integrated without external dependencies
- See `edi --help` for usage details

---

## Job Control

The unified shell provides comprehensive job control for managing foreground and background processes, similar to bash and other modern shells.

### Background Jobs

Run commands in the background by appending `&` to the command:

```bash
# Start a long-running process in background
sleep 30 &
[1] 12345

# Shell returns immediately, you can continue working
echo "Still responsive"

# Start multiple background jobs
sleep 10 &    # [1] created
sleep 20 &    # [2] created
sleep 30 &    # [3] created
```

**Benefits:**
- Run long processes without blocking the shell
- Execute multiple tasks simultaneously
- Continue interactive work while jobs run

### The jobs Command

List all background and stopped jobs:

```bash
# Basic format - shows job status
jobs
[1]-  Running              sleep 10 &
[2]+  Running              sleep 20 &

# Long format - includes process IDs
jobs -l
[1]-  12345  Running              sleep 10 &
[2]+  12346  Running              sleep 20 &

# Show PIDs only (useful for scripting)
jobs -p
12345
12346

# Show only running jobs
jobs -r

# Show only stopped jobs
jobs -s
```

**Job Status:**
- `Running` - Job is currently executing
- `Stopped` - Job is suspended (via Ctrl+Z)
- `Done` - Job has completed

**Job Markers:**
- `+` - Most recent job (default target for fg/bg commands)
- `-` - Second most recent job
- No marker - Other jobs

**Options:**
- `-l` - Long format with PIDs
- `-p` - PIDs only (one per line)
- `-r` - Show only running jobs
- `-s` - Show only stopped (suspended) jobs

### Signal Handling

The shell handles two important keyboard signals:

#### Ctrl+Z - Stop (Suspend) Process

Stops the currently running foreground process and returns control to the shell:

```bash
# Start a long-running command
sleep 60

# Press Ctrl+Z to stop it
^Z
[1]+  Stopped                 sleep 60

# Shell prompt returns, job is suspended
jobs
[1]+  Stopped                 sleep 60
```

**Use cases:**
- Pause a long-running command temporarily
- Switch to another task without killing the process
- Resume later with `fg` or `bg`

#### Ctrl+C - Interrupt (Terminate) Process

Terminates the currently running foreground process:

```bash
# Start a command
sleep 60

# Press Ctrl+C to kill it
^C

# Command terminated, shell continues
```

**Important Notes:**
- Ctrl+C and Ctrl+Z only affect **foreground** processes
- Shell itself ignores these signals at the prompt
- Background jobs are immune to Ctrl+C
- You can't stop the shell itself with Ctrl+C (use `exit` or Ctrl+D)

### The fg Command

Bring a background or stopped job to the foreground:

```bash
# Start a background job
sleep 30 &
[1] 12345

# Bring it to foreground (becomes blocking)
fg
sleep 30 &

# Or specify job number
fg %1

# Or without % prefix
fg 1
```

**Usage Patterns:**
- `fg` - Bring most recent job to foreground
- `fg %n` - Bring job number n to foreground
- `fg %string` - Bring job matching string (if implemented)

**What happens:**
1. Job is moved from background to foreground
2. If job was stopped, it's automatically resumed
3. Shell waits for job to complete or be stopped again
4. Job receives keyboard signals (Ctrl+C, Ctrl+Z)
5. Terminal output is visible

**Example workflow:**
```bash
# Start in background
sleep 100 &
[1] 12345

# Later, bring to foreground
fg
sleep 100 &

# Now can interrupt with Ctrl+C or stop with Ctrl+Z
```

### The bg Command

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

# Job continues, shell is free
jobs
[1]+  Running                 sleep 60 &
```

**Usage Patterns:**
- `bg` - Resume most recent stopped job
- `bg %n` - Resume stopped job number n

**What happens:**
1. Stopped job receives SIGCONT signal
2. Job status changes from Stopped to Running
3. Job continues execution in background
4. Shell prompt remains available

**Important:**
- `bg` only works on stopped jobs
- Trying to `bg` a running job gives error: "job already in background"
- Trying to `bg` with no stopped jobs gives error: "no stopped jobs"

### Complete Job Control Workflow

Here's a practical example using all job control features:

```bash
# 1. Start a long compilation in background
make all &
[1] 12345

# 2. Start a file monitor in foreground
tail -f /var/log/app.log

# 3. Need to check something - stop the tail
^Z
[2]+  Stopped                 tail -f /var/log/app.log

# 4. Start another task
grep -r "pattern" . &
[3] 12348

# 5. List all jobs
jobs
[1]-  Running                 make all &
[2]+  Stopped                 tail -f /var/log/app.log
[3]   Running                 grep -r "pattern" . &

# 6. Resume the tail in background
bg %2
[2]+ tail -f /var/log/app.log &

# 7. Bring grep to foreground to see results
fg %3

# 8. Interrupt grep when done
^C

# 9. Check remaining jobs
jobs
[1]+  Running                 make all &
[2]-  Running                 tail -f /var/log/app.log &

# 10. Bring tail to foreground to stop it
fg %2

# 11. Stop and kill it
^C
```

### Job Completion Notifications

When background jobs complete, you'll see a notification message:

```bash
# Start a short background job
sleep 2 &
[1] 12345

# Continue working...
ls

# After job completes (on next command or Enter):
[1]+  Done                    sleep 2 &
```

**Automatic cleanup:**
- Completed jobs are marked as "Done"
- Jobs are automatically removed from the job list
- No zombie processes accumulate
- SIGCHLD signals are handled properly

### Advanced Job Control Features

#### Pipeline as Background Job

Run entire pipelines in the background:

```bash
# Complex pipeline in background
cat large_file.txt | grep "pattern" | sort | uniq > results.txt &
[1] 12345

# All commands in pipeline run in background
jobs
[1]+  Running                 cat large_file.txt | grep "pattern" | sort | uniq > results.txt &
```

#### Job Numbers and Reuse

Job numbers are assigned sequentially:
- Start from [1]
- Increment for each new job
- When jobs complete, numbers may be reused
- Job IDs are shown in `jobs` output

```bash
sleep 5 &     # [1]
sleep 5 &     # [2]
sleep 5 &     # [3]
# Wait for job 1 to finish
sleep 5 &     # May become [1] again or [4]
```

#### Signal Isolation

The shell properly isolates signals:

**Foreground jobs:**
- Receive Ctrl+C (SIGINT) - terminates
- Receive Ctrl+Z (SIGTSTP) - suspends
- Own the terminal

**Background jobs:**
- Don't receive Ctrl+C
- Can't be suspended with Ctrl+Z
- Don't own the terminal
- Continue running regardless of shell activity

**Shell itself:**
- Ignores Ctrl+C when at prompt
- Ignores Ctrl+Z when at prompt
- Only exits with `exit` command or Ctrl+D

#### Process Groups

The shell uses process groups for proper signal handling:
- Each job runs in its own process group
- Pipelines: all commands in same process group
- Signals sent to entire process group
- Handles complex pipelines correctly

### Common Job Control Patterns

#### Pattern 1: Background compilation
```bash
# Start build, continue working
make clean && make all &
[1] 12345

# Do other work
vim src/main.c

# Check if build done
jobs
[1]+  Running                 make clean && make all &
```

#### Pattern 2: Multiple parallel tasks
```bash
# Start several independent jobs
./process_images.sh &
./generate_reports.sh &
./backup_data.sh &

# Monitor progress
jobs -l

# Bring one to foreground to see output
fg %1
```

#### Pattern 3: Pause and resume
```bash
# Start interactive command
vim large_file.txt

# Need shell quickly - pause vim
^Z
[1]+  Stopped                 vim large_file.txt

# Do quick command
ls -la

# Resume vim
fg
```

#### Pattern 4: Check and terminate
```bash
# Start background job
sleep 100 &
[1] 12345

# Later, bring to foreground and terminate
fg
^C
```

### Troubleshooting Job Control

**"no current job" error:**
- Trying to use `fg` or `bg` with empty job list
- Solution: Start a job first, or specify job number

**"no such job" error:**
- Job number doesn't exist
- Solution: Run `jobs` to see available jobs

**"job already in background" error:**
- Trying to `bg` a running background job
- Solution: Job is already running, no action needed

**Job stuck in Stopped state:**
- Forgot to resume after Ctrl+Z
- Solution: Use `bg %n` to resume or `fg %n` then ^C to kill

**Can't stop the shell with Ctrl+C:**
- This is intentional behavior
- Solution: Use `exit` or Ctrl+D to exit

**Zombie processes:**
- Should not occur - shell automatically reaps children
- Solution: If you see zombies, it's a bug - report it

### Job Control Best Practices

1. **Use background jobs for long tasks:**
   ```bash
   long_running_script.sh &
   ```

2. **Check jobs periodically:**
   ```bash
   jobs    # Quick check
   ```

3. **Use Ctrl+Z for quick breaks:**
   - Pause editor to run a command
   - Resume with `fg`

4. **Clean up stopped jobs:**
   - Don't leave jobs stopped indefinitely
   - Either `bg` to resume or `fg` then ^C to kill

5. **Use job numbers explicitly:**
   ```bash
   fg %1    # More explicit than just 'fg'
   ```

6. **Monitor job completion:**
   - Press Enter periodically to see "Done" messages
   - Or check with `jobs` command

---

## Package Management (APT)

The unified shell includes a comprehensive package management system similar to Debian's APT.

### Overview

The APT subsystem allows you to:
- Install and manage software packages
- Automatically resolve dependencies
- Verify package integrity
- Search and browse available packages
- Safely remove packages with dependency checking

### Getting Started with APT

#### Initialize the Package System

First-time setup (creates `~/.ushell/` directory structure):

```bash
apt init
```

Output:
```
Initializing package system...
Package system initialized.
Created directory structure in ~/.ushell/
Loaded 3 package(s) from index.
```

#### Update the Package Index

Reload the package index from disk:

```bash
apt update
```

Output:
```
Updating package index...
Package index loaded.
Found 6 package(s), 0 installed.
```

### Listing Packages

#### List All Available Packages

```bash
apt list
```

Output:
```
Package              Version    Status   Description
-------              -------    ------   -----------
hello                1.0.0               A simple hello world program
mathlib              2.1.0               Mathematical functions library
textutils            1.5.2               Text processing utilities
basepkg              1.0                 Base package with no dependencies
dependpkg            1.0                 Package that depends on basepkg

5 package(s) listed.
```

#### List Only Installed Packages

```bash
apt list --installed
```

Output:
```
Package              Version    Status   Description
-------              -------    ------   -----------
basepkg              1.0        [inst]   Base package with no dependencies
dependpkg            1.0        [inst]   Package that depends on basepkg

2 package(s) listed.
```

### Searching for Packages

Search by keyword in name or description:

```bash
apt search math
```

Output:
```
mathlib - Mathematical functions library (version 2.1.0)

1 package(s) found.
```

### Viewing Package Details

```bash
apt show mathlib
```

Output:
```
Package: mathlib
Version: 2.1.0
Status: not installed
Filename: mathlib-2.1.0.tar.gz
Dependencies: none
Description: Mathematical functions library
```

For installed packages, shows installation date:
```bash
apt show basepkg
```

Output:
```
Package: basepkg
Version: 1.0
Status: installed
Installed: 2025-11-25 21:34:56
Filename: basepkg-1.0.tar.gz
Dependencies: none
Description: Base package with no dependencies
```

### Installing Packages

#### Basic Installation

Install a package (checks dependencies):

```bash
apt install basepkg
```

Output:
```
Installing package 'basepkg'...
Checking dependencies...
Dependencies satisfied.
Creating package directory: /home/user/.ushell/packages/basepkg
Extracting package archive...
Package extracted successfully.
Creating package metadata...
Setting up executables...
Made 1 executable(s) accessible.

========================================
Successfully installed: basepkg (version 1.0)
========================================

Package binaries are located in:
  /home/user/.ushell/packages/basepkg/bin

Or restart ushell to automatically include package paths.
```

#### Install with Missing Dependencies

If dependencies are missing, you'll get an error:

```bash
apt install dependpkg
```

Output:
```
Installing package 'dependpkg'...
Checking dependencies...
apt install: missing dependencies: basepkg

Install dependencies first:
  apt install basepkg
Or use --auto-install flag to install dependencies automatically.
apt install: dependency check failed
```

#### Automatic Dependency Resolution

Use `--auto-install` flag to automatically install dependencies:

```bash
apt install dependpkg --auto-install
```

Output:
```
Auto-installing dependencies for dependpkg...
Resolving dependencies for 'dependpkg'...

The following packages will be installed:
  1. basepkg (dependency)
  2. dependpkg (requested)

Installing dependency 1/2: basepkg
[...installation output...]
Successfully installed: basepkg (version 1.0)

Installing dependency 2/2: dependpkg
[...installation output...]
Successfully installed: dependpkg (version 1.0)
```

#### Already Installed Packages

Attempting to reinstall a package:

```bash
apt install basepkg
```

Output:
```
Installing package 'basepkg'...
Package 'basepkg' version 1.0 is already installed.
To reinstall, first run: apt remove basepkg
```

### Removing Packages

#### Basic Removal

Remove an installed package:

```bash
apt remove basepkg
```

If other packages depend on it, you'll see a warning:
```
Removing package 'basepkg'...

WARNING: The following packages depend on 'basepkg':
  dependpkg

Removing 'basepkg' may break these packages.
Consider removing dependent packages first, or use --force flag.
Removing package directory: /home/user/.ushell/packages/basepkg
Package files removed successfully.

========================================
Successfully removed: basepkg
========================================
```

#### Force Removal

Use `--force` to bypass dependent warnings:

```bash
apt remove basepkg --force
```

Output:
```
Removing package 'basepkg' (forced)...
Skipping dependent checking (--force flag is set).
Removing package directory: /home/user/.ushell/packages/basepkg
Package files removed successfully.

========================================
Successfully removed: basepkg
========================================
```

#### Removing Non-installed Packages

```bash
apt remove nonexistent
```

Output:
```
Removing package 'nonexistent'...
apt remove: package 'nonexistent' not found in repository
Run 'apt list' to see available packages.
```

### Verifying Package Integrity

Check if an installed package is intact:

```bash
apt verify basepkg
```

Output (successful):
```
Verifying package 'basepkg'...
  [OK] Package directory exists
  [OK] METADATA file present
  [OK] METADATA content valid
  [OK] bin/ directory present
  [OK] Found 1 executable(s)

Package verification: PASSED
```

Output (if files are missing):
```
Verifying package 'basepkg'...
  [OK] Package directory exists
  [OK] METADATA file present
  [OK] METADATA content valid
  [FAIL] bin/ directory missing
  [FAIL] No executables found

Package verification: FAILED
Some files are missing or corrupted.
Consider reinstalling the package.
```

### Dependency Management

#### Understanding Dependencies

Packages can declare dependencies in the repository index:

```
PackageName: myapp
Version: 1.0
Description: My application
Depends: basepkg, libutils
Filename: myapp-1.0.tar.gz
```

#### Dependency Resolution Order

APT automatically determines the correct installation order:

```bash
apt install myapp --auto-install
```

APT will install:
1. basepkg (dependency of myapp)
2. libutils (dependency of myapp)
3. myapp (requested package)

#### Circular Dependency Detection

APT detects circular dependencies:

```bash
# If pkgA depends on pkgB, and pkgB depends on pkgA
apt install pkgA --auto-install
```

Output:
```
Circular dependency detected: pkgA -> pkgB -> pkgA
Cannot resolve dependencies due to circular dependency
```

### APT Directory Structure

Packages are stored in `~/.ushell/`:

```
~/.ushell/
├── packages/           # Installed packages
│   ├── basepkg/
│   │   ├── bin/        # Executable files
│   │   │   └── basecmd
│   │   ├── lib/        # Libraries (optional)
│   │   └── METADATA    # Package metadata
│   └── dependpkg/
│       ├── bin/
│       │   └── dependcmd
│       └── METADATA
├── repo/
│   ├── available/      # Package archives (.tar.gz)
│   │   ├── basepkg-1.0.tar.gz
│   │   └── dependpkg-1.0.tar.gz
│   ├── cache/          # Downloaded packages (future)
│   └── index.txt       # Package index
└── apt.conf            # Configuration file
```

### Package Format

Packages are `.tar.gz` archives with this structure:

```
packagename-version/
├── bin/                # Executables (required)
│   ├── command1
│   └── command2
└── lib/                # Libraries (optional)
    └── library.so
```

After installation, APT creates a `METADATA` file:

```
Name: packagename
Version: 1.0
Description: Package description
InstallDate: 2025-11-25 12:34:56
Filename: packagename-1.0.tar.gz
Depends: dep1, dep2
```

### PATH Integration

Package binaries are automatically added to PATH when the shell starts:

1. Install a package:
   ```bash
   apt install myapp --auto-install
   ```

2. Exit and restart ushell:
   ```bash
   exit
   ./ushell
   ```

3. The command is now available:
   ```bash
   mycommand
   ```

### APT Command Reference

| Command | Description | Flags |
|---------|-------------|-------|
| `apt init` | Initialize package system | None |
| `apt update` | Refresh package index | None |
| `apt list` | List all packages | `--installed` |
| `apt search <term>` | Search for packages | None |
| `apt show <pkg>` | Show package details | None |
| `apt install <pkg>` | Install a package | `--auto-install` |
| `apt remove <pkg>` | Remove a package | `--force` |
| `apt verify <pkg>` | Verify package integrity | None |
| `apt help` | Show APT help message | None |

### Common APT Workflows

#### Installing Multiple Packages

```bash
# Install packages one by one
apt install basepkg
apt install libutils
apt install myapp

# Or use --auto-install for dependencies
apt install myapp --auto-install
```

#### Updating a Package

```bash
# Remove old version and install new version
apt remove oldpkg
apt update
apt install newpkg
```

#### Cleaning Up

```bash
# Remove unused packages
apt list --installed
apt remove unused1
apt remove unused2
apt remove unused3
```

#### Troubleshooting Packages

```bash
# Check if package is installed correctly
apt verify packagename

# Reinstall if verification fails
apt remove packagename
apt install packagename
```

### APT Tips

1. **Always run `apt update` after adding new packages to the repository**
   ```bash
   # After adding files to ~/.ushell/repo/available/
   apt update
   ```

2. **Use `--auto-install` for complex dependency chains**
   ```bash
   apt install complex-app --auto-install
   ```

3. **Check dependencies before manual installation**
   ```bash
   apt show packagename
   # Look at the "Dependencies:" line
   ```

4. **Verify packages after installation**
   ```bash
   apt install mypackage
   apt verify mypackage
   ```

5. **Use `--force` carefully**
   ```bash
   # Only use --force if you know what you're doing
   apt remove basepkg --force
   ```

### APT Limitations

- No remote repository support (local packages only)
- No package downloading from the internet
- No version comparison (single version per package)
- No package upgrade command (remove + install)
- No transaction rollback
- No package signing/verification (GPG)

### Adding Custom Packages

To add your own packages:

1. **Create package directory:**
   ```bash
   mkdir -p mypackage-1.0/bin
   echo '#!/bin/bash' > mypackage-1.0/bin/mycommand
   echo 'echo "My command works!"' >> mypackage-1.0/bin/mycommand
   chmod +x mypackage-1.0/bin/mycommand
   ```

2. **Create archive:**
   ```bash
   tar czf mypackage-1.0.tar.gz mypackage-1.0/
   ```

3. **Move to repository:**
   ```bash
   mv mypackage-1.0.tar.gz ~/.ushell/repo/available/
   ```

4. **Add to index:**
   ```bash
   cat >> ~/.ushell/repo/index.txt << EOF

   PackageName: mypackage
   Version: 1.0
   Description: My custom package
   Filename: mypackage-1.0.tar.gz
   
   EOF
   ```

5. **Update and install:**
   ```bash
   apt update
   apt install mypackage
   ```

---

## Integrated Tools

All tools are built into the shell and execute without forking:

### myls - List Directory

```bash
# List current directory
myls

# List specific directory
myls /tmp
myls /home/user/documents

# Use with globs
myls *.txt

# In pipelines
myls | myfd .c
```

### mycat - Display Files

```bash
# Display single file
mycat file.txt

# Display multiple files
mycat file1.txt file2.txt file3.txt

# With redirection
mycat < input.txt
mycat file.txt > output.txt

# In pipelines
mycat data.txt | grep pattern
```

### mycp - Copy Files

```bash
# Copy single file
mycp source.txt destination.txt

# Copy to directory
mycp file.txt /tmp/

# Copy with glob
mycp *.txt backup/

# With variables
set SOURCE=data.txt
set DEST=backup.txt
mycp $SOURCE $DEST
```

### mymv - Move/Rename Files

```bash
# Rename file
mymv oldname.txt newname.txt

# Move to directory
mymv file.txt /tmp/

# Move multiple files (with globs)
mymv *.log logs/
```

### myrm - Remove Files

```bash
# Remove single file
myrm file.txt

# Remove multiple files
myrm file1.txt file2.txt file3.txt

# Remove with glob
myrm *.tmp
myrm old_*.log

# Careful - no confirmation!
```

### mymkdir - Create Directories

```bash
# Create single directory
mymkdir newdir

# Create multiple directories
mymkdir dir1 dir2 dir3

# Create nested directories
mymkdir -p parent/child/grandchild

# With variables
set BACKUP_DIR=backup_$(date +%Y%m%d)
mymkdir $BACKUP_DIR
```

### myrmdir - Remove Empty Directories

```bash
# Remove empty directory
myrmdir emptydir

# Remove multiple empty directories
myrmdir dir1 dir2 dir3

# Note: fails if directory not empty
```

### mytouch - Create/Update Files

```bash
# Create new file
mytouch newfile.txt

# Update timestamp
mytouch existing.txt

# Create multiple files
mytouch file1.txt file2.txt file3.txt

# Create with glob patterns
mytouch test{1,2,3}.txt
```

### mystat - File Information

```bash
# Show file status
mystat file.txt

# Multiple files
mystat *.txt

# Directory status
mystat /tmp
```

Shows:
- File size
- Permissions
- Inode number
- Link count
- UID/GID
- Timestamps

### myfd - Find Files

```bash
# Find in current directory
myfd pattern

# Find in specific directory
myfd pattern /home/user

# Find C files
myfd .c

# Find and process
myfd test | mycat

# Find with wildcards
myfd "*.txt"
```

---

## Tips and Tricks

### Variable Naming

```bash
# Use descriptive names
set USER_NAME=john
set DATA_DIR=/var/data
set MAX_COUNT=100

# Use uppercase for constants
set PI=3.14159
set MAX_RETRIES=3
```

### Combining Features

```bash
# Variables + arithmetic
set X=10
set Y=5
echo Result: $((X * Y + 20))

# Variables + conditionals
set FILE=test.txt
if mycat $FILE then
    echo File $FILE exists
fi

# Globs + pipelines
myls *.c | myfd main

# Redirection + variables
set OUTPUT=result.txt
echo Data > $OUTPUT
mycat input.txt >> $OUTPUT
```

### Efficient File Operations

```bash
# Backup files
mycp important.txt important.txt.bak

# Create directory structure
mymkdir -p project/src project/include project/tests

# Quick file search
myfd pattern | mycat

# Batch operations
myrm *.tmp
mycp *.txt backup/
```

### Using Arithmetic

```bash
# Calculate before using
set SIZE=$((1024 * 1024))
echo Size in bytes: $SIZE

# Counter patterns
set COUNT=0
set COUNT=$((COUNT + 1))
```

### Shell Productivity

```bash
# Quick directory navigation
set PROJ=/home/user/projects
cd $PROJ

# Reusable commands with variables
set BACKUP_DIR=/backup
set DATE=$(date +%Y%m%d)
mymkdir $BACKUP_DIR/$DATE

# Template creation
echo "# New File" > template.txt
echo "Author: $USER" >> template.txt
echo "Date: $(date)" >> template.txt
```

---

## Troubleshooting

### Common Issues

#### Command Not Found
```bash
# Issue:
mycommand
# ushell: command not found: mycommand

# Solution: Check spelling and PATH
echo $PATH
help  # See available commands
```

#### Variable Not Expanding
```bash
# Issue:
set X=10
echo X
# Output: X (not 10)

# Solution: Use $ prefix
echo $X
# Output: 10
```

#### Permission Denied
```bash
# Issue:
./script.sh
# ushell: permission denied: ./script.sh

# Solution: Check file permissions
mystat script.sh
chmod +x script.sh  # If needed
```

#### File Not Found
```bash
# Issue:
mycat nonexistent.txt
# Error: cannot open file

# Solution: Check if file exists
myls
mystat nonexistent.txt
```

#### Glob Not Expanding
```bash
# Issue:
echo *.txt
# Output: *.txt (no files matched)

# Solution: Check if matching files exist
myls
myls *.txt
```

### Error Messages

The shell provides specific error messages:

- **Command not found**: Command doesn't exist or not in PATH
- **Permission denied**: No execute permission on file
- **No such file or directory**: File or directory doesn't exist
- **Not a directory**: Tried to cd to a file
- **Cannot create file**: Permission or disk space issue

### Getting Help

```bash
# Show all commands
help

# Show version info
version

# Check environment
env

# Display current directory
pwd
```

### Terminal Display Issues

#### Prompt/Text Corruption
```bash
# Issue: Text appears garbled or overlapping
# Cause: Terminal size changed or ANSI escape issues

# Solution 1: Reset terminal size
# Resize terminal window or press Ctrl+L (if supported)

# Solution 2: Exit and restart shell
exit
./ushell
```

#### Multi-line Commands Not Wrapping Correctly
```bash
# Issue: Very long commands display incorrectly

# Cause: Terminal width detection or extreme line lengths

# Solution: The shell detects terminal width automatically,
# but extremely long single-line strings (>500 chars) may
# display improperly. Break into multiple commands:

# Instead of:
echo "very very very ... very long string"

# Use:
set PART1="first part"
set PART2="second part"
echo $PART1 $PART2
```

#### Tab Completion Not Working
```bash
# Issue: Tab key doesn't complete commands

# Cause: No matching files/commands or ambiguous match

# Solution 1: Type more characters to narrow match
ca<TAB>          # Ambiguous: cat, calc, etc.
cat<TAB>         # Completes to: cat

# Solution 2: Press Tab twice to see all matches
my<TAB><TAB>     # Shows: mycat mycp myls...
```

#### History Not Saving
```bash
# Issue: Commands not appearing in history

# Cause: Permissions on ~/.ushell_history or disk full

# Solution: Check file permissions
ls -la ~/.ushell_history
# Should show write permissions for user

# If missing, the file will be created on next command
```

### Debugging Tips

```bash
# Check variable values
echo $VAR

# Verify file existence
myls filename
mystat filename

# Test conditionals
if true then echo Success fi
if false then echo Failure fi

# Test arithmetic
echo $((2 + 2))

# Check PATH
echo $PATH

# View command history
history

# Check last command
# Press Up Arrow to recall
```

### Best Practices

1. **Always check file existence before operations**
   ```bash
   if myls file.txt then
       mycp file.txt backup.txt
   fi
   ```

2. **Use descriptive variable names**
   ```bash
   set SOURCE_FILE=data.txt
   set DEST_DIR=backup
   ```

3. **Test commands before using in conditionals**
   ```bash
   myls test.txt
   if myls test.txt then echo Found fi
   ```

4. **Be careful with destructive operations**
   ```bash
   # Check before removing
   myls old_*.log
   myrm old_*.log
   ```

5. **Use help when unsure**
   ```bash
   help
   version
   ```

---

## AI-Assisted Command Suggestions

### Overview

Unified shell includes an AI helper that can translate natural language queries into shell commands. This feature helps you discover commands and correct syntax without leaving the shell.

### Basic Usage

Start any line with `@` to ask for a command suggestion:

```bash
ushell:~> @list all C files
Suggestion: myfd .c
Accept? [y/n/e]:
```

The shell will:
1. Send your query to the AI helper
2. Display the suggested command
3. Ask for confirmation before executing

### Confirmation Options

When presented with a suggestion, you have three choices:

- **y (yes)**: Execute the suggested command immediately
- **n (no)**: Discard the suggestion and return to the prompt
- **e (edit)**: View the suggestion and type your own edited version

**Example of editing:**
```bash
ushell:~> @remove all log files
Suggestion: myrm *.log
Accept? [y/n/e]: e
Original: myrm *.log
Enter edited command: myrm old_*.log
```

### Shell State Context

By default, the AI helper receives context about your current shell session to provide better suggestions:

- Current working directory
- Username
- Recent command history (last 5 commands)
- Environment variables (excluding sensitive ones)

**Context-aware example:**
```bash
ushell:~/Documents> @show files here
# AI knows you're in ~/Documents and suggests:
Suggestion: myls
```

### Privacy Controls

#### Disabling Context Sharing

If you prefer not to share shell state with the AI helper, set `USHELL_AI_CONTEXT=0`:

```bash
export USHELL_AI_CONTEXT=0
```

This disables all context collection and sharing. Only your natural language query will be sent to the AI helper.

#### What's Excluded Automatically

Even with context enabled, the shell automatically filters out sensitive information:

- Environment variables containing: PASSWORD, TOKEN, KEY, SECRET, CREDENTIAL
- Values are truncated to 200 characters maximum
- Only recent commands (not full history) are included

### Configuration

#### AI Helper Script Location

Set `USHELL_AI_HELPER` to specify a custom AI helper script:

```bash
export USHELL_AI_HELPER=/path/to/custom_ai.py
```

Default: `./aiIntegr/ushell_ai.py`

#### OpenAI Integration (Optional)

For enhanced suggestions using OpenAI models:

```bash
# Install OpenAI Python package
pip install openai

# Set API key
export OPENAI_API_KEY="your-api-key-here"

# Optional: specify model (default: gpt-4o-mini)
export USHELL_LLM_MODEL="gpt-4o"
```

Without an API key, the AI helper uses built-in heuristic matching (no network required).

#### Debug Mode

Enable debug output to see what's happening behind the scenes:

```bash
export USHELL_AI_DEBUG=1
```

Debug messages appear on stderr showing:
- Catalog loading
- Context generation
- Suggestion selection process

### How It Works

1. **Query Detection**: Shell detects `@` prefix and extracts your query
2. **Context Collection**: If enabled, gathers current shell state as JSON
3. **AI Helper Call**: Executes Python script with query and optional context
4. **Suggestion**: Helper returns single command line suggestion
5. **Confirmation**: Shell prompts you to accept, reject, or edit
6. **Execution**: On acceptance, command runs through normal shell pipeline
7. **History**: Both query and executed command are saved to history

### Tips for Best Results

1. **Be specific**: "list C files" is better than "show files"
2. **Use natural language**: "copy file.txt to backup" works well
3. **Context helps**: AI knows your current directory and recent commands
4. **Edit when needed**: Use 'e' option to refine suggestions
5. **Privacy first**: Disable context if working with sensitive data

### Example Queries

```bash
# File operations
@list all files
@copy readme to backup
@remove old log files
@create a new directory called test

# Navigation
@go to home directory
@show current path

# Search
@find all Python files
@search for TODO in source files

# Package management (if apt is available)
@search for git package
@install python

# System info
@show environment variables
@list recent commands
```

### Troubleshooting

**"AI helper not found"**
- Check that `aiIntegr/ushell_ai.py` exists and is executable
- Or set `USHELL_AI_HELPER` to correct path

**"AI helper produced no output"**
- Try enabling debug mode: `export USHELL_AI_DEBUG=1`
- Check that Python 3 is available: `python3 --version`

**Poor suggestions**
- Be more specific in your query
- Consider installing OpenAI package for better results
- Use 'e' option to edit suggestions

**Context not working**
- Verify `USHELL_AI_CONTEXT` is not set to 0
- Enable debug mode to see context generation

---

## Summary

### Quick Reference

| Category | Commands |
|----------|----------|
| **Navigation** | cd, pwd |
| **Display** | echo, mycat, myls |
| **Files** | mycp, mymv, myrm, mytouch, mystat |
| **Directories** | mymkdir, myrmdir |
| **Search** | myfd |
| **Variables** | set, export, unset, env |
| **System** | help, version, exit |

### Feature Reference

| Feature | Syntax | Example |
|---------|--------|---------|
| **Variable** | $VAR | echo $HOME |
| **Arithmetic** | $((expr)) | echo $((5 + 3)) |
| **Conditional** | if cmd then ... fi | if myls file then echo Found fi |
| **Pipeline** | cmd1 \| cmd2 | myls \| myfd .c |
| **Redirect Out** | cmd > file | echo data > file.txt |
| **Redirect Append** | cmd >> file | echo data >> file.txt |
| **Redirect In** | cmd < file | mycat < file.txt |
| **Glob Star** | * | echo *.txt |
| **Glob Question** | ? | echo file?.txt |
| **Glob Range** | [a-z] | echo [0-9]*.txt |
| **Glob Negate** | [!...] | echo [!a]*.txt |

---

**End of User Guide**

For developer documentation, see [DEVELOPER_GUIDE.md](DEVELOPER_GUIDE.md).  
For examples, see the [examples/](../examples/) directory.
