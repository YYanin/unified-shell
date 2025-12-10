#ifndef THREADING_H
#define THREADING_H

#include <pthread.h>
#include "builtins.h"
#include "environment.h"

/**
 * Thread context structure for executing built-in commands
 * 
 * This structure holds all necessary information for executing
 * a built-in command in a separate thread, including:
 * - The function pointer to the built-in command
 * - Deep copy of arguments (to avoid race conditions)
 * - Pointer to the shell environment
 * - Execution status and completion flag
 * - Thread synchronization primitives
 */
typedef struct {
    builtin_func func;       /* Built-in function to execute */
    char **argv;             /* Arguments (deep copy for thread safety) */
    Env *env;                /* Pointer to shell environment */
    int status;              /* Exit status after execution */
    int completed;           /* 1 when thread finishes, 0 otherwise */
    pthread_t thread_id;     /* Thread identifier */
    pthread_mutex_t lock;    /* Mutex for status access synchronization */
} BuiltinThreadContext;

/**
 * builtin_thread_wrapper - Thread entry point for built-in execution
 * 
 * This function is called when a new thread is created to execute
 * a built-in command. It casts the void* argument to BuiltinThreadContext*,
 * executes the built-in function, stores the result, and marks completion.
 * 
 * @param arg: Pointer to BuiltinThreadContext structure (cast from void*)
 * @return: NULL (pthread convention)
 * 
 * Thread safety: This function is thread-safe. It operates on its own
 * context and uses the mutex to protect status updates.
 */
void* builtin_thread_wrapper(void *arg);

/**
 * create_thread_context - Allocate and initialize thread context
 * 
 * Creates a new thread context for executing a built-in command.
 * Makes a deep copy of argv to ensure thread safety (the calling
 * thread may modify or free its argv array).
 * 
 * @param func: Built-in function pointer to execute
 * @param argv: NULL-terminated array of arguments
 * @param env: Pointer to shell environment
 * @return: Pointer to newly allocated context, or NULL on error
 * 
 * Memory: Caller is responsible for freeing the returned context
 * using free_thread_context() after the thread completes.
 */
BuiltinThreadContext* create_thread_context(builtin_func func, char **argv, Env *env);

/**
 * free_thread_context - Free thread context and all allocated memory
 * 
 * Frees all memory associated with a thread context, including:
 * - Deep copied argv strings
 * - argv array itself
 * - Mutex resources
 * - Context structure
 * 
 * @param ctx: Thread context to free
 * 
 * Thread safety: This function should only be called after the thread
 * has completed (pthread_join) to avoid use-after-free bugs.
 */
void free_thread_context(BuiltinThreadContext *ctx);

/**
 * execute_builtin_threaded - Execute built-in command in a thread
 * 
 * Main entry point for threaded built-in execution. This function:
 * 1. Creates a thread context (with deep copied argv)
 * 2. Creates a new thread to execute the built-in
 * 3. Waits for thread completion (pthread_join)
 * 4. Extracts the exit status
 * 5. Frees the context
 * 
 * On thread creation failure, falls back to direct execution
 * of the built-in in the current thread.
 * 
 * @param func: Built-in function pointer to execute
 * @param argv: NULL-terminated array of arguments
 * @param env: Pointer to shell environment
 * @return: Exit status of the built-in command (0 for success)
 * 
 * Thread safety: This function is thread-safe. It creates isolated
 * context for each execution.
 */
int execute_builtin_threaded(builtin_func func, char **argv, Env *env);

/* ========================================================================
 * THREAD POOL IMPLEMENTATION
 * ======================================================================== */

/**
 * Thread pool structure for managing worker threads
 * 
 * A thread pool maintains a fixed number of worker threads that process
 * tasks from a queue. This is more efficient than creating a new thread
 * for each task, especially when tasks are short-lived.
 * 
 * Features:
 * - Fixed number of worker threads (configurable at creation)
 * - Work queue with mutex protection
 * - Condition variables for signaling work availability
 * - Graceful shutdown mechanism
 */
typedef struct {
    pthread_t *threads;              /* Array of worker thread IDs */
    int num_threads;                 /* Number of worker threads */
    
    BuiltinThreadContext **queue;    /* Work queue (array of contexts) */
    int queue_size;                  /* Current number of items in queue */
    int queue_capacity;              /* Maximum queue capacity */
    int queue_front;                 /* Front index for dequeue */
    int queue_rear;                  /* Rear index for enqueue */
    
    int shutdown;                    /* Shutdown flag (1 = shutting down) */
    
    pthread_mutex_t queue_mutex;     /* Protects queue access */
    pthread_cond_t work_available;   /* Signals work in queue */
    pthread_cond_t work_done;        /* Signals task completion */
} ThreadPool;

/**
 * thread_pool_create - Create and initialize a thread pool
 * 
 * Allocates a thread pool, creates worker threads, and initializes
 * all synchronization primitives. Worker threads start immediately
 * and wait for work to be submitted.
 * 
 * @param num_threads: Number of worker threads to create
 * @param queue_capacity: Maximum number of pending tasks in queue
 * @return: Pointer to thread pool, or NULL on error
 * 
 * Memory: Caller must destroy pool with thread_pool_destroy()
 * when done to clean up resources and join threads.
 */
ThreadPool* thread_pool_create(int num_threads, int queue_capacity);

/**
 * thread_pool_destroy - Shut down and clean up thread pool
 * 
 * Sets shutdown flag, signals all workers, waits for them to finish,
 * and frees all allocated resources including the pool structure.
 * 
 * @param pool: Thread pool to destroy
 * 
 * Thread safety: This function should only be called when no more
 * work will be submitted. It will complete all pending work before
 * shutting down.
 */
void thread_pool_destroy(ThreadPool *pool);

/**
 * thread_pool_worker - Worker thread main loop
 * 
 * This is the entry point for each worker thread. It continuously:
 * 1. Waits for work to be available in the queue
 * 2. Dequeues a task (BuiltinThreadContext)
 * 3. Executes the built-in command
 * 4. Signals completion
 * 5. Repeats until shutdown flag is set
 * 
 * @param arg: Pointer to ThreadPool structure (cast from void*)
 * @return: NULL (pthread convention)
 * 
 * Thread safety: Uses mutex and condition variables for safe access
 * to shared queue.
 */
void* thread_pool_worker(void *arg);

/**
 * thread_pool_submit - Submit work to thread pool
 * 
 * Adds a task to the work queue. If queue is full, this function
 * waits (blocks) until space becomes available. A worker thread
 * will eventually pick up and execute this task.
 * 
 * @param pool: Thread pool to submit work to
 * @param ctx: Thread context with built-in to execute
 * @return: 0 on success, -1 on error
 * 
 * Thread safety: This function is thread-safe and can be called
 * from multiple threads concurrently.
 */
int thread_pool_submit(ThreadPool *pool, BuiltinThreadContext *ctx);

/**
 * thread_pool_wait - Wait for all pending tasks to complete
 * 
 * Blocks until the work queue is empty and all workers are idle.
 * Useful for synchronization when you need to ensure all submitted
 * work has finished before proceeding.
 * 
 * @param pool: Thread pool to wait on
 * 
 * Thread safety: This function is thread-safe but should typically
 * be called by a single coordinating thread.
 */
void thread_pool_wait(ThreadPool *pool);

#endif /* THREADING_H */
