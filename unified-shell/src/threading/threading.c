#include "threading.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * builtin_thread_wrapper - Thread entry point for built-in execution
 * 
 * This function runs in a separate thread and executes the built-in
 * command specified in the context. It safely updates the status
 * and completion flag when done.
 * 
 * @param arg: Pointer to BuiltinThreadContext (cast from void*)
 * @return: NULL (pthread convention)
 * 
 * Thread safety: Operates on isolated context with mutex protection
 */
void* builtin_thread_wrapper(void *arg) {
    BuiltinThreadContext *ctx = (BuiltinThreadContext *)arg;
    
    if (ctx == NULL || ctx->func == NULL) {
        return NULL;
    }
    
    /* Execute the built-in command and store the result */
    int status = ctx->func(ctx->argv, ctx->env);
    
    /* Lock mutex to safely update status and completion flag */
    pthread_mutex_lock(&ctx->lock);
    ctx->status = status;
    ctx->completed = 1;
    pthread_mutex_unlock(&ctx->lock);
    
    return NULL;
}

/**
 * argv_count - Count number of arguments in argv array
 * 
 * @param argv: NULL-terminated array of strings
 * @return: Number of arguments (not including NULL terminator)
 */
static int argv_count(char **argv) {
    int count = 0;
    if (argv == NULL) {
        return 0;
    }
    while (argv[count] != NULL) {
        count++;
    }
    return count;
}

/**
 * argv_deep_copy - Create a deep copy of argv array
 * 
 * Allocates new array and duplicates all strings to ensure
 * thread safety (original argv may be modified or freed).
 * 
 * @param argv: NULL-terminated array of strings to copy
 * @return: New NULL-terminated array (caller must free), or NULL on error
 */
static char** argv_deep_copy(char **argv) {
    int count;
    char **copy;
    int i;
    
    if (argv == NULL) {
        return NULL;
    }
    
    /* Count arguments */
    count = argv_count(argv);
    
    /* Allocate array (including space for NULL terminator) */
    copy = (char **)malloc((count + 1) * sizeof(char *));
    if (copy == NULL) {
        perror("argv_deep_copy: malloc failed");
        return NULL;
    }
    
    /* Copy each string */
    for (i = 0; i < count; i++) {
        copy[i] = strdup(argv[i]);
        if (copy[i] == NULL) {
            /* strdup failed, free what we've allocated so far */
            perror("argv_deep_copy: strdup failed");
            for (int j = 0; j < i; j++) {
                free(copy[j]);
            }
            free(copy);
            return NULL;
        }
    }
    
    /* NULL terminate the array */
    copy[count] = NULL;
    
    return copy;
}

/**
 * create_thread_context - Allocate and initialize thread context
 * 
 * Creates a new thread context with deep copied argv for thread safety.
 * Initializes all fields and the mutex.
 * 
 * @param func: Built-in function pointer to execute
 * @param argv: NULL-terminated array of arguments (will be deep copied)
 * @param env: Pointer to shell environment
 * @return: Pointer to new context, or NULL on error
 * 
 * Memory: Caller must free with free_thread_context() after thread completes
 */
BuiltinThreadContext* create_thread_context(builtin_func func, char **argv, Env *env) {
    BuiltinThreadContext *ctx;
    
    if (func == NULL || argv == NULL || env == NULL) {
        fprintf(stderr, "create_thread_context: invalid arguments\n");
        return NULL;
    }
    
    /* Allocate context structure */
    ctx = (BuiltinThreadContext *)malloc(sizeof(BuiltinThreadContext));
    if (ctx == NULL) {
        perror("create_thread_context: malloc failed");
        return NULL;
    }
    
    /* Deep copy argv for thread safety */
    ctx->argv = argv_deep_copy(argv);
    if (ctx->argv == NULL) {
        fprintf(stderr, "create_thread_context: failed to copy argv\n");
        free(ctx);
        return NULL;
    }
    
    /* Initialize context fields */
    ctx->func = func;
    ctx->env = env;
    ctx->status = 0;
    ctx->completed = 0;
    ctx->thread_id = 0;
    
    /* Initialize mutex for status synchronization */
    if (pthread_mutex_init(&ctx->lock, NULL) != 0) {
        perror("create_thread_context: mutex init failed");
        
        /* Free deep copied argv */
        int i = 0;
        while (ctx->argv[i] != NULL) {
            free(ctx->argv[i]);
            i++;
        }
        free(ctx->argv);
        free(ctx);
        return NULL;
    }
    
    return ctx;
}

/**
 * free_thread_context - Free thread context and all resources
 * 
 * Frees argv strings, argv array, destroys mutex, and frees context.
 * Should only be called after thread has completed (pthread_join).
 * 
 * @param ctx: Thread context to free
 */
void free_thread_context(BuiltinThreadContext *ctx) {
    int i;
    
    if (ctx == NULL) {
        return;
    }
    
    /* Free all argv strings */
    if (ctx->argv != NULL) {
        i = 0;
        while (ctx->argv[i] != NULL) {
            free(ctx->argv[i]);
            i++;
        }
        free(ctx->argv);
    }
    
    /* Destroy mutex */
    pthread_mutex_destroy(&ctx->lock);
    
    /* Free context structure */
    free(ctx);
}

/**
 * execute_builtin_threaded - Execute built-in command in a new thread
 * 
 * Main entry point for threaded built-in execution. Creates thread,
 * waits for completion, and returns exit status. Falls back to
 * direct execution if thread creation fails.
 * 
 * @param func: Built-in function pointer to execute
 * @param argv: NULL-terminated array of arguments
 * @param env: Pointer to shell environment
 * @return: Exit status of built-in (0 for success), or -1 on error
 */
int execute_builtin_threaded(builtin_func func, char **argv, Env *env) {
    BuiltinThreadContext *ctx;
    int status;
    int ret;
    
    if (func == NULL || argv == NULL || env == NULL) {
        fprintf(stderr, "execute_builtin_threaded: invalid arguments\n");
        return -1;
    }
    
    /* Create thread context with deep copied argv */
    ctx = create_thread_context(func, argv, env);
    if (ctx == NULL) {
        fprintf(stderr, "execute_builtin_threaded: failed to create context, falling back to direct execution\n");
        /* Fall back to direct execution in current thread */
        return func(argv, env);
    }
    
    /* Create thread to execute the built-in */
    ret = pthread_create(&ctx->thread_id, NULL, builtin_thread_wrapper, ctx);
    if (ret != 0) {
        fprintf(stderr, "execute_builtin_threaded: pthread_create failed (%d), falling back to direct execution\n", ret);
        /* Clean up context and fall back to direct execution */
        free_thread_context(ctx);
        return func(argv, env);
    }
    
    /* Wait for thread to complete */
    ret = pthread_join(ctx->thread_id, NULL);
    if (ret != 0) {
        fprintf(stderr, "execute_builtin_threaded: pthread_join failed (%d)\n", ret);
        free_thread_context(ctx);
        return -1;
    }
    
    /* Extract exit status from context */
    pthread_mutex_lock(&ctx->lock);
    status = ctx->status;
    pthread_mutex_unlock(&ctx->lock);
    
    /* Free context resources */
    free_thread_context(ctx);
    
    return status;
}

/* ========================================================================
 * THREAD POOL IMPLEMENTATION
 * ======================================================================== */

/**
 * thread_pool_worker - Worker thread main loop
 * 
 * Each worker thread runs this function. It continuously waits for work,
 * executes tasks, and signals completion until shutdown.
 * 
 * @param arg: Pointer to ThreadPool (cast from void*)
 * @return: NULL (pthread convention)
 */
void* thread_pool_worker(void *arg) {
    ThreadPool *pool = (ThreadPool *)arg;
    BuiltinThreadContext *ctx;
    
    if (pool == NULL) {
        return NULL;
    }
    
    while (1) {
        /* Lock mutex to access queue */
        pthread_mutex_lock(&pool->queue_mutex);
        
        /* Wait for work or shutdown signal */
        while (pool->queue_size == 0 && !pool->shutdown) {
            pthread_cond_wait(&pool->work_available, &pool->queue_mutex);
        }
        
        /* Check for shutdown */
        if (pool->shutdown && pool->queue_size == 0) {
            pthread_mutex_unlock(&pool->queue_mutex);
            break;
        }
        
        /* Dequeue work item (circular queue) */
        ctx = pool->queue[pool->queue_front];
        pool->queue_front = (pool->queue_front + 1) % pool->queue_capacity;
        pool->queue_size--;
        
        /* Signal that space is available in queue */
        pthread_cond_signal(&pool->work_done);
        
        pthread_mutex_unlock(&pool->queue_mutex);
        
        /* Execute the task */
        if (ctx != NULL && ctx->func != NULL) {
            int status = ctx->func(ctx->argv, ctx->env);
            
            /* Update context with result */
            pthread_mutex_lock(&ctx->lock);
            ctx->status = status;
            ctx->completed = 1;
            pthread_mutex_unlock(&ctx->lock);
        }
    }
    
    return NULL;
}

/**
 * thread_pool_create - Create and initialize thread pool
 * 
 * Creates worker threads and initializes synchronization primitives.
 * 
 * @param num_threads: Number of worker threads
 * @param queue_capacity: Maximum queue size
 * @return: Pointer to thread pool, or NULL on error
 */
ThreadPool* thread_pool_create(int num_threads, int queue_capacity) {
    ThreadPool *pool;
    int i;
    
    if (num_threads <= 0 || queue_capacity <= 0) {
        fprintf(stderr, "thread_pool_create: invalid parameters\n");
        return NULL;
    }
    
    /* Allocate pool structure */
    pool = (ThreadPool *)malloc(sizeof(ThreadPool));
    if (pool == NULL) {
        perror("thread_pool_create: malloc failed");
        return NULL;
    }
    
    /* Initialize pool fields */
    pool->num_threads = num_threads;
    pool->queue_capacity = queue_capacity;
    pool->queue_size = 0;
    pool->queue_front = 0;
    pool->queue_rear = 0;
    pool->shutdown = 0;
    
    /* Allocate thread array */
    pool->threads = (pthread_t *)malloc(num_threads * sizeof(pthread_t));
    if (pool->threads == NULL) {
        perror("thread_pool_create: thread array malloc failed");
        free(pool);
        return NULL;
    }
    
    /* Allocate queue array */
    pool->queue = (BuiltinThreadContext **)malloc(queue_capacity * sizeof(BuiltinThreadContext *));
    if (pool->queue == NULL) {
        perror("thread_pool_create: queue array malloc failed");
        free(pool->threads);
        free(pool);
        return NULL;
    }
    
    /* Initialize queue to NULL */
    for (i = 0; i < queue_capacity; i++) {
        pool->queue[i] = NULL;
    }
    
    /* Initialize mutex */
    if (pthread_mutex_init(&pool->queue_mutex, NULL) != 0) {
        perror("thread_pool_create: mutex init failed");
        free(pool->queue);
        free(pool->threads);
        free(pool);
        return NULL;
    }
    
    /* Initialize condition variables */
    if (pthread_cond_init(&pool->work_available, NULL) != 0) {
        perror("thread_pool_create: work_available cond init failed");
        pthread_mutex_destroy(&pool->queue_mutex);
        free(pool->queue);
        free(pool->threads);
        free(pool);
        return NULL;
    }
    
    if (pthread_cond_init(&pool->work_done, NULL) != 0) {
        perror("thread_pool_create: work_done cond init failed");
        pthread_cond_destroy(&pool->work_available);
        pthread_mutex_destroy(&pool->queue_mutex);
        free(pool->queue);
        free(pool->threads);
        free(pool);
        return NULL;
    }
    
    /* Create worker threads */
    for (i = 0; i < num_threads; i++) {
        if (pthread_create(&pool->threads[i], NULL, thread_pool_worker, pool) != 0) {
            fprintf(stderr, "thread_pool_create: failed to create worker thread %d\n", i);
            
            /* Set shutdown flag and signal workers to exit */
            pool->shutdown = 1;
            pthread_cond_broadcast(&pool->work_available);
            
            /* Wait for already-created threads to finish */
            for (int j = 0; j < i; j++) {
                pthread_join(pool->threads[j], NULL);
            }
            
            /* Clean up */
            pthread_cond_destroy(&pool->work_done);
            pthread_cond_destroy(&pool->work_available);
            pthread_mutex_destroy(&pool->queue_mutex);
            free(pool->queue);
            free(pool->threads);
            free(pool);
            return NULL;
        }
    }
    
    return pool;
}

/**
 * thread_pool_destroy - Shut down and clean up thread pool
 * 
 * Sets shutdown flag, waits for workers to finish, and frees resources.
 * 
 * @param pool: Thread pool to destroy
 */
void thread_pool_destroy(ThreadPool *pool) {
    int i;
    
    if (pool == NULL) {
        return;
    }
    
    /* Set shutdown flag */
    pthread_mutex_lock(&pool->queue_mutex);
    pool->shutdown = 1;
    pthread_mutex_unlock(&pool->queue_mutex);
    
    /* Wake up all worker threads */
    pthread_cond_broadcast(&pool->work_available);
    
    /* Wait for all threads to finish */
    for (i = 0; i < pool->num_threads; i++) {
        pthread_join(pool->threads[i], NULL);
    }
    
    /* Destroy synchronization primitives */
    pthread_cond_destroy(&pool->work_done);
    pthread_cond_destroy(&pool->work_available);
    pthread_mutex_destroy(&pool->queue_mutex);
    
    /* Free memory */
    free(pool->queue);
    free(pool->threads);
    free(pool);
}

/**
 * thread_pool_submit - Submit work to thread pool
 * 
 * Adds task to queue. Blocks if queue is full.
 * 
 * @param pool: Thread pool
 * @param ctx: Thread context to execute
 * @return: 0 on success, -1 on error
 */
int thread_pool_submit(ThreadPool *pool, BuiltinThreadContext *ctx) {
    if (pool == NULL || ctx == NULL) {
        fprintf(stderr, "thread_pool_submit: invalid arguments\n");
        return -1;
    }
    
    pthread_mutex_lock(&pool->queue_mutex);
    
    /* Wait for space in queue if full */
    while (pool->queue_size == pool->queue_capacity && !pool->shutdown) {
        pthread_cond_wait(&pool->work_done, &pool->queue_mutex);
    }
    
    /* Check if shutting down */
    if (pool->shutdown) {
        pthread_mutex_unlock(&pool->queue_mutex);
        fprintf(stderr, "thread_pool_submit: pool is shutting down\n");
        return -1;
    }
    
    /* Enqueue work item (circular queue) */
    pool->queue[pool->queue_rear] = ctx;
    pool->queue_rear = (pool->queue_rear + 1) % pool->queue_capacity;
    pool->queue_size++;
    
    /* Signal that work is available */
    pthread_cond_signal(&pool->work_available);
    
    pthread_mutex_unlock(&pool->queue_mutex);
    
    return 0;
}

/**
 * thread_pool_wait - Wait for all pending tasks to complete
 * 
 * Blocks until queue is empty and all workers are idle.
 * 
 * @param pool: Thread pool to wait on
 */
void thread_pool_wait(ThreadPool *pool) {
    if (pool == NULL) {
        return;
    }
    
    pthread_mutex_lock(&pool->queue_mutex);
    
    /* Wait until queue is empty */
    while (pool->queue_size > 0) {
        pthread_cond_wait(&pool->work_done, &pool->queue_mutex);
    }
    
    pthread_mutex_unlock(&pool->queue_mutex);
}
