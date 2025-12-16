#ifndef SHELL_H
#define SHELL_H

// Constants
#define MAX_LINE 1024
#define PROMPT "unified-shell> "

/*
 * get_shell_state_json - Gather current shell state as JSON
 * 
 * Returns malloc'd JSON string with cwd, user, history, env
 * Caller must free the returned string
 */
char* get_shell_state_json(void);

#endif // SHELL_H
