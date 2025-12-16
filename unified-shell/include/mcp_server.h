#ifndef MCP_SERVER_H
#define MCP_SERVER_H

#include <pthread.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include "environment.h"

/*
 * MCP Server - Model Context Protocol Server for Unified Shell
 * 
 * This module implements a TCP-based MCP server that allows external AI agents
 * to discover and execute shell operations via a JSON protocol.
 * 
 * Protocol: Newline-delimited JSON over TCP
 * Default Port: 9000
 * Methods: initialize, list_tools, call_tool, get_execution_status, cancel_execution
 */

/* Maximum buffer sizes for MCP operations */
#define MCP_BUFFER_SIZE 16384
#define MCP_MAX_CLIENTS 10
#define MCP_DEFAULT_PORT 9000
#define MCP_CMD_TIMEOUT 30
#define MCP_RATE_LIMIT 10

/*
 * MCPRequest - Represents an incoming MCP request
 * 
 * Fields:
 *   id: Request identifier for correlation (allocated string)
 *   method: Method name (initialize, list_tools, call_tool)
 *   params: JSON string containing method parameters
 */
typedef struct {
    char *id;                    /* Request ID (for correlation) */
    char *method;                /* Method name (initialize, list_tools, call_tool) */
    char *params;                /* JSON parameters string */
} MCPRequest;

/*
 * MCPResponse - Represents an outgoing MCP response
 * 
 * Fields:
 *   id: Matching request ID for correlation
 *   type: Response type (response, error, notification)
 *   result: JSON string containing response data or error
 */
typedef struct {
    char *id;                    /* Matching request ID */
    char *type;                  /* response, error, notification */
    char *result;                /* JSON result string */
} MCPResponse;

/*
 * MCPServerConfig - MCP server configuration and state
 * 
 * Fields:
 *   port: TCP port number (default 9000)
 *   enabled: Server enabled flag (0=disabled, 1=enabled)
 *   sockfd: Server socket file descriptor
 *   server_thread: Thread handle for server accept loop
 *   lock: Mutex for thread-safe operations
 *   running: Flag indicating server is running
 *   env: Pointer to shell environment
 *   active_clients: Current number of active client connections
 */
typedef struct {
    int port;                    /* TCP port (default 9000) */
    int enabled;                 /* 0=disabled, 1=enabled */
    int sockfd;                  /* Server socket descriptor */
    pthread_t server_thread;     /* MCP server thread */
    pthread_mutex_t lock;        /* Mutex for thread safety */
    int running;                 /* Server running flag */
    Env *env;                    /* Shell environment pointer */
    int active_clients;          /* Number of active client connections */
} MCPServerConfig;

/*
 * MCPExecution - Track ongoing command execution
 * 
 * Used for status queries and cancellation of long-running operations
 */
typedef struct {
    char *id;                    /* Unique execution ID */
    char *tool_name;             /* Tool being executed */
    int client_fd;               /* Client connection */
    pid_t child_pid;             /* Child process ID */
    time_t start_time;           /* Execution start time */
    int status;                  /* Current status (0=running, 1=completed, 2=failed) */
} MCPExecution;

/* 
 * Server Lifecycle Functions
 */

/*
 * mcp_server_create - Allocate and initialize MCP server configuration
 * 
 * Parameters:
 *   port: TCP port to listen on (use MCP_DEFAULT_PORT for default)
 *   env: Pointer to shell environment
 * 
 * Returns:
 *   Pointer to initialized MCPServerConfig, or NULL on failure
 */
MCPServerConfig* mcp_server_create(int port, Env *env);

/*
 * mcp_server_destroy - Stop server and free all resources
 * 
 * Parameters:
 *   config: Server configuration to destroy
 * 
 * Note: This function calls mcp_server_stop() if server is running
 */
void mcp_server_destroy(MCPServerConfig *config);

/*
 * mcp_server_start - Create socket, bind, listen, and start server thread
 * 
 * Parameters:
 *   config: Server configuration
 * 
 * Returns:
 *   0 on success, -1 on failure
 */
int mcp_server_start(MCPServerConfig *config);

/*
 * mcp_server_stop - Stop server thread and close socket
 * 
 * Parameters:
 *   config: Server configuration
 */
void mcp_server_stop(MCPServerConfig *config);

/*
 * Server Thread Functions
 */

/*
 * mcp_server_thread - Thread entry point for MCP server (accept loop)
 * 
 * Parameters:
 *   arg: Pointer to MCPServerConfig
 * 
 * Returns:
 *   NULL (pthread convention)
 */
void* mcp_server_thread(void *arg);

/*
 * mcp_client_handler - Handle individual client connection
 * 
 * Parameters:
 *   arg: Pointer to structure containing client_fd and env
 * 
 * Returns:
 *   NULL (pthread convention)
 */
void* mcp_client_handler(void *arg);

/*
 * Message Handling Functions
 */

/*
 * mcp_send_message - Send newline-delimited JSON message to client
 * 
 * Parameters:
 *   client_fd: Client socket file descriptor
 *   json: JSON string to send (newline will be appended)
 * 
 * Returns:
 *   0 on success, -1 on failure
 */
int mcp_send_message(int client_fd, const char *json);

/*
 * mcp_recv_message - Receive newline-delimited JSON message from client
 * 
 * Parameters:
 *   client_fd: Client socket file descriptor
 *   buffer: Buffer to store received message
 *   bufsize: Size of buffer
 * 
 * Returns:
 *   Number of bytes received (excluding newline), -1 on error, 0 on disconnect
 */
int mcp_recv_message(int client_fd, char *buffer, size_t bufsize);

/*
 * mcp_parse_request - Parse JSON request into MCPRequest structure
 * 
 * Parameters:
 *   json: JSON string to parse
 *   req: Pointer to MCPRequest to populate (allocates strings internally)
 * 
 * Returns:
 *   0 on success, -1 on parse error
 * 
 * Note: Caller must free req->id, req->method, req->params when done
 */
int mcp_parse_request(const char *json, MCPRequest *req);

/*
 * mcp_build_response - Build JSON response string from MCPResponse structure
 * 
 * Parameters:
 *   resp: Response structure to convert to JSON
 * 
 * Returns:
 *   Dynamically allocated JSON string, or NULL on failure
 * 
 * Note: Caller must free returned string
 */
char* mcp_build_response(MCPResponse *resp);

/*
 * mcp_free_request - Free allocated memory in MCPRequest
 * 
 * Parameters:
 *   req: Request structure to free
 */
void mcp_free_request(MCPRequest *req);

/*
 * Protocol Handler Functions
 */

/*
 * mcp_handle_initialize - Handle initialize method
 * 
 * Returns server information and version
 */
int mcp_handle_initialize(int client_fd, MCPRequest *req, Env *env);

/*
 * mcp_handle_list_tools - Handle list_tools method
 * 
 * Returns array of available tools with schemas
 */
int mcp_handle_list_tools(int client_fd, MCPRequest *req, Env *env);

/*
 * mcp_handle_call_tool - Handle call_tool method
 * 
 * Executes specified tool with arguments and returns output
 */
int mcp_handle_call_tool(int client_fd, MCPRequest *req, Env *env);

/*
 * mcp_handle_get_execution_status - Handle get_execution_status method
 * 
 * Query status of a running or completed execution
 * Parameters in params: execution_id (string)
 * Returns: execution_id, tool, status, elapsed_time, pid
 */
int mcp_handle_get_execution_status(int client_fd, MCPRequest *req, Env *env);

/*
 * mcp_handle_cancel_execution - Handle cancel_execution method
 * 
 * Send SIGTERM to child process to cancel execution
 * Parameters in params: execution_id (string)
 * Returns: status (cancelled or error)
 */
int mcp_handle_cancel_execution(int client_fd, MCPRequest *req, Env *env);

/*
 * mcp_handle_request - Route request to appropriate handler
 * 
 * Parameters:
 *   client_fd: Client socket
 *   json_req: JSON request string
 *   env: Shell environment
 * 
 * Returns:
 *   0 on success, -1 on error
 */
int mcp_handle_request(int client_fd, const char *json_req, Env *env);

/*
 * Notification Functions
 */

/*
 * mcp_send_notification - Send notification message to client
 * 
 * Parameters:
 *   client_fd: Client socket
 *   event: Event type (tool_progress, tool_started, tool_completed, tool_failed)
 *   message: Notification message
 * 
 * Returns:
 *   0 on success, -1 on failure
 */
int mcp_send_notification(int client_fd, const char *event, const char *message);

#endif /* MCP_SERVER_H */
