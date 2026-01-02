#ifndef TOOLS_H
#define TOOLS_H

/**
 * @file tools.h
 * @brief Header file for integrated shell tools
 * 
 * This file declares all tool main functions that are integrated
 * into the shell as built-in commands.
 */

// Tool main function declarations
int tool_ls_main(int argc, char **argv);
int tool_cat_main(int argc, char **argv);
int tool_cp_main(int argc, char **argv);
int tool_mv_main(int argc, char **argv);
int tool_rm_main(int argc, char **argv);
int tool_mkdir_main(int argc, char **argv);
int tool_rmdir_main(int argc, char **argv);
int tool_touch_main(int argc, char **argv);
int tool_stat_main(int argc, char **argv);
int tool_find_main(int argc, char **argv);

// Tool dispatch system
typedef int (*tool_func)(int argc, char **argv);
tool_func find_tool(const char *name);

#endif // TOOLS_H
