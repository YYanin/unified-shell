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
    {"ls", tool_ls_main},
    {"cat", tool_cat_main},
    {"cp", tool_cp_main},
    {"mv", tool_mv_main},
    {"rm", tool_rm_main},
    {"mkdir", tool_mkdir_main},
    {"rmdir", tool_rmdir_main},
    {"touch", tool_touch_main},
    {"stat", tool_stat_main},
    {"find", tool_find_main},
    {NULL, NULL}  // Sentinel to mark end of table
};

/**
 * @brief Find a tool function by name
 * 
 * @param name The name of the tool to find (e.g., "ls")
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
