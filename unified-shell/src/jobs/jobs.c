/**
 * jobs.c - Job Control Implementation for Unified Shell
 * 
 * This module implements the job control system for tracking and managing
 * background and foreground jobs. It provides functions to add, remove,
 * query, and update job status.
 * 
 * Key features:
 *   - Job tracking with unique job IDs
 *   - Status monitoring (running, stopped, done)
 *   - Background job management
 *   - Non-blocking status updates
 */

#include "jobs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <pthread.h>

/* ============================================================================
 * Global Job List
 * ============================================================================ */

/* Global job list accessible by all job functions */
static JobList g_job_list;

/* Next job ID to assign (increments for each new job) */
static int g_next_job_id = 1;

/* Mutex to protect job list operations (thread-safe access) */
static pthread_mutex_t jobs_mutex = PTHREAD_MUTEX_INITIALIZER;

/* ============================================================================
 * Helper Functions
 * ============================================================================ */

/**
 * job_status_to_string - Convert JobStatus to human-readable string
 * 
 * @param status: JobStatus value
 * 
 * Returns: String representation of status
 */
const char* job_status_to_string(JobStatus status) {
    switch (status) {
        case JOB_RUNNING: return "Running";
        case JOB_STOPPED: return "Stopped";
        case JOB_DONE:    return "Done";
        default:          return "Unknown";
    }
}

/**
 * find_job_index - Find array index for a given job ID
 * 
 * @param job_id: Job ID to find
 * 
 * Returns: Array index if found, -1 if not found
 */
static int find_job_index(int job_id) {
    for (int i = 0; i < g_job_list.count; i++) {
        if (g_job_list.jobs[i].job_id == job_id) {
            return i;
        }
    }
    return -1;
}

/**
 * find_job_index_by_pid - Find array index for a given PID
 * 
 * @param pid: Process ID to find
 * 
 * Returns: Array index if found, -1 if not found
 */
static int find_job_index_by_pid(pid_t pid) {
    for (int i = 0; i < g_job_list.count; i++) {
        if (g_job_list.jobs[i].pid == pid) {
            return i;
        }
    }
    return -1;
}

/* ============================================================================
 * Public API Implementation
 * ============================================================================ */

/**
 * jobs_init - Initialize the job control system
 * 
 * Resets the job list and prepares for tracking jobs.
 * Should be called once during shell startup.
 */
void jobs_init(void) {
    /* Zero out the entire job list structure */
    memset(&g_job_list, 0, sizeof(JobList));
    
    /* Reset job counter */
    g_job_list.count = 0;
    
    /* Reset next job ID */
    g_next_job_id = 1;
}

/**
 * jobs_add - Add a new job to the job list
 * 
 * Creates a job entry with assigned job ID, PID, command, and status.
 * Job is marked as JOB_RUNNING initially.
 * 
 * @param pid: Process ID of the job
 * @param cmd: Command string (null-terminated)
 * @param bg:  1 if background job, 0 if foreground
 * 
 * Returns: Assigned job ID on success, -1 if job list is full
 */
int jobs_add(pid_t pid, const char *cmd, int bg) {
    pthread_mutex_lock(&jobs_mutex);
    
    /* Check if job list is full */
    if (g_job_list.count >= MAX_JOBS) {
        fprintf(stderr, "jobs: job list full (max %d jobs)\n", MAX_JOBS);
        pthread_mutex_unlock(&jobs_mutex);
        return -1;
    }
    
    /* Validate inputs */
    if (pid <= 0) {
        fprintf(stderr, "jobs: invalid PID %d\n", pid);
        pthread_mutex_unlock(&jobs_mutex);
        return -1;
    }
    
    if (cmd == NULL) {
        fprintf(stderr, "jobs: command cannot be NULL\n");
        pthread_mutex_unlock(&jobs_mutex);
        return -1;
    }
    
    /* Get pointer to next available job slot */
    Job *job = &g_job_list.jobs[g_job_list.count];
    
    /* Assign job ID (sequential) */
    job->job_id = g_next_job_id++;
    
    /* Set process ID */
    job->pid = pid;
    
    /* Copy command string (truncate if too long) */
    strncpy(job->command, cmd, MAX_CMD_LEN - 1);
    job->command[MAX_CMD_LEN - 1] = '\0';  /* Ensure null termination */
    
    /* Initialize status to running */
    job->status = JOB_RUNNING;
    
    /* Set background flag */
    job->background = bg;
    
    /* Increment job count */
    g_job_list.count++;
    
    int job_id = job->job_id;
    pthread_mutex_unlock(&jobs_mutex);
    return job_id;
}

/**
 * jobs_get - Retrieve a job by its job ID
 * 
 * @param job_id: Job ID to look up
 * 
 * Returns: Pointer to Job structure if found, NULL otherwise
 */
Job* jobs_get(int job_id) {
    pthread_mutex_lock(&jobs_mutex);
    int index = find_job_index(job_id);
    
    if (index < 0) {
        pthread_mutex_unlock(&jobs_mutex);
        return NULL;  /* Job not found */
    }
    
    Job *result = &g_job_list.jobs[index];
    pthread_mutex_unlock(&jobs_mutex);
    return result;
}

/**
 * jobs_get_by_pid - Retrieve a job by its process ID
 * 
 * @param pid: Process ID to look up
 * 
 * Returns: Pointer to Job structure if found, NULL otherwise
 */
Job* jobs_get_by_pid(pid_t pid) {
    pthread_mutex_lock(&jobs_mutex);
    int index = find_job_index_by_pid(pid);
    
    if (index < 0) {
        pthread_mutex_unlock(&jobs_mutex);
        return NULL;  /* Job not found */
    }
    
    Job *result = &g_job_list.jobs[index];
    pthread_mutex_unlock(&jobs_mutex);
    return result;
}

/**
 * jobs_get_by_index - Get job by array index (not job_id)
 * 
 * @param index: Array index (0-based)
 * 
 * Returns: Pointer to Job if index valid, NULL otherwise
 */
Job* jobs_get_by_index(int index) {
    pthread_mutex_lock(&jobs_mutex);
    if (index < 0 || index >= g_job_list.count) {
        pthread_mutex_unlock(&jobs_mutex);
        return NULL;
    }
    
    Job *result = &g_job_list.jobs[index];
    pthread_mutex_unlock(&jobs_mutex);
    return result;
}

/**
 * jobs_remove - Remove a job from the job list
 * 
 * Removes the job by shifting remaining jobs down in the array.
 * This does not kill the process, only removes it from tracking.
 * 
 * @param job_id: Job ID to remove
 * 
 * Returns: 0 on success, -1 if job not found
 */
int jobs_remove(int job_id) {
    pthread_mutex_lock(&jobs_mutex);
    
    int index = find_job_index(job_id);
    
    if (index < 0) {
        pthread_mutex_unlock(&jobs_mutex);
        return -1;  /* Job not found */
    }
    
    /* Shift all jobs after this one down by one position */
    for (int i = index; i < g_job_list.count - 1; i++) {
        g_job_list.jobs[i] = g_job_list.jobs[i + 1];
    }
    
    /* Decrement job count */
    g_job_list.count--;
    
    /* Zero out the last slot */
    memset(&g_job_list.jobs[g_job_list.count], 0, sizeof(Job));
    
    pthread_mutex_unlock(&jobs_mutex);
    return 0;
}

/**
 * jobs_update_status - Update status of all jobs
 * 
 * Checks each job using waitpid() with WNOHANG (non-blocking).
 * Updates status to JOB_DONE for completed jobs.
 * Does not remove jobs from list (use jobs_cleanup() for that).
 */
void jobs_update_status(void) {
    int status;
    pid_t result;
    
    pthread_mutex_lock(&jobs_mutex);
    
    /* Iterate through all jobs */
    for (int i = 0; i < g_job_list.count; i++) {
        Job *job = &g_job_list.jobs[i];
        
        /* Skip jobs that are already done */
        if (job->status == JOB_DONE) {
            continue;
        }
        
        /* Check job status without blocking (WNOHANG) */
        result = waitpid(job->pid, &status, WNOHANG | WUNTRACED | WCONTINUED);
        
        if (result == 0) {
            /* Process still running, no status change */
            continue;
        }
        else if (result < 0) {
            /* Error checking status (process may have been reaped) */
            /* Mark as done if we can't check status */
            job->status = JOB_DONE;
        }
        else {
            /* Status changed - update accordingly */
            if (WIFEXITED(status) || WIFSIGNALED(status)) {
                /* Process has terminated (exited or killed) */
                job->status = JOB_DONE;
            }
            else if (WIFSTOPPED(status)) {
                /* Process was stopped (Ctrl+Z or SIGSTOP) */
                job->status = JOB_STOPPED;
            }
            else if (WIFCONTINUED(status)) {
                /* Process was resumed (SIGCONT) */
                job->status = JOB_RUNNING;
            }
        }
    }
    
    pthread_mutex_unlock(&jobs_mutex);
}

/**
 * jobs_print_all - Print all jobs in the job list
 * 
 * Displays each job with format: [job_id]status  command
 * Example: [1]+ Running  sleep 10 &
 * 
 * The '+' marker indicates the current job (most recent).
 */
void jobs_print_all(void) {
    pthread_mutex_lock(&jobs_mutex);
    
    /* Check if job list is empty */
    if (g_job_list.count == 0) {
        pthread_mutex_unlock(&jobs_mutex);
        printf("No jobs.\n");
        return;
    }
    
    /* Print header */
    printf("Job ID  PID     Status    Background  Command\n");
    printf("------  ------  --------  ----------  -------\n");
    
    /* Print each job */
    for (int i = 0; i < g_job_list.count; i++) {
        Job *job = &g_job_list.jobs[i];
        
        /* Job marker: + for most recent, - for second most recent */
        char marker = ' ';
        if (i == g_job_list.count - 1) {
            marker = '+';  /* Most recent job */
        } else if (i == g_job_list.count - 2) {
            marker = '-';  /* Second most recent */
        }
        
        /* Print job information */
        printf("[%d]%c   %-6d  %-8s  %-10s  %s\n",
               job->job_id,
               marker,
               job->pid,
               job_status_to_string(job->status),
               job->background ? "yes" : "no",
               job->command);
    }
    
    pthread_mutex_unlock(&jobs_mutex);
}

/**
 * jobs_count - Get the number of active jobs
 * 
 * Returns: Current number of jobs in the list
 */
int jobs_count(void) {
    pthread_mutex_lock(&jobs_mutex);
    int count = g_job_list.count;
    pthread_mutex_unlock(&jobs_mutex);
    return count;
}

/**
 * jobs_cleanup - Remove all completed jobs from the list
 * 
 * Iterates through the job list and removes all jobs with status JOB_DONE.
 * This prevents the job list from filling up with finished jobs.
 * 
 * Returns: Number of jobs removed
 */
int jobs_cleanup(void) {
    pthread_mutex_lock(&jobs_mutex);
    int removed = 0;
    
    /* Iterate backwards to avoid index shifting issues */
    for (int i = g_job_list.count - 1; i >= 0; i--) {
        if (g_job_list.jobs[i].status == JOB_DONE) {
            /* Temporarily unlock to avoid deadlock in jobs_remove */
            int job_id = g_job_list.jobs[i].job_id;
            pthread_mutex_unlock(&jobs_mutex);
            jobs_remove(job_id);
            pthread_mutex_lock(&jobs_mutex);
            removed++;
        }
    }
    
    pthread_mutex_unlock(&jobs_mutex);
    return removed;
}
