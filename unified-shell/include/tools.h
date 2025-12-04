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
int tool_myls_main(int argc, char **argv);
int tool_mycat_main(int argc, char **argv);
int tool_mycp_main(int argc, char **argv);
int tool_mymv_main(int argc, char **argv);
int tool_myrm_main(int argc, char **argv);
int tool_mymkdir_main(int argc, char **argv);
int tool_myrmdir_main(int argc, char **argv);
int tool_mytouch_main(int argc, char **argv);
int tool_mystat_main(int argc, char **argv);
int tool_myfd_main(int argc, char **argv);

// Tool dispatch system
typedef int (*tool_func)(int argc, char **argv);
tool_func find_tool(const char *name);

#endif // TOOLS_H
