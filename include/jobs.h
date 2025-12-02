/**
 * jobs.h - Job Control for Unified Shell
 * 
 * This header defines the job control system for managing background
 * and foreground processes. It provides data structures and functions
 * for tracking job status, adding/removing jobs, and querying job information.
 * 
 * Job control features:
 *   - Track running, stopped, and completed jobs
 *   - Background job execution (& operator)
 *   - Job listing and management
 *   - Signal handling for job status changes
 */

#ifndef JOBS_H
#define JOBS_H

#include <sys/types.h>

/* ============================================================================
 * Constants and Limits
 * ============================================================================ */

/* Maximum number of jobs that can be tracked simultaneously */
#define MAX_JOBS 64

/* Maximum length of command string stored for each job */
#define MAX_CMD_LEN 1024

/* ============================================================================
 * Job Status Enumeration
 * ============================================================================ */

/**
 * JobStatus - Current state of a job
 * 
 * JOB_RUNNING: Job is currently executing
 * JOB_STOPPED: Job has been suspended (Ctrl+Z or SIGSTOP)
 * JOB_DONE:    Job has completed execution
 */
typedef enum {
    JOB_RUNNING,
    JOB_STOPPED,
    JOB_DONE
} JobStatus;

/* ============================================================================
 * Job Data Structure
 * ============================================================================ */

/**
 * Job - Represents a single job (process or pipeline)
 * 
 * Fields:
 *   job_id:     Unique job identifier (1, 2, 3, ...)
 *   pid:        Process ID of the job
 *   command:    Command string that started the job
 *   status:     Current status (running/stopped/done)
 *   background: 1 if job was started in background, 0 otherwise
 */
typedef struct {
    int job_id;                  /* Job number (user-visible ID) */
    pid_t pid;                   /* Process ID */
    char command[MAX_CMD_LEN];   /* Command that started this job */
    JobStatus status;            /* Current status */
    int background;              /* 1 = background job, 0 = foreground */
} Job;

/* ============================================================================
 * Job List Structure
 * ============================================================================ */

/**
 * JobList - Container for all tracked jobs
 * 
 * Fields:
 *   jobs:  Array of Job structures (fixed size)
 *   count: Number of active jobs in the array
 */
typedef struct {
    Job jobs[MAX_JOBS];          /* Array of jobs */
    int count;                   /* Number of active jobs */
} JobList;

/* ============================================================================
 * Job Management Functions
 * ============================================================================ */

/**
 * jobs_init - Initialize the job control system
 * 
 * Sets up the global job list and prepares the system for tracking jobs.
 * Should be called once during shell initialization.
 * 
 * Returns: void
 */
void jobs_init(void);

/**
 * jobs_add - Add a new job to the job list
 * 
 * Creates a new job entry with the given PID and command. Assigns a unique
 * job ID and initializes status to JOB_RUNNING.
 * 
 * @param pid: Process ID of the job
 * @param cmd: Command string that started the job
 * @param bg:  1 if background job, 0 if foreground
 * 
 * Returns: Job ID on success, -1 if job list is full
 */
int jobs_add(pid_t pid, const char *cmd, int bg);

/**
 * jobs_get - Retrieve a job by its job ID
 * 
 * Looks up a job in the job list by its user-visible job ID.
 * 
 * @param job_id: Job ID to look up (1, 2, 3, ...)
 * 
 * Returns: Pointer to Job structure if found, NULL if not found
 */
Job* jobs_get(int job_id);

/**
 * jobs_get_by_pid - Retrieve a job by its process ID
 * 
 * Looks up a job in the job list by its process ID.
 * 
 * @param pid: Process ID to look up
 * 
 * Returns: Pointer to Job structure if found, NULL if not found
 */
Job* jobs_get_by_pid(pid_t pid);

/**
 * jobs_remove - Remove a job from the job list
 * 
 * Removes the job with the specified job ID from tracking.
 * This does not kill the process, only removes it from the list.
 * 
 * @param job_id: Job ID to remove
 * 
 * Returns: 0 on success, -1 if job not found
 */
int jobs_remove(int job_id);

/**
 * jobs_update_status - Update status of all jobs
 * 
 * Checks the status of all tracked jobs using waitpid() with WNOHANG.
 * Updates job status to JOB_DONE for completed jobs.
 * Should be called periodically to keep job status current.
 * 
 * Returns: void
 */
void jobs_update_status(void);

/**
 * jobs_print_all - Print all jobs in the job list
 * 
 * Displays all tracked jobs with their job ID, status, and command.
 * Format: [job_id] status  command
 * Example: [1]+ Running  sleep 10 &
 * 
 * Returns: void
 */
void jobs_print_all(void);

/**
 * jobs_count - Get the number of active jobs
 * 
 * Returns the current count of jobs in the job list.
 * 
 * Returns: Number of active jobs
 */
int jobs_count(void);

/**
 * jobs_cleanup - Clean up completed jobs
 * 
 * Removes all jobs with status JOB_DONE from the job list.
 * This prevents the job list from filling up with completed jobs.
 * 
 * Returns: Number of jobs removed
 */
int jobs_cleanup(void);

/**
 * job_status_to_string - Convert JobStatus to human-readable string
 * @status: Job status enum value
 * 
 * Returns: String representation of status ("Running", "Stopped", or "Done")
 *          Never returns NULL
 */
const char* job_status_to_string(JobStatus status);

/**
 * jobs_get_by_index - Get job by array index (not job_id)
 * @index: Array index (0-based)
 * 
 * Returns: Pointer to Job if index valid, NULL otherwise
 *          Use for iterating through all jobs
 */
Job* jobs_get_by_index(int index);

#endif /* JOBS_H */
