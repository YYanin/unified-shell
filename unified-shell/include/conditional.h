#ifndef CONDITIONAL_H
#define CONDITIONAL_H

#include "environment.h"

/**
 * Global variable to track last exit status
 */
extern int last_exit_status;

/**
 * Parse a conditional statement
 * @param line Input line to parse
 * @param condition Output: condition command (caller must free)
 * @param then_block Output: then block commands (caller must free)
 * @param else_block Output: else block commands or NULL (caller must free if not NULL)
 * @return 1 if line is a conditional, 0 if not, -1 on error
 */
int parse_conditional(char *line, char **condition, char **then_block, char **else_block);

/**
 * Execute a conditional statement
 * @param condition Command to evaluate (determines true/false)
 * @param then_block Commands to execute if condition succeeds (exit status 0)
 * @param else_block Commands to execute if condition fails (can be NULL)
 * @param env Environment for command execution
 * @return Exit status of executed block
 */
int execute_conditional(char *condition, char *then_block, char *else_block, Env *env);

#endif /* CONDITIONAL_H */
