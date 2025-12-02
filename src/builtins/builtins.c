#include "builtins.h"
#include "history.h"
#include "apt.h"
#include "jobs.h"
#include "signals.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <sys/wait.h>
#include <termios.h>

/**
 * Built-in commands table
 */
static Builtin builtins[] = {
    {"cd", builtin_cd},
    {"pwd", builtin_pwd},
    {"echo", builtin_echo},
    {"export", builtin_export},
    {"exit", builtin_exit},
    {"set", builtin_set},
    {"unset", builtin_unset},
    {"env", builtin_env},
    {"help", builtin_help},
    {"version", builtin_version},
    {"history", builtin_history},
    {"edi", builtin_edi},
    {"apt", builtin_apt},
    {"jobs", builtin_jobs},
    {"fg", builtin_fg},
    {"bg", builtin_bg},
    {NULL, NULL}  // Sentinel
};

/**
 * Find a built-in command
 */
builtin_func find_builtin(const char *name) {
    if (name == NULL) {
        return NULL;
    }
    
    for (int i = 0; builtins[i].name != NULL; i++) {
        if (strcmp(name, builtins[i].name) == 0) {
            return builtins[i].func;
        }
    }
    
    return NULL;
}

/**
 * cd - Change directory
 * Usage: cd [directory]
 */
int builtin_cd(char **argv, Env *env) {
    (void)env;  // Unused for now
    
    const char *path;
    
    if (argv[1] == NULL) {
        // No argument - go to HOME
        path = getenv("HOME");
        if (path == NULL) {
            fprintf(stderr, "cd: HOME not set\n");
            return 1;
        }
    } else {
        path = argv[1];
    }
    
    if (chdir(path) != 0) {
        perror("cd");
        return 1;
    }
    
    return 0;
}

/**
 * pwd - Print working directory
 * Usage: pwd
 */
int builtin_pwd(char **argv, Env *env) {
    (void)argv;  // Unused
    (void)env;   // Unused
    
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        perror("pwd");
        return 1;
    }
    
    printf("%s\n", cwd);
    return 0;
}

/**
 * echo - Print arguments
 * Usage: echo [args...]
 */
int builtin_echo(char **argv, Env *env) {
    (void)env;  // Unused
    
    for (int i = 1; argv[i] != NULL; i++) {
        if (i > 1) {
            printf(" ");
        }
        printf("%s", argv[i]);
    }
    printf("\n");
    
    return 0;
}

/**
 * export - Set environment variable
 * Usage: export VAR=value
 */
int builtin_export(char **argv, Env *env) {
    if (argv[1] == NULL) {
        fprintf(stderr, "export: usage: export VAR=value\n");
        return 1;
    }
    
    // Parse VAR=value
    char *eq = strchr(argv[1], '=');
    if (eq == NULL) {
        fprintf(stderr, "export: invalid format (use VAR=value)\n");
        return 1;
    }
    
    // Split into name and value
    size_t name_len = eq - argv[1];
    char *name = malloc(name_len + 1);
    if (name == NULL) {
        perror("malloc");
        return 1;
    }
    
    strncpy(name, argv[1], name_len);
    name[name_len] = '\0';
    
    char *value = eq + 1;
    
    // Set in environment
    env_set(env, name, value);
    
    // Also set in system environment for child processes
    setenv(name, value, 1);
    
    free(name);
    return 0;
}

/**
 * exit - Exit the shell
 * Usage: exit [code]
 */
int builtin_exit(char **argv, Env *env) {
    (void)env;  // Unused
    
    int code = 0;
    
    if (argv[1] != NULL) {
        code = atoi(argv[1]);
    }
    
    exit(code);
}

/**
 * set - Set shell variable (not exported)
 * Usage: set VAR=value
 */
int builtin_set(char **argv, Env *env) {
    if (argv[1] == NULL) {
        // No arguments - print all variables
        env_print(env);
        return 0;
    }
    
    // Parse VAR=value
    char *eq = strchr(argv[1], '=');
    if (eq == NULL) {
        fprintf(stderr, "set: invalid format (use VAR=value)\n");
        return 1;
    }
    
    // Split into name and value
    size_t name_len = eq - argv[1];
    char *name = malloc(name_len + 1);
    if (name == NULL) {
        perror("malloc");
        return 1;
    }
    
    strncpy(name, argv[1], name_len);
    name[name_len] = '\0';
    
    char *value = eq + 1;
    
    // Set in environment (shell-local only)
    env_set(env, name, value);
    
    free(name);
    return 0;
}

/**
 * unset - Remove shell variable
 * Usage: unset VAR
 */
int builtin_unset(char **argv, Env *env) {
    if (argv[1] == NULL) {
        fprintf(stderr, "unset: usage: unset VAR\n");
        return 1;
    }
    
    env_unset(env, argv[1]);
    
    // Also unset from system environment
    unsetenv(argv[1]);
    
    return 0;
}

/**
 * env - Print environment
 * Usage: env
 */
int builtin_env(char **argv, Env *env) {
    (void)argv;  // Unused
    
    env_print(env);
    
    return 0;
}

/**
 * help - Display built-in commands
 * Usage: help
 */
int builtin_help(char **argv, Env *env) {
    (void)argv;  // Unused
    (void)env;   // Unused
    
    printf("Unified Shell (ushell) - Built-in Commands:\n\n");
    printf("  cd [dir]           Change directory (default: $HOME)\n");
    printf("  pwd                Print working directory\n");
    printf("  echo [args...]     Display arguments\n");
    printf("  export VAR=value   Set and export environment variable\n");
    printf("  set VAR=value      Set variable (shell only)\n");
    printf("  unset VAR          Remove variable\n");
    printf("  env                Display all environment variables\n");
    printf("  edi [file]         Vi-like text editor (modes: normal, insert, command)\n");
    printf("  help               Display this help message\n");
    printf("  version            Display version information\n");
    printf("  exit               Exit the shell\n");
    printf("\nJob Control:\n");
    printf("  jobs [-l|-p|-r|-s] List background jobs\n");
    printf("  fg [%%n]            Bring job to foreground (default: most recent)\n");
    printf("  bg [%%n]            Resume stopped job in background\n");
    printf("  cmd &              Run command in background\n");
    printf("\nIntegrated Tools:\n");
    printf("  myls, mycat, mycp, mymv, myrm, mymkdir, myrmdir, mytouch, mystat, myfd\n");
    printf("\nFeatures:\n");
    printf("  - Variables: $VAR or ${VAR}\n");
    printf("  - Arithmetic: $((expression))\n");
    printf("  - Pipelines: cmd1 | cmd2\n");
    printf("  - Redirection: < > >>\n");
    printf("  - Conditionals: if cmd then ... fi\n");
    printf("  - Glob expansion: * ? [abc] [a-z] [!abc]\n");
    printf("  - Job Control: & (background), Ctrl+Z (stop), Ctrl+C (interrupt)\n");
    
    return 0;
}

/**
 * version - Display version information
 * Usage: version
 */
int builtin_version(char **argv, Env *env) {
    (void)argv;  // Unused
    (void)env;   // Unused
    
    printf("Unified Shell (ushell) v1.0.0\n");
    printf("Build date: %s %s\n", __DATE__, __TIME__);
    printf("Features: variables, arithmetic, pipelines, conditionals, glob expansion\n");
    printf("Integrated tools: 10 file utilities + file finder\n");
    
    return 0;
}

/**
 * history - Display command history
 * Usage: history
 */
int builtin_history(char **argv, Env *env) {
    (void)env;  // Unused
    
    int count = history_count();
    
    // Check for clear option
    if (argv[1] != NULL && strcmp(argv[1], "-c") == 0) {
        history_clear();
        printf("History cleared\n");
        return 0;
    }
    
    if (count == 0) {
        printf("No history\n");
        return 0;
    }
    
    // Display history (most recent last)
    for (int i = count - 1; i >= 0; i--) {
        const char *entry = history_get(i);
        if (entry != NULL) {
            printf("%5d  %s\n", count - i, entry);
        }
    }
    
    return 0;
}

/**
 * jobs - List background jobs
 * Usage: jobs [-l] [-p] [-r] [-s]
 * 
 * Options:
 *   -l    Long format (include PID)
 *   -p    Show PIDs only
 *   -r    Show running jobs only
 *   -s    Show stopped jobs only
 * 
 * Display format:
 *   [N]+  Status    Command
 *   [N]-  Status    Command
 *   
 * Where:
 *   [N]   Job number
 *   +     Current job (most recent)
 *   -     Previous job (second most recent)
 *   Status Running/Stopped/Done
 *   Command Original command string
 */
int builtin_jobs(char **argv, Env *env) {
    (void)env;  // Unused
    
    // Parse options
    int long_format = 0;     // -l: include PID
    int pid_only = 0;        // -p: show PIDs only
    int running_only = 0;    // -r: running jobs only
    int stopped_only = 0;    // -s: stopped jobs only
    
    // Simple option parsing (no getopt to keep it simple)
    for (int i = 1; argv[i] != NULL; i++) {
        if (argv[i][0] == '-') {
            for (int j = 1; argv[i][j] != '\0'; j++) {
                switch (argv[i][j]) {
                    case 'l':
                        long_format = 1;
                        break;
                    case 'p':
                        pid_only = 1;
                        break;
                    case 'r':
                        running_only = 1;
                        break;
                    case 's':
                        stopped_only = 1;
                        break;
                    default:
                        fprintf(stderr, "jobs: invalid option -- '%c'\n", argv[i][j]);
                        fprintf(stderr, "Usage: jobs [-l] [-p] [-r] [-s]\n");
                        return 1;
                }
            }
        } else {
            fprintf(stderr, "jobs: unexpected argument: %s\n", argv[i]);
            fprintf(stderr, "Usage: jobs [-l] [-p] [-r] [-s]\n");
            return 1;
        }
    }
    
    // Update job statuses before displaying
    jobs_update_status();
    
    // Get job count
    int count = jobs_count();
    if (count == 0) {
        // No jobs - silent (like bash)
        return 0;
    }
    
    // Display jobs based on format
    if (pid_only) {
        // PID only format: just print PIDs, one per line
        for (int i = 0; i < count; i++) {
            Job *job = jobs_get_by_index(i);  // Use array index
            if (job != NULL) {
                // Apply filters
                if (running_only && job->status != JOB_RUNNING) continue;
                if (stopped_only && job->status != JOB_STOPPED) continue;
                
                printf("%d\n", job->pid);
            }
        }
    } else {
        // Normal or long format
        for (int i = 0; i < count; i++) {
            Job *job = jobs_get_by_index(i);  // Use array index
            if (job == NULL) continue;
            
            // Apply filters
            if (running_only && job->status != JOB_RUNNING) continue;
            if (stopped_only && job->status != JOB_STOPPED) continue;
            
            // Determine current/previous marker
            // + for most recent (last in list), - for second most recent
            char marker = ' ';
            if (i == count - 1) {
                marker = '+';
            } else if (i == count - 2) {
                marker = '-';
            }
            
            // Get status string
            const char *status = job_status_to_string(job->status);
            
            // Print based on format
            if (long_format) {
                // Long format: [N]+ PID Status Command
                printf("[%d]%c  %-7d %-20s %s\n", 
                       job->job_id, marker, job->pid, status, job->command);
            } else {
                // Default format: [N]+ Status Command
                printf("[%d]%c  %-20s %s\n", 
                       job->job_id, marker, status, job->command);
            }
        }
    }
    
    return 0;
}

/**
 * fg - Bring background job to foreground
 * Usage: fg [%job_id]
 * 
 * Arguments:
 *   none    Bring most recent job to foreground
 *   %n      Bring job n to foreground
 * 
 * Brings a background or stopped job to the foreground, gives it terminal control,
 * and waits for it to complete or stop. If the job is stopped, sends SIGCONT to resume it.
 */
int builtin_fg(char **argv, Env *env) {
    (void)env;  // Unused
    
    Job *job = NULL;
    int job_id = -1;
    
    // Determine which job to foreground
    if (argv[1] == NULL) {
        // No argument - use most recent job (highest job_id or last in array)
        int count = jobs_count();
        if (count == 0) {
            fprintf(stderr, "fg: no current job\n");
            return 1;
        }
        
        // Get most recent job (last in array)
        job = jobs_get_by_index(count - 1);
        if (job == NULL) {
            fprintf(stderr, "fg: no current job\n");
            return 1;
        }
        job_id = job->job_id;
        
    } else {
        // Parse job specification (%n format or just n)
        const char *arg = argv[1];
        
        if (arg[0] == '%') {
            arg++;  // Skip % prefix
        }
        
        // Parse job ID
        char *endptr;
        job_id = (int)strtol(arg, &endptr, 10);
        
        if (*endptr != '\0' || job_id <= 0) {
            fprintf(stderr, "fg: invalid job id: %s\n", argv[1]);
            return 1;
        }
        
        // Look up job
        job = jobs_get(job_id);
        if (job == NULL) {
            fprintf(stderr, "fg: %d: no such job\n", job_id);
            return 1;
        }
    }
    
    // Update job status before proceeding
    jobs_update_status();
    
    // Get fresh reference in case status changed
    job = jobs_get(job_id);
    if (job == NULL) {
        fprintf(stderr, "fg: job %d has terminated\n", job_id);
        return 1;
    }
    
    // Print the command being foregrounded
    printf("%s\n", job->command);
    fflush(stdout);
    
    // If job is stopped, send SIGCONT to resume it
    if (job->status == JOB_STOPPED) {
        if (kill(job->pid, SIGCONT) < 0) {
            perror("fg: kill");
            return 1;
        }
    }
    
    // Update job status to running
    job->status = JOB_RUNNING;
    job->background = 0;  // No longer a background job
    
    // Give terminal control to the job's process group
    // Note: The job's process group ID is the same as its PID (it's the group leader)
    if (tcsetpgrp(STDIN_FILENO, job->pid) < 0) {
        perror("fg: tcsetpgrp");
        // Continue anyway - job might not need terminal
    }
    
    // Set foreground job PID so signals (Ctrl+C, Ctrl+Z) go to this job
    foreground_job_pid = job->pid;
    
    // Wait for the job to complete or stop
    int status;
    pid_t result;
    
    while (1) {
        result = waitpid(job->pid, &status, WUNTRACED);
        
        if (result < 0) {
            if (errno == EINTR) {
                // Interrupted by signal, continue waiting
                continue;
            }
            perror("fg: waitpid");
            break;
        }
        
        if (WIFEXITED(status) || WIFSIGNALED(status)) {
            // Job completed
            job->status = JOB_DONE;
            jobs_remove(job_id);
            break;
        } else if (WIFSTOPPED(status)) {
            // Job was stopped (Ctrl+Z)
            job->status = JOB_STOPPED;
            job->background = 1;  // Back to background
            printf("\n[%d]+  Stopped                 %s\n", job->job_id, job->command);
            break;
        }
    }
    
    // Restore terminal control to the shell
    // Shell's process group ID is its PID (it's also a group leader)
    tcsetpgrp(STDIN_FILENO, getpgrp());
    
    // Clear foreground job
    foreground_job_pid = 0;
    
    return 0;
}

/**
 * bg - Resume stopped job in background
 * Usage: bg [%job_id]
 * 
 * Arguments:
 *   none    Resume most recent stopped job in background
 *   %n      Resume job n in background
 * 
 * Resumes a stopped job by sending SIGCONT and marks it as running in background.
 * The job continues executing without blocking the shell prompt.
 */
int builtin_bg(char **argv, Env *env) {
    (void)env;  // Unused
    
    Job *job = NULL;
    int job_id = -1;
    
    // Determine which job to background
    if (argv[1] == NULL) {
        // No argument - use most recent stopped job
        int count = jobs_count();
        if (count == 0) {
            fprintf(stderr, "bg: no current job\n");
            return 1;
        }
        
        // Search backwards for most recent stopped job
        for (int i = count - 1; i >= 0; i--) {
            Job *candidate = jobs_get_by_index(i);
            if (candidate && candidate->status == JOB_STOPPED) {
                job = candidate;
                job_id = job->job_id;
                break;
            }
        }
        
        if (job == NULL) {
            fprintf(stderr, "bg: no stopped jobs\n");
            return 1;
        }
        
    } else {
        // Parse job specification (%n format or just n)
        const char *arg = argv[1];
        
        if (arg[0] == '%') {
            arg++;  // Skip % prefix
        }
        
        // Parse job ID
        char *endptr;
        job_id = (int)strtol(arg, &endptr, 10);
        
        if (*endptr != '\0' || job_id <= 0) {
            fprintf(stderr, "bg: invalid job id: %s\n", argv[1]);
            return 1;
        }
        
        // Look up job
        job = jobs_get(job_id);
        if (job == NULL) {
            fprintf(stderr, "bg: %d: no such job\n", job_id);
            return 1;
        }
    }
    
    // Update job status before proceeding
    jobs_update_status();
    
    // Get fresh reference in case status changed
    job = jobs_get(job_id);
    if (job == NULL) {
        fprintf(stderr, "bg: job %d has terminated\n", job_id);
        return 1;
    }
    
    // Check if job is already running
    if (job->status == JOB_RUNNING) {
        fprintf(stderr, "bg: job %d already in background\n", job_id);
        return 0;
    }
    
    // Send SIGCONT to resume the stopped job
    if (kill(job->pid, SIGCONT) < 0) {
        perror("bg: kill");
        return 1;
    }
    
    // Update job status to running in background
    job->status = JOB_RUNNING;
    job->background = 1;
    
    // Print confirmation message
    printf("[%d]+ %s &\n", job->job_id, job->command);
    
    return 0;
}
