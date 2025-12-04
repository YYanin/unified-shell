#ifndef EXECUTOR_H
#define EXECUTOR_H

#include "environment.h"

/**
 * Command structure for pipelines
 */
typedef struct {
    char **argv;      // NULL-terminated argument array
    char *infile;     // Input redirection file (NULL if none)
    char *outfile;    // Output redirection file (NULL if none)
    int append;       // 1 if >> (append), 0 if > (truncate)
    int background;   // 1 if command should run in background (&)
} Command;

/**
 * Execute a command with arguments
 * @param argv NULL-terminated array of command arguments
 * @param env Environment for the shell
 * @return Exit status of the command (0-255), or -1 on fork/exec failure
 */
int execute_command(char **argv, Env *env);

/**
 * Tokenize a command line into arguments
 * @param line Input command line
 * @return NULL-terminated array of strings (must be freed by caller)
 *         Returns NULL on error
 */
char** tokenize_command(char *line);

/**
 * Free memory allocated by tokenize_command
 * @param argv Array to free
 */
void free_tokens(char **argv);

/**
 * Parse a command line into a pipeline of commands
 * @param line Input command line
 * @param commands Output array of Command structures (caller must free)
 * @param count Output number of commands in pipeline
 * @return 0 on success, -1 on error
 */
int parse_pipeline(char *line, Command **commands, int *count);

/**
 * Execute a pipeline of commands
 * @param commands Array of Command structures
 * @param count Number of commands in pipeline
 * @param env Environment for the shell
 * @return Exit status of last command
 */
int execute_pipeline(Command *commands, int count, Env *env);

/**
 * Free pipeline commands
 * @param commands Array of Command structures
 * @param count Number of commands
 */
void free_pipeline(Command *commands, int count);

#endif /* EXECUTOR_H */
