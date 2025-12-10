#!/bin/bash
# test_help.sh - Help System Tests
# Tests for --help flag support in all built-in commands

set -e  # Exit on error

SHELL_BIN="./ushell"
TEST_COUNT=0
PASS_COUNT=0
FAIL_COUNT=0

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo "========================================"
echo "Help System Test Suite"
echo "========================================"
echo ""

# Helper function to run a test
run_test() {
    local test_name="$1"
    local test_command="$2"
    
    TEST_COUNT=$((TEST_COUNT + 1))
    echo -n "Test $TEST_COUNT: $test_name ... "
    
    if eval "$test_command" > /dev/null 2>&1; then
        echo -e "${GREEN}PASS${NC}"
        PASS_COUNT=$((PASS_COUNT + 1))
    else
        echo -e "${RED}FAIL${NC}"
        FAIL_COUNT=$((FAIL_COUNT + 1))
    fi
}

# Test 1-17: --help flag for all built-ins
echo "--- Built-in Commands --help Tests ---"
BUILTINS="cd pwd echo export set unset env help version history jobs fg bg commands exit"

for cmd in $BUILTINS; do
    run_test "$cmd --help shows help" \
        "echo '$cmd --help' | $SHELL_BIN | grep -q 'NAME'"
done

# Test: apt --help
run_test "apt --help shows help" \
    "echo 'apt --help' | $SHELL_BIN | grep -q 'NAME'"

# Test: -h short flag variant
echo ""
echo "--- Short Flag -h Tests ---"
run_test "pwd -h shows help" \
    "echo 'pwd -h' | $SHELL_BIN | grep -q 'NAME'"

run_test "echo -h shows help" \
    "echo 'echo -h' | $SHELL_BIN | grep -q 'NAME'"

# Test: Help flag in different positions
echo ""
echo "--- Flag Position Tests ---"
run_test "echo --help arg shows help" \
    "echo 'echo --help test' | $SHELL_BIN | grep -q 'NAME'"

run_test "cd /tmp --help shows help" \
    "echo 'cd /tmp --help' | $SHELL_BIN | grep -q 'NAME'"

# Test: Help doesn't execute command
echo ""
echo "--- Command Non-Execution Tests ---"
run_test "cd --help doesn't change directory" \
    "echo 'cd /tmp --help' | $SHELL_BIN | grep -q 'Change the current working directory'"

run_test "echo --help doesn't echo arguments" \
    "! echo 'echo --help test' | $SHELL_BIN | grep -q '^test$'"

# Test: Help command with arguments
echo ""
echo "--- help Command Tests ---"
run_test "help shows general help" \
    "echo 'help' | $SHELL_BIN | grep -q 'Built-in Commands'"

run_test "help cd shows cd help" \
    "echo 'help cd' | $SHELL_BIN | grep -q 'Change the current working directory'"

run_test "help pwd shows pwd help" \
    "echo 'help pwd' | $SHELL_BIN | grep -q 'Print the current working directory'"

run_test "help invalidcommand shows error" \
    "echo 'help invalidcommand' | $SHELL_BIN 2>&1 | grep -q 'no help available'"

# Test: Help exit status
echo ""
echo "--- Exit Status Tests ---"
run_test "pwd --help exits with status 0" \
    "echo 'pwd --help' | $SHELL_BIN > /dev/null && [ \$? -eq 0 ]"

run_test "help cd exits with status 0" \
    "echo 'help cd' | $SHELL_BIN > /dev/null && [ \$? -eq 0 ]"

# Test: Help content quality
echo ""
echo "--- Help Content Tests ---"
run_test "cd --help shows USAGE section" \
    "echo 'cd --help' | $SHELL_BIN | grep -q 'USAGE'"

run_test "pwd --help shows DESCRIPTION section" \
    "echo 'pwd --help' | $SHELL_BIN | grep -q 'DESCRIPTION'"

run_test "echo --help shows EXAMPLES section" \
    "echo 'echo --help' | $SHELL_BIN | grep -q 'EXAMPLES'"

# Test: Help with threading enabled
echo ""
echo "--- Help with Threading Tests ---"
run_test "Help works with threading enabled" \
    "export USHELL_THREAD_BUILTINS=1 && echo 'pwd --help' | $SHELL_BIN | grep -q 'NAME'"

run_test "help command works with threading" \
    "export USHELL_THREAD_BUILTINS=1 && echo 'help cd' | $SHELL_BIN | grep -q 'Change the current working directory'"

# Summary
echo ""
echo "========================================"
echo "Test Summary"
echo "========================================"
echo "Total Tests:  $TEST_COUNT"
echo -e "Passed:       ${GREEN}$PASS_COUNT${NC}"
echo -e "Failed:       ${RED}$FAIL_COUNT${NC}"
echo ""

if [ $FAIL_COUNT -eq 0 ]; then
    echo -e "${GREEN}All help tests passed!${NC}"
    exit 0
else
    echo -e "${RED}Some help tests failed.${NC}"
    exit 1
fi
