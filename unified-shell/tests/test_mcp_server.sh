#!/bin/bash
################################################################################
# MCP Server Test Suite
# Automated tests for unified shell MCP integration
#
# Usage:
#   ./test_mcp_server.sh            # Run all tests
#   ./test_mcp_server.sh basic      # Run only basic tests
#   ./test_mcp_server.sh security   # Run only security tests
################################################################################

set -e  # Exit on error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Test counters
TESTS_RUN=0
TESTS_PASSED=0
TESTS_FAILED=0

# Configuration
MCP_PORT=${USHELL_MCP_PORT:-9000}
SHELL_BIN="../ushell"
TIMEOUT=5

# Cleanup function
cleanup() {
    if [ -n "$SHELL_PID" ]; then
        echo -e "${YELLOW}Cleaning up shell process (PID: $SHELL_PID)${NC}"
        kill $SHELL_PID 2>/dev/null || true
        wait $SHELL_PID 2>/dev/null || true
    fi
}
trap cleanup EXIT

################################################################################
# Test Infrastructure
################################################################################

log_test() {
    echo -e "${BLUE}[TEST]${NC} $1"
}

log_pass() {
    echo -e "${GREEN}[PASS]${NC} $1"
    ((TESTS_PASSED++))
}

log_fail() {
    echo -e "${RED}[FAIL]${NC} $1"
    ((TESTS_FAILED++))
}

log_info() {
    echo -e "${YELLOW}[INFO]${NC} $1"
}

run_test() {
    ((TESTS_RUN++))
    log_test "$1"
}

# Send JSON-RPC request to MCP server
send_mcp_request() {
    local request="$1"
    echo "$request" | timeout $TIMEOUT nc localhost $MCP_PORT 2>&1
}

# Wait for shell to be ready
wait_for_shell() {
    local max_attempts=10
    local attempt=0
    
    while [ $attempt -lt $max_attempts ]; do
        if nc -z localhost $MCP_PORT 2>/dev/null; then
            log_info "MCP server ready on port $MCP_PORT"
            return 0
        fi
        sleep 0.5
        ((attempt++))
    done
    
    log_fail "MCP server failed to start within $((max_attempts / 2)) seconds"
    return 1
}

# Start shell with MCP enabled
start_shell() {
    log_info "Starting shell with MCP enabled on port $MCP_PORT..."
    
    # Check if shell binary exists
    if [ ! -f "$SHELL_BIN" ]; then
        log_fail "Shell binary not found at $SHELL_BIN"
        exit 1
    fi
    
    # Start shell in background
    USHELL_MCP_ENABLED=1 USHELL_MCP_PORT=$MCP_PORT "$SHELL_BIN" > /dev/null 2>&1 &
    SHELL_PID=$!
    
    # Wait for server to be ready
    if ! wait_for_shell; then
        return 1
    fi
    
    log_info "Shell started (PID: $SHELL_PID)"
    return 0
}

################################################################################
# Basic Tests
################################################################################

test_connection() {
    run_test "Connection to MCP server"
    
    if nc -z localhost $MCP_PORT 2>/dev/null; then
        log_pass "Successfully connected to port $MCP_PORT"
        return 0
    else
        log_fail "Could not connect to port $MCP_PORT"
        return 1
    fi
}

test_initialize() {
    run_test "Initialize MCP session"
    
    local request='{"id":"1","method":"initialize","params":{}}'
    local response=$(send_mcp_request "$request")
    
    if echo "$response" | grep -q '"type":"response"'; then
        if echo "$response" | grep -q '"server"'; then
            log_pass "Initialize returned valid response"
            return 0
        fi
    fi
    
    log_fail "Initialize failed or returned invalid response"
    log_info "Response: $response"
    return 1
}

test_list_tools() {
    run_test "List available tools"
    
    local request='{"id":"2","method":"list_tools","params":{}}'
    local response=$(send_mcp_request "$request")
    
    if echo "$response" | grep -q '"tools"'; then
        # Count tools
        local tool_count=$(echo "$response" | grep -o '"name"' | wc -l)
        if [ $tool_count -gt 40 ]; then
            log_pass "Listed $tool_count tools"
            return 0
        fi
    fi
    
    log_fail "list_tools failed or returned insufficient tools"
    return 1
}

test_call_tool_pwd() {
    run_test "Execute tool: pwd"
    
    local request='{"id":"3","method":"call_tool","params":{"tool":"pwd","args":{}}}'
    local response=""
    
    # Read all messages (notifications + response)
    while IFS= read -r line; do
        response="$response$line"
        if echo "$line" | grep -q '"type":"response"'; then
            break
        fi
    done < <(echo "$request" | timeout $TIMEOUT nc localhost $MCP_PORT 2>&1)
    
    if echo "$response" | grep -q '"output"'; then
        if echo "$response" | grep -q '"exit_code":0'; then
            log_pass "pwd executed successfully"
            return 0
        fi
    fi
    
    log_fail "pwd execution failed"
    log_info "Response: $response"
    return 1
}

test_call_tool_echo() {
    run_test "Execute tool with arguments: echo"
    
    local request='{"id":"4","method":"call_tool","params":{"tool":"echo","args":{"text":"Hello MCP"}}}'
    local response=""
    
    # Read all messages
    while IFS= read -r line; do
        response="$response$line"
        if echo "$line" | grep -q '"type":"response"'; then
            break
        fi
    done < <(echo "$request" | timeout $TIMEOUT nc localhost $MCP_PORT 2>&1)
    
    if echo "$response" | grep -q "Hello MCP"; then
        log_pass "echo executed with correct output"
        return 0
    fi
    
    log_fail "echo execution failed or incorrect output"
    return 1
}

################################################################################
# Special Tool Tests
################################################################################

test_get_shell_context() {
    run_test "Special tool: get_shell_context"
    
    local request='{"id":"5","method":"call_tool","params":{"tool":"get_shell_context","args":{}}}'
    local response=""
    
    while IFS= read -r line; do
        response="$response$line"
        if echo "$line" | grep -q '"type":"response"'; then
            break
        fi
    done < <(echo "$request" | timeout $TIMEOUT nc localhost $MCP_PORT 2>&1)
    
    if echo "$response" | grep -q '"cwd"'; then
        if echo "$response" | grep -q '"user"'; then
            log_pass "get_shell_context returned context data"
            return 0
        fi
    fi
    
    log_fail "get_shell_context failed"
    return 1
}

test_search_commands() {
    run_test "Special tool: search_commands"
    
    local request='{"id":"6","method":"call_tool","params":{"tool":"search_commands","args":{"query":"list files"}}}'
    local response=""
    
    while IFS= read -r line; do
        response="$response$line"
        if echo "$line" | grep -q '"type":"response"'; then
            break
        fi
    done < <(echo "$request" | timeout $TIMEOUT nc localhost $MCP_PORT 2>&1)
    
    if echo "$response" | grep -q "results"; then
        log_pass "search_commands returned results"
        return 0
    fi
    
    log_fail "search_commands failed"
    return 1
}

test_suggest_command() {
    run_test "Special tool: suggest_command"
    
    local request='{"id":"7","method":"call_tool","params":{"tool":"suggest_command","args":{"query":"find python files"}}}'
    local response=""
    
    while IFS= read -r line; do
        response="$response$line"
        if echo "$line" | grep -q '"type":"response"'; then
            break
        fi
    done < <(echo "$request" | timeout $TIMEOUT nc localhost $MCP_PORT 2>&1)
    
    if echo "$response" | grep -q "command"; then
        log_pass "suggest_command returned suggestion"
        return 0
    fi
    
    log_fail "suggest_command failed"
    return 1
}

################################################################################
# Security Tests
################################################################################

test_blacklisted_command() {
    run_test "Security: Blacklisted command (sudo)"
    
    local request='{"id":"8","method":"call_tool","params":{"tool":"sudo","args":{"command":"ls"}}}'
    local response=$(send_mcp_request "$request")
    
    if echo "$response" | grep -q '"type":"error"'; then
        if echo "$response" | grep -q "blacklisted"; then
            log_pass "Blacklisted command correctly rejected"
            return 0
        fi
    fi
    
    log_fail "Blacklisted command was not properly rejected"
    return 1
}

test_command_injection() {
    run_test "Security: Command injection attempt"
    
    local request='{"id":"9","method":"call_tool","params":{"tool":"echo","args":{"text":"test; rm -rf /"}}}'
    local response=""
    
    while IFS= read -r line; do
        response="$response$line"
        if echo "$line" | grep -q '"type":"error"'; then
            break
        fi
        if echo "$line" | grep -q '"type":"response"'; then
            break
        fi
    done < <(echo "$request" | timeout $TIMEOUT nc localhost $MCP_PORT 2>&1)
    
    # Should either error or only echo the text (not execute rm)
    if echo "$response" | grep -q '"type":"error"'; then
        log_pass "Command injection blocked by sanitization"
        return 0
    elif echo "$response" | grep -q "test; rm -rf /"; then
        # If it echoed the text literally, that's also OK
        log_pass "Command injection treated as literal text"
        return 0
    fi
    
    log_fail "Command injection test inconclusive"
    return 1
}

test_path_traversal() {
    run_test "Security: Path traversal attempt"
    
    local request='{"id":"10","method":"call_tool","params":{"tool":"mycat","args":{"file":"../../etc/passwd"}}}'
    local response=""
    
    while IFS= read -r line; do
        response="$response$line"
        if echo "$line" | grep -q '"type":"error"'; then
            break
        fi
        if echo "$line" | grep -q '"type":"response"'; then
            break
        fi
    done < <(echo "$request" | timeout $TIMEOUT nc localhost $MCP_PORT 2>&1)
    
    if echo "$response" | grep -q '"type":"error"'; then
        if echo "$response" | grep -q "dangerous"; then
            log_pass "Path traversal correctly blocked"
            return 0
        fi
    fi
    
    log_fail "Path traversal was not properly blocked"
    return 1
}

################################################################################
# Error Handling Tests
################################################################################

test_invalid_method() {
    run_test "Error handling: Invalid method"
    
    local request='{"id":"11","method":"nonexistent_method","params":{}}'
    local response=$(send_mcp_request "$request")
    
    if echo "$response" | grep -q '"type":"error"'; then
        log_pass "Invalid method returned error response"
        return 0
    fi
    
    log_fail "Invalid method did not return error"
    return 1
}

test_invalid_tool() {
    run_test "Error handling: Invalid tool name"
    
    local request='{"id":"12","method":"call_tool","params":{"tool":"nonexistent_tool","args":{}}}'
    local response=$(send_mcp_request "$request")
    
    if echo "$response" | grep -q '"type":"error"'; then
        log_pass "Invalid tool returned error response"
        return 0
    fi
    
    log_fail "Invalid tool did not return error"
    return 1
}

test_malformed_json() {
    run_test "Error handling: Malformed JSON"
    
    local request='{"id":"13","method":"initialize"'  # Missing closing brace
    local response=$(send_mcp_request "$request")
    
    # Server should either close connection or send error
    if [ -z "$response" ] || echo "$response" | grep -q "error"; then
        log_pass "Malformed JSON handled appropriately"
        return 0
    fi
    
    log_fail "Malformed JSON not properly handled"
    return 1
}

################################################################################
# Concurrent Execution Tests
################################################################################

test_concurrent_requests() {
    run_test "Concurrent execution: Multiple simultaneous requests"
    
    # Send 3 requests in parallel
    (echo '{"id":"14a","method":"call_tool","params":{"tool":"pwd","args":{}}}' | nc localhost $MCP_PORT > /tmp/mcp_test_14a.out 2>&1) &
    (echo '{"id":"14b","method":"call_tool","params":{"tool":"echo","args":{"text":"test1"}}}' | nc localhost $MCP_PORT > /tmp/mcp_test_14b.out 2>&1) &
    (echo '{"id":"14c","method":"call_tool","params":{"tool":"echo","args":{"text":"test2"}}}' | nc localhost $MCP_PORT > /tmp/mcp_test_14c.out 2>&1) &
    
    # Wait for all to complete
    wait
    
    # Check all got responses
    local passed=true
    for suffix in 14a 14b 14c; do
        if ! grep -q '"type":"response"' /tmp/mcp_test_${suffix}.out 2>/dev/null; then
            passed=false
        fi
        rm -f /tmp/mcp_test_${suffix}.out
    done
    
    if [ "$passed" = true ]; then
        log_pass "All concurrent requests completed successfully"
        return 0
    fi
    
    log_fail "Some concurrent requests failed"
    return 1
}

################################################################################
# Main Test Runner
################################################################################

print_banner() {
    echo ""
    echo "================================================================"
    echo "  Unified Shell - MCP Server Test Suite"
    echo "================================================================"
    echo ""
}

print_summary() {
    echo ""
    echo "================================================================"
    echo "  Test Summary"
    echo "================================================================"
    echo -e "Total Tests:   ${TESTS_RUN}"
    echo -e "${GREEN}Passed:        ${TESTS_PASSED}${NC}"
    echo -e "${RED}Failed:        ${TESTS_FAILED}${NC}"
    echo ""
    
    if [ $TESTS_FAILED -eq 0 ]; then
        echo -e "${GREEN}All tests passed!${NC}"
        return 0
    else
        echo -e "${RED}Some tests failed.${NC}"
        return 1
    fi
}

run_basic_tests() {
    echo -e "\n${BLUE}=== Basic Functionality Tests ===${NC}\n"
    test_connection
    test_initialize
    test_list_tools
    test_call_tool_pwd
    test_call_tool_echo
}

run_special_tool_tests() {
    echo -e "\n${BLUE}=== Special Tool Tests ===${NC}\n"
    test_get_shell_context
    test_search_commands
    test_suggest_command
}

run_security_tests() {
    echo -e "\n${BLUE}=== Security Tests ===${NC}\n"
    test_blacklisted_command
    test_command_injection
    test_path_traversal
}

run_error_tests() {
    echo -e "\n${BLUE}=== Error Handling Tests ===${NC}\n"
    test_invalid_method
    test_invalid_tool
    test_malformed_json
}

run_concurrent_tests() {
    echo -e "\n${BLUE}=== Concurrent Execution Tests ===${NC}\n"
    test_concurrent_requests
}

main() {
    print_banner
    
    # Parse command line arguments
    local test_suite="${1:-all}"
    
    # Start the shell
    if ! start_shell; then
        log_fail "Failed to start shell"
        exit 1
    fi
    
    # Run requested test suites
    case "$test_suite" in
        basic)
            run_basic_tests
            ;;
        special)
            run_special_tool_tests
            ;;
        security)
            run_security_tests
            ;;
        error)
            run_error_tests
            ;;
        concurrent)
            run_concurrent_tests
            ;;
        all)
            run_basic_tests
            run_special_tool_tests
            run_security_tests
            run_error_tests
            run_concurrent_tests
            ;;
        *)
            echo "Usage: $0 [basic|special|security|error|concurrent|all]"
            exit 1
            ;;
    esac
    
    # Print summary and exit
    print_summary
    exit $?
}

# Run main
main "$@"
