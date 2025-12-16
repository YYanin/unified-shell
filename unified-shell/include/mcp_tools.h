#ifndef MCP_TOOLS_H
#define MCP_TOOLS_H

#include <stddef.h>

/*
 * MCP Tools - Tool catalog management for MCP server
 * 
 * Loads commands.json and transforms it into MCP tool format with schemas.
 * Provides tool discovery and validation for MCP call_tool operations.
 */

/*
 * mcp_tools_load_catalog - Load commands.json and build MCP tools JSON
 * 
 * Reads the commands.json file from aiIntegr directory and transforms
 * each command into an MCP tool with proper JSON schema.
 * 
 * Parameters:
 *   catalog_path: Path to commands.json file (if NULL, uses default)
 * 
 * Returns:
 *   Dynamically allocated JSON string with tools array (caller must free)
 *   NULL on error
 * 
 * Example output:
 *   {"tools":[{"name":"cd","description":"Change directory",...}]}
 */
char* mcp_tools_load_catalog(const char *catalog_path);

/*
 * mcp_tools_validate_tool - Check if a tool exists in catalog
 * 
 * Parameters:
 *   tool_name: Name of tool to validate
 *   catalog: Cached catalog JSON (from mcp_tools_load_catalog)
 * 
 * Returns:
 *   1 if tool exists, 0 if not found
 */
int mcp_tools_validate_tool(const char *tool_name, const char *catalog);

/*
 * mcp_tools_get_tool_info - Get information about a specific tool
 * 
 * Parameters:
 *   tool_name: Name of tool
 *   catalog: Cached catalog JSON
 *   info_buffer: Buffer to store tool info (JSON)
 *   buffer_size: Size of info_buffer
 * 
 * Returns:
 *   0 on success, -1 if tool not found
 */
int mcp_tools_get_tool_info(const char *tool_name, const char *catalog, 
                             char *info_buffer, size_t buffer_size);

/*
 * mcp_tools_resolve_alias - Resolve tool alias to actual command name
 * 
 * Supports descriptive tool names (e.g., "list_directory" -> "ls").
 * 
 * Parameters:
 *   tool_name: Tool name or alias
 * 
 * Returns:
 *   Actual command name (static string, do not free)
 *   Original name if no alias found
 */
const char* mcp_tools_resolve_alias(const char *tool_name);

/*
 * Special Tool Handlers (Prompt 7: AI Helper Integration)
 */

/*
 * mcp_handle_get_shell_context - Get current shell context as JSON
 * 
 * Returns shell state including cwd, user, history, environment variables.
 * 
 * Parameters:
 *   params: JSON params (include_history, include_env - not yet implemented)
 * 
 * Returns:
 *   Dynamically allocated JSON string (caller must free)
 */
char* mcp_handle_get_shell_context(const char *params);

/*
 * mcp_handle_search_commands - Search command catalog with natural language
 * 
 * Uses keyword matching to find relevant commands based on query.
 * 
 * Parameters:
 *   params: JSON params with "query" and optional "limit"
 * 
 * Returns:
 *   Dynamically allocated JSON with ranked results (caller must free)
 */
char* mcp_handle_search_commands(const char *params);

/*
 * mcp_handle_suggest_command - Suggest command from natural language query
 * 
 * Takes natural language input and suggests appropriate shell command.
 * 
 * Parameters:
 *   params: JSON params with "query"
 * 
 * Returns:
 *   Dynamically allocated JSON with command and explanation (caller must free)
 */
char* mcp_handle_suggest_command(const char *params);

#endif /* MCP_TOOLS_H */
