#include "signals.h"
#include "jobs.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <errno.h>

/**
 * Global flag indicating a child process has changed state
 * Set by SIGCHLD handler, checked in REPL loop
 */
volatile sig_atomic_t child_exited = 0;

/**
 * Current foreground job PID
 * 0 = shell is in foreground, no job running
 * non-zero = PID of foreground process
 */
pid_t foreground_job_pid = 0;

/**
 * sigchld_handler - Handle child process termination/stop (SIGCHLD)
 * 
 * This handler is called when:
 * - A child process exits
 * - A child process stops (Ctrl+Z)
 * - A child process resumes (if SA_NOCLDSTOP not set)
 * 
 * The handler sets a flag to trigger job status update in the main loop.
 * We don't do the actual reaping here because signal handlers should be
 * kept minimal and async-signal-safe.
 */
void sigchld_handler(int sig) {
    (void)sig;  // Unused
    
    // Save errno to restore later (required for async-signal-safety)
    int saved_errno = errno;
    
    // Set flag to notify main loop
    // This is async-signal-safe (just setting a flag)
    child_exited = 1;
    
    // Restore errno
    errno = saved_errno;
}

/**
 * sigtstp_handler - Handle Ctrl+Z (SIGTSTP)
 * 
 * When user presses Ctrl+Z:
 * - If a foreground job is running: forward SIGTSTP to that job
 * - If no foreground job: ignore (don't stop the shell itself)
 * 
 * This allows Ctrl+Z to stop foreground processes while keeping
 * the shell responsive.
 */
void sigtstp_handler(int sig) {
    (void)sig;  // Unused
    
    int saved_errno = errno;
    
    // If there's a foreground job, send it SIGTSTP
    if (foreground_job_pid > 0) {
        // Send to the entire process group (negative PID)
        // This ensures all processes in a pipeline receive the signal
        kill(-foreground_job_pid, SIGTSTP);
    }
    // If foreground_job_pid == 0, shell is in foreground, ignore signal
    
    errno = saved_errno;
}

/**
 * sigint_handler - Handle Ctrl+C (SIGINT)
 * 
 * When user presses Ctrl+C:
 * - If a foreground job is running: forward SIGINT to that job
 * - If no foreground job: print newline and continue shell
 * 
 * This allows Ctrl+C to interrupt foreground processes while
 * keeping the shell running.
 */
void sigint_handler(int sig) {
    (void)sig;  // Unused
    
    int saved_errno = errno;
    
    // If there's a foreground job, send it SIGINT
    if (foreground_job_pid > 0) {
        // Send to the entire process group (negative PID)
        // This ensures all processes in a pipeline receive the signal
        kill(-foreground_job_pid, SIGINT);
    } else {
        // No foreground job - just print newline and continue
        // Use write() as it's async-signal-safe (printf is not)
        write(STDOUT_FILENO, "\n", 1);
    }
    
    errno = saved_errno;
}

/**
 * setup_signal_handlers - Configure all signal handlers for job control
 * 
 * Sets up:
 * - SIGINT (Ctrl+C): Interrupt foreground job
 * - SIGTSTP (Ctrl+Z): Stop foreground job  
 * - SIGCHLD: Detect child process state changes
 * - SIGTTOU: Ignore (prevents background job from stopping on terminal output)
 * - SIGTTIN: Ignore (prevents background job from stopping on terminal input)
 * 
 * Should be called once during shell initialization.
 */
void setup_signal_handlers(void) {
    struct sigaction sa;
    
    // Set up SIGINT handler (Ctrl+C)
    sa.sa_handler = sigint_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;  // Restart interrupted system calls
    if (sigaction(SIGINT, &sa, NULL) < 0) {
        perror("sigaction(SIGINT)");
        exit(1);
    }
    
    // Set up SIGTSTP handler (Ctrl+Z)
    sa.sa_handler = sigtstp_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGTSTP, &sa, NULL) < 0) {
        perror("sigaction(SIGTSTP)");
        exit(1);
    }
    
    // Set up SIGCHLD handler (child process exit/stop)
    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    // SA_RESTART: restart interrupted system calls
    // SA_NOCLDSTOP: don't notify when children stop (only when they exit)
    // Note: We actually DO want notification for stops, so we don't use SA_NOCLDSTOP
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) < 0) {
        perror("sigaction(SIGCHLD)");
        exit(1);
    }
    
    // Ignore SIGTTOU - background jobs writing to terminal shouldn't stop
    signal(SIGTTOU, SIG_IGN);
    
    // Ignore SIGTTIN - background jobs reading from terminal shouldn't stop
    signal(SIGTTIN, SIG_IGN);
    
    // Note: We don't ignore SIGQUIT (Ctrl+\) - it should terminate shell
    // Note: We don't ignore SIGTERM - it should terminate shell gracefully
}
