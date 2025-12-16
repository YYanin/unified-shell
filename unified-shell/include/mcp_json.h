#ifndef MCP_JSON_H
#define MCP_JSON_H

#include <stddef.h>

/*
 * MCP JSON Helper - Simple JSON parsing and building for MCP protocol
 * 
 * This is an educational implementation for basic MCP operations.
 * For production use, replace with cJSON or jsmn library.
 * 
 * Supports:
 * - Extracting string fields from JSON objects
 * - Building simple JSON responses
 * - Escaping special characters in JSON strings
 */

/*
 * mcp_json_extract_string - Extract string value from JSON object
 * 
 * Parameters:
 *   json: JSON string to search
 *   field: Field name to extract (e.g., "id", "method")
 *   output: Buffer to store extracted value
 *   output_size: Size of output buffer
 * 
 * Returns:
 *   1 if field found and extracted, 0 if not found
 * 
 * Example:
 *   char id[64];
 *   mcp_json_extract_string(json, "id", id, sizeof(id));
 */
int mcp_json_extract_string(const char *json, const char *field, char *output, size_t output_size);

/*
 * mcp_json_escape - Escape special characters for JSON string
 * 
 * Parameters:
 *   input: String to escape
 *   output: Buffer for escaped string
 *   output_size: Size of output buffer
 * 
 * Escapes: " \ / \b \f \n \r \t
 */
void mcp_json_escape(const char *input, char *output, size_t output_size);

/*
 * mcp_json_build_response - Build a simple JSON response
 * 
 * Parameters:
 *   id: Request ID
 *   type: Response type ("response", "error", "notification")
 *   result: Result JSON string (already formatted)
 * 
 * Returns:
 *   Dynamically allocated JSON string (caller must free)
 */
char* mcp_json_build_response(const char *id, const char *type, const char *result);

/*
 * mcp_json_build_error - Build an error response
 * 
 * Parameters:
 *   id: Request ID
 *   error_message: Error message
 * 
 * Returns:
 *   Dynamically allocated JSON string (caller must free)
 */
char* mcp_json_build_error(const char *id, const char *error_message);

#endif /* MCP_JSON_H */
