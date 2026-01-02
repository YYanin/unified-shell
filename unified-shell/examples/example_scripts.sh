#!/usr/bin/env bash
# Example Scripts for Unified Shell (ushell)
# Run with: ./ushell < example_scripts.sh

# ============================================
# BASIC COMMANDS
# ============================================

echo "=== Basic Commands ==="
pwd
echo Hello, World!
echo

# ============================================
# VARIABLES
# ============================================

echo "=== Variables ==="
set NAME=Alice
echo Hello, $NAME
set X=100
echo X = $X
echo

# ============================================
# ARITHMETIC
# ============================================

echo "=== Arithmetic Operations ==="
echo 5 + 3 = $((5 + 3))
echo 10 - 4 = $((10 - 4))
echo 6 * 7 = $((6 * 7))
echo 20 / 5 = $((20 / 5))
echo 17 % 5 = $((17 % 5))
echo

echo "=== Arithmetic with Variables ==="
set A=15
set B=7
echo A = $A
echo B = $B
echo A + B = $((A + B))
echo A * B = $((A * B))
echo

# ============================================
# BUILT-IN COMMANDS
# ============================================

echo "=== Built-in Commands ==="
pwd
cd /tmp
pwd
cd -
echo Current user: $USER
echo Home directory: $HOME
echo

# ============================================
# ENVIRONMENT VARIABLES
# ============================================

echo "=== Environment Variables ==="
export MY_VAR=TestValue
echo MY_VAR = $MY_VAR
unset MY_VAR
echo After unset: $MY_VAR
echo

# ============================================
# FILE OPERATIONS
# ============================================

echo "=== File Operations ==="
touch example1.txt
touch example2.txt
ls example*.txt
echo "Test content" > example1.txt
cat example1.txt
cp example1.txt example_backup.txt
cat example_backup.txt
rm example*.txt example_backup.txt
echo

# ============================================
# DIRECTORY OPERATIONS
# ============================================

echo "=== Directory Operations ==="
mkdir test_dir
ls test_dir
cd test_dir
pwd
cd ..
rmdir test_dir
echo

# ============================================
# CONDITIONALS
# ============================================

echo "=== Conditionals ==="
if touch test_cond.txt then
    echo File created successfully
fi

if ls test_cond.txt then
    echo File exists
fi

rm test_cond.txt
echo

# ============================================
# PIPELINES
# ============================================

echo "=== Pipelines ==="
echo Creating test files for pipeline demo
touch file1.c file2.c file3.txt
ls | find .c
rm file*.c file*.txt
echo

# ============================================
# I/O REDIRECTION
# ============================================

echo "=== I/O Redirection ==="
echo "Line 1" > output.txt
echo "Line 2" >> output.txt
echo "Line 3" >> output.txt
cat output.txt
cat < output.txt
rm output.txt
echo

# ============================================
# GLOB PATTERNS
# ============================================

echo "=== Glob Patterns ==="
touch test1.txt test2.txt test3.txt data1.txt data2.txt
echo All .txt files:
ls *.txt
echo Files starting with 'test':
ls test*.txt
echo Files matching test[0-9].txt:
ls test[0-9].txt
rm test*.txt data*.txt
echo

# ============================================
# COMPLEX EXAMPLES
# ============================================

echo "=== Complex Example 1: File Processing ==="
touch data.txt
echo "Processing data..." > data.txt
echo "Step 1: Complete" >> data.txt
echo "Step 2: Complete" >> data.txt
cat data.txt
rm data.txt
echo

echo "=== Complex Example 2: Variables + Arithmetic ==="
set WIDTH=10
set HEIGHT=20
set AREA=$((WIDTH * HEIGHT))
echo Rectangle: ${WIDTH}x${HEIGHT}
echo Area: $AREA square units
echo

echo "=== Complex Example 3: Multiple Operations ==="
mkdir workspace
cd workspace
touch project.c project.h
ls
cd ..
rm workspace/project.c workspace/project.h
rmdir workspace
echo

# ============================================
# HELP AND VERSION
# ============================================

echo "=== Help and Version ==="
version
echo
echo "For detailed help, run: help"
echo

# ============================================
# COMPLETION MESSAGE
# ============================================

echo "==================================="
echo "All example scripts completed!"
echo "==================================="
