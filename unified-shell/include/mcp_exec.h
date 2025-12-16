#ifndef MCP_EXEC_H
#define MCP_EXEC_H

#include "environment.h"

/*
 * MCP Execution - Safe command execution for MCP server
 * 
 * Provides sanitization, validation, and safe execution of shell commands
 * requested via MCP call_tool method.
 */

/* Maximum sizes for command execution */
#define MCP_MAX_OUTPUT 32768    /* 32KB max output */
#define MCP_MAX_ARGS 32         /* Max arguments per command */
#define MCP_EXEC_TIMEOUT 30     /* 30 second timeout */

/*
 * MCPExecResult - Result of command execution
 */
typedef struct {
    int exit_code;              /* Command exit code */
    char *stdout_data;          /* Captured stdout (allocated) */
    char *stderr_data;          /* Captured stderr (allocated) */
    int timed_out;              /* 1 if command timed out */
} MCPExecResult;

/*
 * mcp_exec_sanitize_arg - Sanitize command argument
 * 
 * Removes dangerous shell metacharacters and validates input.
 * 
 * Parameters:
 *   input: Argument to sanitize
 *   output: Buffer for sanitized output
 *   output_size: Size of output buffer
 * 
 * Returns:
 *   0 on success, -1 if argument is invalid/dangerous
 */
int mcp_exec_sanitize_arg(const char *input, char *output, size_t output_size);

/*
 * mcp_exec_command - Execute a shell command safely
 * 
 * Executes command with arguments, capturing output and enforcing limits.
 * 
 * Parameters:
 *   command: Command name to execute
 *   args: NULL-terminated array of arguments
 *   env: Shell environment
 *   result: Structure to store execution result (caller must free strings)
 * 
 * Returns:
 *   0 on successful execution (check result->exit_code for command status)
 *   -1 on error (command not found, fork failed, etc.)
 */
int mcp_exec_command(const char *command, char **args, Env *env, MCPExecResult *result);

/*
 * mcp_exec_free_result - Free resources in MCPExecResult
 * 
 * Parameters:
 *   result: Result structure to free
 */
void mcp_exec_free_result(MCPExecResult *result);

/*
 * mcp_exec_is_safe_command - Check if command is in whitelist
 * 
 * Parameters:
 *   command: Command name to check
 * 
 * Returns:
 *   1 if command is whitelisted/safe, 0 if not allowed
 */
int mcp_exec_is_safe_command(const char *command);

/*
 * mcp_exec_init_audit_log - Initialize audit logging (Prompt 5)
 * 
 * Opens audit log file for recording command executions.
 * 
 * Parameters:
 *   log_path: Path to audit log file (NULL to use USHELL_MCP_AUDIT_LOG env var)
 */
void mcp_exec_init_audit_log(const char *log_path);

/*
 * mcp_exec_close_audit_log - Close audit log (Prompt 5)
 */
void mcp_exec_close_audit_log(void);

/*
 * mcp_exec_validate_integer - Validate integer argument with range check (Prompt 5)
 * 
 * Safely parses integer and validates against min/max constraints.
 * 
 * Parameters:
 *   input: String to parse
 *   value: Pointer to store parsed integer
 *   min: Minimum allowed value
 *   max: Maximum allowed value
 * 
 * Returns:
 *   0 on success, -1 on parse error or out of range
 */
int mcp_exec_validate_integer(const char *input, int *value, int min, int max);

#endif /* MCP_EXEC_H */
