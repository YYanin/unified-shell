#!/bin/bash
################################################################################
# MCP Server Quick Start Guide
# How to run and test the unified shell MCP server
################################################################################

cat << 'EOF'
================================================================================
  UNIFIED SHELL - MCP SERVER QUICK START
================================================================================

PROBLEM: The shell is interactive and exits when there's no input.
SOLUTION: Keep the shell running with continuous input or in a subprocess.

================================================================================
  METHOD 1: Keep Shell Alive with Sleep (Recommended for Testing)
================================================================================

# Start shell in background with long-running input
(sleep 3600 | USHELL_MCP_ENABLED=1 ./ushell) &
SHELL_PID=$!
echo "Shell running with PID: $SHELL_PID"

# Wait a moment for server to start
sleep 2

# Verify server is listening
lsof -i :9000

# Now you can use Python clients:
python3 test_mcp_quick.py
python3 examples/mcp_client_example.py

# When done, kill the shell:
kill $SHELL_PID


================================================================================
  METHOD 2: Interactive Shell in One Terminal, Client in Another
================================================================================

# Terminal 1: Start interactive shell with MCP
USHELL_MCP_ENABLED=1 ./ushell

# Terminal 2: Run your MCP client
python3 test_mcp_quick.py
python3 examples/mcp_client_example.py

# When done in Terminal 1:
exit


================================================================================
  METHOD 3: Use the Automated Test Script
================================================================================

# This script handles everything automatically:
./tests/test_mcp_server.sh

# Or test specific categories:
./tests/test_mcp_server.sh basic
./tests/test_mcp_server.sh security


================================================================================
  METHOD 4: One-Liner for Quick Test
================================================================================

(sleep 60 | USHELL_MCP_ENABLED=1 ./ushell) >/dev/null 2>&1 & sleep 2 && python3 test_mcp_quick.py; pkill ushell


================================================================================
  TROUBLESHOOTING
================================================================================

ISSUE: Port already in use
FIX:
  # Kill any existing shells
  pkill -9 ushell
  
  # Clear the port environment variable if set
  unset USHELL_MCP_PORT
  
  # Start fresh
  (sleep 3600 | USHELL_MCP_ENABLED=1 ./ushell) &

ISSUE: Connection refused
FIX:
  # Check if shell is running
  ps aux | grep ushell
  
  # Check if port is listening
  lsof -i :9000
  
  # Make sure MCP is enabled
  USHELL_MCP_ENABLED=1 ./ushell

ISSUE: Shell exits immediately
FIX:
  # Don't run shell without input - use sleep method:
  (sleep 3600 | USHELL_MCP_ENABLED=1 ./ushell) &


================================================================================
  CONFIGURATION
================================================================================

Environment Variables:
  USHELL_MCP_ENABLED=1         # Required to enable MCP server
  USHELL_MCP_PORT=9000         # Default port (optional)
  USHELL_MCP_AUDIT_LOG=/path   # Enable audit logging (optional)
  USHELL_MCP_MAX_CLIENTS=10    # Max concurrent clients (optional)

Example with custom settings:
  USHELL_MCP_ENABLED=1 \
  USHELL_MCP_PORT=9001 \
  USHELL_MCP_AUDIT_LOG=/tmp/mcp.log \
  ./ushell


================================================================================
  QUICK TESTS
================================================================================

Test 1: Basic connectivity
  python3 test_mcp_quick.py

Test 2: Full client examples
  python3 examples/mcp_client_example.py

Test 3: AI agent demo
  python3 examples/mcp_ai_agent_example.py "find all Python files"

Test 4: Complete test suite
  ./tests/test_mcp_server.sh


================================================================================
  SUCCESS INDICATORS
================================================================================

When MCP server starts correctly, you'll see:
  MCP Server: Listening on port 9000 (max clients: 10)
  [MCP server enabled on port 9000]

When a client connects:
  MCP Server: Accepted connection from 127.0.0.1 (clients: 1/10)

When everything is working:
  ✓ lsof -i :9000 shows ushell listening
  ✓ Python clients connect successfully
  ✓ test_mcp_quick.py shows all tests passing


================================================================================

For comprehensive documentation, see: docs/MCP_INTEGRATION.md

EOF
