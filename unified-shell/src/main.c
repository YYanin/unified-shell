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

/**
 * get_shell_state_json - Gather current shell state as JSON for AI context
 * 
 * Collects information about the current shell session to help the AI
 * provide better, context-aware command suggestions. The JSON includes:
 * - Current working directory (cwd)
 * - Current user (user)
 * - Recent command history (last 5 commands)
 * - Shell environment variables (excluding sensitive ones)
 * 
 * Privacy: Automatically filters out variables containing sensitive keywords
 * like PASSWORD, TOKEN, KEY, SECRET, etc. from the environment variables.
 * 
 * Returns:
 *   Malloc'd JSON string with shell state (caller must free)
 *   NULL on error
 * 
 * Example JSON output:
 *   {"cwd":"/home/user","user":"user","history":["ls","cd tmp"],
 *    "env":{"PATH":"/usr/bin","HOME":"/home/user"}}
 */
char* get_shell_state_json(void) {
    // Allocate buffer for JSON output
    char *json = malloc(MAX_LINE * 4);  // Large buffer for JSON
    if (!json) {
        return NULL;
    }
    
    char *pos = json;
    int remaining = MAX_LINE * 4;
    
    // Start JSON object
    int written = snprintf(pos, remaining, "{");
    if (written >= remaining) goto overflow;
    pos += written;
    remaining -= written;
    
    // Add current working directory
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd))) {
        // Escape backslashes and quotes in cwd
        char escaped_cwd[PATH_MAX * 2];
        const char *src = cwd;
        char *dst = escaped_cwd;
        while (*src && (dst - escaped_cwd) < (int)sizeof(escaped_cwd) - 2) {
            if (*src == '"' || *src == '\\') {
                *dst++ = '\\';
            }
            *dst++ = *src++;
        }
        *dst = '\0';
        
        written = snprintf(pos, remaining, "\"cwd\":\"%s\"", escaped_cwd);
        if (written >= remaining) goto overflow;
        pos += written;
        remaining -= written;
    }
    
    // Add current user
    const char *user = getenv("USER");
    if (user) {
        written = snprintf(pos, remaining, ",\"user\":\"%s\"", user);
        if (written >= remaining) goto overflow;
        pos += written;
        remaining -= written;
    }
    
    // Add recent history (last 5 commands)
    written = snprintf(pos, remaining, ",\"history\":[");
    if (written >= remaining) goto overflow;
    pos += written;
    remaining -= written;
    
    int history_size = history_count();
    int commands_to_show = (history_size < 5) ? history_size : 5;
    for (int i = 0; i < commands_to_show; i++) {
        const char *cmd = history_get(i);
        if (cmd) {
            // Escape quotes and backslashes in command
            char escaped_cmd[MAX_LINE * 2];
            const char *src = cmd;
            char *dst = escaped_cmd;
            while (*src && (dst - escaped_cmd) < (int)sizeof(escaped_cmd) - 2) {
                if (*src == '"' || *src == '\\') {
                    *dst++ = '\\';
                }
                *dst++ = *src++;
            }
            *dst = '\0';
            
            written = snprintf(pos, remaining, "%s\"%s\"", (i > 0) ? "," : "", escaped_cmd);
            if (written >= remaining) goto overflow;
            pos += written;
            remaining -= written;
        }
    }
    
    written = snprintf(pos, remaining, "]");
    if (written >= remaining) goto overflow;
    pos += written;
    remaining -= written;
    
    // Add environment variables (excluding sensitive ones)
    written = snprintf(pos, remaining, ",\"env\":{");
    if (written >= remaining) goto overflow;
    pos += written;
    remaining -= written;
    
    // Access extern environ to iterate through environment variables
    extern char **environ;
    int first_var = 1;
    for (char **env = environ; *env != NULL; env++) {
        char *var = *env;
        char *equals = strchr(var, '=');
        if (equals) {
            // Extract variable name
            int name_len = equals - var;
            char var_name[256];
            if (name_len >= (int)sizeof(var_name)) continue;
            
            strncpy(var_name, var, name_len);
            var_name[name_len] = '\0';
            
            // Filter out sensitive variables
            if (strstr(var_name, "PASSWORD") || strstr(var_name, "TOKEN") ||
                strstr(var_name, "KEY") || strstr(var_name, "SECRET") ||
                strstr(var_name, "CREDENTIAL")) {
                continue;  // Skip sensitive variables
            }
            
            // Get variable value
            const char *value = equals + 1;
            
            // Escape quotes and backslashes in name and value
            char escaped_name[512];
            char escaped_value[MAX_LINE];
            
            const char *src = var_name;
            char *dst = escaped_name;
            while (*src && (dst - escaped_name) < (int)sizeof(escaped_name) - 2) {
                if (*src == '"' || *src == '\\') {
                    *dst++ = '\\';
                }
                *dst++ = *src++;
            }
            *dst = '\0';
            
            src = value;
            dst = escaped_value;
            int val_len = 0;
            while (*src && val_len < 200 && (dst - escaped_value) < (int)sizeof(escaped_value) - 2) {
                if (*src == '"' || *src == '\\') {
                    *dst++ = '\\';
                }
                *dst++ = *src++;
                val_len++;
            }
            *dst = '\0';
            
            // Add comma before each variable except first
            written = snprintf(pos, remaining, "%s\"%s\":\"%s\"",
                             first_var ? "" : ",", escaped_name, escaped_value);
            if (written >= remaining) goto overflow;
            pos += written;
            remaining -= written;
            first_var = 0;
        }
    }
    
    // Close env object and main JSON object
    written = snprintf(pos, remaining, "}}");
    if (written >= remaining) goto overflow;
    
    return json;
    
overflow:
    free(json);
    fprintf(stderr, "ushell: JSON buffer overflow in get_shell_state_json\n");
    return NULL;
}

/**
 * call_ai_helper - Execute Python AI helper script and capture output
 * 
 * Runs the ushell_ai.py script with the user's query and captures the
 * suggested command from stdout. The script path can be configured via
 * the USHELL_AI_HELPER environment variable.
 * 
 * The function:
 * - Checks if the script exists and is executable
 * - Builds command line with proper argument escaping
 * - Uses popen() to execute and capture output
 * - Reads exactly one line from stdout (the suggestion)
 * - Returns dynamically allocated string (caller must free)
 * 
 * Args:
 *   query: Natural language query to send to AI helper
 * 
 * Returns:
 *   Malloc'd string containing suggested command (caller must free)
 *   NULL on error (script not found, execution failed, empty output)
 * 
 * Example:
 *   char *suggestion = call_ai_helper("list all files");
 *   if (suggestion) {
 *       printf("Suggestion: %s\n", suggestion);
 *       free(suggestion);
 *   }
 */
char* call_ai_helper(const char *query) {
    // Get AI helper script path from environment, or use default
    const char *helper_path = getenv("USHELL_AI_HELPER");
    if (helper_path == NULL) {
        // Default: look for script in aiIntegr subdirectory
        helper_path = "./aiIntegr/ushell_ai.py";
    }
    
    // Check if script exists and is executable
    if (access(helper_path, X_OK) != 0) {
        fprintf(stderr, "ushell: AI helper not found or not executable: %s\n", helper_path);
        fprintf(stderr, "Set USHELL_AI_HELPER environment variable to specify location.\n");
        return NULL;
    }
    
    // Check if context passing is enabled (default: yes)
    const char *context_enabled_str = getenv("USHELL_AI_CONTEXT");
    int context_enabled = 1;  // Default to enabled
    if (context_enabled_str != NULL && strcmp(context_enabled_str, "0") == 0) {
        context_enabled = 0;
    }
    
    // Generate shell state context if enabled
    char *context_json = NULL;
    char context_file[PATH_MAX] = {0};
    if (context_enabled) {
        context_json = get_shell_state_json();
        if (context_json) {
            // Create temporary file for context
            strcpy(context_file, "/tmp/ushell_context_XXXXXX");
            int fd = mkstemp(context_file);
            if (fd != -1) {
                // Write context JSON to file
                ssize_t written = write(fd, context_json, strlen(context_json));
                close(fd);
                
                if (written < 0) {
                    fprintf(stderr, "ushell: failed to write context file\n");
                    unlink(context_file);
                    context_file[0] = '\0';
                }
            } else {
                fprintf(stderr, "ushell: failed to create context temp file\n");
                context_file[0] = '\0';
            }
            free(context_json);
        }
    }
    
    // Build command line with escaped query
    // Format: <script_path> [--context <file>] "<query>"
    // We need to escape quotes in the query to prevent shell injection
    char command[MAX_LINE * 2];
    char escaped_query[MAX_LINE * 2];
    
    // Simple escape: replace " with \"
    const char *src = query;
    char *dst = escaped_query;
    while (*src && (dst - escaped_query) < (int)sizeof(escaped_query) - 2) {
        if (*src == '"' || *src == '\\' || *src == '$' || *src == '`') {
            *dst++ = '\\';  // Add escape character
        }
        *dst++ = *src++;
    }
    *dst = '\0';
    
    // Build full command with optional context
    if (context_file[0] != '\0') {
        snprintf(command, sizeof(command), "%s --context \"%s\" \"%s\"", 
                 helper_path, context_file, escaped_query);
    } else {
        snprintf(command, sizeof(command), "%s \"%s\"", helper_path, escaped_query);
    }
    
    // Execute command and capture output
    FILE *fp = popen(command, "r");
    if (fp == NULL) {
        fprintf(stderr, "ushell: failed to execute AI helper\n");
        if (context_file[0] != '\0') {
            unlink(context_file);
        }
        return NULL;
    }
    
    // Read suggestion from stdout (exactly one line)
    char suggestion[MAX_LINE];
    if (fgets(suggestion, sizeof(suggestion), fp) == NULL) {
        pclose(fp);
        if (context_file[0] != '\0') {
            unlink(context_file);
        }
        fprintf(stderr, "ushell: AI helper produced no output\n");
        return NULL;
    }
    
    // Close pipe and check exit status
    int status = pclose(fp);
    if (status != 0) {
        fprintf(stderr, "ushell: AI helper exited with status %d\n", status);
        // Continue anyway - we might have gotten a suggestion
    }
    
    // Clean up context file if created
    if (context_file[0] != '\0') {
        unlink(context_file);
    }
    
    // Remove trailing newline if present
    size_t len = strlen(suggestion);
    if (len > 0 && suggestion[len - 1] == '\n') {
        suggestion[len - 1] = '\0';
    }
    
    // Check if suggestion is empty
    if (strlen(suggestion) == 0) {
        fprintf(stderr, "ushell: AI helper returned empty suggestion\n");
        return NULL;
    }
    
    // Return malloc'd copy (caller must free)
    return strdup(suggestion);
}

/**
 * read_confirmation - Read user's y/n/e confirmation response
 * 
 * Reads input from stdin and returns user's choice.
 * Accepts: y/Y (yes), n/N (no), e/E (edit), or Enter (defaults to no)
 * 
 * Returns:
 *   'y', 'n', or 'e' - user's choice (lowercase)
 *   'n' on Enter (safe default) or EOF
 */
char read_confirmation(void) {
    char buffer[10];
    
    // Read user input
    if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
        // EOF or error - treat as cancel
        return 'n';
    }
    
    // Get first character
    char response = buffer[0];
    
    // Handle Enter key (empty input) as 'n' (safe default)
    if (response == '\n') {
        return 'n';
    }
    
    // Convert to lowercase
    if (response >= 'A' && response <= 'Z') {
        response = response + ('a' - 'A');
    }
    
    // Validate and return
    if (response == 'y' || response == 'n' || response == 'e') {
        return response;
    }
    
    // Invalid input - treat as 'n' (safe default)
    return 'n';
}

/**
 * execute_ai_suggestion - Execute an AI-suggested command
 * 
 * Takes a command string, parses it, and executes it through the
 * normal shell command pipeline. Handles variable expansion,
 * conditionals, and pipelines. Adds command to history.
 * 
 * Args:
 *   cmd: The command string to execute (not NULL)
 * 
 * Returns:
 *   Exit status of the executed command, or -1 on error
 */
int execute_ai_suggestion(char *cmd) {
    if (cmd == NULL || strlen(cmd) == 0) {
        return -1;
    }
    
    // Make a working copy (expansion modifies the string)
    char line[MAX_LINE];
    strncpy(line, cmd, MAX_LINE - 1);
    line[MAX_LINE - 1] = '\0';
    
    // Add original command to history before execution
    history_add(cmd);
    
    // Expand variables (e.g., $HOME, $USER)
    expand_variables_inplace(line, shell_env, MAX_LINE);
    
    // Parse and execute conditionals (if/then/else)
    char *condition = NULL;
    char *then_block = NULL;
    char *else_block = NULL;
    
    int is_conditional = parse_conditional(line, &condition, &then_block, &else_block);
    
    if (is_conditional == 1) {
        // Execute conditional statement
        int status = execute_conditional(condition, then_block, else_block, shell_env);
        free(condition);
        free(then_block);
        free(else_block);
        return status;
    } else if (is_conditional == 0) {
        // Execute pipeline or simple command
        Command *commands = NULL;
        int count = 0;
        
        if (parse_pipeline(line, &commands, &count) < 0) {
            fprintf(stderr, "ushell: parse error in AI suggestion\n");
            return -1;
        }
        
        if (count > 0 && commands != NULL) {
            int status = execute_pipeline(commands, count, shell_env);
            free_pipeline(commands, count);
            return status;
        }
    }
    
    // Parse error or invalid command
    return -1;
}

/**
 * handle_ai_query - Process AI query starting with @ symbol
 * 
 * When user enters a line starting with @, this function:
 * 1. Validates the query is not empty
 * 2. Calls the Python AI helper script to get a suggestion
 * 3. Displays the suggestion to the user
 * 4. Prompts for user confirmation (y/n/e)
 * 5. Executes command if user confirms with 'y'
 * 6. Allows editing if user chooses 'e'
 * 7. Cancels if user chooses 'n' or Enter
 * 
 * Args:
 *   query: The query string after the @ symbol (whitespace trimmed)
 * 
 * Returns:
 *   0 on success, -1 on error (empty query or helper failure)
 * 
 * Examples:
 *   "@list all files" -> gets suggestion, prompts for confirmation
 *   "@" (empty) -> prints error and returns -1
 */
int handle_ai_query(const char *query) {
    // Validate query is not empty
    if (query == NULL || strlen(query) == 0) {
        fprintf(stderr, "ushell: AI query error: empty query after @\n");
        fprintf(stderr, "\nUsage: @<natural language query>\n");
        fprintf(stderr, "  Ask the AI to suggest commands based on natural language.\n\n");
        fprintf(stderr, "Examples:\n");
        fprintf(stderr, "  @list all c files\n");
        fprintf(stderr, "  @find files modified today\n");
        fprintf(stderr, "  @show disk usage\n\n");
        fprintf(stderr, "Confirmation options:\n");
        fprintf(stderr, "  y - Execute the suggested command\n");
        fprintf(stderr, "  n - Cancel and return to prompt\n");
        fprintf(stderr, "  e - Edit the suggestion before executing\n\n");
        fprintf(stderr, "See 'help' for more information.\n");
        return -1;
    }
    
    // Call AI helper to get command suggestion
    char *suggestion = call_ai_helper(query);
    
    if (suggestion == NULL) {
        // Error already printed by call_ai_helper
        return -1;
    }
    
    // Display suggestion to user
    printf("AI Suggestion: %s\n", suggestion);
    
    // Prompt for user confirmation
    printf("Execute this command? (y/n/e): ");
    fflush(stdout);
    
    char response = read_confirmation();
    
    if (response == 'y') {
        // User confirmed - execute the suggestion
        printf("Executing: %s\n", suggestion);
        int status = execute_ai_suggestion(suggestion);
        free(suggestion);
        return status;
        
    } else if (response == 'e') {
        // User wants to edit the command
        printf("Edit command: %s\n", suggestion);
        printf("Enter edited command (or press Enter to cancel): ");
        fflush(stdout);
        
        char edited[MAX_LINE];
        if (fgets(edited, sizeof(edited), stdin) != NULL) {
            // Remove trailing newline
            size_t len = strlen(edited);
            if (len > 0 && edited[len-1] == '\n') {
                edited[len-1] = '\0';
            }
            
            // Check if user entered something
            if (strlen(edited) > 0) {
                printf("Executing: %s\n", edited);
                int status = execute_ai_suggestion(edited);
                free(suggestion);
                return status;
            }
        }
        
        // User cancelled edit or entered empty string
        printf("Edit cancelled.\n");
        free(suggestion);
        return 0;
        
    } else {
        // User cancelled (chose 'n' or pressed Enter)
        printf("Command cancelled.\n");
        free(suggestion);
        return 0;
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
 * Special flags:
 *   --commands-json: Print JSON catalog and exit (for AI helper)
 * 
 * Returns: 0 on normal exit
 */
int main(int argc, char **argv) {
    // Check for --commands-json flag before initialization
    // This allows AI helper to query available commands
    if (argc == 2 && strcmp(argv[1], "--commands-json") == 0) {
        // Call commands builtin with --json flag
        char *cmd_argv[] = {"commands", "--json", NULL};
        builtin_commands(cmd_argv, NULL);
        return 0;
    }
    
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
        
        // ===== AI Query Detection =====
        // Check if line starts with @ (after trimming leading whitespace)
        // This triggers AI-assisted command suggestion mode
        const char *trimmed = line;
        while (*trimmed == ' ' || *trimmed == '\t') {
            trimmed++;  // Skip leading whitespace
        }
        
        if (*trimmed == '@') {
            // Extract query after @ symbol
            const char *query = trimmed + 1;  // Skip the @ character
            
            // Trim leading whitespace from query
            while (*query == ' ' || *query == '\t') {
                query++;
            }
            
            // Process AI query (don't add to history yet)
            // The original "@..." line will be added to history
            // If user accepts suggestion, the executed command will also be added
            history_add(line);  // Add the @ query to history
            
            // Handle the AI query
            handle_ai_query(query);
            
            // Skip normal command processing
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
