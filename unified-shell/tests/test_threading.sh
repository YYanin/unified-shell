#!/bin/bash
# test_threading.sh - Threading Feature Tests
# Tests for multithreading support in unified shell

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
echo "Threading Feature Test Suite"
echo "========================================"
echo ""

# Helper function to run a test
run_test() {
    local test_name="$1"
    local test_command="$2"
    local expected_result="$3"
    
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

# Test 1: Shell starts with threading enabled
echo "--- Basic Threading Tests ---"
run_test "Shell starts with USHELL_THREAD_BUILTINS=1" \
    "export USHELL_THREAD_BUILTINS=1 && echo 'exit' | $SHELL_BIN 2>&1 | grep -q 'Threading enabled'"

# Test 2: Single built-in executes in thread
run_test "Single built-in executes correctly" \
    "export USHELL_THREAD_BUILTINS=1 && echo 'pwd' | $SHELL_BIN > /dev/null"

# Test 3: Multiple built-ins execute sequentially
run_test "Multiple built-ins execute sequentially" \
    "export USHELL_THREAD_BUILTINS=1 && printf 'pwd\necho test\ncd /tmp\npwd\nexit\n' | $SHELL_BIN > /dev/null"

# Test 4: Environment modification from thread
run_test "Environment modification thread-safe" \
    "export USHELL_THREAD_BUILTINS=1 && printf 'export TEST=value\necho \$TEST\nexit\n' | $SHELL_BIN | grep -q 'value'"

# Test 5: History accessible from threads
run_test "History works with threading" \
    "export USHELL_THREAD_BUILTINS=1 && printf 'pwd\necho test\nhistory\nexit\n' | $SHELL_BIN | grep -q 'pwd'"

# Test 6: Thread pool with custom size
echo ""
echo "--- Thread Pool Tests ---"
run_test "Thread pool accepts custom size" \
    "export USHELL_THREAD_BUILTINS=1 && export USHELL_THREAD_POOL_SIZE=4 && echo 'exit' | $SHELL_BIN 2>&1 | grep -q '4 worker threads'"

# Test 7: Thread pool handles multiple commands
run_test "Thread pool handles multiple commands" \
    "export USHELL_THREAD_BUILTINS=1 && export USHELL_THREAD_POOL_SIZE=2 && for i in {1..10}; do echo 'echo test\$i'; done | $SHELL_BIN > /dev/null"

# Test 8: Graceful shutdown with threading
run_test "Shell exits cleanly with threading enabled" \
    "export USHELL_THREAD_BUILTINS=1 && echo 'exit' | $SHELL_BIN > /dev/null"

# Test 9: Threading disabled works normally
echo ""
echo "--- Backward Compatibility Tests ---"
run_test "Shell works without threading (unset)" \
    "unset USHELL_THREAD_BUILTINS && printf 'pwd\necho test\nexit\n' | $SHELL_BIN > /dev/null"

run_test "Shell works without threading (=0)" \
    "export USHELL_THREAD_BUILTINS=0 && printf 'pwd\necho test\nexit\n' | $SHELL_BIN > /dev/null"

# Test 10: Thread safety stress test
echo ""
echo "--- Stress Tests ---"
run_test "Concurrent built-ins stress test" \
    "export USHELL_THREAD_BUILTINS=1 && for i in {1..50}; do echo 'echo test\$i'; done | $SHELL_BIN > /dev/null"

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
    echo -e "${GREEN}All threading tests passed!${NC}"
    exit 0
else
    echo -e "${RED}Some threading tests failed.${NC}"
    exit 1
fi
