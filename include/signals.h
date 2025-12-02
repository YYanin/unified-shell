#ifndef SIGNALS_H
#define SIGNALS_H

#include <signal.h>

/**
 * Signal handling for job control
 * 
 * This module provides comprehensive signal handling for the shell:
 * - SIGINT (Ctrl+C): Terminates foreground jobs, not shell
 * - SIGTSTP (Ctrl+Z): Stops foreground jobs, returns to shell prompt
 * - SIGCHLD: Reaps zombie processes, updates job status
 * - SIGTTOU/SIGTTIN: Handles terminal control issues
 */

/**
 * Global flag set by SIGCHLD handler
 * Must be volatile sig_atomic_t for async-signal-safety
 */
extern volatile sig_atomic_t child_exited;

/**
 * Global variable to track current foreground job PID
 * When 0, no foreground job is running (shell is in foreground)
 * Updated by fg command and foreground process execution
 */
extern pid_t foreground_job_pid;

/**
 * Setup all signal handlers for job control
 * Should be called once during shell initialization
 */
void setup_signal_handlers(void);

/**
 * Signal handler for SIGCHLD (child process exit/stop)
 * Reaps zombie processes and updates job status
 */
void sigchld_handler(int sig);

/**
 * Signal handler for SIGTSTP (Ctrl+Z)
 * Stops the foreground job and returns control to shell
 */
void sigtstp_handler(int sig);

/**
 * Signal handler for SIGINT (Ctrl+C)
 * Terminates the foreground job without affecting the shell
 */
void sigint_handler(int sig);

#endif /* SIGNALS_H */
