# MCP Integration Documentation

## Table of Contents

1. [Overview](#overview)
2. [Architecture](#architecture)
3. [Protocol Specification](#protocol-specification)
4. [Tool Catalog](#tool-catalog)
5. [Security Model](#security-model)
6. [Configuration](#configuration)
7. [Client Development](#client-development)
8. [Examples](#examples)
9. [Troubleshooting](#troubleshooting)

---

## Overview

The unified shell includes a **Model Context Protocol (MCP)** server that enables AI agents and external tools to interact with the shell programmatically. The MCP server provides:

- **Structured command execution** via JSON-RPC protocol
- **Tool discovery** with automatic schema generation
- **Safe execution** with multi-layer security
- **Progress tracking** with real-time notifications
- **AI helper tools** for intelligent command assistance

### Key Features

- ✅ **50+ Shell Tools**: All builtins and utilities exposed as MCP tools
- ✅ **Special AI Tools**: Context retrieval, command search, natural language suggestions
- ✅ **Security**: Blacklist, whitelist, input sanitization, resource limits
- ✅ **Notifications**: Real-time execution progress updates
- ✅ **Concurrent Execution**: Support for multiple simultaneous tool calls
- ✅ **Audit Logging**: Complete execution history for compliance

---

## Architecture

### System Design

```
┌─────────────────────────────────────────────────────────────┐
│                       AI Agent / Client                      │
│                    (Python, Node.js, etc.)                   │
└───────────────────────────┬─────────────────────────────────┘
                            │ TCP Connection (port 9000)
                            │ Newline-delimited JSON
                            ▼
┌─────────────────────────────────────────────────────────────┐
│                     MCP Server Thread                        │
│  ┌───────────────────────────────────────────────────────┐  │
│  │  Protocol Handler (mcp_server.c)                      │  │
│  │  • initialize, list_tools, call_tool                  │  │
│  │  • get_execution_status, cancel_execution             │  │
│  │  • Notifications (tool_started, completed, failed)    │  │
│  └───────────────────────────────────────────────────────┘  │
│  ┌───────────────────────────────────────────────────────┐  │
│  │  Tool Catalog (mcp_tools.c)                           │  │
│  │  • Load from commands.json                            │  │
│  │  • Generate JSON schemas for arguments                │  │
│  │  • 8 tool aliases for better discoverability          │  │
│  │  • Special tools: context, search, suggest            │  │
│  └───────────────────────────────────────────────────────┘  │
│  ┌───────────────────────────────────────────────────────┐  │
│  │  Security Layer (mcp_exec.c)                          │  │
│  │  • Command blacklist (sudo, rm, chmod, etc.)          │  │
│  │  • Input sanitization (shell metacharacters)          │  │
│  │  • Path validation (no .., /etc, /sys)                │  │
│  │  • Resource limits (CPU, memory, processes)           │  │
│  │  • Audit logging (JSON format)                        │  │
│  └───────────────────────────────────────────────────────┘  │
│  ┌───────────────────────────────────────────────────────┐  │
│  │  Execution Engine (mcp_exec.c)                        │  │
│  │  • Fork/exec model with pipes                         │  │
│  │  • stdout/stderr capture                              │  │
│  │  • Exit code tracking                                 │  │
│  │  • Execution tracking (32 concurrent max)             │  │
│  └───────────────────────────────────────────────────────┘  │
└───────────────────────────┬─────────────────────────────────┘
                            │ Direct execution
                            ▼
┌─────────────────────────────────────────────────────────────┐
│                    Shell Execution Layer                     │
│  • Builtins (cd, echo, export, history, etc.)               │
│  • Core tools (myls, mycat, mycp, mymv, etc.)               │
│  • System commands (find, grep, head, tail, etc.)           │
└─────────────────────────────────────────────────────────────┘
```

### Component Responsibilities

| Component | File | Responsibilities |
|-----------|------|------------------|
| **MCP Server** | `src/mcp_server/mcp_server.c` | TCP server, protocol handlers, notifications |
| **Tool Catalog** | `src/mcp_server/mcp_tools.c` | Tool discovery, schema generation, special tools |
| **Execution Engine** | `src/mcp_server/mcp_exec.c` | Command execution, security, resource limits |
| **JSON Processing** | `src/mcp_server/mcp_json.c` | JSON parsing, message construction |

---

## Protocol Specification

### Transport Layer

- **Protocol**: TCP
- **Port**: 9000 (default, configurable via `USHELL_MCP_PORT`)
- **Format**: Newline-delimited JSON (one message per line)
- **Encoding**: UTF-8

### Message Format

All messages follow JSON-RPC 2.0 structure:

```json
{
  "id": "unique_request_id",
  "method": "method_name",
  "params": {
    "key": "value"
  }
}
```

### Supported Methods

#### 1. initialize

Initialize MCP session and get server information.

**Request:**
```json
{
  "id": "1",
  "method": "initialize",
  "params": {}
}
```

**Response:**
```json
{
  "id": "1",
  "type": "response",
  "result": {
    "server": "unified-shell-mcp",
    "version": "1.0",
    "protocol_version": "1.0"
  }
}
```

#### 2. list_tools

Enumerate all available tools.

**Request:**
```json
{
  "id": "2",
  "method": "list_tools",
  "params": {}
}
```

**Response:**
```json
{
  "id": "2",
  "type": "response",
  "result": {
    "tools": [
      {
        "name": "ls",
        "description": "List directory contents",
        "schema": {
          "type": "object",
          "properties": {
            "path": {"type": "string", "description": "Directory path"}
          }
        },
        "aliases": ["list_files", "dir"]
      }
    ]
  }
}
```

#### 3. call_tool

Execute a tool with arguments.

**Request:**
```json
{
  "id": "3",
  "method": "call_tool",
  "params": {
    "tool": "ls",
    "args": {
      "path": "/home/user"
    }
  }
}
```

**Notifications (fire-and-forget):**
```json
{"type": "notification", "event": "tool_started", "tool": "ls", "execution_id": "exec_123"}
```

**Response:**
```json
{
  "id": "3",
  "type": "response",
  "result": {
    "output": "file1.txt\nfile2.txt\ndir/\n",
    "exit_code": 0,
    "execution_id": "exec_123"
  }
}
```

**Error Response:**
```json
{
  "id": "3",
  "type": "error",
  "error": "Command 'rm' is blacklisted for security reasons"
}
```

#### 4. get_execution_status

Query status of a running or completed execution.

**Request:**
```json
{
  "id": "4",
  "method": "get_execution_status",
  "params": {
    "execution_id": "exec_123"
  }
}
```

**Response:**
```json
{
  "id": "4",
  "type": "response",
  "result": {
    "status": "completed",
    "exit_code": 0,
    "output": "file1.txt\nfile2.txt\n"
  }
}
```

#### 5. cancel_execution

Cancel a running execution (sends SIGTERM to child process).

**Request:**
```json
{
  "id": "5",
  "method": "cancel_execution",
  "params": {
    "execution_id": "exec_123"
  }
}
```

**Response:**
```json
{
  "id": "5",
  "type": "response",
  "result": {
    "cancelled": true
  }
}
```

### Notification Events

Notifications are sent during tool execution (no `id` field, fire-and-forget):

| Event | When Sent | Fields |
|-------|-----------|--------|
| `tool_started` | Tool execution begins | `tool`, `execution_id` |
| `tool_completed` | Tool finishes successfully | `tool`, `execution_id`, `exit_code` |
| `tool_failed` | Tool fails or crashes | `tool`, `execution_id`, `error` |
| `tool_progress` | Progress update (future) | `tool`, `execution_id`, `progress` |

---

## Tool Catalog

### Standard Tools (from commands.json)

The shell exposes 45+ tools including:

**Builtin Commands:**
- `cd`, `echo`, `export`, `history`, `jobs`, `bg`, `fg`, `kill`, `exit`, `help`

**Core File Tools:**
- `myls`, `mycat`, `mycp`, `mymv`, `myrm`, `mytouch`, `mymkdir`, `myrmdir`, `mystat`

**System Commands:**
- `find`, `grep`, `head`, `tail`, `wc`, `sort`, `uniq`, `cut`, `awk`, `sed`, `ps`, `top`, `df`, `du`

### Special AI Tools

#### get_shell_context

Returns comprehensive shell environment context.

```json
{
  "tool": "get_shell_context",
  "args": {}
}
```

**Output:**
```json
{
  "cwd": "/home/user/project",
  "user": "user",
  "history": ["ls -la", "cd project", "cat README.md"],
  "env": {
    "PATH": "/usr/bin:/bin",
    "HOME": "/home/user"
  }
}
```

#### search_commands

Search for commands by keyword with relevance scoring.

```json
{
  "tool": "search_commands",
  "args": {
    "query": "list files"
  }
}
```

**Output:**
```json
{
  "query": "list files",
  "results": [
    {"name": "myls", "description": "List directory contents", "score": 15},
    {"name": "find", "description": "Search for files", "score": 10}
  ]
}
```

#### suggest_command

Translate natural language to shell commands.

```json
{
  "tool": "suggest_command",
  "args": {
    "query": "find all Python files"
  }
}
```

**Output:**
```json
{
  "query": "find all Python files",
  "command": "find . -name '*.py'",
  "explanation": "Find all files with .py extension recursively"
}
```

### Tool Aliases

8 descriptive aliases for better discoverability:

| Alias | Maps To | Use Case |
|-------|---------|----------|
| `list_files` | `myls` | More descriptive than "ls" |
| `view_file` | `mycat` | Better intent clarity |
| `copy_file` | `mycp` | Explicit action |
| `move_file` | `mymv` | Explicit action |
| `delete_file` | `myrm` | Explicit action |
| `create_file` | `mytouch` | Better for AI agents |
| `make_directory` | `mymkdir` | Descriptive name |
| `remove_directory` | `myrmdir` | Explicit action |

---

## Security Model

### Multi-Layer Security

#### 1. Command Blacklist

13 dangerous commands blocked by default:

```c
sudo, su, rm, chmod, chown, chgrp, kill, killall, 
pkill, reboot, shutdown, halt, init
```

**Rationale**: Prevents privilege escalation, system damage, and unauthorized access.

#### 2. Input Sanitization

21 shell metacharacters blocked in arguments:

```
; | & $ ` < > ( ) { } [ ] * ? ~ ! #
```

**Rationale**: Prevents command injection and shell escaping.

#### 3. Path Validation

Blocked patterns:
- `..` (directory traversal)
- `/etc`, `/sys`, `/proc` (system directories)
- `/root`, `/boot` (privileged areas)

**Rationale**: Prevents unauthorized file access.

#### 4. Resource Limits

| Resource | Limit | Enforcement |
|----------|-------|-------------|
| **CPU Time** | 30 seconds | `RLIMIT_CPU` |
| **Memory** | 256 MB | `RLIMIT_AS` |
| **Processes** | 10 | `RLIMIT_NPROC` |
| **File Size** | 10 MB | `RLIMIT_FSIZE` |
| **File Descriptors** | 50 | `RLIMIT_NOFILE` |

**Rationale**: Prevents resource exhaustion and DoS attacks.

#### 5. Audit Logging

All command executions logged in JSON format:

```json
{
  "timestamp": "2025-01-15T10:30:45Z",
  "user": "user",
  "command": "ls",
  "args": ["-la", "/home/user"],
  "status": "success",
  "exit_code": 0,
  "execution_time_ms": 45
}
```

**Log Location**: Set via `USHELL_MCP_AUDIT_LOG` environment variable.

### Security Best Practices

1. **Run as non-root user** - Never run the shell as root
2. **Use whitelist mode** - Only enable needed commands
3. **Monitor audit logs** - Watch for suspicious patterns
4. **Network isolation** - Bind to localhost only in production
5. **Regular updates** - Keep the shell updated for security fixes

---

## Configuration

### Environment Variables

| Variable | Default | Description |
|----------|---------|-------------|
| `USHELL_MCP_ENABLED` | `0` | Enable MCP server (`1` = enabled) |
| `USHELL_MCP_PORT` | `9000` | TCP port for MCP server |
| `USHELL_MCP_AUDIT_LOG` | (none) | Path to audit log file |
| `USHELL_MCP_MAX_CLIENTS` | `5` | Maximum concurrent clients |
| `USHELL_MCP_TIMEOUT` | `30` | Client timeout in seconds |

### Startup Example

```bash
# Enable MCP server on default port
USHELL_MCP_ENABLED=1 ./ushell

# Custom port with audit logging
USHELL_MCP_ENABLED=1 USHELL_MCP_PORT=9001 \
  USHELL_MCP_AUDIT_LOG=/var/log/ushell_audit.log ./ushell
```

### Commands Catalog

Tools are loaded from `aiIntegr/commands.json`. To customize:

1. Edit `commands.json` to add/remove commands
2. Each entry must have: `name`, `description`, `args` (array of argument names)
3. Restart the shell to reload

Example command entry:
```json
{
  "name": "grep",
  "description": "Search for pattern in files",
  "args": ["pattern", "file"]
}
```

---

## Client Development

### Connection Flow

1. **Connect** to TCP socket (default: `localhost:9000`)
2. **Initialize** session with `initialize` method
3. **Discover tools** with `list_tools` method
4. **Execute tools** with `call_tool` method
5. **Handle notifications** asynchronously
6. **Close** connection when done

### Python Client Example

```python
import socket
import json

class MCPClient:
    def __init__(self, host='localhost', port=9000):
        self.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.socket.connect((host, port))
        self.buffer = ""
    
    def send_request(self, req_id, method, params=None):
        msg = {"id": req_id, "method": method, "params": params or {}}
        self.socket.send((json.dumps(msg) + "\n").encode())
    
    def receive_message(self):
        while '\n' not in self.buffer:
            self.buffer += self.socket.recv(4096).decode()
        line, self.buffer = self.buffer.split('\n', 1)
        return json.loads(line)
    
    def call_tool(self, tool, args):
        self.send_request("1", "call_tool", {"tool": tool, "args": args})
        # Skip notifications
        while True:
            msg = self.receive_message()
            if msg.get("type") in ["response", "error"]:
                return msg

# Usage
client = MCPClient()
client.send_request("1", "initialize", {})
print(client.receive_message())

client.send_request("2", "list_tools", {})
print(client.receive_message())

result = client.call_tool("pwd", {})
print(result["result"]["output"])
```

### Node.js Client Example

```javascript
const net = require('net');

class MCPClient {
  constructor(host = 'localhost', port = 9000) {
    this.socket = net.createConnection(port, host);
    this.buffer = '';
    this.requestId = 0;
  }

  sendRequest(method, params = {}) {
    this.requestId++;
    const msg = JSON.stringify({
      id: this.requestId.toString(),
      method,
      params
    }) + '\n';
    this.socket.write(msg);
  }

  receiveMessage() {
    return new Promise((resolve) => {
      const handler = (data) => {
        this.buffer += data.toString();
        if (this.buffer.includes('\n')) {
          const [line, rest] = this.buffer.split('\n', 2);
          this.buffer = rest;
          this.socket.off('data', handler);
          resolve(JSON.parse(line));
        }
      };
      this.socket.on('data', handler);
    });
  }

  async initialize() {
    this.sendRequest('initialize', {});
    return await this.receiveMessage();
  }

  async callTool(tool, args = {}) {
    this.sendRequest('call_tool', { tool, args });
    // Skip notifications until response
    while (true) {
      const msg = await this.receiveMessage();
      if (msg.type === 'response' || msg.type === 'error') {
        return msg;
      }
    }
  }
}

// Usage
(async () => {
  const client = new MCPClient();
  await client.initialize();
  const result = await client.callTool('ls', { path: '.' });
  console.log(result.result.output);
})();
```

---

## Examples

See `examples/` directory for complete examples:

1. **mcp_client_example.py** - Simple Python client demonstrating all operations
2. **mcp_ai_agent_example.py** - Mock AI agent with intelligent tool selection
3. **tutorial.txt** - Interactive tutorial with exercises

### Quick Test

```bash
# Terminal 1: Start shell with MCP enabled
USHELL_MCP_ENABLED=1 ./ushell

# Terminal 2: Test with netcat
echo '{"id":"1","method":"initialize","params":{}}' | nc localhost 9000

# Or use Python client
cd examples
python3 mcp_client_example.py
```

---

## Troubleshooting

### Common Issues

#### 1. Connection Refused

**Symptoms**: `Connection refused` error when connecting to port 9000

**Solutions**:
- Ensure MCP is enabled: `USHELL_MCP_ENABLED=1 ./ushell`
- Check port availability: `lsof -i :9000`
- Try custom port: `USHELL_MCP_PORT=9001 ./ushell`

#### 2. Command Blacklisted

**Symptoms**: `error: "Command 'rm' is blacklisted"`

**Solutions**:
- Use safe alternatives: `myrm` instead of `rm`
- Check blacklist in `mcp_exec.c:is_command_blacklisted()`
- Consider if operation is actually needed

#### 3. Input Sanitization Error

**Symptoms**: `error: "Invalid characters in argument"`

**Solutions**:
- Remove shell metacharacters: `;`, `|`, `&`, `$`, etc.
- Pass complex arguments via files instead
- Use escaping where supported

#### 4. Path Validation Error

**Symptoms**: `error: "Path contains dangerous pattern"`

**Solutions**:
- Use absolute paths without `..`
- Avoid system directories (`/etc`, `/sys`, etc.)
- Work in user-writable areas

#### 5. Resource Limit Exceeded

**Symptoms**: Tool execution terminates early

**Solutions**:
- Check timeout limits (default: 30s CPU time)
- Reduce memory usage (limit: 256MB)
- Avoid fork bombs (limit: 10 processes)

### Debug Mode

Enable verbose logging (requires recompilation):

```c
// In mcp_server.c
#define MCP_DEBUG 1
```

This will log all protocol messages to stderr.

### Audit Log Analysis

Check audit log for patterns:

```bash
# Count executions per command
jq -r '.command' /var/log/ushell_audit.log | sort | uniq -c | sort -rn

# Find failed executions
jq 'select(.status == "failed")' /var/log/ushell_audit.log

# Get execution time stats
jq -r '.execution_time_ms' /var/log/ushell_audit.log | \
  awk '{sum+=$1; count++} END {print "Avg:", sum/count "ms"}'
```

---

## Additional Resources

- **User Guide**: [docs/USER_GUIDE.md](USER_GUIDE.md)
- **Developer Guide**: [docs/DEVELOPER_GUIDE.md](DEVELOPER_GUIDE.md)
- **Example Scripts**: [examples/](../examples/)
- **Test Suite**: [tests/test_mcp_server.sh](../tests/test_mcp_server.sh)

---

*Last Updated: 2025-01-15*
