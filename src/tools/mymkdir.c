/**
 * @file mymkdir.c
 * @brief A simple implementation of the 'mkdir' command in C.
 *
 * This program creates directories. It supports a -p option to create
 * parent directories as needed.
 *
 * Compilation:
 * gcc mymkdir.c -o mymkdir
 *
 * Example Usages:
 *
 * # Create a single directory
 * ./mymkdir new_folder
 *
 * # Create multiple directories
 * ./mymkdir drafts assets
 *
 * # Create nested directories (will fail without -p)
 * ./mymkdir project/src/components
 *
 * # Create nested directories using the -p flag
 * ./mymkdir -p project/src/components
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>

#define MAX_PATH_LEN 4096
#define DEFAULT_MODE 0775 // rwxrwxr-x

// --- Forward Declarations ---
void create_directory(const char *path, bool create_parents);
int mkdir_p(const char *path);

// --- Main Function ---
int tool_mymkdir_main(int argc, char **argv) {
    bool create_parents = false;
    char *paths[argc];
    int path_count = 0;

    if (argc < 2) {
        fprintf(stderr, "mymkdir: missing operand\n");
        return 1;
    }

    // --- Argument Parsing ---
    // Separate options from paths
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-p") == 0) {
            create_parents = true;
        } else if (argv[i][0] == '-') {
            fprintf(stderr, "mymkdir: invalid option '%s'\n", argv[i]);
            return 1;
        } else {
            paths[path_count++] = argv[i];
        }
    }

    if (path_count == 0) {
        fprintf(stderr, "mymkdir: missing operand\n");
        return 1;
    }

    // --- Core Logic ---
    // Process each path provided
    for (int i = 0; i < path_count; i++) {
        create_directory(paths[i], create_parents);
    }

    return 0;
}

/**
 * @brief Decides whether to do a simple mkdir or a recursive one.
 *
 * @param path The directory path to create.
 * @param create_parents If true, create parent directories as needed.
 */
void create_directory(const char *path, bool create_parents) {
    if (create_parents) {
        // mkdir_p handles its own errors, including EEXIST.
        mkdir_p(path);
    } else {
        if (mkdir(path, DEFAULT_MODE) != 0) {
            // Provide the specific error message when the file exists
            if (errno == EEXIST) {
                fprintf(stderr, "mymkdir: cannot create directory '%s': File exists\n", path);
            } else {
                fprintf(stderr, "mymkdir: cannot create directory '%s': %s\n", path, strerror(errno));
            }
        }
    }
}

/**
 * @brief Creates a directory and any necessary parent directories.
 * This function mimics the behavior of 'mkdir -p'.
 *
 * @param path The full directory path to create.
 * @return 0 on success, -1 on failure.
 */
int mkdir_p(const char *path) {
    // Make a mutable copy of the path since we'll be modifying it
    char tmp_path[MAX_PATH_LEN];
    snprintf(tmp_path, sizeof(tmp_path), "%s", path);

    // Handle and remove any trailing slash
    size_t len = strlen(tmp_path);
    if (len > 0 && tmp_path[len - 1] == '/') {
        tmp_path[len - 1] = '\0';
    }

    // Iterate through the path and create each component
    // Start after the first character in case it's a leading '/'
    for (char *p = tmp_path + 1; *p; p++) {
        if (*p == '/') {
            *p = '\0'; // Temporarily terminate the string

            // Create the directory component
            if (mkdir(tmp_path, DEFAULT_MODE) != 0) {
                // It's only an error if the directory doesn't already exist
                if (errno != EEXIST) {
                    fprintf(stderr, "mymkdir: cannot create directory '%s': %s\n", tmp_path, strerror(errno));
                    return -1;
                }
            }
            *p = '/'; // Restore the slash
        }
    }

    // Create the final, full directory
    if (mkdir(tmp_path, DEFAULT_MODE) != 0) {
        if (errno != EEXIST) {
            fprintf(stderr, "mymkdir: cannot create directory '%s': %s\n", tmp_path, strerror(errno));
            return -1;
        }
    }

    return 0;
}
/*
### How to Compile and Run

1.  **Save the Code:** Save the content above into a file named `mymkdir.c`.
2.  **Compile:** Open your terminal and use GCC to compile the program:
    ```sh
    gcc mymkdir.c -o mymkdir
    ```
3.  **Run:** You can now use the program as described.

    **Example Usages:**

    * **Create a single directory:**
        ```sh
        ./mymkdir new_folder
        ```

    * **Attempt to create an existing directory (will show an error):**
        ```sh
        ./mymkdir new_folder
        # Output: mymkdir: cannot create directory 'new_folder': File exists
        ```

    * **Create multiple directories at once:**
        ```sh
        ./mymkdir drafts assets
        ```

    * **Attempt to create a nested path without `-p` (will fail):**
        ```sh
        ./mymkdir project/src
        # Output: mymkdir: cannot create directory 'project/src': No such file or directory
        ```

    * **Successfully create a nested path with `-p`:**
        ```sh
        ./mymkdir -p project/src/components
*/
