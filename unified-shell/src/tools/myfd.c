/**
 * @file myfd.c
 * @brief A parallel file search utility, similar to 'fd' or 'find'.
 *
 * This program searches for filesystem entries recursively. It supports
 * glob pattern matching, filtering by type and extension, and can include
 * hidden files. Directory traversal is parallelized using pthreads to
 * improve performance on large filesystems.
 *
 * Compilation:
 * gcc myfd.c -o myfd -pthread
 *
 * Example Usage:
 * ./myfd --hidden -e rs -t f "^test_" /home/user/projects
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fnmatch.h>
#include <pthread.h>
#include <unistd.h>
#include <errno.h>
#include <limits.h>

#define MAX_GITIGNORE_PATTERNS 100
#define MAX_QUEUE_SIZE 4096

// --- Global Thread-Safe Structures ---

// A thread-safe queue for directories to be processed.
typedef struct {
    char* paths[MAX_QUEUE_SIZE];
    int head;
    int tail;
    int count;
    pthread_mutex_t mutex;
    pthread_cond_t can_push;
    pthread_cond_t can_pop;
} WorkQueue;

// --- Global Variables ---
WorkQueue g_work_queue;
int g_active_threads = 0;
bool g_work_done = false;
pthread_mutex_t g_active_threads_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t g_print_mutex = PTHREAD_MUTEX_INITIALIZER;
volatile bool g_error_occurred = false;

// Configuration for the search operation.
typedef struct {
    const char *pattern;
    const char *extension;
    char type_filter; // 'f' for file, 'd' for directory, 0 for none
    bool show_hidden;
    bool match_full_path;
} SearchConfig;

// Holds patterns from a .gitignore file.
typedef struct {
    char *patterns[MAX_GITIGNORE_PATTERNS];
    int count;
} Gitignore;


// --- Forward Declarations ---
void *worker_thread(void *arg);
void process_directory(const char *dir_path, const SearchConfig *config);
Gitignore myfd_load_gitignore(const char *dir_path);
void myfd_free_gitignore(Gitignore *gi);
bool myfd_is_ignored(const char *path, const Gitignore *gi);
void queue_init(WorkQueue *q);
void queue_push(WorkQueue *q, const char *path);
char* queue_pop(WorkQueue *q);
void queue_destroy(WorkQueue *q);

// --- Main Function ---
int tool_myfd_main(int argc, char **argv) {
    SearchConfig config = { .pattern = "*", .extension = NULL, .type_filter = 0, .show_hidden = false, .match_full_path = false };
    char *start_path = ".";
    char *pattern_arg = NULL;
    char *allocated_pattern = NULL; // To keep track of malloc'd memory

    // --- Argument Parsing ---
    int path_args = 0;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--hidden") == 0 || strcmp(argv[i], "-H") == 0) {
            config.show_hidden = true;
        } else if (strcmp(argv[i], "-e") == 0 && i + 1 < argc) {
            config.extension = argv[++i];
        } else if (strcmp(argv[i], "-t") == 0 && i + 1 < argc) {
            config.type_filter = argv[++i][0];
            if (config.type_filter != 'f' && config.type_filter != 'd') {
                fprintf(stderr, "myfd: invalid type '%c'. Use 'f' or 'd'.\n", config.type_filter);
                return 1;
            }
        } else if (strcmp(argv[i], "--full-path") == 0) {
            config.match_full_path = true;
        } else if (argv[i][0] == '-') {
            fprintf(stderr, "myfd: unknown option '%s'\n", argv[i]);
            return 1;
        } else {
            if (path_args == 0) pattern_arg = argv[i];
            else if (path_args == 1) start_path = argv[i];
            else {
                fprintf(stderr, "myfd: too many path arguments\n");
                return 1;
            }
            path_args++;
        }
    }

    // Determine which argument is the pattern and which is the path
    if (path_args == 1) { // Only one non-option arg
        struct stat st;
        // If it's a directory, it's the path, and pattern is default '*'
        if (stat(pattern_arg, &st) == 0 && S_ISDIR(st.st_mode)) {
            start_path = pattern_arg;
        } else { // Otherwise, it's the pattern
            config.pattern = pattern_arg;
        }
    } else if (path_args == 2) { // Both pattern and path given
        config.pattern = pattern_arg;
    }
    
    // If the pattern ended up being an empty string from the command line,
    // treat it as the default "match all" pattern.
    if (config.pattern != NULL && strcmp(config.pattern, "") == 0) {
        config.pattern = "*";
    }

    // If the user provides a literal pattern without wildcards,
    // wrap it in '*' to perform a "contains" search by default.
    if (config.pattern != NULL && strcmp(config.pattern, "*") != 0 &&
        strchr(config.pattern, '*') == NULL &&
        strchr(config.pattern, '?') == NULL &&
        strchr(config.pattern, '[') == NULL) {

        // Allocate enough space for "*pattern*" + null terminator
        allocated_pattern = malloc(strlen(config.pattern) + 3);
        if (allocated_pattern == NULL) {
            perror("myfd: malloc");
            return 1;
        }
        sprintf(allocated_pattern, "*%s*", config.pattern);
        config.pattern = allocated_pattern;
    }


    // --- Threading Setup ---
    int num_threads = sysconf(_SC_NPROCESSORS_ONLN);
    if (num_threads <= 0) num_threads = 2; // Default to 2 threads if detection fails
    pthread_t threads[num_threads];

    queue_init(&g_work_queue);

    // Create worker threads
    for (int i = 0; i < num_threads; i++) {
        if (pthread_create(&threads[i], NULL, worker_thread, &config) != 0) {
            perror("myfd: pthread_create");
            free(allocated_pattern); // Clean up on error
            return 1;
        }
    }

    // --- Start Search ---
    char initial_path[PATH_MAX];
    if (realpath(start_path, initial_path) == NULL) {
        fprintf(stderr, "myfd: invalid start path '%s': %s\n", start_path, strerror(errno));
        free(allocated_pattern); // Clean up on error
        return 1;
    }
    queue_push(&g_work_queue, initial_path);

    // --- Wait for Completion ---
    while (true) {
        pthread_mutex_lock(&g_active_threads_mutex);
        bool done = (g_work_queue.count == 0 && g_active_threads == 0);
        pthread_mutex_unlock(&g_active_threads_mutex);
        if (done) break;
        usleep(10000); // Sleep briefly to avoid busy-waiting
    }

    // --- Shutdown ---
    g_work_done = true;
    pthread_cond_broadcast(&g_work_queue.can_pop); // Wake up any waiting threads so they can exit

    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }
    
    free(allocated_pattern); // Clean up the pattern string if we allocated it
    queue_destroy(&g_work_queue);
    pthread_mutex_destroy(&g_active_threads_mutex);
    pthread_mutex_destroy(&g_print_mutex);

    return g_error_occurred ? 1 : 0;
}

/**
 * @brief The main function for each worker thread.
 * Pops directories from the work queue and processes them until all work is done.
 */
void *worker_thread(void *arg) {
    SearchConfig *config = (SearchConfig *)arg;

    while (true) {
        char *dir_path = queue_pop(&g_work_queue);

        if (dir_path == NULL) { // NULL is the signal that all work is done
            break;
        }

        // Increment active thread count
        pthread_mutex_lock(&g_active_threads_mutex);
        g_active_threads++;
        pthread_mutex_unlock(&g_active_threads_mutex);

        process_directory(dir_path, config);
        free(dir_path);

        // Decrement active thread count
        pthread_mutex_lock(&g_active_threads_mutex);
        g_active_threads--;
        pthread_mutex_unlock(&g_active_threads_mutex);
    }
    return NULL;
}

/**
 * @brief Scans a single directory, matches files, and enqueues subdirectories.
 */
void process_directory(const char *dir_path, const SearchConfig *config) {
    DIR *dir = opendir(dir_path);
    if (!dir) {
        pthread_mutex_lock(&g_print_mutex);
        fprintf(stderr, "myfd: cannot read directory '%s': %s\n", dir_path, strerror(errno));
        pthread_mutex_unlock(&g_print_mutex);
        g_error_occurred = true;
        return;
    }

    Gitignore gi = myfd_load_gitignore(dir_path);
    struct dirent *entry;

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        // --- Filtering ---
        if (!config->show_hidden && entry->d_name[0] == '.') {
            continue;
        }

        if (myfd_is_ignored(entry->d_name, &gi)) {
            continue;
        }

        char full_path[PATH_MAX];
        snprintf(full_path, sizeof(full_path), "%s/%s", dir_path, entry->d_name);

        struct stat st;
        if (lstat(full_path, &st) != 0) continue;

        bool is_dir = S_ISDIR(st.st_mode);

        if (config->type_filter) {
            if (config->type_filter == 'f' && is_dir) continue;
            if (config->type_filter == 'd' && !is_dir) continue;
        }

        if (config->extension) {
            if (is_dir) continue; // Extensions only apply to files
            const char *dot = strrchr(entry->d_name, '.');
            if (!dot || strcmp(dot + 1, config->extension) != 0) {
                continue;
            }
        }

        // --- Pattern Matching ---
        const char *match_target = config->match_full_path ? full_path : entry->d_name;
        if (fnmatch(config->pattern, match_target, FNM_PATHNAME) == 0) {
            pthread_mutex_lock(&g_print_mutex);
            printf("%s\n", full_path);
            pthread_mutex_unlock(&g_print_mutex);
        }

        // If it's a directory, add it to the queue for further processing
        if (is_dir) {
            queue_push(&g_work_queue, full_path);
        }
    }

    myfd_free_gitignore(&gi);
    closedir(dir);
}


// --- Utility and Queue Functions ---

void queue_init(WorkQueue *q) {
    q->head = 0;
    q->tail = 0;
    q->count = 0;
    pthread_mutex_init(&q->mutex, NULL);
    pthread_cond_init(&q->can_push, NULL);
    pthread_cond_init(&q->can_pop, NULL);
}

void queue_push(WorkQueue *q, const char *path) {
    pthread_mutex_lock(&q->mutex);
    while (q->count == MAX_QUEUE_SIZE) {
        pthread_cond_wait(&q->can_push, &q->mutex);
    }
    q->paths[q->tail] = strdup(path);
    q->tail = (q->tail + 1) % MAX_QUEUE_SIZE;
    q->count++;
    pthread_cond_signal(&q->can_pop);
    pthread_mutex_unlock(&q->mutex);
}

char* queue_pop(WorkQueue *q) {
    pthread_mutex_lock(&q->mutex);
    while (q->count == 0) {
        if (g_work_done) {
            pthread_mutex_unlock(&q->mutex);
            return NULL; // Signal to terminate
        }
        pthread_cond_wait(&q->can_pop, &q->mutex);
    }
    if (q->count == 0 && g_work_done) { // Re-check after waking up
         pthread_mutex_unlock(&q->mutex);
         return NULL;
    }
    char *path = q->paths[q->head];
    q->head = (q->head + 1) % MAX_QUEUE_SIZE;
    q->count--;
    pthread_cond_signal(&q->can_push);
    pthread_mutex_unlock(&q->mutex);
    return path;
}

void queue_destroy(WorkQueue *q) {
    // Free any remaining paths in the queue
    while (q->count > 0) {
        free(q->paths[q->head]);
        q->head = (q->head + 1) % MAX_QUEUE_SIZE;
        q->count--;
    }
    pthread_mutex_destroy(&q->mutex);
    pthread_cond_destroy(&q->can_push);
    pthread_cond_destroy(&q->can_pop);
}

Gitignore myfd_load_gitignore(const char *dir_path) {
    Gitignore gi = { .count = 0 };
    char gitignore_path[PATH_MAX];
    snprintf(gitignore_path, sizeof(gitignore_path), "%s/.gitignore", dir_path);

    FILE *f = fopen(gitignore_path, "r");
    if (!f) return gi;

    char *line = NULL;
    size_t len = 0;
    while (getline(&line, &len, f) != -1 && gi.count < MAX_GITIGNORE_PATTERNS) {
        // Remove trailing newline
        line[strcspn(line, "\r\n")] = 0;
        // Ignore empty lines and comments
        if (strlen(line) > 0 && line[0] != '#') {
            gi.patterns[gi.count++] = strdup(line);
        }
    }
    free(line);
    fclose(f);
    return gi;
}

void myfd_free_gitignore(Gitignore *gi) {
    for (int i = 0; i < gi->count; i++) {
        free(gi->patterns[i]);
    }
    gi->count = 0;
}

bool myfd_is_ignored(const char *path, const Gitignore *gi) {
    for (int i = 0; i < gi->count; i++) {
        // fnmatch should be used with FNM_PATHNAME for gitignore-style matching
        if (fnmatch(gi->patterns[i], path, FNM_PATHNAME) == 0) {
            return true;
        }
    }
    return false;
}


/*
```

### How to Compile and Run

1.  **Save the Code:** Save the content above into a file named `myfd.c`.
2.  **Compile:** Open your terminal and compile the program using GCC. The `-pthread` flag is **essential** to link the POSIX threads library.
    ```sh
    gcc myfd.c -o myfd -pthread
    ```
3.  **Run:** You can now execute the program.

    **Setup for Examples:**
    ```sh
    # Create a test directory structure
    mkdir -p my_project/src my_project/tests .hidden_dir
    touch my_project/src/main.rs my_project/src/lib.rs my_project/tests/test_main.rs
    touch my_project/.DS_Store .hidden_dir/secret.txt
    echo "target/" > my_project/.gitignore
    mkdir my_project/target
    touch my_project/target/app
    ```

    **Example Usages:**

    * **Find all entries (ignores hidden, respects .gitignore):**
        ```sh
        ./myfd "" my_project
        # Output might include:
        # my_project/src
        # my_project/src/main.rs
        # my_project/src/lib.rs
        # my_project/tests
        # my_project/tests/test_main.rs
        # (Note: .DS_Store and target/ are ignored)
        ```

    * **Find all entries, including hidden ones:**
        ```sh
        ./myfd --hidden "" my_project
        # Output will now include my_project/.DS_Store
        ```

    * **Find only files (`-t f`) with the `.rs` extension (`-e rs`):**
        ```sh
        ./myfd -t f -e rs "" my_project
        # Output:
        # my_project/src/main.rs
        # my_project/src/lib.rs
        # my_project/tests/test_main.rs
        ```

    * **Find files starting with "test" using a glob pattern:**
        ```sh
        ./myfd "test*" my_project
        # Output:
        # my_project/tests/test_main.rs
        
*/
