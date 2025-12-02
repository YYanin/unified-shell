/**
 * @file mytouch.c
 * @brief A simple implementation of the 'touch' command in C.
 *
 * This program creates empty files if they do not exist, or updates
 * the access and modification timestamps of existing files to the current time.
 *
 * Compilation:
 * gcc mytouch.c -o mytouch
 *
 * Example Usages:
 *
 * # Create a new, empty file
 * ./mytouch new_file.txt
 *
 * # Create multiple files
 * ./mytouch file1.txt file2.log
 *
 * # Update the timestamp of an existing file
 * ./mytouch existing_document.md
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <utime.h>
#include <errno.h>
#include <sys/stat.h>

// --- Forward Declaration ---
void touch_file(const char *path);

// --- Main Function ---
int tool_mytouch_main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "mytouch: missing file operand\n");
        return 1;
    }

    // --- Core Logic ---
    // Process each path provided
    for (int i = 1; i < argc; i++) {
        touch_file(argv[i]);
    }

    return 0;
}

/**
 * @brief Creates a file if it doesn't exist, and updates the access
 * and modification times for a file to the current time.
 *
 * @param path The path to the file to create or update.
 */
void touch_file(const char *path) {
    // First, check if the file exists.
    int file_exists = (access(path, F_OK) == 0);

    if (!file_exists) {
        // File does not exist, so create it.
        // O_CREAT: create the file if it does not exist.
        // O_WRONLY: open for writing only.
        int fd = open(path, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH); // 0644 permissions
        if (fd == -1) {
            fprintf(stderr, "mytouch: cannot touch '%s': %s\n", path, strerror(errno));
            return;
        }
        close(fd);
    } else {
        // File exists, so update its timestamps.
        // Passing NULL to utime() sets the access and modification
        // times to the current time.
        if (utime(path, NULL) != 0) {
            fprintf(stderr, "mytouch: cannot touch '%s': %s\n", path, strerror(errno));
        }
    }
}
/*
```

### How to Compile and Run

1.  **Save the Code:** Save the content above into a file named `mytouch.c`.
2.  **Compile:** Open your terminal and use GCC to compile the program:
    ```sh
    gcc mytouch.c -o mytouch
    ```
3.  **Run:** You can now use the program to create or update files.

    **Example Usages:**

    * **Create a new file:**
        ```sh
        ./mytouch new_file.txt
        ls -l new_file.txt  # Verify it was created
        ```

    * **Create multiple files at once:**
        ```sh
        ./mytouch report.log config.yml
        ```

    * **Update the timestamp of an existing file:**
        ```sh
        # Wait a moment after creation
        sleep 2
        ./mytouch new_file.txt
        ls -l new_file.txt  # Verify the timestamp has changed
        
*/
