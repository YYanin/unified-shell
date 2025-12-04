#include "executor.h"
#include "builtins.h"
#include "tools.h"
#include "glob.h"
#include "jobs.h"
#include "signals.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ctype.h>
#include <errno.h>

#define MAX_EXPANDED_ARGS 1024

/**
 * Execute a command with fork/exec
 */
int execute_command(char **argv, Env *env) {
    if (argv == NULL || argv[0] == NULL) {
        return -1;
    }

    // Check if it's a built-in command
    builtin_func builtin = find_builtin(argv[0]);
    if (builtin != NULL) {
        // Execute built-in in parent process
        return builtin(argv, env);
    }

    // Check if it's an integrated tool
    tool_func tool = find_tool(argv[0]);
    if (tool != NULL) {
        // Count argc
        int argc = 0;
        while (argv[argc] != NULL) {
            argc++;
        }
        // Execute tool in parent process (tools handle their own output)
        return tool(argc, argv);
    }

    // Not a built-in or tool, fork/exec
    pid_t pid = fork();
    
    if (pid < 0) {
        perror("fork");
        return -1;
    }
    
    if (pid == 0) {
        // Child process
        execvp(argv[0], argv);
        // If execvp returns, it failed - provide detailed error
        if (errno == ENOENT) {
            fprintf(stderr, "ushell: command not found: %s\n", argv[0]);
        } else if (errno == EACCES) {
            fprintf(stderr, "ushell: permission denied: %s\n", argv[0]);
        } else {
            perror("ushell");
        }
        exit(127);
    } else {
        // Parent process - this is a foreground job
        foreground_job_pid = pid;
        
        int status;
        if (waitpid(pid, &status, WUNTRACED) < 0) {
            perror("waitpid");
            foreground_job_pid = 0;
            return -1;
        }
        
        // Check if process was stopped (Ctrl+Z)
        if (WIFSTOPPED(status)) {
            // Add stopped job to job list
            char cmd_line[256];
            int offset = 0;
            for (int i = 0; argv[i] != NULL && offset < 250; i++) {
                if (i > 0) {
                    cmd_line[offset++] = ' ';
                }
                offset += snprintf(cmd_line + offset, 256 - offset, "%s", argv[i]);
            }
            cmd_line[offset] = '\0';
            
            int job_id = jobs_add(pid, cmd_line, 0);
            if (job_id > 0) {
                Job *job = jobs_get(job_id);
                if (job) {
                    job->status = JOB_STOPPED;
                    printf("\n[%d]+  Stopped                 %s\n", job_id, cmd_line);
                }
            }
            foreground_job_pid = 0;
            return 0;
        }
        
        foreground_job_pid = 0;
        
        if (WIFEXITED(status)) {
            return WEXITSTATUS(status);
        }
        
        return -1;
    }
}

/**
 * Helper: Check if character is a delimiter
 */
static int is_delimiter(char c) {
    return (c == ' ' || c == '\t' || c == '\n');
}

/**
 * Tokenize command line into arguments
 * Handles basic quoted strings for space preservation
 */
char** tokenize_command(char *line) {
    if (line == NULL) {
        return NULL;
    }

    // Initial allocation for tokens
    int capacity = 16;
    int count = 0;
    char **tokens = malloc(capacity * sizeof(char*));
    if (tokens == NULL) {
        perror("malloc");
        return NULL;
    }

    char *ptr = line;
    
    while (*ptr != '\0') {
        // Skip leading whitespace
        while (is_delimiter(*ptr)) {
            ptr++;
        }
        
        if (*ptr == '\0') {
            break;
        }
        
        // Check if we need to expand tokens array
        if (count >= capacity - 1) {
            capacity *= 2;
            char **new_tokens = realloc(tokens, capacity * sizeof(char*));
            if (new_tokens == NULL) {
                perror("realloc");
                free_tokens(tokens);
                return NULL;
            }
            tokens = new_tokens;
        }
        
        // Handle quoted strings
        if (*ptr == '"' || *ptr == '\'') {
            char quote = *ptr;
            ptr++; // Skip opening quote
            
            char *start = ptr;
            while (*ptr != '\0' && *ptr != quote) {
                ptr++;
            }
            
            int len = ptr - start;
            tokens[count] = malloc(len + 1);
            if (tokens[count] == NULL) {
                perror("malloc");
                free_tokens(tokens);
                return NULL;
            }
            strncpy(tokens[count], start, len);
            tokens[count][len] = '\0';
            count++;
            
            if (*ptr == quote) {
                ptr++; // Skip closing quote
            }
        } else {
            // Regular token
            char *start = ptr;
            while (*ptr != '\0' && !is_delimiter(*ptr)) {
                ptr++;
            }
            
            int len = ptr - start;
            tokens[count] = malloc(len + 1);
            if (tokens[count] == NULL) {
                perror("malloc");
                free_tokens(tokens);
                return NULL;
            }
            strncpy(tokens[count], start, len);
            tokens[count][len] = '\0';
            count++;
        }
    }
    
    // NULL-terminate the array
    tokens[count] = NULL;
    
    return tokens;
}

/**
 * Free tokens array
 */
void free_tokens(char **argv) {
    if (argv == NULL) {
        return;
    }
    
    for (int i = 0; argv[i] != NULL; i++) {
        free(argv[i]);
    }
    free(argv);
}

/**
 * Expand glob patterns in argv
 * Returns new argv with globs expanded, or NULL on error
 */
char **expand_globs_in_argv(char **argv) {
    if (argv == NULL) {
        return NULL;
    }
    
    // Count arguments
    int argc = 0;
    while (argv[argc] != NULL) {
        argc++;
    }
    
    // Build new argv with expanded globs
    char **new_argv = malloc(sizeof(char *) * MAX_EXPANDED_ARGS);
    if (new_argv == NULL) {
        return NULL;
    }
    
    int new_argc = 0;
    for (int i = 0; i < argc; i++) {
        int match_count = 0;
        char **matches = expand_glob(argv[i], &match_count);
        
        if (matches != NULL && match_count > 0) {
            // Glob expanded - add all matches
            for (int j = 0; j < match_count; j++) {
                new_argv[new_argc++] = strdup(matches[j]);
            }
            free_glob_matches(matches, match_count);
        } else {
            // No expansion - keep literal
            new_argv[new_argc++] = strdup(argv[i]);
        }
    }
    
    new_argv[new_argc] = NULL;
    return new_argv;
}

/**
 * Parse a command line into a pipeline
 * Splits by | and handles < > >> redirections
 */
int parse_pipeline(char *line, Command **commands, int *count) {
    if (line == NULL || commands == NULL || count == NULL) {
        return -1;
    }

    *commands = NULL;
    *count = 0;

    // Check for background execution indicator (&)
    // Must be at the end of the entire command line
    int background = 0;
    char *line_copy = strdup(line);
    if (line_copy == NULL) {
        perror("strdup");
        return -1;
    }
    
    // Trim trailing whitespace and check for &
    char *end = line_copy + strlen(line_copy) - 1;
    while (end > line_copy && is_delimiter(*end)) {
        end--;
    }
    if (end > line_copy && *end == '&') {
        background = 1;
        *end = '\0';  // Remove the & from the command
        // Trim any whitespace before the &
        end--;
        while (end > line_copy && is_delimiter(*end)) {
            *end = '\0';
            end--;
        }
    }

    // Count pipes to allocate command array
    int pipe_count = 0;
    for (char *p = line_copy; *p; p++) {
        if (*p == '|') pipe_count++;
    }
    int cmd_count = pipe_count + 1;

    Command *cmds = calloc(cmd_count, sizeof(Command));
    if (cmds == NULL) {
        perror("calloc");
        free(line_copy);
        return -1;
    }

    // Parse each command segment
    char *cmd_start = line_copy;
    int cmd_idx = 0;

    for (char *p = line_copy; ; p++) {
        if (*p == '|' || *p == '\0') {
            char end_char = *p;
            *p = '\0';  // Null-terminate this segment

            // Parse this command segment for redirections
            char *segment = cmd_start;
            
            // Trim leading/trailing whitespace
            while (*segment && is_delimiter(*segment)) segment++;
            char *seg_end = segment + strlen(segment) - 1;
            while (seg_end > segment && is_delimiter(*seg_end)) {
                *seg_end = '\0';
                seg_end--;
            }

            // Parse redirections
            char *infile = NULL;
            char *outfile = NULL;
            int append = 0;
            
            // Simple tokenization for redirection detection
            char segment_copy[1024];
            strncpy(segment_copy, segment, sizeof(segment_copy) - 1);
            segment_copy[sizeof(segment_copy) - 1] = '\0';

            // Look for < > >>
            char *redir_ptr = segment_copy;
            char cmd_part[1024];
            int cmd_len = 0;

            while (*redir_ptr) {
                if (*redir_ptr == '<') {
                    *redir_ptr++ = '\0';
                    while (*redir_ptr && is_delimiter(*redir_ptr)) redir_ptr++;
                    char *filename_start = redir_ptr;
                    while (*redir_ptr && !is_delimiter(*redir_ptr) && *redir_ptr != '>' && *redir_ptr != '<') {
                        redir_ptr++;
                    }
                    char saved = *redir_ptr;
                    *redir_ptr = '\0';
                    if (infile) free(infile);
                    infile = strdup(filename_start);
                    *redir_ptr = saved;
                } else if (*redir_ptr == '>') {
                    if (*(redir_ptr + 1) == '>') {
                        append = 1;
                        *redir_ptr++ = '\0';
                        *redir_ptr++ = '\0';
                    } else {
                        append = 0;
                        *redir_ptr++ = '\0';
                    }
                    while (*redir_ptr && is_delimiter(*redir_ptr)) redir_ptr++;
                    char *filename_start = redir_ptr;
                    while (*redir_ptr && !is_delimiter(*redir_ptr) && *redir_ptr != '>' && *redir_ptr != '<') {
                        redir_ptr++;
                    }
                    char saved = *redir_ptr;
                    *redir_ptr = '\0';
                    if (outfile) free(outfile);
                    outfile = strdup(filename_start);
                    *redir_ptr = saved;
                } else {
                    cmd_part[cmd_len++] = *redir_ptr++;
                }
            }
            cmd_part[cmd_len] = '\0';

            // Tokenize the command part (without redirections)
            char **argv = tokenize_command(cmd_part);
            if (argv == NULL) {
                free(infile);
                free(outfile);
                free_pipeline(cmds, cmd_idx);
                free(line_copy);
                return -1;
            }
            
            // Expand glob patterns in arguments
            char **expanded_argv = expand_globs_in_argv(argv);
            free_tokens(argv);  // Free original argv
            if (expanded_argv == NULL) {
                free(infile);
                free(outfile);
                free_pipeline(cmds, cmd_idx);
                free(line_copy);
                return -1;
            }

            cmds[cmd_idx].argv = expanded_argv;
            cmds[cmd_idx].infile = infile;
            cmds[cmd_idx].outfile = outfile;
            cmds[cmd_idx].append = append;
            cmds[cmd_idx].background = background;  // Set background flag from line parse
            cmd_idx++;

            if (end_char == '\0') break;
            cmd_start = p + 1;
        }
    }

    free(line_copy);
    *commands = cmds;
    *count = cmd_count;
    return 0;
}

/**
 * Execute a pipeline of commands
 */
int execute_pipeline(Command *commands, int count, Env *env) {
    if (commands == NULL || count <= 0) {
        return -1;
    }

    // Single command without redirection or background - check if it's a built-in
    // Background jobs must go through full pipeline execution to handle job tracking
    if (count == 1 && commands[0].infile == NULL && commands[0].outfile == NULL && !commands[0].background) {
        return execute_command(commands[0].argv, env);
    }

    int pipes[count - 1][2];
    pid_t pids[count];

    // Create pipes
    for (int i = 0; i < count - 1; i++) {
        if (pipe(pipes[i]) < 0) {
            perror("pipe");
            return -1;
        }
    }

    // Fork and execute each command
    for (int i = 0; i < count; i++) {
        pids[i] = fork();

        if (pids[i] < 0) {
            perror("fork");
            return -1;
        }
        
        // Parent: set process group for child immediately after fork
        // This ensures the process group is set before any signals arrive
        if (pids[i] > 0) {
            if (i == 0) {
                // First process becomes group leader (pgid = pid)
                setpgid(pids[i], pids[i]);
            } else {
                // Other processes join first process's group
                setpgid(pids[i], pids[0]);
            }
        }

        if (pids[i] == 0) {
            // Child process
            
            // Put process in its own process group for job control
            // This allows the shell to manage it independently with signals
            // For pipelines, all processes should be in the same group
            if (i == 0) {
                // First process in pipeline becomes group leader
                setpgid(0, 0);
            } else {
                // Other processes join the first process's group
                setpgid(0, pids[0]);
            }

            // Set up input redirection
            if (i == 0) {
                // First command - check for input file
                if (commands[i].infile != NULL) {
                    int fd = open(commands[i].infile, O_RDONLY);
                    if (fd < 0) {
                        perror(commands[i].infile);
                        exit(1);
                    }
                    dup2(fd, STDIN_FILENO);
                    close(fd);
                }
            } else {
                // Not first - read from previous pipe
                dup2(pipes[i - 1][0], STDIN_FILENO);
            }

            // Set up output redirection
            if (i == count - 1) {
                // Last command - check for output file
                if (commands[i].outfile != NULL) {
                    int flags = O_WRONLY | O_CREAT;
                    flags |= commands[i].append ? O_APPEND : O_TRUNC;
                    int fd = open(commands[i].outfile, flags, 0644);
                    if (fd < 0) {
                        perror(commands[i].outfile);
                        exit(1);
                    }
                    dup2(fd, STDOUT_FILENO);
                    close(fd);
                }
            } else {
                // Not last - write to next pipe
                dup2(pipes[i][1], STDOUT_FILENO);
            }

            // Close all pipe fds in child
            for (int j = 0; j < count - 1; j++) {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }

            // Check for built-in commands (run in child for pipelines)
            builtin_func builtin = find_builtin(commands[i].argv[0]);
            if (builtin != NULL) {
                int ret = builtin(commands[i].argv, env);
                exit(ret);
            }

            // Check for integrated tools
            tool_func tool = find_tool(commands[i].argv[0]);
            if (tool != NULL) {
                int argc = 0;
                while (commands[i].argv[argc] != NULL) {
                    argc++;
                }
                int ret = tool(argc, commands[i].argv);
                exit(ret);
            }

            // Execute external command
            execvp(commands[i].argv[0], commands[i].argv);
            fprintf(stderr, "ushell: command not found: %s\n", commands[i].argv[0]);
            exit(127);
        }
    }

    // Parent closes all pipes
    for (int i = 0; i < count - 1; i++) {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }

    // Check if this is a background job
    int is_background = commands[0].background;
    
    if (is_background) {
        // Background job: don't wait, add to job list
        // For pipelines, track the last process (rightmost command)
        pid_t job_pid = pids[count - 1];
        
        // Reconstruct full command line for job display
        char cmd_line[MAX_CMD_LEN];
        int offset = 0;
        for (int i = 0; i < count && offset < MAX_CMD_LEN - 10; i++) {
            if (i > 0) {
                offset += snprintf(cmd_line + offset, MAX_CMD_LEN - offset, " | ");
            }
            for (int j = 0; commands[i].argv[j] != NULL && offset < MAX_CMD_LEN - 10; j++) {
                if (j > 0) {
                    cmd_line[offset++] = ' ';
                }
                offset += snprintf(cmd_line + offset, MAX_CMD_LEN - offset, "%s", commands[i].argv[j]);
            }
        }
        if (offset < MAX_CMD_LEN - 2) {
            cmd_line[offset++] = ' ';
            cmd_line[offset++] = '&';
            cmd_line[offset] = '\0';
        }
        
        // Add job to tracking system
        int job_id = jobs_add(job_pid, cmd_line, 1);
        if (job_id > 0) {
            printf("[%d] %d\n", job_id, job_pid);
            fflush(stdout);  // Make sure it prints immediately
        }
        
        return 0;  // Background jobs always return success to shell
    } else {
        // Foreground job: wait for all children as normal
        // Set foreground_job_pid so signals (Ctrl+C, Ctrl+Z) go to the job
        foreground_job_pid = pids[count - 1];  // Track last process in pipeline
        
        int last_status = 0;
        for (int i = 0; i < count; i++) {
            int status;
            if (waitpid(pids[i], &status, WUNTRACED) < 0) {
                perror("waitpid");
            }
            
            // Check if process was stopped (Ctrl+Z)
            if (WIFSTOPPED(status)) {
                // Job was stopped - add to job list
                char cmd_line[MAX_CMD_LEN];
                int offset = 0;
                for (int j = 0; j < count && offset < MAX_CMD_LEN - 10; j++) {
                    if (j > 0) {
                        offset += snprintf(cmd_line + offset, MAX_CMD_LEN - offset, " | ");
                    }
                    for (int k = 0; commands[j].argv[k] != NULL && offset < MAX_CMD_LEN - 10; k++) {
                        if (k > 0) {
                            cmd_line[offset++] = ' ';
                        }
                        offset += snprintf(cmd_line + offset, MAX_CMD_LEN - offset, "%s", commands[j].argv[k]);
                    }
                }
                cmd_line[offset] = '\0';
                
                int job_id = jobs_add(pids[count - 1], cmd_line, 0);
                if (job_id > 0) {
                    Job *job = jobs_get(job_id);
                    if (job) {
                        job->status = JOB_STOPPED;
                        printf("\n[%d]+  Stopped                 %s\n", job_id, cmd_line);
                    }
                }
                foreground_job_pid = 0;  // Clear foreground job
                return 0;
            }
            
            if (i == count - 1 && WIFEXITED(status)) {
                last_status = WEXITSTATUS(status);
            }
        }
        
        // Clear foreground job when done
        foreground_job_pid = 0;
        
        return last_status;
    }
}

/**
 * Free pipeline commands
 */
void free_pipeline(Command *commands, int count) {
    if (commands == NULL) {
        return;
    }

    for (int i = 0; i < count; i++) {
        free_tokens(commands[i].argv);
        free(commands[i].infile);
        free(commands[i].outfile);
    }
    free(commands);
}
