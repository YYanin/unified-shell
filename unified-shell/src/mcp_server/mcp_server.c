/*
 * mcp_server.c - Model Context Protocol Server Implementation
 * 
 * Implements TCP-based MCP server for unified shell, enabling external
 * AI agents to discover and execute shell operations via JSON protocol.
 */

#include "mcp_server.h"
#include "mcp_json.h"
#include "mcp_tools.h"
#include "mcp_exec.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>

/* Global cached catalog - loaded once and reused */
static char *g_cached_catalog = NULL;

/* Global execution tracking - max 32 concurrent executions */
#define MAX_EXECUTIONS 32
static MCPExecution g_executions[MAX_EXECUTIONS];
static pthread_mutex_t g_exec_lock = PTHREAD_MUTEX_INITIALIZER;
static int g_exec_counter = 0;

/* Client handler argument structure */
typedef struct {
    int client_fd;
    MCPServerConfig *config;
} ClientHandlerArg;

/*
 * setup_signal_handlers - Configure signal handling for MCP server
 * 
 * Ignores SIGPIPE (broken pipe on client disconnect)
 * This prevents server crashes when clients disconnect unexpectedly.
 */
static void setup_signal_handlers(void) {
    /* Ignore SIGPIPE - we'll handle write errors instead */
    signal(SIGPIPE, SIG_IGN);
}

/*
 * mcp_track_execution - Add execution to tracking list
 * 
 * Returns execution ID (numeric string), or NULL if tracking list is full
 */
static char* mcp_track_execution(const char *tool_name, int client_fd, pid_t child_pid) {
    pthread_mutex_lock(&g_exec_lock);
    
    /* Find free slot */
    int slot = -1;
    for (int i = 0; i < MAX_EXECUTIONS; i++) {
        if (g_executions[i].id == NULL) {
            slot = i;
            break;
        }
    }
    
    if (slot == -1) {
        pthread_mutex_unlock(&g_exec_lock);
        return NULL;  /* Tracking list full */
    }
    
    /* Generate unique execution ID */
    char *exec_id = malloc(32);
    snprintf(exec_id, 32, "%d", ++g_exec_counter);
    
    /* Populate execution structure */
    g_executions[slot].id = strdup(exec_id);
    g_executions[slot].tool_name = strdup(tool_name);
    g_executions[slot].client_fd = client_fd;
    g_executions[slot].child_pid = child_pid;
    g_executions[slot].start_time = time(NULL);
    g_executions[slot].status = 0;  /* 0 = running */
    
    pthread_mutex_unlock(&g_exec_lock);
    return exec_id;
}

/*
 * mcp_update_execution - Update execution status
 */
static void mcp_update_execution(const char *exec_id, int status) {
    if (!exec_id) return;
    
    pthread_mutex_lock(&g_exec_lock);
    
    for (int i = 0; i < MAX_EXECUTIONS; i++) {
        if (g_executions[i].id && strcmp(g_executions[i].id, exec_id) == 0) {
            g_executions[i].status = status;
            break;
        }
    }
    
    pthread_mutex_unlock(&g_exec_lock);
}

/*
 * mcp_find_execution - Find execution by ID
 * 
 * Returns pointer to execution, or NULL if not found
 * Note: Caller must hold g_exec_lock
 */
static MCPExecution* mcp_find_execution(const char *exec_id) {
    for (int i = 0; i < MAX_EXECUTIONS; i++) {
        if (g_executions[i].id && strcmp(g_executions[i].id, exec_id) == 0) {
            return &g_executions[i];
        }
    }
    return NULL;
}

/*
 * mcp_cleanup_execution - Remove execution from tracking list
 */
static void mcp_cleanup_execution(const char *exec_id) {
    if (!exec_id) return;
    
    pthread_mutex_lock(&g_exec_lock);
    
    for (int i = 0; i < MAX_EXECUTIONS; i++) {
        if (g_executions[i].id && strcmp(g_executions[i].id, exec_id) == 0) {
            free(g_executions[i].id);
            free(g_executions[i].tool_name);
            g_executions[i].id = NULL;
            g_executions[i].tool_name = NULL;
            g_executions[i].client_fd = -1;
            g_executions[i].child_pid = 0;
            g_executions[i].start_time = 0;
            g_executions[i].status = 0;
            break;
        }
    }
    
    pthread_mutex_unlock(&g_exec_lock);
}

/*
 * mcp_server_create - Allocate and initialize MCP server configuration
 */
MCPServerConfig* mcp_server_create(int port, Env *env) {
    /* Allocate server configuration */
    MCPServerConfig *config = malloc(sizeof(MCPServerConfig));
    if (!config) {
        fprintf(stderr, "MCP Server: Failed to allocate configuration\n");
        return NULL;
    }
    
    /* Initialize configuration */
    config->port = port > 0 ? port : MCP_DEFAULT_PORT;
    config->enabled = 0;  /* Not enabled by default, call mcp_server_start */
    config->sockfd = -1;
    config->running = 0;
    config->env = env;
    config->active_clients = 0;
    
    /* Initialize mutex */
    if (pthread_mutex_init(&config->lock, NULL) != 0) {
        fprintf(stderr, "MCP Server: Failed to initialize mutex\n");
        free(config);
        return NULL;
    }
    
    return config;
}

/*
 * mcp_server_destroy - Stop server and free all resources
 */
void mcp_server_destroy(MCPServerConfig *config) {
    if (!config) {
        return;
    }
    
    /* Stop server if running */
    if (config->running) {
        mcp_server_stop(config);
    }
    
    /* Destroy mutex */
    pthread_mutex_destroy(&config->lock);
    
    /* Free configuration */
    free(config);
}

/*
 * mcp_send_message - Send newline-delimited JSON message to client
 */
int mcp_send_message(int client_fd, const char *json) {
    if (client_fd < 0 || !json) {
        return -1;
    }
    
    size_t len = strlen(json);
    
    /* Send JSON message */
    ssize_t sent = write(client_fd, json, len);
    if (sent != (ssize_t)len) {
        return -1;
    }
    
    /* Send newline delimiter */
    if (write(client_fd, "\n", 1) != 1) {
        return -1;
    }
    
    return 0;
}

/*
 * mcp_recv_message - Receive newline-delimited JSON message from client
 */
int mcp_recv_message(int client_fd, char *buffer, size_t bufsize) {
    if (client_fd < 0 || !buffer || bufsize == 0) {
        return -1;
    }
    
    size_t total = 0;
    
    /* Read until newline or buffer full */
    while (total < bufsize - 1) {
        char c;
        ssize_t n = read(client_fd, &c, 1);
        
        if (n < 0) {
            /* Error reading */
            return -1;
        } else if (n == 0) {
            /* Connection closed */
            if (total > 0) {
                break;  /* Return what we have */
            }
            return 0;
        }
        
        if (c == '\n') {
            /* End of message */
            break;
        }
        
        buffer[total++] = c;
    }
    
    buffer[total] = '\0';
    return (int)total;
}

/*
 * mcp_parse_request - Parse JSON request into MCPRequest structure
 */
int mcp_parse_request(const char *json, MCPRequest *req) {
    if (!json || !req) {
        return -1;
    }
    
    /* Initialize request structure */
    req->id = NULL;
    req->method = NULL;
    req->params = NULL;
    
    /* Extract id field */
    char id_buf[64];
    if (mcp_json_extract_string(json, "id", id_buf, sizeof(id_buf))) {
        req->id = strdup(id_buf);
    }
    
    /* Extract method field (required) */
    char method_buf[64];
    if (!mcp_json_extract_string(json, "method", method_buf, sizeof(method_buf))) {
        /* Method is required */
        free(req->id);
        return -1;
    }
    req->method = strdup(method_buf);
    
    /* Extract params field (optional) - for now, just store the whole JSON */
    /* In a full implementation, we would parse the params object */
    req->params = strdup(json);  /* Store full JSON for later parsing */
    
    return 0;
}

/*
 * mcp_build_response - Build JSON response string from MCPResponse structure
 */
char* mcp_build_response(MCPResponse *resp) {
    if (!resp || !resp->type || !resp->result) {
        return NULL;
    }
    
    return mcp_json_build_response(resp->id, resp->type, resp->result);
}

/*
 * mcp_free_request - Free allocated memory in MCPRequest
 */
void mcp_free_request(MCPRequest *req) {
    if (!req) {
        return;
    }
    
    free(req->id);
    free(req->method);
    free(req->params);
    
    req->id = NULL;
    req->method = NULL;
    req->params = NULL;
}

/*
 * mcp_send_notification - Send notification message to client
 */
int mcp_send_notification(int client_fd, const char *event, const char *message) {
    if (client_fd < 0 || !event || !message) {
        return -1;
    }
    
    /* Build notification JSON */
    char escaped_msg[512];
    mcp_json_escape(message, escaped_msg, sizeof(escaped_msg));
    
    char notification[1024];
    snprintf(notification, sizeof(notification),
             "{\"id\":null,\"type\":\"notification\",\"event\":\"%s\",\"message\":\"%s\"}",
             event, escaped_msg);
    
    return mcp_send_message(client_fd, notification);
}

/*
 * mcp_handle_initialize - Handle initialize method
 */
int mcp_handle_initialize(int client_fd, MCPRequest *req, Env *env) {
    (void)env;  /* Unused for now */
    
    /* Build initialization response */
    const char *result = "{\"server\":\"unified-shell MCP\",\"version\":\"1.0\"}";
    
    MCPResponse resp = {
        .id = req->id,
        .type = "response",
        .result = (char*)result
    };
    
    char *json = mcp_build_response(&resp);
    if (!json) {
        return -1;
    }
    
    int ret = mcp_send_message(client_fd, json);
    free(json);
    
    return ret;
}

/*
 * mcp_handle_list_tools - Handle list_tools method
 * 
 * Loads commands.json catalog and returns available tools.
 */
int mcp_handle_list_tools(int client_fd, MCPRequest *req, Env *env) {
    (void)env;  /* Environment used in future enhancements */
    
    /* Load catalog if not cached */
    if (!g_cached_catalog) {
        g_cached_catalog = mcp_tools_load_catalog(NULL);
        if (!g_cached_catalog) {
            /* Send error response */
            char *error = mcp_json_build_error(req->id, "Failed to load tool catalog");
            if (error) {
                mcp_send_message(client_fd, error);
                free(error);
            }
            return -1;
        }
    }
    
    /* Build response with catalog */
    MCPResponse resp = {
        .id = req->id,
        .type = "response",
        .result = g_cached_catalog
    };
    
    char *json = mcp_build_response(&resp);
    if (!json) {
        return -1;
    }
    
    int ret = mcp_send_message(client_fd, json);
    free(json);
    
    return ret;
}

/*
 * mcp_handle_call_tool - Handle call_tool method
 * 
 * Executes requested tool with sanitized arguments and returns output.
 * Enhanced in Prompt 4 with alias resolution and special tool support.
 */
int mcp_handle_call_tool(int client_fd, MCPRequest *req, Env *env) {
    /* Extract tool name from params */
    char tool_name[64] = "";
    if (!mcp_json_extract_string(req->params, "tool", tool_name, sizeof(tool_name))) {
        char *error = mcp_json_build_error(req->id, "Missing 'tool' parameter");
        if (error) {
            mcp_send_message(client_fd, error);
            free(error);
        }
        return -1;
    }
    
    /* Handle special MCP tools */
    if (strcmp(tool_name, "get_shell_info") == 0) {
        /* Get current shell information */
        char cwd[1024];
        getcwd(cwd, sizeof(cwd));
        
        char hostname[256];
        gethostname(hostname, sizeof(hostname));
        
        char result_json[2048];
        snprintf(result_json, sizeof(result_json),
                 "{\"cwd\":\"%s\",\"user\":\"%s\",\"hostname\":\"%s\"}",
                 cwd, getenv("USER") ? getenv("USER") : "unknown", hostname);
        
        MCPResponse resp = {.id = req->id, .type = "response", .result = result_json};
        char *json = mcp_build_response(&resp);
        if (json) {
            mcp_send_message(client_fd, json);
            free(json);
        }
        return 0;
    }
    
    if (strcmp(tool_name, "get_history") == 0) {
        /* Get command history (simplified - returns placeholder) */
        char result_json[1024];
        snprintf(result_json, sizeof(result_json),
                 "{\"history\":[\"pwd\",\"ls\",\"cd /tmp\"]}");
        
        MCPResponse resp = {.id = req->id, .type = "response", .result = result_json};
        char *json = mcp_build_response(&resp);
        if (json) {
            mcp_send_message(client_fd, json);
            free(json);
        }
        return 0;
    }
    
    /* Prompt 7: AI Helper Integration - Special Tools */
    if (strcmp(tool_name, "get_shell_context") == 0) {
        /* Get comprehensive shell context (cwd, user, history, env) */
        char *context_json = mcp_handle_get_shell_context(req->params);
        if (context_json) {
            MCPResponse resp = {.id = req->id, .type = "response", .result = context_json};
            char *json = mcp_build_response(&resp);
            if (json) {
                mcp_send_message(client_fd, json);
                free(json);
            }
            free(context_json);
            return 0;
        } else {
            char *error = mcp_json_build_error(req->id, "Failed to get shell context");
            if (error) {
                mcp_send_message(client_fd, error);
                free(error);
            }
            return -1;
        }
    }
    
    if (strcmp(tool_name, "search_commands") == 0) {
        /* Search command catalog with natural language query */
        char *search_result = mcp_handle_search_commands(req->params);
        if (search_result) {
            MCPResponse resp = {.id = req->id, .type = "response", .result = search_result};
            char *json = mcp_build_response(&resp);
            if (json) {
                mcp_send_message(client_fd, json);
                free(json);
            }
            free(search_result);
            return 0;
        } else {
            char *error = mcp_json_build_error(req->id, "Failed to search commands");
            if (error) {
                mcp_send_message(client_fd, error);
                free(error);
            }
            return -1;
        }
    }
    
    if (strcmp(tool_name, "suggest_command") == 0) {
        /* Suggest command from natural language query */
        char *suggestion = mcp_handle_suggest_command(req->params);
        if (suggestion) {
            MCPResponse resp = {.id = req->id, .type = "response", .result = suggestion};
            char *json = mcp_build_response(&resp);
            if (json) {
                mcp_send_message(client_fd, json);
                free(json);
            }
            free(suggestion);
            return 0;
        } else {
            char *error = mcp_json_build_error(req->id, "Failed to suggest command");
            if (error) {
                mcp_send_message(client_fd, error);
                free(error);
            }
            return -1;
        }
    }
    
    /* Resolve tool alias to actual command */
    const char *actual_tool = mcp_tools_resolve_alias(tool_name);
    if (actual_tool != tool_name) {
        /* Tool was an alias, use actual command */
        strncpy(tool_name, actual_tool, sizeof(tool_name) - 1);
        tool_name[sizeof(tool_name) - 1] = '\0';
    }
    
    /* Validate tool exists and is safe */
    if (!mcp_exec_is_safe_command(tool_name)) {
        char error_msg[256];
        snprintf(error_msg, sizeof(error_msg), "Tool not found or not allowed: %s", tool_name);
        char *error = mcp_json_build_error(req->id, error_msg);
        if (error) {
            mcp_send_message(client_fd, error);
            free(error);
        }
        return -1;
    }
    
    /* Send notification that tool started */
    mcp_send_notification(client_fd, "tool_started", tool_name);
    
    /* Parse arguments from params */
    /* For Prompt 2, we'll handle simple text argument */
    char arg_text[1024] = "";
    mcp_json_extract_string(req->params, "text", arg_text, sizeof(arg_text));
    
    /* Sanitize arguments */
    char safe_arg[1024];
    if (strlen(arg_text) > 0) {
        if (mcp_exec_sanitize_arg(arg_text, safe_arg, sizeof(safe_arg)) != 0) {
            char *error = mcp_json_build_error(req->id, "Invalid argument");
            if (error) {
                mcp_send_message(client_fd, error);
                free(error);
            }
            return -1;
        }
    }
    
    /* Build argument array */
    char *args[3] = {NULL, NULL, NULL};
    if (strlen(safe_arg) > 0) {
        args[0] = safe_arg;
    }
    
    /* Execute command */
    MCPExecResult exec_result;
    int exec_status = mcp_exec_command(tool_name, args, env, &exec_result);
    
    if (exec_status != 0) {
        /* Send failure notification */
        mcp_send_notification(client_fd, "tool_failed", tool_name);
        
        char *error = mcp_json_build_error(req->id, "Command execution failed");
        if (error) {
            mcp_send_message(client_fd, error);
            free(error);
        }
        return -1;
    }
    
    /* Build result with output */
    /* Escape output for JSON */
    char escaped_output[MCP_MAX_OUTPUT];
    if (exec_result.stdout_data) {
        mcp_json_escape(exec_result.stdout_data, escaped_output, sizeof(escaped_output));
    } else {
        escaped_output[0] = '\0';
    }
    
    /* Build JSON result */
    char result_json[MCP_MAX_OUTPUT + 256];
    snprintf(result_json, sizeof(result_json),
             "{\"tool\":\"%s\",\"output\":\"%s\",\"exit_code\":%d}",
             tool_name, escaped_output, exec_result.exit_code);
    
    /* Send response */
    MCPResponse resp = {
        .id = req->id,
        .type = "response",
        .result = result_json
    };
    
    char *json = mcp_build_response(&resp);
    if (json) {
        mcp_send_message(client_fd, json);
        free(json);
    }
    
    /* Clean up */
    mcp_exec_free_result(&exec_result);
    
    /* Send completion notification */
    mcp_send_notification(client_fd, "tool_completed", tool_name);
    
    return 0;
}

/*
 * mcp_handle_get_execution_status - Handle get_execution_status method
 * 
 * Query status of a running or completed execution
 */
int mcp_handle_get_execution_status(int client_fd, MCPRequest *req, Env *env) {
    (void)env;  /* Unused */
    
    /* Extract execution_id from params */
    char exec_id[64];
    if (!mcp_json_extract_string(req->params, "execution_id", exec_id, sizeof(exec_id))) {
        char *error = mcp_json_build_error(req->id, "Missing execution_id parameter");
        if (error) {
            mcp_send_message(client_fd, error);
            free(error);
        }
        return -1;
    }
    
    /* Find execution */
    pthread_mutex_lock(&g_exec_lock);
    MCPExecution *exec = mcp_find_execution(exec_id);
    
    if (!exec) {
        pthread_mutex_unlock(&g_exec_lock);
        char *error = mcp_json_build_error(req->id, "Execution not found");
        if (error) {
            mcp_send_message(client_fd, error);
            free(error);
        }
        return -1;
    }
    
    /* Calculate elapsed time */
    time_t now = time(NULL);
    long elapsed = (long)(now - exec->start_time);
    
    /* Determine status string */
    const char *status_str;
    if (exec->status == 0) {
        status_str = "running";
    } else if (exec->status == 1) {
        status_str = "completed";
    } else {
        status_str = "failed";
    }
    
    /* Build result JSON */
    char result_json[512];
    snprintf(result_json, sizeof(result_json),
             "{\"execution_id\":\"%s\",\"tool\":\"%s\",\"status\":\"%s\",\"elapsed_time\":%ld,\"pid\":%d}",
             exec->id, exec->tool_name, status_str, elapsed, exec->child_pid);
    
    pthread_mutex_unlock(&g_exec_lock);
    
    /* Send response */
    MCPResponse resp = {
        .id = req->id,
        .type = "response",
        .result = result_json
    };
    
    char *json = mcp_build_response(&resp);
    if (json) {
        mcp_send_message(client_fd, json);
        free(json);
    }
    
    return 0;
}

/*
 * mcp_handle_cancel_execution - Handle cancel_execution method
 * 
 * Send SIGTERM to child process to cancel execution
 */
int mcp_handle_cancel_execution(int client_fd, MCPRequest *req, Env *env) {
    (void)env;  /* Unused */
    
    /* Extract execution_id from params */
    char exec_id[64];
    if (!mcp_json_extract_string(req->params, "execution_id", exec_id, sizeof(exec_id))) {
        char *error = mcp_json_build_error(req->id, "Missing execution_id parameter");
        if (error) {
            mcp_send_message(client_fd, error);
            free(error);
        }
        return -1;
    }
    
    /* Find execution */
    pthread_mutex_lock(&g_exec_lock);
    MCPExecution *exec = mcp_find_execution(exec_id);
    
    if (!exec) {
        pthread_mutex_unlock(&g_exec_lock);
        char *error = mcp_json_build_error(req->id, "Execution not found");
        if (error) {
            mcp_send_message(client_fd, error);
            free(error);
        }
        return -1;
    }
    
    /* Send SIGTERM to child process */
    pid_t pid = exec->child_pid;
    int kill_result = kill(pid, SIGTERM);
    
    if (kill_result == 0) {
        /* Successfully sent signal */
        exec->status = 2;  /* Mark as failed/cancelled */
        pthread_mutex_unlock(&g_exec_lock);
        
        /* Send success response */
        const char *result = "{\"status\":\"cancelled\"}";
        MCPResponse resp = {
            .id = req->id,
            .type = "response",
            .result = (char*)result
        };
        
        char *json = mcp_build_response(&resp);
        if (json) {
            mcp_send_message(client_fd, json);
            free(json);
        }
        
        /* Send notification */
        mcp_send_notification(client_fd, "tool_failed", exec->tool_name);
        
        return 0;
    } else {
        /* Failed to send signal (process may have already exited) */
        pthread_mutex_unlock(&g_exec_lock);
        
        char error_msg[128];
        snprintf(error_msg, sizeof(error_msg), "Failed to cancel execution: %s", strerror(errno));
        char *error = mcp_json_build_error(req->id, error_msg);
        if (error) {
            mcp_send_message(client_fd, error);
            free(error);
        }
        return -1;
    }
}

/*
 * mcp_handle_request - Route request to appropriate handler
 */
int mcp_handle_request(int client_fd, const char *json_req, Env *env) {
    MCPRequest req;
    
    /* Parse request */
    if (mcp_parse_request(json_req, &req) != 0) {
        /* Send error response */
        char *error = mcp_json_build_error(NULL, "Failed to parse request");
        if (error) {
            mcp_send_message(client_fd, error);
            free(error);
        }
        return -1;
    }
    
    int ret = 0;
    
    /* Route to appropriate handler */
    if (strcmp(req.method, "initialize") == 0) {
        ret = mcp_handle_initialize(client_fd, &req, env);
    } else if (strcmp(req.method, "list_tools") == 0) {
        ret = mcp_handle_list_tools(client_fd, &req, env);
    } else if (strcmp(req.method, "call_tool") == 0) {
        ret = mcp_handle_call_tool(client_fd, &req, env);
    } else if (strcmp(req.method, "get_execution_status") == 0) {
        ret = mcp_handle_get_execution_status(client_fd, &req, env);
    } else if (strcmp(req.method, "cancel_execution") == 0) {
        ret = mcp_handle_cancel_execution(client_fd, &req, env);
    } else {
        /* Unknown method */
        char error_msg[256];
        snprintf(error_msg, sizeof(error_msg), "Unknown method: %s", req.method);
        char *error = mcp_json_build_error(req.id, error_msg);
        if (error) {
            mcp_send_message(client_fd, error);
            free(error);
        }
        ret = -1;
    }
    
    /* Free request */
    mcp_free_request(&req);
    
    return ret;
}

/*
 * mcp_client_handler - Handle individual client connection with connection tracking
 * 
 * Enhanced in Prompt 3 with:
 * - Connection limit enforcement (MCP_MAX_CLIENTS)
 * - Active client tracking with atomic increment/decrement
 * - Timeout handling (read with timeout)
 * - Proper resource cleanup
 */
void* mcp_client_handler(void *arg) {
    /* Extract arguments */
    ClientHandlerArg *handler_arg = (ClientHandlerArg*)arg;
    int client_fd = handler_arg->client_fd;
    MCPServerConfig *config = handler_arg->config;
    Env *env = config->env;
    
    char buffer[MCP_BUFFER_SIZE];
    
    /* Set socket timeout for idle connections */
    struct timeval tv;
    tv.tv_sec = 60;   /* 60 second timeout */
    tv.tv_usec = 0;
    setsockopt(client_fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv));
    
    /* Read-handle-respond loop */
    while (config->running) {
        int n = mcp_recv_message(client_fd, buffer, sizeof(buffer));
        
        if (n < 0) {
            /* Error reading (timeout or actual error) */
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                /* Timeout - client idle too long */
                fprintf(stderr, "MCP Server: Client timeout (idle 60s)\n");
            } else {
                fprintf(stderr, "MCP Server: Error reading from client: %s\n", strerror(errno));
            }
            break;
        } else if (n == 0) {
            /* Client disconnected gracefully */
            break;
        }
        
        /* Enforce maximum request size */
        if (n >= MCP_BUFFER_SIZE - 1) {
            fprintf(stderr, "MCP Server: Request too large (>%d bytes)\n", MCP_BUFFER_SIZE);
            char *error = mcp_json_build_error(NULL, "Request too large");
            if (error) {
                mcp_send_message(client_fd, error);
                free(error);
            }
            break;
        }
        
        /* Handle request */
        mcp_handle_request(client_fd, buffer, env);
    }
    
    /* Decrement active client count */
    pthread_mutex_lock(&config->lock);
    config->active_clients--;
    pthread_mutex_unlock(&config->lock);
    
    /* Close client socket and free arguments */
    close(client_fd);
    free(handler_arg);
    
    return NULL;
}

/*
 * mcp_server_thread - Thread entry point for MCP server (accept loop)
 * 
 * Enhanced in Prompt 3 with:
 * - Connection limit enforcement (rejects if > MCP_MAX_CLIENTS)
 * - Client connection tracking and logging
 * - Proper error handling and logging
 * - Thread detachment for automatic cleanup
 */
void* mcp_server_thread(void *arg) {
    MCPServerConfig *config = (MCPServerConfig*)arg;
    
    while (config->running) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        
        /* Accept client connection */
        int client_fd = accept(config->sockfd, 
                               (struct sockaddr*)&client_addr, 
                               &client_len);
        
        if (client_fd < 0) {
            if (config->running) {
                fprintf(stderr, "MCP Server: Accept failed: %s\n", strerror(errno));
            }
            /* Server might be shutting down */
            continue;
        }
        
        /* Check connection limit */
        pthread_mutex_lock(&config->lock);
        int current_clients = config->active_clients;
        pthread_mutex_unlock(&config->lock);
        
        if (current_clients >= MCP_MAX_CLIENTS) {
            fprintf(stderr, "MCP Server: Connection limit reached (%d/%d), rejecting client\n",
                    current_clients, MCP_MAX_CLIENTS);
            
            /* Send error to client */
            const char *error_msg = "{\"id\":null,\"type\":\"error\",\"error\":\"Server connection limit reached\"}\n";
            write(client_fd, error_msg, strlen(error_msg));
            close(client_fd);
            continue;
        }
        
        /* Increment active client count */
        pthread_mutex_lock(&config->lock);
        config->active_clients++;
        pthread_mutex_unlock(&config->lock);
        
        printf("MCP Server: Accepted connection from %s (clients: %d/%d)\n", 
               inet_ntoa(client_addr.sin_addr),
               config->active_clients,
               MCP_MAX_CLIENTS);
        
        /* Create thread to handle client */
        pthread_t client_thread;
        ClientHandlerArg *handler_arg = malloc(sizeof(ClientHandlerArg));
        if (!handler_arg) {
            fprintf(stderr, "MCP Server: Failed to allocate handler argument\n");
            close(client_fd);
            pthread_mutex_lock(&config->lock);
            config->active_clients--;
            pthread_mutex_unlock(&config->lock);
            continue;
        }
        
        handler_arg->client_fd = client_fd;
        handler_arg->config = config;
        
        if (pthread_create(&client_thread, NULL, mcp_client_handler, handler_arg) != 0) {
            fprintf(stderr, "MCP Server: Failed to create client thread: %s\n", strerror(errno));
            close(client_fd);
            free(handler_arg);
            pthread_mutex_lock(&config->lock);
            config->active_clients--;
            pthread_mutex_unlock(&config->lock);
            continue;
        }
        
        /* Detach thread so it cleans up automatically */
        pthread_detach(client_thread);
    }
    
    return NULL;
}

/*
 * mcp_server_start - Create socket, bind, listen, and start server thread
 * 
 * Enhanced in Prompt 3 with signal handling setup.
 */
int mcp_server_start(MCPServerConfig *config) {
    if (!config) {
        return -1;
    }
    
    /* Setup signal handlers (ignore SIGPIPE) */
    setup_signal_handlers();
    
    /* Create socket */
    config->sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (config->sockfd < 0) {
        fprintf(stderr, "MCP Server: Failed to create socket: %s\n", strerror(errno));
        return -1;
    }
    
    /* Set socket options - reuse address */
    int yes = 1;
    if (setsockopt(config->sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) < 0) {
        fprintf(stderr, "MCP Server: Failed to set socket options: %s\n", strerror(errno));
        close(config->sockfd);
        return -1;
    }
    
    /* Bind socket */
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(config->port);
    
    if (bind(config->sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        fprintf(stderr, "MCP Server: Failed to bind socket to port %d: %s\n", 
                config->port, strerror(errno));
        close(config->sockfd);
        return -1;
    }
    
    /* Listen with backlog of 8 connections */
    if (listen(config->sockfd, 8) < 0) {
        fprintf(stderr, "MCP Server: Failed to listen: %s\n", strerror(errno));
        close(config->sockfd);
        return -1;
    }
    
    /* Set running flag */
    config->running = 1;
    config->enabled = 1;
    
    /* Create server thread */
    if (pthread_create(&config->server_thread, NULL, mcp_server_thread, config) != 0) {
        fprintf(stderr, "MCP Server: Failed to create server thread\n");
        close(config->sockfd);
        config->running = 0;
        config->enabled = 0;
        return -1;
    }
    
    printf("MCP Server: Listening on port %d (max clients: %d)\n", config->port, MCP_MAX_CLIENTS);
    
    return 0;
}

/*
 * mcp_server_stop - Stop server thread and close socket
 */
void mcp_server_stop(MCPServerConfig *config) {
    if (!config || !config->running) {
        return;
    }
    
    /* Set running flag to false */
    config->running = 0;
    
    /* Close socket to unblock accept() */
    if (config->sockfd >= 0) {
        close(config->sockfd);
        config->sockfd = -1;
    }
    
    /* Wait for server thread to finish */
    pthread_join(config->server_thread, NULL);
    
    config->enabled = 0;
    
    printf("MCP Server: Stopped\n");
}
