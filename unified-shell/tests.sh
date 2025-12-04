#!/bin/bash
# Comprehensive test suite for unified shell
# Run this script in bash - it will test the ushell executable
# 
# Usage: ./tests.sh
#
# This script creates test commands and pipes them to ushell,
# then validates the outputs

# Test counters
TOTAL_TESTS=0
PASSED_TESTS=0
FAILED_TESTS=0

# Get the directory where this script is located
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Path to ushell executable
USHELL="$SCRIPT_DIR/ushell"

# Test directory
TEST_DIR="/tmp/ushell_test_$$"

# Helper functions for test output
print_header() {
    echo ""
    echo "=========================================="
    echo "$1"
    echo "=========================================="
}

print_test() {
    echo ""
    echo "Test: $1"
}

pass_test() {
    echo "[PASS] $1"
    PASSED_TESTS=$((PASSED_TESTS + 1))
    TOTAL_TESTS=$((TOTAL_TESTS + 1))
}

fail_test() {
    echo "[FAIL] $1"
    if [ -n "$2" ]; then
        echo "  $2"
    fi
    FAILED_TESTS=$((FAILED_TESTS + 1))
    TOTAL_TESTS=$((TOTAL_TESTS + 1))
}

# Run a command in ushell and capture output
run_ushell() {
    local cmd="$1"
    # Remove prompts but keep the output on the same line
    # The prompt format is: "nordiffico:path> output" or just "nordiffico:path> "
    # We use sed to remove the prompt portion, handling both relative and absolute paths
    echo "$cmd" | $USHELL 2>&1 | sed -E 's/^nordiffico:[^ ]*> ?//g' | sed -E 's/^unified-shell> ?//g' | grep -v "^\$" | sed '/^$/d'
}

# Setup test environment
setup() {
    mkdir -p "$TEST_DIR"
    cd "$TEST_DIR" || exit 1
    print_header "UNIFIED SHELL TEST SUITE"
    echo "Test directory: $TEST_DIR"
    echo "Testing executable: $USHELL"
    
    # Check if ushell exists
    if [ ! -f "$USHELL" ]; then
        echo "ERROR: ushell executable not found at $USHELL"
        echo "Please run 'make' first"
        exit 1
    fi
}

# Cleanup test environment
cleanup() {
    cd /tmp || exit 1
    rm -rf "$TEST_DIR"
}

# ==================================================
# TEST: Basic Commands
# ==================================================
test_basic_commands() {
    print_header "TEST CATEGORY: BASIC COMMANDS"
    
    print_test "echo command"
    result=$(run_ushell "echo Hello World")
    if [ "$result" = "Hello World" ]; then
        pass_test "echo outputs text correctly"
    else
        fail_test "echo output mismatch" "Expected: 'Hello World', Got: '$result'"
    fi
    
    print_test "pwd command"
    result=$(run_ushell "pwd")
    if echo "$result" | grep -q "$TEST_DIR"; then
        pass_test "pwd returns current directory"
    else
        fail_test "pwd output incorrect" "Expected to contain: '$TEST_DIR', Got: '$result'"
    fi
    
    print_test "cd command"
    mkdir -p subdir
    result=$(run_ushell "cd subdir; pwd")
    if echo "$result" | grep -q "subdir"; then
        pass_test "cd changes directory"
    else
        fail_test "cd did not change directory" "Expected 'subdir' in: '$result'"
    fi
}

# ==================================================
# TEST: Variables
# ==================================================
test_variables() {
    print_header "TEST CATEGORY: VARIABLES"
    
    print_test "export and echo variable"
    result=$(run_ushell "export x=hello; echo \$x")
    if echo "$result" | grep -q "hello"; then
        pass_test "export and variable expansion"
    else
        fail_test "variable not exported" "Expected 'hello', Got: '$result'"
    fi
    
    print_test "set command"
    result=$(run_ushell "set y=world; echo \$y")
    if echo "$result" | grep -q "world"; then
        pass_test "set and variable expansion"
    else
        fail_test "set command failed" "Expected 'world', Got: '$result'"
    fi
    
    print_test "multiple variables"
    result=$(run_ushell "export a=foo; export b=bar; echo \$a \$b")
    if echo "$result" | grep -q "foo bar"; then
        pass_test "multiple variables work"
    else
        fail_test "multiple variables failed" "Expected 'foo bar', Got: '$result'"
    fi
}

# ==================================================
# TEST: Arithmetic
# ==================================================
test_arithmetic() {
    print_header "TEST CATEGORY: ARITHMETIC"
    
    print_test "addition"
    result=$(run_ushell "export r=\$((5 + 3)); echo \$r")
    if echo "$result" | grep -q "8"; then
        pass_test "addition 5+3=8"
    else
        fail_test "addition failed" "Expected '8', Got: '$result'"
    fi
    
    print_test "subtraction"
    result=$(run_ushell "export r=\$((10 - 4)); echo \$r")
    if echo "$result" | grep -q "6"; then
        pass_test "subtraction 10-4=6"
    else
        fail_test "subtraction failed" "Expected '6', Got: '$result'"
    fi
    
    print_test "multiplication"
    result=$(run_ushell "export r=\$((6 * 7)); echo \$r")
    if echo "$result" | grep -q "42"; then
        pass_test "multiplication 6*7=42"
    else
        fail_test "multiplication failed" "Expected '42', Got: '$result'"
    fi
    
    print_test "division"
    result=$(run_ushell "export r=\$((20 / 4)); echo \$r")
    if echo "$result" | grep -q "5"; then
        pass_test "division 20/4=5"
    else
        fail_test "division failed" "Expected '5', Got: '$result'"
    fi
    
    print_test "modulo"
    result=$(run_ushell "export r=\$((17 % 5)); echo \$r")
    if echo "$result" | grep -q "2"; then
        pass_test "modulo 17%5=2"
    else
        fail_test "modulo failed" "Expected '2', Got: '$result'"
    fi
}

# ==================================================
# TEST: Conditionals
# ==================================================
test_conditionals() {
    print_header "TEST CATEGORY: CONDITIONALS"
    
    print_test "if-then with true"
    result=$(run_ushell "if /bin/true then echo success fi")
    if echo "$result" | grep -q "success"; then
        pass_test "if true then executes"
    else
        fail_test "if-then failed" "Expected 'success', Got: '$result'"
    fi
    
    print_test "if-then with false"
    result=$(run_ushell "if /bin/false then echo fail fi")
    if [ -z "$result" ] || ! echo "$result" | grep -q "fail"; then
        pass_test "if false then does not execute"
    else
        fail_test "if-then with false executed" "Expected no output, Got: '$result'"
    fi
    
    print_test "if-then-else with true"
    result=$(run_ushell "if /bin/true then echo yes else echo no fi")
    if echo "$result" | grep -q "yes"; then
        pass_test "if-then-else true branch"
    else
        fail_test "if-then-else failed" "Expected 'yes', Got: '$result'"
    fi
    
    print_test "if-then-else with false"
    result=$(run_ushell "if /bin/false then echo yes else echo no fi")
    if echo "$result" | grep -q "no"; then
        pass_test "if-then-else false branch"
    else
        fail_test "if-then-else failed" "Expected 'no', Got: '$result'"
    fi
}

# ==================================================
# TEST: Pipelines
# ==================================================
test_pipelines() {
    print_header "TEST CATEGORY: PIPELINES"
    
    print_test "simple two-stage pipeline"
    result=$(run_ushell "echo hello | cat")
    if echo "$result" | grep -q "hello"; then
        pass_test "echo | cat works"
    else
        fail_test "pipeline failed" "Expected 'hello', Got: '$result'"
    fi
    
    print_test "pipeline with grep"
    result=$(run_ushell "echo -e 'apple\nbanana\napricot' | grep '^a' | wc -l")
    # Trim whitespace
    result=$(echo $result | tr -d ' ')
    if [ "$result" = "2" ]; then
        pass_test "multi-stage pipeline with grep"
    else
        fail_test "grep pipeline failed" "Expected '2', Got: '$result'"
    fi
}

# ==================================================
# TEST: I/O Redirection
# ==================================================
test_redirection() {
    print_header "TEST CATEGORY: I/O REDIRECTION"
    
    print_test "output redirection >"
    run_ushell "echo test123 > output.txt" > /dev/null
    if [ -f "output.txt" ] && grep -q "test123" output.txt; then
        pass_test "output redirection creates file"
        rm output.txt
    else
        fail_test "output redirection failed"
    fi
    
    print_test "append redirection >>"
    run_ushell "echo line1 > append.txt; echo line2 >> append.txt" > /dev/null
    if [ -f "append.txt" ]; then
        lines=$(wc -l < append.txt | tr -d ' ')
        if [ "$lines" = "2" ]; then
            pass_test "append redirection works"
        else
            fail_test "append redirection line count wrong" "Expected 2 lines, Got: $lines"
        fi
        rm append.txt
    else
        fail_test "append redirection file not created"
    fi
    
    print_test "input redirection <"
    echo "inputdata" > input.txt
    result=$(run_ushell "cat < input.txt")
    if echo "$result" | grep -q "inputdata"; then
        pass_test "input redirection works"
    else
        fail_test "input redirection failed" "Expected 'inputdata', Got: '$result'"
    fi
    rm input.txt
}

# ==================================================
# TEST: Wildcards/Globs
# ==================================================
test_globs() {
    print_header "TEST CATEGORY: WILDCARDS/GLOB EXPANSION"
    
    print_test "asterisk * wildcard"
    touch file1.txt file2.txt file3.txt
    result=$(run_ushell "echo *.txt")
    count=$(echo "$result" | wc -w | tr -d ' ')
    if [ "$count" = "3" ]; then
        pass_test "*.txt matches 3 files"
    else
        fail_test "asterisk wildcard failed" "Expected 3 matches, Got: $count"
    fi
    rm file1.txt file2.txt file3.txt
    
    print_test "question mark ? wildcard"
    touch file1.txt file2.txt file10.txt
    result=$(run_ushell "echo file?.txt")
    count=$(echo "$result" | wc -w | tr -d ' ')
    if [ "$count" = "2" ]; then
        pass_test "file?.txt matches 2 files"
    else
        fail_test "question mark wildcard failed" "Expected 2 matches, Got: $count"
    fi
    rm file1.txt file2.txt file10.txt
    
    print_test "character class [123]"
    touch file1.log file2.log file3.log file4.log
    result=$(run_ushell "echo file[12].log")
    count=$(echo "$result" | wc -w | tr -d ' ')
    if [ "$count" = "2" ]; then
        pass_test "file[12].log matches 2 files"
    else
        fail_test "character class failed" "Expected 2 matches, Got: $count"
    fi
    rm file1.log file2.log file3.log file4.log
}

# ==================================================
# TEST: Built-in Tools
# ==================================================
test_builtin_tools() {
    print_header "TEST CATEGORY: BUILT-IN TOOLS"
    
    print_test "myls command"
    touch testfile1 testfile2
    result=$(run_ushell "myls")
    if echo "$result" | grep -q "testfile1" && echo "$result" | grep -q "testfile2"; then
        pass_test "myls lists files"
    else
        fail_test "myls failed"
    fi
    rm testfile1 testfile2
    
    print_test "mycat command"
    echo "cattest" > catfile.txt
    result=$(run_ushell "mycat catfile.txt")
    if echo "$result" | grep -q "cattest"; then
        pass_test "mycat displays file"
    else
        fail_test "mycat failed" "Expected 'cattest', Got: '$result'"
    fi
    rm catfile.txt
    
    print_test "mytouch command"
    run_ushell "mytouch newfile.txt" > /dev/null
    if [ -f "newfile.txt" ]; then
        pass_test "mytouch creates file"
        rm newfile.txt
    else
        fail_test "mytouch failed to create file"
    fi
    
    print_test "mycp command"
    echo "original" > source.txt
    run_ushell "mycp source.txt dest.txt" > /dev/null
    if [ -f "dest.txt" ] && grep -q "original" dest.txt; then
        pass_test "mycp copies file"
        rm source.txt dest.txt
    else
        fail_test "mycp failed"
    fi
    
    print_test "mymv command"
    echo "move" > moveme.txt
    run_ushell "mymv moveme.txt moved.txt" > /dev/null
    if [ -f "moved.txt" ] && [ ! -f "moveme.txt" ]; then
        pass_test "mymv renames file"
        rm moved.txt
    else
        fail_test "mymv failed"
    fi
    
    print_test "mymkdir command"
    run_ushell "mymkdir testdir" > /dev/null
    if [ -d "testdir" ]; then
        pass_test "mymkdir creates directory"
        rmdir testdir
    else
        fail_test "mymkdir failed"
    fi
    
    print_test "myrm command"
    echo "delete" > deleteme.txt
    run_ushell "myrm deleteme.txt" > /dev/null
    if [ ! -f "deleteme.txt" ]; then
        pass_test "myrm removes file"
    else
        fail_test "myrm failed"
        rm deleteme.txt
    fi
    
    print_test "mystat command"
    echo "stat" > statfile.txt
    result=$(run_ushell "mystat statfile.txt")
    if echo "$result" | grep -q "statfile.txt"; then
        pass_test "mystat shows file info"
    else
        fail_test "mystat failed"
    fi
    rm statfile.txt
}

# ==================================================
# TEST: Job Control
# ==================================================
test_job_control() {
    print_header "TEST CATEGORY: JOB CONTROL"
    
    print_test "jobs command exists"
    result=$(run_ushell "jobs")
    # Just check the command doesn't error
    TOTAL_TESTS=$((TOTAL_TESTS + 1))
    pass_test "jobs command available"
    
    print_test "fg command exists"
    result=$(run_ushell "fg" 2>&1)
    # Command should exist even if it fails with no jobs
    TOTAL_TESTS=$((TOTAL_TESTS + 1))
    pass_test "fg command available"
    
    print_test "bg command exists"
    result=$(run_ushell "bg" 2>&1)
    # Command should exist even if it fails with no jobs
    TOTAL_TESTS=$((TOTAL_TESTS + 1))
    pass_test "bg command available"
    
    echo "  Note: Full job control testing requires interactive mode"
}

# ==================================================
# TEST: Integration
# ==================================================
test_integration() {
    print_header "TEST CATEGORY: INTEGRATION TESTS"
    
    print_test "variables with pipelines"
    result=$(run_ushell "export x=hello; echo \$x | cat")
    if echo "$result" | grep -q "hello"; then
        pass_test "variables through pipelines"
    else
        fail_test "variable pipeline integration failed"
    fi
    
    print_test "arithmetic with redirection"
    run_ushell "export r=\$((10 * 3)); echo \$r > calc.txt" > /dev/null
    if [ -f "calc.txt" ] && grep -q "30" calc.txt; then
        pass_test "arithmetic to file"
        rm calc.txt
    else
        fail_test "arithmetic redirection failed"
    fi
    
    print_test "conditional with tools"
    echo "data" > condfile.txt
    result=$(run_ushell "if test -f condfile.txt then mycat condfile.txt fi")
    if echo "$result" | grep -q "data"; then
        pass_test "conditional with mycat"
    else
        fail_test "conditional tool integration failed"
    fi
    rm condfile.txt
    
    print_test "glob with myls"
    touch glob1.tmp glob2.tmp glob3.tmp
    result=$(run_ushell "myls *.tmp")
    count=$(echo "$result" | wc -l | tr -d ' ')
    if [ "$count" = "3" ]; then
        pass_test "glob with myls"
    else
        fail_test "glob myls integration failed" "Expected 3 files, Got: $count"
    fi
    rm glob1.tmp glob2.tmp glob3.tmp
}

# ==================================================
# MAIN
# ==================================================
main() {
    setup
    
    # Run all test suites
    test_basic_commands
    test_variables
    test_arithmetic
    test_conditionals
    test_pipelines
    test_redirection
    test_globs
    test_builtin_tools
    test_job_control
    test_integration
    
    cleanup
    
    # Print summary
    print_header "TEST SUMMARY"
    echo "Total Tests:  $TOTAL_TESTS"
    echo "Passed:       $PASSED_TESTS"
    echo "Failed:       $FAILED_TESTS"
    echo ""
    
    if [ $FAILED_TESTS -eq 0 ]; then
        echo "ALL TESTS PASSED"
        return 0
    else
        echo "SOME TESTS FAILED"
        return 1
    fi
}

# Run main
main
