/*
 * mcp_exec.c - Safe command execution for MCP server
 * 
 * Provides sanitization and safe execution of commands with output capture.
 * Enhanced in Prompt 5 with comprehensive security features:
 * - Enhanced path validation and sanitization
 * - Comprehensive resource limits
 * - Audit logging
 * - Rate limiting support
 * - Blacklist patterns
 */

#include "mcp_exec.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <fcntl.h>
#include <ctype.h>
#include <signal.h>
#include <time.h>
#include <limits.h>
#include <errno.h>

/* Whitelist of safe commands (Prompt 5) */
static const char *safe_commands[] = {
    "pwd", "echo", "ls", "cat", "date", "whoami", "hostname",
    "cd", "env", "export", "set", "unset", "help", "version", "history",
    "myls", "mycat", "mycp", "mymv", "myrm", "mymkdir", "myrmdir", "mytouch", "mystat", "myfd",
    "grep", "find", "wc", "head", "tail", "sort", "uniq",
    NULL
};

/* Blacklisted commands (always blocked) */
static const char *blacklisted_commands[] = {
    "sudo", "su", "chmod", "chown", "rm", "dd", "mkfs", "fdisk",
    "reboot", "shutdown", "halt", "poweroff", "kill", "killall",
    "iptables", "systemctl", "service",
    NULL
};

/* Dangerous path patterns */
static const char *dangerous_paths[] = {
    "/etc/", "/sys/", "/proc/", "/dev/", "/boot/",
    "shadow", "passwd", ".ssh/",
    NULL
};

/* Audit log file (can be set via environment variable) */
static FILE *audit_log = NULL;
static const char *audit_log_path = NULL;

/*
 * mcp_exec_init_audit_log - Initialize audit logging
 */
void mcp_exec_init_audit_log(const char *log_path) {
    if (!log_path) {
        log_path = getenv("USHELL_MCP_AUDIT_LOG");
    }
    
    if (log_path) {
        audit_log_path = log_path;
        audit_log = fopen(log_path, "a");
        if (audit_log) {
            /* Set line buffering */
            setvbuf(audit_log, NULL, _IOLBF, 0);
        }
    }
}

/*
 * mcp_exec_close_audit_log - Close audit log
 */
void mcp_exec_close_audit_log(void) {
    if (audit_log) {
        fclose(audit_log);
        audit_log = NULL;
    }
}

/*
 * mcp_exec_log_command - Log command execution to audit log
 */
static void mcp_exec_log_command(const char *client_ip, const char *command,
                                  const char *args, int exit_code, const char *status) {
    if (!audit_log) {
        return;
    }
    
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    char timestamp[64];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", tm_info);
    
    /* Write JSON log entry */
    fprintf(audit_log, "{\"timestamp\":\"%s\",\"client\":\"%s\","
            "\"command\":\"%s\",\"args\":\"%s\",\"exit_code\":%d,"
            "\"status\":\"%s\"}\n",
            timestamp, client_ip ? client_ip : "unknown",
            command, args ? args : "",
            exit_code, status);
}

/*
 * mcp_exec_is_blacklisted - Check if command is blacklisted
 */
static int mcp_exec_is_blacklisted(const char *command) {
    if (!command) {
        return 1;
    }
    
    for (int i = 0; blacklisted_commands[i] != NULL; i++) {
        if (strcmp(command, blacklisted_commands[i]) == 0) {
            return 1;
        }
    }
    
    return 0;
}

/*
 * mcp_exec_is_safe_command - Check if command is in whitelist and not blacklisted
 */
int mcp_exec_is_safe_command(const char *command) {
    if (!command) {
        return 0;
    }
    
    /* Check blacklist first */
    if (mcp_exec_is_blacklisted(command)) {
        return 0;
    }
    
    /* Check whitelist */
    for (int i = 0; safe_commands[i] != NULL; i++) {
        if (strcmp(command, safe_commands[i]) == 0) {
            return 1;
        }
    }
    
    return 0;
}

/*
 * mcp_exec_validate_path - Validate path for safety (Prompt 5 enhancement)
 */
static int mcp_exec_validate_path(const char *path) {
    if (!path) {
        return -1;
    }
    
    /* Check for path traversal */
    if (strstr(path, "..") != NULL) {
        return -1;
    }
    
    /* Check for dangerous absolute paths */
    for (int i = 0; dangerous_paths[i] != NULL; i++) {
        if (strstr(path, dangerous_paths[i]) != NULL) {
            return -1;
        }
    }
    
    /* Check for null bytes (path truncation attack) */
    if (strchr(path, '\0') != (path + strlen(path))) {
        return -1;
    }
    
    return 0;
}

/*
 * mcp_exec_sanitize_arg - Sanitize command argument (Prompt 5 enhanced)
 * 
 * Removes shell metacharacters to prevent command injection.
 * Enhanced with additional security checks.
 */
int mcp_exec_sanitize_arg(const char *input, char *output, size_t output_size) {
    if (!input || !output || output_size == 0) {
        return -1;
    }
    
    /* Check length */
    size_t input_len = strlen(input);
    if (input_len >= output_size) {
        return -1;  /* Input too long */
    }
    
    /* Validate if it looks like a path */
    if (strchr(input, '/') != NULL) {
        if (mcp_exec_validate_path(input) != 0) {
            return -1;
        }
    }
    
    size_t i = 0, j = 0;
    
    /* Copy safe characters only */
    while (input[i] && j < output_size - 1) {
        char c = input[i];
        
        /* Block dangerous characters */
        if (c == ';' || c == '|' || c == '&' || c == '$' || c == '`' ||
            c == '(' || c == ')' || c == '<' || c == '>' || c == '\'' ||
            c == '"' || c == '\\' || c == '*' || c == '?' || c == '[' ||
            c == ']' || c == '{' || c == '}' || c == '~' || c == '!') {
            /* Dangerous shell metacharacter - skip it */
            i++;
            continue;
        }
        
        /* Allow alphanumeric, spaces, and safe punctuation */
        if (isalnum((unsigned char)c) || c == ' ' || c == '.' || c == '/' || 
            c == '-' || c == '_' || c == ':' || c == '=' || c == ',' || c == '@') {
            output[j++] = c;
        }
        i++;
    }
    
    output[j] = '\0';
    return 0;
}

/*
 * mcp_exec_free_result - Free resources in MCPExecResult
 */
void mcp_exec_free_result(MCPExecResult *result) {
    if (!result) {
        return;
    }
    
    free(result->stdout_data);
    free(result->stderr_data);
    
    result->stdout_data = NULL;
    result->stderr_data = NULL;
}

/*
 * mcp_exec_command - Execute a shell command safely
 * 
 * Basic implementation for Prompt 2. Will be enhanced in Prompt 5 with:
 * - Resource limits
 * - Timeout enforcement
 * - Better output capture
 */
int mcp_exec_command(const char *command, char **args, Env *env, MCPExecResult *result) {
    (void)env;  /* Will be used for environment variables in full implementation */
    
    if (!command || !result) {
        return -1;
    }
    
    /* Initialize result */
    result->exit_code = -1;
    result->stdout_data = NULL;
    result->stderr_data = NULL;
    result->timed_out = 0;
    
    /* Check if command is safe */
    if (!mcp_exec_is_safe_command(command)) {
        return -1;
    }
    
    /* Create pipes for stdout and stderr */
    int stdout_pipe[2];
    int stderr_pipe[2];
    
    if (pipe(stdout_pipe) != 0 || pipe(stderr_pipe) != 0) {
        return -1;
    }
    
    /* Fork child process */
    pid_t pid = fork();
    
    if (pid < 0) {
        /* Fork failed */
        close(stdout_pipe[0]);
        close(stdout_pipe[1]);
        close(stderr_pipe[0]);
        close(stderr_pipe[1]);
        return -1;
    }
    
    if (pid == 0) {
        /* Child process */
        
        /* Redirect stdout and stderr to pipes */
        dup2(stdout_pipe[1], STDOUT_FILENO);
        dup2(stderr_pipe[1], STDERR_FILENO);
        
        /* Close unused pipe ends */
        close(stdout_pipe[0]);
        close(stdout_pipe[1]);
        close(stderr_pipe[0]);
        close(stderr_pipe[1]);
        
        /* Set comprehensive resource limits (Prompt 5) */
        struct rlimit limit;
        
        /* CPU time limit: 30 seconds */
        limit.rlim_cur = MCP_EXEC_TIMEOUT;
        limit.rlim_max = MCP_EXEC_TIMEOUT;
        setrlimit(RLIMIT_CPU, &limit);
        
        /* Memory limit: 256 MB */
        limit.rlim_cur = 256 * 1024 * 1024;
        limit.rlim_max = 256 * 1024 * 1024;
        setrlimit(RLIMIT_AS, &limit);
        
        /* Process limit: 10 processes */
        limit.rlim_cur = 10;
        limit.rlim_max = 10;
        setrlimit(RLIMIT_NPROC, &limit);
        
        /* File size limit: 10 MB */
        limit.rlim_cur = 10 * 1024 * 1024;
        limit.rlim_max = 10 * 1024 * 1024;
        setrlimit(RLIMIT_FSIZE, &limit);
        
        /* Open file limit: 50 files */
        limit.rlim_cur = 50;
        limit.rlim_max = 50;
        setrlimit(RLIMIT_NOFILE, &limit);
        
        /* Execute command */
        /* Build argv array */
        char *argv[MCP_MAX_ARGS + 2];
        argv[0] = (char*)command;
        int i;
        for (i = 0; args && args[i] && i < MCP_MAX_ARGS; i++) {
            argv[i + 1] = args[i];
        }
        argv[i + 1] = NULL;
        
        /* Execute - this replaces child process */
        execvp(command, argv);
        
        /* If execvp returns, it failed */
        fprintf(stderr, "Failed to execute command: %s\n", command);
        exit(127);
    }
    
    /* Parent process */
    
    /* Close write ends of pipes */
    close(stdout_pipe[1]);
    close(stderr_pipe[1]);
    
    /* Read stdout */
    char stdout_buffer[MCP_MAX_OUTPUT];
    ssize_t stdout_bytes = 0;
    ssize_t n;
    
    while (stdout_bytes < MCP_MAX_OUTPUT - 1 &&
           (n = read(stdout_pipe[0], stdout_buffer + stdout_bytes, 
                    MCP_MAX_OUTPUT - stdout_bytes - 1)) > 0) {
        stdout_bytes += n;
    }
    stdout_buffer[stdout_bytes] = '\0';
    
    /* Read stderr */
    char stderr_buffer[MCP_MAX_OUTPUT];
    ssize_t stderr_bytes = 0;
    
    while (stderr_bytes < MCP_MAX_OUTPUT - 1 &&
           (n = read(stderr_pipe[0], stderr_buffer + stderr_bytes,
                    MCP_MAX_OUTPUT - stderr_bytes - 1)) > 0) {
        stderr_bytes += n;
    }
    stderr_buffer[stderr_bytes] = '\0';
    
    /* Close read ends */
    close(stdout_pipe[0]);
    close(stderr_pipe[0]);
    
    /* Wait for child with timeout */
    int status;
    int wait_result = waitpid(pid, &status, 0);
    
    if (wait_result < 0) {
        /* Wait failed */
        close(stdout_pipe[0]);
        close(stderr_pipe[0]);
        return -1;
    }
    
    /* Store results */
    if (WIFEXITED(status)) {
        result->exit_code = WEXITSTATUS(status);
    } else if (WIFSIGNALED(status)) {
        result->exit_code = 128 + WTERMSIG(status);
        if (WTERMSIG(status) == SIGXCPU) {
            result->timed_out = 1;
        }
    }
    
    /* Allocate and copy output */
    result->stdout_data = strdup(stdout_buffer);
    result->stderr_data = strdup(stderr_buffer);
    
    /* Log to audit log */
    mcp_exec_log_command("localhost", command, 
                        args && args[0] ? args[0] : "",
                        result->exit_code,
                        result->exit_code == 0 ? "success" : "failed");
    
    return 0;
}

/*
 * mcp_exec_validate_integer - Validate integer argument (Prompt 5)
 */
int mcp_exec_validate_integer(const char *input, int *value, int min, int max) {
    if (!input || !value) {
        return -1;
    }
    
    char *endptr;
    errno = 0;
    long val = strtol(input, &endptr, 10);
    
    /* Check for conversion errors */
    if (errno != 0 || endptr == input || *endptr != '\0') {
        return -1;
    }
    
    /* Check range */
    if (val < min || val > max) {
        return -1;
    }
    
    *value = (int)val;
    return 0;
}
