/**
 * @file mycat.c
 * @brief A simple implementation of the 'cat' command in C.
 *
 * This program reads one or more files and prints their contents to
 * standard output in sequence.
 *
 * Compilation:
 * gcc mycat.c -o mycat
 *
 * Example Usages:
 *
 * # Display the contents of a single file
 * ./mycat chapter1.txt
 *
 * # Concatenate and display multiple files
 * ./mycat header.txt main.txt footer.txt
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>

// --- Forward Declaration ---
void display_file(const char *path);

// --- Main Function ---
int tool_mycat_main(int argc, char **argv) {
    // If no arguments are given, mycat should do nothing and exit.
    if (argc < 2) {
        return 0;
    }

    // --- Core Logic ---
    // Process each file path provided as an argument
    for (int i = 1; i < argc; i++) {
        display_file(argv[i]);
    }

    return 0;
}

/**
 * @brief Reads a file and prints its contents to standard output.
 * Handles errors for non-existent files, directories, and permissions.
 *
 * @param path The path of the file to be displayed.
 */
void display_file(const char *path) {
    struct stat path_stat;

    // First, check if the path exists and what type it is.
    // stat() will set errno if the file doesn't exist.
    if (stat(path, &path_stat) != 0) {
        fprintf(stderr, "mycat: '%s': %s\n", path, strerror(errno));
        return;
    }

    // Check if the path is a directory
    if (S_ISDIR(path_stat.st_mode)) {
        fprintf(stderr, "mycat: '%s': Is a directory\n", path);
        return;
    }

    // Attempt to open the file for reading
    FILE *file = fopen(path, "r");
    if (file == NULL) {
        // This will catch other errors like permission denied (EACCES)
        fprintf(stderr, "mycat: '%s': %s\n", path, strerror(errno));
        return;
    }

    // Read the file in chunks and write to stdout for efficiency
    char buffer[BUFSIZ]; // BUFSIZ is a standard buffer size defined in stdio.h
    size_t bytes_read;
    while ((bytes_read = fread(buffer, 1, sizeof(buffer), file)) > 0) {
        // fwrite returns the number of items successfully written.
        // If it's not equal to bytes_read, an error occurred.
        if (fwrite(buffer, 1, bytes_read, stdout) != bytes_read) {
            perror("mycat: write error");
            break;
        }
    }

    // Check if the loop terminated because of a read error
    if (ferror(file)) {
        fprintf(stderr, "mycat: error reading '%s'\n", path);
    }

    fclose(file);
}
/*
```

### How to Compile and Run

1.  **Save the Code:** Save the content above into a file named `mycat.c`.
2.  **Compile:** Open your terminal and use GCC to compile the program:
    ```sh
    gcc mycat.c -o mycat
    ```
3.  **Run:** You can now use the program to display file contents.

    **Setup for Examples:**
    ```sh
    # Create some test files and a directory
    echo "This is the header." > header.txt
    echo "This is the main content." > main.txt
    mkdir a_directory
    ```

    **Example Usages:**

    * **Display a single file:**
        ```sh
        ./mycat header.txt
        # Output: This is the header.
        ```

    * **Concatenate and display multiple files:**
        ```sh
        ./mycat header.txt main.txt
        # Output:
        # This is the header.
        # This is the main content.
        ```

    * **Attempt to use a non-existent file:**
        ```sh
        ./mycat no_such_file.txt
        # Output: mycat: 'no_such_file.txt': No such file or directory
        
*/
