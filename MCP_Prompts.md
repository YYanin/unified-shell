# MCP (Model Context Protocol) Integration Prompts for Unified Shell

This document contains step-by-step prompts to implement Model Context Protocol (MCP) server functionality into the unified shell, enabling external AI agents to discover and execute shell operations via a TCP socket-based JSON protocol.

## Overview

### Feature: MCP Server Integration
Currently, the shell has basic AI integration via `ushell_ai.py` that uses heuristic and optional LLM-based command suggestions. This enhancement will:
- Implement a TCP-based MCP server inside the shell
- Expose shell capabilities as discoverable MCP tools
- Allow external AI agents to connect and execute shell operations
- Support newline-delimited JSON protocol for tool discovery and execution
- Run MCP server in background thread to avoid blocking shell operations
- Maintain backward compatibility with existing @ AI helper functionality

Architecture:
- MCP server runs in dedicated thread listening on configurable port (default: 9000)
- Client connects and sends JSON messages for initialize, list_tools, call_tool
- Server exposes shell builtins and tools as MCP tools with proper schemas
- Thread-safe execution of shell commands from MCP requests
- Progress notifications and error reporting back to client

### Current AI Integration
The shell already has:
- `ushell_ai.py` - Natural language to command helper (@ prefix)
- `commands.json` - Catalog of available commands with descriptions
- RAG-like command selection using keyword scoring
- Optional OpenAI integration for LLM-based suggestions

### New MCP Functionality
Will add:
- TCP server accepting MCP protocol connections
- Tool discovery via list_tools (built from commands.json catalog)
- Tool execution via call_tool (mapping to actual shell commands)
- Notifications for progress and status updates
- Optional: Integration with existing AI helper for enhanced context

---

## Feature: MCP Server for Unified Shell

### Prompt 1: Setup MCP Server Infrastructure

#### Prompt
```
Set up the MCP server infrastructure for the unified shell:

1. Create new header file: include/mcp_server.h
   - Include pthread.h, sys/socket.h, arpa/inet.h, json support
   - Define MCP server configuration structure:
     typedef struct {
         int port;                    // TCP port (default 9000)
         int enabled;                 // 0=disabled, 1=enabled
         int sockfd;                  // Server socket descriptor
         pthread_t server_thread;     // MCP server thread
         pthread_mutex_t lock;        // Mutex for thread safety
         int running;                 // Server running flag
         Env *env;                    // Shell environment pointer
     } MCPServerConfig;
   
   - Define MCP message structures:
     typedef struct {
         char *id;                    // Request ID (for correlation)
         char *method;                // Method name (initialize, list_tools, call_tool)
         char *params;                // JSON parameters string
     } MCPRequest;
     
     typedef struct {
         char *id;                    // Matching request ID
         char *type;                  // response, error, notification
         char *result;                // JSON result string
     } MCPResponse;
   
2. Define MCP server functions:
   - MCPServerConfig* mcp_server_create(int port, Env *env)
     Allocates and initializes server configuration
   
   - void mcp_server_destroy(MCPServerConfig *config)
     Stops server and frees all resources
   
   - void* mcp_server_thread(void *arg)
     Thread entry point for MCP server (accept loop)
   
   - void* mcp_client_handler(void *arg)
     Handles individual client connection (forked or threaded)
   
   - int mcp_server_start(MCPServerConfig *config)
     Creates socket, binds, listens, starts server thread
   
   - void mcp_server_stop(MCPServerConfig *config)
     Stops server thread and closes socket
   
   - int mcp_parse_request(const char *json, MCPRequest *req)
     Parses JSON request into MCPRequest structure
   
   - char* mcp_build_response(MCPResponse *resp)
     Builds JSON response string from MCPResponse structure
   
   - int mcp_send_message(int client_fd, const char *json)
     Sends newline-delimited JSON message to client
   
   - int mcp_recv_message(int client_fd, char *buffer, size_t bufsize)
     Receives newline-delimited JSON message from client

3. Add JSON parsing library:
   - Option 1: Use cJSON (lightweight, easy to integrate)
   - Option 2: Use jsmn (minimal, tokenizer-based)
   - Option 3: Simple custom parser for educational purposes
   - Add to Makefile with appropriate includes/libraries

4. Update Makefile:
   - Add -lpthread if not already present
   - Add JSON library (e.g., -lcjson if using cJSON)
   - Add src/mcp_server/mcp_server.c to compilation
   - Add include/mcp_server.h to headers

5. Add MCP server configuration to shell.h:
   - Add MCPServerConfig* mcp_config field to main shell context
   - Initialize in shell startup
   - Clean up in shell shutdown

6. Add environment variable support:
   - USHELL_MCP_ENABLED (0/1) - Enable/disable MCP server
   - USHELL_MCP_PORT (default: 9000) - TCP port for server
   - USHELL_MCP_DEBUG (0/1) - Enable debug logging
```

#### Manual Tests
1. **Header file created**:
   ```bash
   ls -la include/mcp_server.h
   # Expected: File exists
   cat include/mcp_server.h
   # Expected: Shows MCP structures and function declarations
   ```

2. **Makefile updated**:
   ```bash
   cd unified-shell
   grep -E "mcp_server|lpthread|cjson" Makefile
   # Expected: Shows MCP server compilation and pthread/json libs
   make clean && make
   # Expected: Compiles without MCP errors
   ```

3. **Basic server initialization test**:
   ```bash
   export USHELL_MCP_ENABLED=0
   ./ushell
   # Type: pwd
   # Expected: Shell works normally, MCP server not started
   ```

4. **Environment variable parsing**:
   ```bash
   export USHELL_MCP_ENABLED=1
   export USHELL_MCP_PORT=9001
   ./ushell
   # Expected: Shell starts (server may not be functional yet)
   ```

---

### Prompt 2: Implement MCP Protocol Handlers

#### Prompt
```
Implement the core MCP protocol message handlers:

1. Create src/mcp_server/mcp_server.c:
   - Implement mcp_server_create():
     * Allocate MCPServerConfig structure
     * Initialize mutex with pthread_mutex_init()
     * Set port, env pointer
     * Initialize running = 0, sockfd = -1
     * Return config
   
   - Implement mcp_server_destroy():
     * Call mcp_server_stop() if running
     * Destroy mutex with pthread_mutex_destroy()
     * Free config structure
   
   - Implement mcp_send_message():
     * Format: send JSON string + newline character
     * Use write() or send() system call
     * Check for errors and handle SIGPIPE
     * Return bytes sent or -1 on error
   
   - Implement mcp_recv_message():
     * Read until newline character found
     * Handle partial reads and buffer overflow
     * Null-terminate the received string
     * Return bytes read or -1 on error
   
   - Implement mcp_parse_request():
     * Parse JSON string using chosen library
     * Extract "id", "method", "params" fields
     * Validate required fields are present
     * Return 0 on success, -1 on parse error
   
   - Implement mcp_build_response():
     * Build JSON object with "id", "type", "result"/"error"
     * Format as newline-delimited JSON
     * Return dynamically allocated string (caller must free)

2. Implement message handler functions:
   - int mcp_handle_initialize(int client_fd, MCPRequest *req, Env *env)
     * Send initialization response with server info
     * Response format:
       {"id":"<req_id>","type":"response","result":{"server":"unified-shell MCP","version":"1.0"}}
     * Return 0 on success
   
   - int mcp_handle_list_tools(int client_fd, MCPRequest *req, Env *env)
     * Load commands.json catalog
     * Build tools array from catalog
     * Each tool has: name, description, schema (parameters)
     * Response format:
       {"id":"<req_id>","type":"response","result":{"tools":[...]}}
     * Return 0 on success
   
   - int mcp_handle_call_tool(int client_fd, MCPRequest *req, Env *env)
     * Parse tool name and arguments from params
     * Validate tool exists in catalog
     * Sanitize arguments (prevent injection)
     * Execute corresponding shell command
     * Capture output (stdout/stderr)
     * Send result back to client
     * Response format:
       {"id":"<req_id>","type":"response","result":{"tool":"<name>","output":"<output>"}}
     * Send progress notifications for long operations
     * Return 0 on success, -1 on error

3. Add request routing:
   - int mcp_handle_request(int client_fd, const char *json_req, Env *env)
     * Parse request using mcp_parse_request()
     * Route to appropriate handler based on method:
       - "initialize" -> mcp_handle_initialize()
       - "list_tools" -> mcp_handle_list_tools()
       - "call_tool" -> mcp_handle_call_tool()
     * Return error response for unknown methods
     * Free allocated memory before returning

4. Add command execution safety:
   - Create whitelist of allowed commands
   - Sanitize all arguments (remove special chars like ;|&$`"')
   - Use safe execution (fork/exec, not system())
   - Set resource limits (timeout, max output size)
   - Validate paths (prevent directory traversal)
   - Log all command executions for audit

5. Add error handling:
   - JSON parse errors -> send error response
   - Invalid method -> send error response
   - Command execution failure -> send error in result
   - Client disconnect -> clean up and exit handler
   - Resource exhaustion -> graceful degradation
```

#### Manual Tests
1. **JSON message parsing**:
   ```bash
   # Create test JSON file
   echo '{"id":"1","method":"initialize","params":{}}' > /tmp/test_mcp.json
   
   # Test parsing (add debug output to parser)
   export USHELL_MCP_DEBUG=1
   export USHELL_MCP_ENABLED=1
   ./ushell
   # Expected: Debug output shows successful JSON parsing
   ```

2. **Initialize handler**:
   ```bash
   # Start shell with MCP enabled
   export USHELL_MCP_ENABLED=1
   ./ushell
   
   # In another terminal, test with netcat
   echo '{"id":"1","method":"initialize","params":{}}' | nc localhost 9000
   # Expected: Receives JSON response with server info
   ```

3. **List tools handler**:
   ```bash
   # Ensure commands.json exists and is valid
   cat unified-shell/aiIntegr/commands.json
   
   # Test list_tools
   echo '{"id":"2","method":"list_tools","params":{}}' | nc localhost 9000
   # Expected: Receives JSON array of available tools/commands
   ```

4. **Call tool handler**:
   ```bash
   # Test safe command execution
   echo '{"id":"3","method":"call_tool","params":{"tool":"pwd","args":{}}}' | nc localhost 9000
   # Expected: Receives current working directory in response
   
   # Test command with arguments
   echo '{"id":"4","method":"call_tool","params":{"tool":"echo","args":{"text":"hello"}}}' | nc localhost 9000
   # Expected: Receives "hello" in response
   ```

5. **Error handling**:
   ```bash
   # Test invalid JSON
   echo 'not valid json' | nc localhost 9000
   # Expected: Receives error response
   
   # Test unknown method
   echo '{"id":"5","method":"invalid_method","params":{}}' | nc localhost 9000
   # Expected: Receives error response for unknown method
   ```

---

### Prompt 3: Implement MCP Server Thread and Client Handler

#### Prompt
```
Implement the MCP server thread and client connection handler:

1. Implement mcp_server_start():
   - Create TCP socket with socket(AF_INET, SOCK_STREAM, 0)
   - Set SO_REUSEADDR option for quick restart
   - Bind to INADDR_ANY and configured port
   - Listen with backlog of 4-8 connections
   - Store socket in config->sockfd
   - Set config->running = 1
   - Create server thread with pthread_create(&config->server_thread, NULL, mcp_server_thread, config)
   - Return 0 on success, -1 on error
   - Log server start with port number

2. Implement mcp_server_thread():
   - Main accept loop while config->running is true
   - Accept incoming connections with accept()
   - For each connection, spawn handler:
     * Option 1: Fork child process (like notebook example)
     * Option 2: Create new thread for each client
     * Option 3: Use thread pool (advanced)
   - Parent/main thread continues accepting
   - Clean up zombie processes with waitpid(-1, NULL, WNOHANG)
   - On shutdown, close listen socket and exit thread

3. Implement mcp_client_handler():
   - Receives client_fd and env pointer as arguments
   - Read-handle-respond loop:
     * Call mcp_recv_message() to get JSON request
     * Call mcp_handle_request() to process and respond
     * Break on client disconnect or error
   - Close client socket when done
   - If forked, exit(0); if threaded, return NULL
   - Log client connections and disconnections

4. Implement mcp_server_stop():
   - Set config->running = 0
   - Close listen socket (config->sockfd)
   - Join server thread with pthread_join()
   - Close any remaining client connections
   - Log server shutdown

5. Add server lifecycle to shell:
   - In shell initialization (main.c):
     * Check USHELL_MCP_ENABLED environment variable
     * If enabled, create and start MCP server
     * Store config in shell context
   - In shell shutdown:
     * Stop and destroy MCP server if running
     * Ensure clean exit with no hanging threads

6. Add signal handling:
   - Graceful shutdown on SIGINT/SIGTERM
   - Ignore SIGPIPE (broken pipe from client disconnect)
   - Ensure MCP server stops before shell exits

7. Add connection limits:
   - Maximum concurrent clients (e.g., 5-10)
   - Connection timeout (e.g., 60 seconds idle)
   - Maximum request size (prevent DoS)
   - Rate limiting per client (optional)
```

#### Manual Tests
1. **Server starts successfully**:
   ```bash
   export USHELL_MCP_ENABLED=1
   export USHELL_MCP_PORT=9000
   ./ushell
   # Expected: Shell starts, shows "MCP server listening on port 9000"
   
   # Check socket is listening
   netstat -tlnp | grep 9000
   # Expected: Shows listening socket on port 9000
   ```

2. **Multiple client connections**:
   ```bash
   # Start shell
   export USHELL_MCP_ENABLED=1
   ./ushell
   
   # Terminal 2: Connect first client
   nc localhost 9000
   {"id":"1","method":"initialize","params":{}}
   # Expected: Receives response
   
   # Terminal 3: Connect second client (concurrent)
   nc localhost 9000
   {"id":"1","method":"list_tools","params":{}}
   # Expected: Both clients work independently
   ```

3. **Client disconnect handling**:
   ```bash
   # Start shell and connect client
   export USHELL_MCP_ENABLED=1
   ./ushell
   
   # Connect and disconnect abruptly
   nc localhost 9000
   ^C  # Ctrl-C to disconnect
   
   # Shell should not crash
   # Type in shell: pwd
   # Expected: Shell still works normally
   ```

4. **Server shutdown**:
   ```bash
   export USHELL_MCP_ENABLED=1
   ./ushell
   # Type: exit
   
   # Check no hanging processes
   ps aux | grep ushell
   # Expected: No zombie or hanging ushell processes
   
   # Check port released
   netstat -tlnp | grep 9000
   # Expected: Port 9000 not in use
   ```

5. **Stress test**:
   ```bash
   # Start shell
   export USHELL_MCP_ENABLED=1
   ./ushell
   
   # Rapid connections
   for i in {1..10}; do
     echo '{"id":"'$i'","method":"initialize","params":{}}' | nc localhost 9000 &
   done
   wait
   # Expected: All requests handled, no crashes
   ```

---

### Prompt 4: Build Tool Catalog from Commands JSON

#### Prompt
```
Implement tool catalog generation from the existing commands.json file:

1. Create catalog loader in src/mcp_server/mcp_tools.c:
   - int mcp_load_catalog(const char *json_path, char **tools_json)
     * Read commands.json file
     * Parse JSON array of commands
     * Transform each command into MCP tool format
     * Build complete tools JSON array
     * Store in dynamically allocated string
     * Return 0 on success, -1 on error
   
   - MCP tool format (from command):
     * name: command name (e.g., "cd", "ls", "myls")
     * description: command summary + description
     * inputSchema: JSON schema for parameters
       - type: "object"
       - properties: derived from command options
       - required: array of required parameters
   
   Example transformation:
   Command JSON:
   {
     "name": "cd",
     "summary": "Change directory",
     "usage": "cd [directory]",
     "options": [{"arg": "directory", "help": "Target directory"}]
   }
   
   MCP Tool JSON:
   {
     "name": "cd",
     "description": "Change directory: Change the current working directory",
     "inputSchema": {
       "type": "object",
       "properties": {
         "directory": {"type": "string", "description": "Target directory"}
       }
     }
   }

2. Add schema generation logic:
   - For each command option:
     * If option has "arg", create schema property
     * Infer type from option context:
       - "path", "directory", "file" -> type: "string"
       - "count", "number", "size" -> type: "integer"
       - "flag", "enable" -> type: "boolean"
     * Add description from option "help" field
   
   - Mark required parameters:
     * Options without defaults are required
     * Options in <angle_brackets> are required
     * Options in [square_brackets] are optional

3. Integrate with list_tools handler:
   - Update mcp_handle_list_tools() to use mcp_load_catalog()
   - Cache catalog in memory (don't reload every time)
   - Support catalog refresh if commands.json changes
   - Add cache invalidation mechanism

4. Add special MCP tools beyond commands.json:
   - Tool: "execute_script"
     * Description: "Execute a series of shell commands"
     * Parameters: commands (array of strings)
     * Execution: Run each command in sequence
   
   - Tool: "get_shell_info"
     * Description: "Get current shell state information"
     * Parameters: none
     * Returns: cwd, user, hostname, env vars
   
   - Tool: "get_history"
     * Description: "Get command history"
     * Parameters: limit (optional, default 10)
     * Returns: Recent commands from history
   
   - Tool: "list_files_detailed"
     * Description: "List files with metadata"
     * Parameters: path, recursive (boolean)
     * Uses myls or ls -la with additional parsing

5. Add tool alias mapping:
   - Map common tool names to actual commands:
     * "list_directory" -> "ls" or "myls"
     * "change_directory" -> "cd"
     * "remove_file" -> "rm" or "myrm"
     * "copy_file" -> "cp" or "mycp"
     * "move_file" -> "mv" or "mymv"
   - Allow AI agents to use descriptive tool names
   - Maintain backward compatibility with command names
```

#### Manual Tests
1. **Catalog loading**:
   ```bash
   # Verify commands.json exists
   cat unified-shell/aiIntegr/commands.json | head -20
   
   # Test catalog loader
   export USHELL_MCP_ENABLED=1
   export USHELL_MCP_DEBUG=1
   ./ushell
   # Expected: Debug output shows catalog loaded successfully
   ```

2. **Tool schema generation**:
   ```bash
   # Request tools list
   echo '{"id":"1","method":"list_tools","params":{}}' | nc localhost 9000 | python3 -m json.tool
   # Expected: Formatted JSON with all tools and proper schemas
   
   # Verify specific tool
   echo '{"id":"1","method":"list_tools","params":{}}' | nc localhost 9000 | grep -A 10 '"name":"cd"'
   # Expected: Shows cd tool with description and inputSchema
   ```

3. **Tool aliases**:
   ```bash
   # Test descriptive tool name
   echo '{"id":"2","method":"call_tool","params":{"tool":"list_directory","args":{"path":"."}}}' | nc localhost 9000
   # Expected: Executes ls/myls command successfully
   
   # Test original command name
   echo '{"id":"3","method":"call_tool","params":{"tool":"ls","args":{"path":"."}}}' | nc localhost 9000
   # Expected: Same result as alias
   ```

4. **Special MCP tools**:
   ```bash
   # Test get_shell_info
   echo '{"id":"4","method":"call_tool","params":{"tool":"get_shell_info","args":{}}}' | nc localhost 9000
   # Expected: Returns current directory, user, etc.
   
   # Test get_history
   echo '{"id":"5","method":"call_tool","params":{"tool":"get_history","args":{"limit":5}}}' | nc localhost 9000
   # Expected: Returns last 5 commands from history
   ```

5. **Schema validation**:
   ```bash
   # All tools should have valid schemas
   echo '{"id":"1","method":"list_tools","params":{}}' | nc localhost 9000 > /tmp/tools.json
   python3 -c "import json; tools=json.load(open('/tmp/tools.json'))['result']['tools']; print('Tools:', len(tools)); [print(f\"{t['name']}: {'inputSchema' in t}\") for t in tools]"
   # Expected: All tools have inputSchema field
   ```

---

### Prompt 5: Implement Secure Command Execution

#### Prompt
```
Implement secure command execution for MCP tool calls:

1. Create command sanitization in src/mcp_server/mcp_exec.c:
   - char* sanitize_path_arg(const char *input)
     * Remove/escape dangerous characters
     * Validate path doesn't escape allowed directories
     * Resolve symlinks and canonical paths
     * Check against blacklist (e.g., /etc/shadow, /proc/self)
     * Return sanitized string or NULL if invalid
   
   - char* sanitize_string_arg(const char *input, int max_length)
     * Remove shell metacharacters: ; | & $ ` " ' \ < > ( )
     * Limit length to prevent buffer overflow
     * Validate UTF-8 encoding
     * Return sanitized string or NULL if invalid
   
   - int validate_integer_arg(const char *input, int *value, int min, int max)
     * Parse integer safely (no overflow)
     * Validate range constraints
     * Return 0 on success, -1 on invalid

2. Implement safe command builder:
   - char** build_argv_array(const char *command, const char *args_json)
     * Parse arguments from JSON
     * Apply appropriate sanitization for each arg type
     * Build NULL-terminated argv array
     * Return array or NULL on error
   
   - void free_argv_array(char **argv)
     * Free all strings in array
     * Free array itself

3. Implement command executor:
   - int mcp_execute_command(const char *tool_name, char **argv, char **output, char **error)
     * Validate tool_name against whitelist
     * Fork child process
     * Setup pipes for stdout/stderr capture
     * Use execve() in child (not system())
     * Set resource limits (CPU time, memory, processes)
     * Set timeout (e.g., 30 seconds)
     * Parent waits for child with timeout
     * Kill child if timeout exceeded
     * Capture and return output/error
     * Return exit status
   
4. Add resource limits (child process):
   - Use setrlimit() before exec:
     * RLIMIT_CPU: 30 seconds
     * RLIMIT_AS: 256 MB memory
     * RLIMIT_NPROC: 10 processes
     * RLIMIT_FSIZE: 10 MB file writes
     * RLIMIT_NOFILE: 50 open files
   
5. Add command whitelist:
   - Create whitelist of allowed commands in config
   - Include all builtin commands
   - Include safe external tools (ls, cat, grep, etc.)
   - Exclude dangerous commands (rm -rf, dd, chmod, etc.)
   - Allow configuration via environment or config file
   
6. Add command blacklist patterns:
   - Block dangerous flags:
     * rm with -rf together
     * chmod with 777
     * Any command writing to /etc or /sys
   - Block path traversal attempts:
     * ../ in paths
     * Symlinks to restricted areas
   
7. Add audit logging:
   - Log all MCP command executions to file
   - Include: timestamp, client IP, tool name, arguments, result
   - Rotate logs to prevent disk fill
   - Format: JSON lines for easy parsing
   
8. Add rate limiting:
   - Track commands per client per time window
   - Limit: 10 commands per 10 seconds per client
   - Return error if limit exceeded
   - Configurable via environment variable
```

#### Manual Tests
1. **Path sanitization**:
   ```bash
   # Test directory traversal attempt
   echo '{"id":"1","method":"call_tool","params":{"tool":"ls","args":{"path":"../../etc"}}}' | nc localhost 9000
   # Expected: Error or sanitized path, not /etc listing
   
   # Test valid path
   echo '{"id":"2","method":"call_tool","params":{"tool":"ls","args":{"path":"./src"}}}' | nc localhost 9000
   # Expected: Successful directory listing
   ```

2. **Command injection prevention**:
   ```bash
   # Test shell metacharacters
   echo '{"id":"3","method":"call_tool","params":{"tool":"echo","args":{"text":"test; rm -rf /"}}}' | nc localhost 9000
   # Expected: Prints "test; rm -rf /" as text, doesn't execute rm
   
   # Test backticks
   echo '{"id":"4","method":"call_tool","params":{"tool":"echo","args":{"text":"`whoami`"}}}' | nc localhost 9000
   # Expected: Prints "`whoami`" literally, doesn't execute
   ```

3. **Whitelist enforcement**:
   ```bash
   # Test allowed command
   echo '{"id":"5","method":"call_tool","params":{"tool":"pwd","args":{}}}' | nc localhost 9000
   # Expected: Returns current directory
   
   # Test blocked command (if not in whitelist)
   echo '{"id":"6","method":"call_tool","params":{"tool":"sudo","args":{"command":"whoami"}}}' | nc localhost 9000
   # Expected: Error - command not in whitelist
   ```

4. **Resource limits**:
   ```bash
   # Test CPU timeout (infinite loop)
   echo '{"id":"7","method":"call_tool","params":{"tool":"bash","args":{"command":"while true; do :; done"}}}' | nc localhost 9000
   # Expected: Killed after timeout, error returned
   
   # Test memory limit
   echo '{"id":"8","method":"call_tool","params":{"tool":"python3","args":{"code":"x=[1]*10**9"}}}' | nc localhost 9000
   # Expected: Killed on memory limit exceeded
   ```

5. **Audit logging**:
   ```bash
   # Execute several commands
   export USHELL_MCP_ENABLED=1
   export USHELL_MCP_AUDIT_LOG=/tmp/ushell_mcp_audit.log
   ./ushell
   
   # Run some tool calls, then check log
   cat /tmp/ushell_mcp_audit.log
   # Expected: JSON log entries with timestamps, commands, results
   ```

6. **Rate limiting**:
   ```bash
   # Rapid-fire commands
   for i in {1..15}; do
     echo '{"id":"'$i'","method":"call_tool","params":{"tool":"pwd","args":{}}}' | nc localhost 9000
   done
   # Expected: First 10 succeed, remaining get rate limit error
   ```

---

### Prompt 6: Add Progress Notifications and Status Updates

#### Prompt
```
Implement progress notifications for long-running operations:

1. Add notification system:
   - int mcp_send_notification(int client_fd, const char *event, const char *message)
     * Build notification JSON:
       {"id":null,"type":"notification","event":"<event>","message":"<message>"}
     * Send to client
     * Don't wait for response (fire and forget)
     * Return 0 on success
   
   - Notification events:
     * "tool_progress" - Command execution progress
     * "tool_started" - Command execution started
     * "tool_completed" - Command execution completed
     * "tool_failed" - Command execution failed

2. Integrate notifications with command execution:
   - Update mcp_execute_command() to send notifications:
     * Send "tool_started" before execution
     * Send "tool_progress" during execution (for long ops)
     * Send "tool_completed" after success
     * Send "tool_failed" on error
   
   - For commands with output streaming:
     * Send progress notification with partial output
     * Useful for commands like find, grep on large directories

3. Add execution status tracking:
   - typedef struct {
       char *id;              // Unique execution ID
       char *tool_name;       // Tool being executed
       int client_fd;         // Client connection
       pid_t child_pid;       // Child process ID
       time_t start_time;     // Execution start time
       int status;            // Current status (running, completed, failed)
     } MCPExecution;
   
   - Track active executions in global list
   - Allow clients to query execution status
   - Support cancellation of long-running operations

4. Implement execution query:
   - Add new MCP method: "get_execution_status"
     * Parameters: execution_id
     * Returns: status, elapsed_time, partial_output
   
   - Add new MCP method: "cancel_execution"
     * Parameters: execution_id
     * Action: Send SIGTERM to child process
     * Returns: success/failure

5. Add progress for specific operations:
   - Large directory listings (myls with many files):
     * Send notification every N files processed
   
   - File operations (mycp, mymv):
     * Send notification with bytes transferred
   
   - Search operations (grep patterns):
     * Send notification with matches found so far

6. Implement output streaming:
   - For long output commands:
     * Stream output in chunks as it's produced
     * Send each chunk as notification
     * Send final result when complete
   
   - Example: large find operation
     * Notification: {"event":"tool_progress","message":"Found 100 files..."}
     * Notification: {"event":"tool_progress","message":"Found 200 files..."}
     * Response: {"result":{"tool":"find","total_files":250}}
```

#### Manual Tests
1. **Basic progress notifications**:
   ```bash
   # Start shell and connect client
   export USHELL_MCP_ENABLED=1
   ./ushell
   
   # Terminal 2: Monitor notifications
   nc localhost 9000 << EOF
   {"id":"1","method":"call_tool","params":{"tool":"sleep","args":{"seconds":5}}}
   EOF
   # Expected: See tool_started, (progress), tool_completed notifications
   ```

2. **Progress during long operations**:
   ```bash
   # Create directory with many files
   mkdir -p /tmp/test_mcp && cd /tmp/test_mcp
   for i in {1..1000}; do touch file_$i.txt; done
   
   # Request listing
   echo '{"id":"2","method":"call_tool","params":{"tool":"ls","args":{"path":"/tmp/test_mcp"}}}' | nc localhost 9000
   # Expected: Progress notifications during listing
   ```

3. **Execution status query**:
   ```bash
   # Start long-running command
   echo '{"id":"3","method":"call_tool","params":{"tool":"sleep","args":{"seconds":30}}}' | nc localhost 9000 &
   sleep 2
   
   # Query status
   echo '{"id":"4","method":"get_execution_status","params":{"execution_id":"3"}}' | nc localhost 9000
   # Expected: Shows status as "running", elapsed time
   ```

4. **Execution cancellation**:
   ```bash
   # Start long command
   echo '{"id":"5","method":"call_tool","params":{"tool":"sleep","args":{"seconds":60}}}' | nc localhost 9000 &
   sleep 2
   
   # Cancel it
   echo '{"id":"6","method":"cancel_execution","params":{"execution_id":"5"}}' | nc localhost 9000
   # Expected: Command terminates, receives cancellation confirmation
   ```

5. **Output streaming**:
   ```bash
   # Command with gradual output
   echo '{"id":"7","method":"call_tool","params":{"tool":"find","args":{"path":"/usr","name":"*.txt"}}}' | nc localhost 9000
   # Expected: Receive output in chunks as find progresses
   ```

---

### Prompt 7: Integrate with Existing AI Helper

#### Prompt
```
Integrate MCP server with existing ushell_ai.py helper for enhanced context:

1. Add context provider to MCP server:
   - Create mcp_get_shell_context() function:
     * Current working directory
     * Current user and hostname  
     * Recently executed commands (from history)
     * Environment variables
     * Available commands from catalog
     * Return as JSON string
   
   - Expose via MCP tool: "get_shell_context"
     * Parameters: include_history (bool), include_env (bool)
     * Returns: complete shell context object

2. Add RAG integration:
   - Create mcp_search_commands() function:
     * Takes natural language query
     * Uses same scoring as ushell_ai.py
     * Returns relevant commands with descriptions
     * Helps AI agent discover appropriate tools
   
   - Expose via MCP tool: "search_commands"
     * Parameters: query (string), limit (int)
     * Returns: ranked list of relevant commands

3. Add command suggestion via MCP:
   - Create tool: "suggest_command"
     * Parameters: query (natural language)
     * Calls ushell_ai.py internally or reuses logic
     * Returns: suggested command with explanation
     * AI agent can use this for better command selection

4. Enhance list_tools with context:
   - Include usage examples in tool descriptions
   - Add "popular" flag for commonly used commands
   - Include parameter constraints and validation rules
   - Add command categories (file ops, system info, etc.)

5. Add bidirectional integration:
   - Allow @ AI helper to use MCP tools internally
   - Share command catalog between both systems
   - Unified configuration for both AI helper and MCP
   - Consistent behavior and command selection

6. Add AI agent hints:
   - Include in tool schemas:
     * Common use cases
     * Example queries that would use this tool
     * Related tools for chaining operations
     * Safety level (safe, caution, dangerous)
   
   - Example enhanced tool schema:
     {
       "name": "rm",
       "description": "Remove files or directories",
       "inputSchema": {...},
       "metadata": {
         "safety": "caution",
         "examples": ["delete temporary files", "remove old logs"],
         "related_tools": ["ls", "find"],
         "category": "file_operations"
       }
     }
```

#### Manual Tests
1. **Shell context retrieval**:
   ```bash
   export USHELL_MCP_ENABLED=1
   ./ushell
   # Type some commands first
   pwd
   ls
   echo "test"
   
   # Request context via MCP
   echo '{"id":"1","method":"call_tool","params":{"tool":"get_shell_context","args":{"include_history":true}}}' | nc localhost 9000 | python3 -m json.tool
   # Expected: Shows current directory, recent commands, etc.
   ```

2. **Command search via MCP**:
   ```bash
   # Search for file-related commands
   echo '{"id":"2","method":"call_tool","params":{"tool":"search_commands","args":{"query":"list files recursively","limit":5}}}' | nc localhost 9000 | python3 -m json.tool
   # Expected: Returns ls, myls, find with descriptions and scores
   ```

3. **Command suggestion via MCP**:
   ```bash
   # Get suggestion
   echo '{"id":"3","method":"call_tool","params":{"tool":"suggest_command","args":{"query":"show all text files in current directory"}}}' | nc localhost 9000
   # Expected: Returns suggested command like "ls *.txt" or "find . -name '*.txt'"
   ```

4. **Enhanced tool metadata**:
   ```bash
   # Request tools with full metadata
   echo '{"id":"4","method":"list_tools","params":{}}' | nc localhost 9000 | python3 -c "import json,sys; tools=json.load(sys.stdin)['result']['tools']; [print(f\"{t['name']}: {t.get('metadata',{}).get('category','none')}\") for t in tools[:10]]"
   # Expected: Shows tools with their categories
   ```

5. **Integrated behavior test**:
   ```bash
   export USHELL_MCP_ENABLED=1
   ./ushell
   
   # Test @ helper
   @list all python files
   # Expected: Suggests command
   
   # Test same via MCP
   echo '{"id":"5","method":"call_tool","params":{"tool":"suggest_command","args":{"query":"list all python files"}}}' | nc localhost 9000
   # Expected: Same or similar suggestion
   ```

---

### Prompt 8: Add Python Client Example and Documentation

#### Prompt
```
Create Python client example and comprehensive documentation:

1. Create examples/mcp_client_example.py:
   - Simple standalone Python client
   - Demonstrates all MCP operations:
     * Connect to server
     * Initialize connection
     * List available tools
     * Call specific tools with arguments
     * Handle responses and errors
     * Receive progress notifications
   
   - Include example workflows:
     * Basic file operations
     * Command chaining (multiple tool calls)
     * Natural language query to command execution
     * Error handling and retry logic
   
   - Add docstrings and inline comments
   - Make it executable and educational

2. Create examples/mcp_ai_agent_example.py:
   - More advanced agent example
   - Mock LLM decision-making (no API key needed)
   - Demonstrates:
     * Tool discovery via list_tools
     * Intelligent tool selection based on query
     * Argument extraction from natural language
     * Execution and result interpretation
     * Multi-step operations
   
   - Include mocked scenarios:
     * "Find all Python files and show their sizes"
     * "What's in my home directory?"
     * "Delete files older than 30 days in /tmp"

3. Create docs/MCP_INTEGRATION.md:
   - Overview of MCP integration
   - Architecture diagram (ASCII art)
   - Protocol specification:
     * Message format
     * Supported methods
     * Request/response examples
   
   - Tool catalog documentation:
     * How tools are generated from commands.json
     * Schema format and conventions
     * Custom tools beyond catalog
   
   - Security considerations:
     * Command whitelist/blacklist
     * Argument sanitization
     * Resource limits
     * Audit logging
   
   - Configuration:
     * Environment variables
     * Default values
     * Security settings
   
   - Integration with external AI:
     * OpenAI agents example
     * Custom agent integration
     * Authentication (future)

4. Update README.md:
   - Add MCP server section
   - Quick start guide:
     * Enable MCP server
     * Test with example client
     * Connect external AI agent
   
   - Add troubleshooting section:
     * Port already in use
     * Connection refused
     * Permission denied
     * Command execution errors

5. Create tests/test_mcp_server.sh:
   - Automated test script for MCP functionality
   - Tests all major operations:
     * Server startup/shutdown
     * Connection handling
     * Each MCP method
     * Error conditions
     * Concurrent clients
   
   - Reports pass/fail for each test
   - Can be run as part of CI/CD

6. Add to examples/tutorial.txt:
   - Interactive tutorial for MCP features
   - Step-by-step walkthrough
   - Practice exercises
   - Expected outputs
```

#### Manual Tests
1. **Python client example**:
   ```bash
   # Ensure MCP server is running
   export USHELL_MCP_ENABLED=1
   ./ushell &
   
   # Run example client
   python3 examples/mcp_client_example.py
   # Expected: Shows successful connection, lists tools, executes command
   ```

2. **AI agent example**:
   ```bash
   # Run with various queries
   python3 examples/mcp_ai_agent_example.py "Find all text files in current directory"
   # Expected: Shows tool selection logic, executes find/ls, displays results
   
   python3 examples/mcp_ai_agent_example.py "What's the current time?"
   # Expected: Uses appropriate tool (date command or get_time)
   ```

3. **Documentation accuracy**:
   ```bash
   # Follow quick start guide from docs
   cat docs/MCP_INTEGRATION.md
   # Follow the steps manually
   # Expected: All examples work as documented
   ```

4. **Automated tests**:
   ```bash
   # Run test suite
   ./tests/test_mcp_server.sh
   # Expected: All tests pass
   # Shows: [PASS] or [FAIL] for each test
   ```

5. **Integration test**:
   ```bash
   # Complete end-to-end scenario
   export USHELL_MCP_ENABLED=1
   ./ushell &
   sleep 2
   
   # Run external AI agent example from notebook
   python3 -c "
   import socket, json
   s = socket.socket()
   s.connect(('localhost', 9000))
   def send(obj): s.send((json.dumps(obj)+'\n').encode())
   def recv(): return json.loads(s.recv(4096).decode())
   send({'id':'1','method':'initialize','params':{}})
   print('Init:', recv())
   send({'id':'2','method':'list_tools','params':{}})
   print('Tools:', len(recv()['result']['tools']))
   send({'id':'3','method':'call_tool','params':{'tool':'pwd','args':{}}})
   print('PWD:', recv()['result']['output'])
   s.close()
   "
   # Expected: Successful three-way handshake and command execution
   ```

---

## Integration with Existing AI Features

### Current @ AI Helper Integration
The shell's existing `ushell_ai.py` integration provides:
- Natural language to command translation
- RAG-like command selection from catalog
- Optional LLM enhancement via OpenAI
- Commands triggered by @ prefix

### MCP Server Integration
The new MCP server provides:
- Network-accessible tool discovery and execution
- Standard protocol for external AI agents
- Richer context and metadata for AI decision making
- Background server operation

### Coexistence Strategy
Both features will coexist and complement each other:

1. **Shared Command Catalog**:
   - Both use `commands.json` as source of truth
   - Consistent command information
   - Single point of maintenance

2. **Complementary Use Cases**:
   - @ helper: Quick local AI suggestions
   - MCP server: External agent integration
   - Both can be enabled simultaneously

3. **Unified Configuration**:
   - USHELL_AI_* variables for AI helper
   - USHELL_MCP_* variables for MCP server
   - Shared: USHELL_CATALOG_FILE

4. **Optional Integration**:
   - MCP "suggest_command" tool can call ushell_ai.py
   - AI helper could query MCP server for enhanced context
   - Future: unified AI backend

### Migration Path
No breaking changes to existing functionality:
- Existing @ AI helper continues to work
- MCP server is opt-in via USHELL_MCP_ENABLED
- Can use either, both, or neither
- Independent enable/disable

---

## Environment Variables Summary

### MCP Server Configuration
- `USHELL_MCP_ENABLED` (0/1, default: 0)
  Enable or disable MCP server
  
- `USHELL_MCP_PORT` (int, default: 9000)
  TCP port for MCP server to listen on
  
- `USHELL_MCP_DEBUG` (0/1, default: 0)
  Enable debug logging for MCP operations
  
- `USHELL_MCP_AUDIT_LOG` (path, default: none)
  Path to audit log file for command tracking
  
- `USHELL_MCP_MAX_CLIENTS` (int, default: 10)
  Maximum concurrent client connections
  
- `USHELL_MCP_TIMEOUT` (int, default: 60)
  Idle connection timeout in seconds
  
- `USHELL_MCP_CMD_TIMEOUT` (int, default: 30)
  Command execution timeout in seconds
  
- `USHELL_MCP_RATE_LIMIT` (int, default: 10)
  Commands per client per 10 seconds

### Existing AI Helper Configuration (unchanged)
- `USHELL_CATALOG_CMD` (command, default: "ushell --commands-json")
- `USHELL_CATALOG_FILE` (path, default: "commands.json")
- `USHELL_AI_DEBUG` (0/1, default: 0)
- `OPENAI_API_KEY` (string, optional)
- `USHELL_LLM_MODEL` (string, default: "gpt-4o-mini")

---

## Security Considerations

### Command Whitelisting
- Only commands in catalog are executable
- Dangerous commands excluded by default
- Configurable whitelist via config file
- Pattern matching for dangerous flag combinations

### Argument Sanitization
- Remove shell metacharacters from all arguments
- Validate paths against directory traversal
- Type checking for integer/boolean parameters
- Length limits to prevent buffer overflow

### Resource Limits
- CPU time limit per command
- Memory limit per child process
- Maximum output size to prevent DoS
- Process count limit
- File descriptor limit

### Network Security
- Listen only on localhost by default
- Optional: add authentication token
- Optional: add TLS support
- Rate limiting per client
- Connection limits

### Audit Logging
- Log all command executions
- Include client info, timestamp, arguments
- Separate log file for security review
- Structured JSON format for parsing

### Recommendations for Production
- Enable audit logging
- Review and customize whitelist
- Add authentication mechanism
- Use TLS for encrypted communication
- Run server in restricted user context
- Monitor logs for suspicious activity
- Set strict resource limits
- Use firewall rules to restrict access

---

## Testing Strategy

### Unit Tests
- Test each MCP message handler independently
- Test sanitization functions with malicious input
- Test JSON parsing with malformed data
- Test resource limit enforcement
- Test catalog loading and tool generation

### Integration Tests
- Test full client-server workflow
- Test concurrent client connections
- Test command execution end-to-end
- Test error propagation and handling
- Test notification delivery

### Security Tests
- Test command injection attempts
- Test directory traversal attempts
- Test resource exhaustion (DoS)
- Test rate limiting effectiveness
- Test whitelist enforcement

### Performance Tests
- Test with high command frequency
- Test with large output commands
- Test with many concurrent clients
- Measure latency and throughput
- Profile memory usage

### Compatibility Tests
- Test with various Python versions
- Test with different JSON libraries
- Test with OpenAI agent integration
- Test with existing @ AI helper
- Test backward compatibility

---

## Future Enhancements

### Potential Additions
1. **WebSocket Support**:
   - Alternative to TCP for browser clients
   - Bidirectional streaming
   - Better for web-based agents

2. **Authentication**:
   - Token-based auth
   - OAuth integration
   - API key management

3. **TLS/SSL**:
   - Encrypted communication
   - Certificate management
   - Secure by default

4. **Advanced Tools**:
   - File upload/download via MCP
   - Interactive command sessions
   - Shell script execution
   - Environment manipulation

5. **Monitoring Dashboard**:
   - Web UI for server status
   - Active connections display
   - Command history viewer
   - Performance metrics

6. **Plugin System**:
   - Custom tool registration
   - External tool providers
   - Dynamic catalog updates
   - Tool versioning

7. **Multi-Shell Support**:
   - Control multiple shell instances
   - Session management
   - Workspace isolation

8. **Advanced AI Integration**:
   - Built-in LLM support
   - Embeddings for better RAG
   - Learning from user corrections
   - Personalized suggestions

---

## Troubleshooting Guide

### Server Won't Start

**Symptom**: MCP server fails to start, port binding error

**Solutions**:
```bash
# Check if port is already in use
netstat -tlnp | grep 9000

# Kill existing process
kill $(lsof -t -i:9000)

# Or use different port
export USHELL_MCP_PORT=9001
```

### Connection Refused

**Symptom**: Client cannot connect to server

**Solutions**:
```bash
# Verify server is running
ps aux | grep ushell

# Check if server is listening
netstat -tlnp | grep 9000

# Check firewall rules
sudo iptables -L | grep 9000

# Try localhost vs 127.0.0.1
nc localhost 9000  # vs
nc 127.0.0.1 9000
```

### Command Execution Fails

**Symptom**: Tool calls return errors

**Solutions**:
```bash
# Check command whitelist
export USHELL_MCP_DEBUG=1

# Verify command exists in catalog
cat unified-shell/aiIntegr/commands.json | grep "command_name"

# Check permissions
ls -la /path/to/command

# Review audit log
cat /tmp/ushell_mcp_audit.log | tail -20
```

### Performance Issues

**Symptom**: Slow command execution, high latency

**Solutions**:
```bash
# Check system resources
top
htop

# Review concurrent connections
netstat -an | grep 9000 | wc -l

# Adjust resource limits
export USHELL_MCP_CMD_TIMEOUT=60
export USHELL_MCP_MAX_CLIENTS=5

# Enable profiling
export USHELL_MCP_DEBUG=1
```

### Memory Leaks

**Symptom**: Shell memory usage grows over time

**Solutions**:
```bash
# Monitor memory
watch -n 1 'ps aux | grep ushell'

# Use valgrind for leak detection
valgrind --leak-check=full ./ushell

# Check for zombie processes
ps aux | grep defunct

# Restart server periodically (temporary fix)
```

---

## Success Criteria

### Functional Requirements
- [ ] MCP server starts on configured port
- [ ] Server accepts multiple concurrent connections
- [ ] Initialize method returns server info
- [ ] List_tools method returns full tool catalog
- [ ] Call_tool executes commands and returns output
- [ ] Progress notifications sent for long operations
- [ ] Error responses for invalid requests
- [ ] Server stops cleanly on shell exit

### Security Requirements
- [ ] Command whitelist enforced
- [ ] All arguments sanitized
- [ ] Resource limits prevent DoS
- [ ] Path traversal attempts blocked
- [ ] Audit logging captures all commands
- [ ] Rate limiting prevents abuse
- [ ] No shell injection vulnerabilities
- [ ] Memory leaks prevented

### Integration Requirements
- [ ] Works alongside existing @ AI helper
- [ ] Shares command catalog with AI helper
- [ ] No breaking changes to existing features
- [ ] Python client examples work correctly
- [ ] External AI agents can connect
- [ ] OpenAI integration example works
- [ ] Documentation is complete and accurate

### Performance Requirements
- [ ] Command execution latency < 100ms overhead
- [ ] Supports 10+ concurrent clients
- [ ] Handles 100+ commands/minute per client
- [ ] Memory usage stable over extended use
- [ ] No noticeable shell slowdown
- [ ] Fast tool catalog loading

### Documentation Requirements
- [ ] Complete MCP protocol documentation
- [ ] Python client examples included
- [ ] Security guide written
- [ ] Troubleshooting guide complete
- [ ] README updated with MCP section
- [ ] Integration guide for AI agents
- [ ] Code comments thorough
- [ ] Architecture documented

---

## Conclusion

This MCP integration will transform the unified shell into a powerful AI-accessible tool platform. External AI agents can discover shell capabilities, execute commands safely, and receive rich context for intelligent automation. The integration maintains backward compatibility while adding cutting-edge AI agent functionality.

The implementation follows the same structured approach as threadPrompts.md, with clear step-by-step prompts, comprehensive tests, and detailed documentation. Each prompt builds on the previous one, creating a complete and secure MCP server implementation.

[END OF MCP_PROMPTS.MD]
