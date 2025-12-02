/**
 * @file myrmdir.c
 * @brief A simple implementation of the 'rmdir' command in C.
 *
 * This program removes one or more empty directories.
 *
 * Compilation:
 * gcc myrmdir.c -o myrmdir
 *
 * Example Usages:
 *
 * # Remove an empty directory
 * ./myrmdir empty_dir
 *
 * # Attempt to remove multiple directories
 * ./myrmdir temp_folder old_logs
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

// --- Forward Declaration ---
void remove_empty_directory(const char *path);

// --- Main Function ---
int tool_myrmdir_main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "myrmdir: missing operand\n");
        return 1;
    }

    // --- Core Logic ---
    // Process each path provided
    for (int i = 1; i < argc; i++) {
        remove_empty_directory(argv[i]);
    }

    return 0;
}

/**
 * @brief Removes a single directory, but only if it is empty.
 * Provides detailed error messages for various failure conditions.
 *
 * @param path The path to the directory to be removed.
 */
void remove_empty_directory(const char *path) {
    struct stat path_stat;

    // Use lstat to check the path without following symlinks
    if (lstat(path, &path_stat) != 0) {
        fprintf(stderr, "myrmdir: failed to remove '%s': %s\n", path, strerror(errno));
        return;
    }

    // Check if the path is actually a directory
    if (!S_ISDIR(path_stat.st_mode)) {
        fprintf(stderr, "myrmdir: failed to remove '%s': Not a directory\n", path);
        return;
    }

    // Attempt to remove the directory
    if (rmdir(path) != 0) {
        // Provide a specific error message if the directory is not empty
        if (errno == ENOTEMPTY) {
            fprintf(stderr, "myrmdir: failed to remove '%s': Directory not empty\n", path);
        } else {
            // Provide a generic message for other errors (e.g., permissions)
            fprintf(stderr, "myrmdir: failed to remove '%s': %s\n", path, strerror(errno));
        }
    }
}
/*
```

### How to Compile and Run

1.  **Save the Code:** Save the content above into a file named `myrmdir.c`.
2.  **Compile:** Open your terminal and use GCC to compile the program:
    ```sh
    gcc myrmdir.c -o myrmdir
    ```
3.  **Run:** You can now use the program as described.

    **Setup for Examples:**
    ```sh
    # Create an empty directory, a non-empty one, and a file
    mkdir empty_dir
    mkdir not_empty_dir
    touch not_empty_dir/file.txt
    touch a_file.txt
    ```

    **Example Usages:**

    * **Successfully remove an empty directory:**
        ```sh
        ./myrmdir empty_dir
        # No output, directory is removed
        ```

    * **Attempt to remove a non-empty directory:**
        ```sh
        ./myrmdir not_empty_dir
        # Output: myrmdir: failed to remove 'not_empty_dir': Directory not empty
        ```

    * **Attempt to remove a path that is not a directory:**
        ```sh
        ./myrmdir a_file.txt
        # Output: myrmdir: failed to remove 'a_file.txt': Not a directory
        
*/
