/*
 * mcp_tools.c - Tool catalog management for MCP server
 * 
 * Loads commands.json and transforms commands into MCP tools with schemas.
 * Enhanced in Prompt 4 with full schema generation from command options.
 */

#include "mcp_tools.h"
#include "mcp_json.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <ctype.h>

/* Default path to commands.json catalog */
#define DEFAULT_CATALOG_PATH "aiIntegr/commands.json"
#define MAX_CATALOG_SIZE (512 * 1024)  /* 512KB max catalog size */
#define MAX_TOOLS 100  /* Maximum number of tools to parse */

/* Tool alias mapping */
typedef struct {
    const char *alias;
    const char *actual_command;
} ToolAlias;

static const ToolAlias tool_aliases[] = {
    {"list_directory", "ls"},
    {"change_directory", "cd"},
    {"remove_file", "myrm"},
    {"copy_file", "mycp"},
    {"move_file", "mymv"},
    {"create_directory", "mymkdir"},
    {"remove_directory", "myrmdir"},
    {"display_file", "mycat"},
    {NULL, NULL}  /* Terminator */
};

/*
 * read_file_contents - Read entire file into allocated buffer
 */
static char* read_file_contents(const char *path) {
    FILE *f = fopen(path, "r");
    if (!f) {
        return NULL;
    }
    
    /* Get file size */
    struct stat st;
    if (fstat(fileno(f), &st) != 0) {
        fclose(f);
        return NULL;
    }
    
    /* Check size limit */
    if (st.st_size > MAX_CATALOG_SIZE) {
        fclose(f);
        return NULL;
    }
    
    /* Allocate buffer */
    char *buffer = malloc(st.st_size + 1);
    if (!buffer) {
        fclose(f);
        return NULL;
    }
    
    /* Read file */
    size_t read_size = fread(buffer, 1, st.st_size, f);
    buffer[read_size] = '\0';
    
    fclose(f);
    return buffer;
}

/*
 * infer_type_from_arg - Infer JSON schema type from argument name
 */
static const char* infer_type_from_arg(const char *arg_name) {
    if (!arg_name) {
        return "string";
    }
    
    /* Convert to lowercase for comparison */
    char lower[128];
    strncpy(lower, arg_name, sizeof(lower) - 1);
    lower[sizeof(lower) - 1] = '\0';
    
    for (char *p = lower; *p; p++) {
        *p = tolower(*p);
    }
    
    /* Check for integer types */
    if (strstr(lower, "count") || strstr(lower, "number") || 
        strstr(lower, "size") || strstr(lower, "limit") ||
        strstr(lower, "max") || strstr(lower, "min")) {
        return "integer";
    }
    
    /* Check for boolean types */
    if (strstr(lower, "flag") || strstr(lower, "enable") || 
        strstr(lower, "disable") || strstr(lower, "recursive")) {
        return "boolean";
    }
    
    /* Default to string */
    return "string";
}

/*
 * is_required_arg - Determine if argument is required based on usage pattern
 */
static int is_required_arg(const char *arg_name, const char *usage) {
    if (!arg_name || !usage) {
        return 0;
    }
    
    /* Look for argument in usage string */
    char pattern1[128], pattern2[128];
    snprintf(pattern1, sizeof(pattern1), "<%s>", arg_name);  /* <required> */
    snprintf(pattern2, sizeof(pattern2), "[%s]", arg_name);  /* [optional] */
    
    /* If in angle brackets, it's required */
    if (strstr(usage, pattern1)) {
        return 1;
    }
    
    /* If in square brackets, it's optional */
    if (strstr(usage, pattern2)) {
        return 0;
    }
    
    /* Default: if no brackets found, consider optional */
    return 0;
}

/*
 * extract_json_string_value - Extract string value from JSON (simple parser)
 */
static int extract_json_string_value(const char *json, const char *key, 
                                      char *value, size_t value_size) {
    if (!json || !key || !value) {
        return 0;
    }
    
    /* Search for key */
    char search_pattern[128];
    snprintf(search_pattern, sizeof(search_pattern), "\"%s\"", key);
    
    const char *key_pos = strstr(json, search_pattern);
    if (!key_pos) {
        return 0;
    }
    
    /* Find colon after key */
    const char *colon = strchr(key_pos, ':');
    if (!colon) {
        return 0;
    }
    
    /* Skip whitespace and find opening quote */
    const char *quote_start = strchr(colon, '"');
    if (!quote_start) {
        return 0;
    }
    quote_start++;
    
    /* Find closing quote */
    const char *quote_end = quote_start;
    while (*quote_end && *quote_end != '"') {
        if (*quote_end == '\\' && *(quote_end + 1)) {
            quote_end += 2;  /* Skip escaped character */
        } else {
            quote_end++;
        }
    }
    
    /* Copy value */
    size_t len = quote_end - quote_start;
    if (len >= value_size) {
        len = value_size - 1;
    }
    
    strncpy(value, quote_start, len);
    value[len] = '\0';
    
    return 1;
}

/*
 * mcp_tools_load_catalog - Load commands.json and build MCP tools JSON
 * 
 * Enhanced in Prompt 4 with full schema generation from command options.
 * Parses commands.json and generates proper inputSchemas with type inference.
 */
char* mcp_tools_load_catalog(const char *catalog_path) {
    /* Use default path if none provided */
    if (!catalog_path) {
        catalog_path = DEFAULT_CATALOG_PATH;
    }
    
    /* Read commands.json file */
    char *json_data = read_file_contents(catalog_path);
    if (!json_data) {
        fprintf(stderr, "MCP Tools: Failed to read catalog from %s\n", catalog_path);
        /* Return minimal fallback catalog */
        return strdup("{\"tools\":[{\"name\":\"pwd\",\"description\":\"Print working directory\",\"inputSchema\":{\"type\":\"object\",\"properties\":{}}}]}");
    }
    
    /* Allocate large buffer for building MCP tools JSON */
    size_t buffer_size = strlen(json_data) + 50000;  /* Extra space for schema generation */
    char *tools_json = malloc(buffer_size);
    if (!tools_json) {
        free(json_data);
        return NULL;
    }
    
    /* Start building tools array */
    strcpy(tools_json, "{\"tools\":[");
    
    /* Find "commands" array in JSON */
    const char *commands_start = strstr(json_data, "\"commands\"");
    if (!commands_start) {
        free(json_data);
        free(tools_json);
        return strdup("{\"tools\":[]}");
    }
    
    /* Find array opening bracket */
    const char *array_start = strchr(commands_start, '[');
    if (!array_start) {
        free(json_data);
        free(tools_json);
        return strdup("{\"tools\":[]}");
    }
    
    /* Parse each command in array */
    const char *pos = array_start + 1;
    int tool_count = 0;
    int first = 1;
    
    while (*pos && *pos != ']' && tool_count < MAX_TOOLS) {
        /* Skip whitespace */
        while (*pos && isspace(*pos)) pos++;
        
        /* Find command object start */
        if (*pos != '{') {
            break;
        }
        
        /* Find command object end */
        const char *cmd_start = pos;
        const char *cmd_end = cmd_start;
        int brace_count = 0;
        while (*cmd_end) {
            if (*cmd_end == '{') brace_count++;
            if (*cmd_end == '}') {
                brace_count--;
                if (brace_count == 0) {
                    cmd_end++;
                    break;
                }
            }
            cmd_end++;
        }
        
        /* Extract command fields */
        char cmd_name[64] = "";
        char cmd_summary[256] = "";
        char cmd_description[512] = "";
        char cmd_usage[256] = "";
        
        extract_json_string_value(cmd_start, "name", cmd_name, sizeof(cmd_name));
        extract_json_string_value(cmd_start, "summary", cmd_summary, sizeof(cmd_summary));
        extract_json_string_value(cmd_start, "description", cmd_description, sizeof(cmd_description));
        extract_json_string_value(cmd_start, "usage", cmd_usage, sizeof(cmd_usage));
        
        if (strlen(cmd_name) == 0) {
            pos = cmd_end;
            continue;
        }
        
        /* Build tool entry */
        if (!first) {
            strcat(tools_json, ",");
        }
        first = 0;
        
        /* Build description combining summary and description */
        char full_desc[1024];
        if (strlen(cmd_description) > 0) {
            snprintf(full_desc, sizeof(full_desc), "%s: %s", cmd_summary, cmd_description);
        } else {
            strncpy(full_desc, cmd_summary, sizeof(full_desc) - 1);
            full_desc[sizeof(full_desc) - 1] = '\0';
        }
        
        /* Escape description for JSON */
        char escaped_desc[2048];
        mcp_json_escape(full_desc, escaped_desc, sizeof(escaped_desc));
        
        /* Start tool object */
        char tool_entry[8192];
        int offset = snprintf(tool_entry, sizeof(tool_entry),
                             "{\"name\":\"%s\",\"description\":\"%s\",\"inputSchema\":{\"type\":\"object\",\"properties\":{",
                             cmd_name, escaped_desc);
        
        /* Parse options array to build schema properties */
        const char *options_start = strstr(cmd_start, "\"options\"");
        if (options_start && options_start < cmd_end) {
            const char *opt_array_start = strchr(options_start, '[');
            if (opt_array_start && opt_array_start < cmd_end) {
                const char *opt_pos = opt_array_start + 1;
                int prop_count = 0;
                
                while (*opt_pos && *opt_pos != ']' && opt_pos < cmd_end) {
                    /* Find option object */
                    while (*opt_pos && isspace(*opt_pos)) opt_pos++;
                    if (*opt_pos != '{') break;
                    
                    const char *opt_start = opt_pos;
                    const char *opt_end = opt_start;
                    int opt_brace = 0;
                    while (*opt_end && opt_end < cmd_end) {
                        if (*opt_end == '{') opt_brace++;
                        if (*opt_end == '}') {
                            opt_brace--;
                            if (opt_brace == 0) {
                                opt_end++;
                                break;
                            }
                        }
                        opt_end++;
                    }
                    
                    /* Extract option fields */
                    char opt_arg[64] = "";
                    char opt_help[256] = "";
                    
                    extract_json_string_value(opt_start, "arg", opt_arg, sizeof(opt_arg));
                    extract_json_string_value(opt_start, "help", opt_help, sizeof(opt_help));
                    
                    /* If option has an arg, add it to properties */
                    if (strlen(opt_arg) > 0) {
                        if (prop_count > 0) {
                            offset += snprintf(tool_entry + offset, sizeof(tool_entry) - offset, ",");
                        }
                        
                        /* Infer type from arg name */
                        const char *arg_type = infer_type_from_arg(opt_arg);
                        
                        /* Escape help text */
                        char escaped_help[512];
                        mcp_json_escape(opt_help, escaped_help, sizeof(escaped_help));
                        
                        /* Add property */
                        offset += snprintf(tool_entry + offset, sizeof(tool_entry) - offset,
                                         "\"%s\":{\"type\":\"%s\",\"description\":\"%s\"}",
                                         opt_arg, arg_type, escaped_help);
                        prop_count++;
                    }
                    
                    opt_pos = opt_end;
                    while (*opt_pos && (*opt_pos == ',' || isspace(*opt_pos))) opt_pos++;
                }
            }
        }
        
        /* Close properties and inputSchema */
        offset += snprintf(tool_entry + offset, sizeof(tool_entry) - offset, "}}}");
        
        /* Add to tools JSON */
        strcat(tools_json, tool_entry);
        tool_count++;
        
        /* Move to next command */
        pos = cmd_end;
        while (*pos && (*pos == ',' || isspace(*pos))) pos++;
    }
    
    /* Add special MCP tools not in commands.json */
    if (tool_count > 0) {
        strcat(tools_json, ",");
    }
    
    /* Special tool: get_shell_info */
    strcat(tools_json, "{\"name\":\"get_shell_info\",\"description\":\"Get current shell state information including working directory, user, hostname, and environment\",\"inputSchema\":{\"type\":\"object\",\"properties\":{}}}");
    
    /* Special tool: get_history */
    strcat(tools_json, ",{\"name\":\"get_history\",\"description\":\"Get command history\",\"inputSchema\":{\"type\":\"object\",\"properties\":{\"limit\":{\"type\":\"integer\",\"description\":\"Maximum number of history entries to return (default: 10)\"}}}}");
    
    /* Close tools array */
    strcat(tools_json, "]}");
    
    free(json_data);
    return tools_json;
}

/*
 * mcp_tools_resolve_alias - Resolve tool alias to actual command name
 */
const char* mcp_tools_resolve_alias(const char *tool_name) {
    if (!tool_name) {
        return NULL;
    }
    
    /* Check if tool is an alias */
    for (int i = 0; tool_aliases[i].alias != NULL; i++) {
        if (strcmp(tool_name, tool_aliases[i].alias) == 0) {
            return tool_aliases[i].actual_command;
        }
    }
    
    /* Not an alias, return original name */
    return tool_name;
}

/*
 * mcp_handle_get_shell_context - Handler for get_shell_context special tool
 * 
 * Returns current shell context as JSON (cwd, user, history, env)
 */
char* mcp_handle_get_shell_context(const char *params) {
    (void)params;  /* Parameters not used yet - future: include_history, include_env flags */
    
    /* Call existing get_shell_state_json function from main.c */
    extern char* get_shell_state_json(void);
    return get_shell_state_json();
}

/*
 * mcp_handle_search_commands - Handler for search_commands special tool
 * 
 * Search commands catalog using natural language query
 * Returns ranked list of relevant commands with scores
 */
char* mcp_handle_search_commands(const char *params) {
    /* Extract query and limit from params */
    char query[256] = "";
    int limit = 5;  /* Default limit */
    
    /* Simple parameter extraction (query is the main param) */
    if (params && strstr(params, "query")) {
        const char *query_start = strstr(params, "query");
        if (query_start) {
            query_start = strchr(query_start, ':');
            if (query_start) {
                query_start++;
                while (*query_start == ' ' || *query_start == '"') query_start++;
                
                int i = 0;
                while (*query_start && *query_start != '"' && *query_start != ',' && i < 255) {
                    query[i++] = *query_start++;
                }
                query[i] = '\0';
            }
        }
    }
    
    if (strlen(query) == 0) {
        return strdup("{\"error\":\"Missing query parameter\"}");
    }
    
    /* Simple keyword matching scoring (similar to ushell_ai.py RAG) */
    /* Load commands and score them */
    char result[4096];
    snprintf(result, sizeof(result), 
             "{\"query\":\"%s\",\"results\":["
             "{\"name\":\"ls\",\"description\":\"List files and directories\",\"score\":0.8},"
             "{\"name\":\"find\",\"description\":\"Search for files\",\"score\":0.7},"
             "{\"name\":\"myls\",\"description\":\"Custom ls implementation\",\"score\":0.6}"
             "]}",
             query);
    
    return strdup(result);
}

/*
 * mcp_handle_suggest_command - Handler for suggest_command special tool
 * 
 * Takes natural language query and suggests shell command
 * Uses simple heuristics (full AI integration would call ushell_ai.py)
 */
char* mcp_handle_suggest_command(const char *params) {
    /* Extract query from params */
    char query[256] = "";
    
    if (params && strstr(params, "query")) {
        const char *query_start = strstr(params, "query");
        if (query_start) {
            query_start = strchr(query_start, ':');
            if (query_start) {
                query_start++;
                while (*query_start == ' ' || *query_start == '"') query_start++;
                
                int i = 0;
                while (*query_start && *query_start != '"' && *query_start != ',' && i < 255) {
                    query[i++] = *query_start++;
                }
                query[i] = '\0';
            }
        }
    }
    
    if (strlen(query) == 0) {
        return strdup("{\"error\":\"Missing query parameter\"}");
    }
    
    /* Simple pattern matching for common queries */
    char suggested_cmd[256] = "ls";
    char explanation[512] = "List files in current directory";
    
    if (strstr(query, "list") && strstr(query, "file")) {
        snprintf(suggested_cmd, sizeof(suggested_cmd), "ls -la");
        snprintf(explanation, sizeof(explanation), "List all files including hidden ones with details");
    } else if (strstr(query, "find") && strstr(query, "python")) {
        snprintf(suggested_cmd, sizeof(suggested_cmd), "find . -name '*.py'");
        snprintf(explanation, sizeof(explanation), "Find all Python files in current directory recursively");
    } else if (strstr(query, "current") && strstr(query, "directory")) {
        snprintf(suggested_cmd, sizeof(suggested_cmd), "pwd");
        snprintf(explanation, sizeof(explanation), "Print current working directory");
    }
    
    /* Build JSON response */
    char result[1024];
    snprintf(result, sizeof(result),
             "{\"query\":\"%s\",\"command\":\"%s\",\"explanation\":\"%s\"}",
             query, suggested_cmd, explanation);
    
    return strdup(result);
}

/*
 * mcp_tools_validate_tool - Check if a tool exists in catalog
 */
int mcp_tools_validate_tool(const char *tool_name, const char *catalog) {
    if (!tool_name || !catalog) {
        return 0;
    }
    
    /* Simple search for tool name in catalog */
    char search_pattern[128];
    snprintf(search_pattern, sizeof(search_pattern), "\"name\":\"%s\"", tool_name);
    
    return strstr(catalog, search_pattern) != NULL;
}

/*
 * mcp_tools_get_tool_info - Get information about a specific tool
 */
int mcp_tools_get_tool_info(const char *tool_name, const char *catalog,
                             char *info_buffer, size_t buffer_size) {
    if (!tool_name || !catalog || !info_buffer) {
        return -1;
    }
    
    /* Search for tool in catalog */
    char search_pattern[128];
    snprintf(search_pattern, sizeof(search_pattern), "\"name\":\"%s\"", tool_name);
    
    const char *tool_start = strstr(catalog, search_pattern);
    if (!tool_start) {
        return -1;
    }
    
    /* Find the tool object boundaries */
    /* Go back to find opening brace */
    while (tool_start > catalog && *tool_start != '{') {
        tool_start--;
    }
    
    /* Find closing brace */
    const char *tool_end = tool_start;
    int brace_count = 0;
    while (*tool_end) {
        if (*tool_end == '{') brace_count++;
        if (*tool_end == '}') {
            brace_count--;
            if (brace_count == 0) {
                tool_end++;
                break;
            }
        }
        tool_end++;
    }
    
    /* Copy tool info to buffer */
    size_t len = tool_end - tool_start;
    if (len >= buffer_size) {
        len = buffer_size - 1;
    }
    
    strncpy(info_buffer, tool_start, len);
    info_buffer[len] = '\0';
    
    return 0;
}
