/**
 * @file tool_dispatch.c
 * @brief Tool dispatch system for integrated shell utilities
 * 
 * This file implements a dispatch table for built-in tools that are
 * integrated into the shell. Tools are executed without requiring PATH
 * lookup or external binaries.
 */

#include <string.h>
#include "tools.h"

/**
 * @brief Tool table entry structure
 */
typedef struct {
    const char *name;
    tool_func func;
} ToolEntry;

/**
 * @brief Dispatch table mapping tool names to their main functions
 * 
 * This table allows the shell to execute integrated tools directly
 * without spawning external processes.
 */
static ToolEntry tool_table[] = {
    {"myls", tool_myls_main},
    {"mycat", tool_mycat_main},
    {"mycp", tool_mycp_main},
    {"mymv", tool_mymv_main},
    {"myrm", tool_myrm_main},
    {"mymkdir", tool_mymkdir_main},
    {"myrmdir", tool_myrmdir_main},
    {"mytouch", tool_mytouch_main},
    {"mystat", tool_mystat_main},
    {"myfd", tool_myfd_main},
    {NULL, NULL}  // Sentinel to mark end of table
};

/**
 * @brief Find a tool function by name
 * 
 * @param name The name of the tool to find (e.g., "myls")
 * @return The tool function pointer if found, NULL otherwise
 */
tool_func find_tool(const char *name) {
    if (name == NULL) {
        return NULL;
    }
    
    for (int i = 0; tool_table[i].name != NULL; i++) {
        if (strcmp(tool_table[i].name, name) == 0) {
            return tool_table[i].func;
        }
    }
    
    return NULL;
}
