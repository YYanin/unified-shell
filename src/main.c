/**
 * main.c - Main entry point and REPL loop for unified shell
 * 
 * This file implements the shell's Read-Eval-Print Loop (REPL) which:
 * - Displays an interactive prompt with current directory
 * - Reads user input with line editing, history, and tab completion
 * - Expands variables ($var syntax)
 * - Parses commands (conditionals, pipelines, simple commands)
 * - Executes commands via built-ins, tools, or external programs
 * - Maintains command history across sessions
 * - Handles signals (Ctrl+C) gracefully
 * 
 * Architecture:
 * - Global environment stores shell variables
 * - Terminal module handles raw input mode for advanced features
 * - History module persists commands to ~/.ushell_history
 * - Completion module provides tab completion for commands/files
 * - Executor module runs commands with proper I/O redirection and piping
 * 
 * Signal handling:
 * - SIGINT (Ctrl+C): Interrupts current line, continues shell
 * - EOF (Ctrl+D): Exits shell gracefully
 * - SIGTERM: Handled by OS default (exits)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <signal.h>
#include "shell.h"
#include "environment.h"
#include "expansion.h"
#include "executor.h"
#include "conditional.h"
#include "history.h"
#include "completion.h"
#include "terminal.h"
#include "apt.h"
#include "jobs.h"
#include "signals.h"

/**
 * Global environment - stores shell variables and their values
 * Accessible to all functions for variable expansion and management
 */
Env *shell_env = NULL;

/**
 * cleanup_shell - Cleanup function called on exit
 * Frees all global resources to prevent memory leaks
 */
void cleanup_shell(void) {
    // Save history before exiting
    history_save(HISTORY_FILE);
    history_free();
    
    if (shell_env) {
        env_free(shell_env);
        shell_env = NULL;
    }
}

// Signal handlers now in src/jobs/signals.c
// See signals.h for declarations

/**
 * get_prompt - Generate dynamic shell prompt with current directory
 * 
 * Creates a bash-style prompt: "username:~/path> "
 * - Displays current username (from $USER environment variable)
 * - Shows current working directory (abbreviated with ~ for home dir)
 * - Ends with "> " to indicate ready for input
 * 
 * Examples:
 *   alice:~/Documents> 
 *   bob:/etc/nginx> 
 *   user:~> (if at home directory)
 * 
 * Returns: Pointer to static buffer containing prompt string
 *          (buffer persists across calls, don't free)
 */
const char* get_prompt(void) {
    static char prompt[MAX_LINE];
    char cwd[PATH_MAX];
    const char *username = getenv("USER");
    
    // Get current working directory
    // If getcwd fails (e.g., directory deleted), use "?" as fallback
    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        strcpy(cwd, "?");
    }
    
    // Abbreviate home directory with ~ for brevity
    // Example: /home/alice/Documents -> ~/Documents
    const char *home = getenv("HOME");
    if (home != NULL && strncmp(cwd, home, strlen(home)) == 0) {
        // Current path starts with home directory
        snprintf(prompt, sizeof(prompt), "%s:~%s> ", 
                 username ? username : "user", cwd + strlen(home));
    } else {
        // Use full path
        snprintf(prompt, sizeof(prompt), "%s:%s> ", 
                 username ? username : "user", cwd);
    }
    
    return prompt;
}

/**
 * main - Shell entry point and REPL loop
 * 
 * Initialization sequence:
 * 1. Create environment for shell variables
 * 2. Load command history from file
 * 3. Set up tab completion
 * 4. Register cleanup handlers
 * 5. Configure signal handling (Ctrl+C)
 * 6. Enter REPL loop
 * 
 * REPL loop:
 * - Read: Get user input with line editing
 * - Expand: Replace $variables with values
 * - Parse: Identify command type (conditional, pipeline, simple)
 * - Execute: Run via appropriate executor
 * - Print: Display output (handled by executed commands)
 * - Loop: Repeat until 'exit' or Ctrl+D
 * 
 * Returns: 0 on normal exit
 */
int main(void) {
    char line[MAX_LINE];
    
    // ===== Initialization Phase =====
    
    // Create environment to store shell variables (key-value pairs)
    shell_env = env_new();
    
    // Initialize apt package system (creates directories if needed)
    // This is non-blocking and safe to call even if not used
    apt_init();
    apt_load_index();  // Load package index silently
    
    // Set up PATH to include installed package binaries
    // This allows running executables from apt-installed packages
    apt_setup_path();
    
    // Initialize history subsystem and load previous commands from disk
    // History is stored in ~/.ushell_history
    history_init();
    history_load(HISTORY_FILE);
    
    // Initialize tab completion with access to environment variables
    completion_init(shell_env);
    
    // Initialize job control system for tracking background processes
    // Sets up job list with capacity for MAX_JOBS concurrent jobs
    jobs_init();
    
    // Register cleanup function to run at exit (saves history, frees memory)
    atexit(cleanup_shell);
    
    // Set up comprehensive signal handling for job control
    // Configures SIGINT, SIGTSTP, SIGCHLD, SIGTTOU, SIGTTIN
    setup_signal_handlers();
    
    // Set up test variables for demonstration/testing
    // These can be used in commands like: echo $name
    env_set(shell_env, "x", "5");
    env_set(shell_env, "name", "Alice");
    env_set(shell_env, "greeting", "Hello");
    env_set(shell_env, "user", "admin");
    
    // Configure terminal module callbacks for interactive features
    // - History: UP/DOWN arrows navigate command history
    // - Completion: TAB key completes commands and filenames
    terminal_set_history_callbacks(history_get_prev, history_get_next);
    terminal_set_completion_callback(completion_generate);
    
    // ===== REPL Loop =====
    
    while (1) {
        // Check for completed background jobs
        // If SIGCHLD was received, update job statuses and clean up
        if (child_exited) {
            child_exited = 0;  // Reset flag
            
            // Update all job statuses (non-blocking waitpid calls)
            // This will detect which jobs have completed
            jobs_update_status();
            
            // Note: Completed jobs are automatically cleaned up by jobs_cleanup()
            // For now, we don't print individual completion messages
            // Users can use 'jobs' builtin to check status
            jobs_cleanup();
        }
        
        // Reset history navigation position for new command
        // This allows UP arrow to start from most recent command
        history_reset_position();
        
        // ===== READ Phase =====
        // Read user input with advanced features:
        // - Line editing (arrow keys, backspace)
        // - Command history (UP/DOWN arrows)
        // - Tab completion for commands and files
        // - Emacs-style keybindings (Ctrl+A, Ctrl+E, etc.)
        char *input = terminal_readline(get_prompt());
        
        // Check for EOF (Ctrl+D pressed on empty line)
        if (input == NULL) {
            printf("\n");
            break;  // Exit shell gracefully
        }
        
        // Copy input to local buffer (input is dynamically allocated)
        strncpy(line, input, MAX_LINE - 1);
        line[MAX_LINE - 1] = '\0';  // Ensure null termination
        free(input);  // Free dynamically allocated input
        
        // Handle 'exit' command to quit shell
        if (strcmp(line, "exit") == 0) {
            break;
        }
        
        // Skip empty lines (just Enter pressed)
        if (strlen(line) == 0) {
            continue;
        }
        
        // Add non-empty command to history
        // This makes it available for UP arrow recall
        history_add(line);
        
        // ===== EXPAND Phase =====
        // Replace $variable references with their values
        // Example: "echo $name" -> "echo Alice"
        expand_variables_inplace(line, shell_env, MAX_LINE);
        
        // ===== PARSE Phase =====
        // Determine command type and parse accordingly
        
        // First, check if it's a conditional statement (if/then/else/fi)
        char *condition = NULL;
        char *then_block = NULL;
        char *else_block = NULL;
        
        int is_conditional = parse_conditional(line, &condition, &then_block, &else_block);
        
        if (is_conditional == 1) {
            // ===== EXECUTE Phase (Conditional) =====
            // Execute if/then/else logic
            // - Evaluate condition
            // - Run then_block if true, else_block if false
            int status = execute_conditional(condition, then_block, else_block, shell_env);
            last_exit_status = status;
            
            // Free dynamically allocated conditional parts
            free(condition);
            free(then_block);
            free(else_block);
        } else if (is_conditional == 0) {
            // ===== EXECUTE Phase (Pipeline/Simple Command) =====
            // Not a conditional - treat as pipeline or simple command
            // Examples:
            //   ls -la                    (simple command)
            //   cat file.txt | grep test  (pipeline)
            //   ls > output.txt           (with redirection)
            
            Command *commands = NULL;
            int count = 0;
            
            // Parse input into command structures
            if (parse_pipeline(line, &commands, &count) < 0) {
                fprintf(stderr, "ushell: parse error\n");
                continue;
            }
            
            // Execute parsed commands
            if (count > 0 && commands != NULL) {
                int status = execute_pipeline(commands, count, shell_env);
                last_exit_status = status;
                if (status == -1) {
                    fprintf(stderr, "ushell: execution failed\n");
                }
                // Free command structures
                free_pipeline(commands, count);
            }
        }
        // Note: is_conditional == -1 means parse error (already reported by parser)
    }
    
    // Cleanup handled by atexit(cleanup_shell)
    
    return 0;
}
