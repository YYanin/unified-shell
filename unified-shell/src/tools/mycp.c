/**
 * @file mycp.c
 * @brief A simple implementation of the 'cp' command in C.
 *
 * This program copies files or directories with support for recursive directory
 * copying (-r) and an interactive prompt before overwriting (-i).
 * It also supports glob patterns for source files.
 *
 * Compilation:
 * gcc mycp.c -o mycp
 *
 * Example Usages:
 * ./mycp file1.txt /path/to/destination/
 * ./mycp -i file1.txt file2.txt # prompts before overwriting file2.txt
 * ./mycp -r src_dir/ dest_dir/
 * ./mycp -ir src_dir/ dest_dir/
 * ./mycp "*.txt" dest_dir/ # copies all .txt files to dest_dir
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <libgen.h>
#include <errno.h>

#define MAX_PATH_LEN 4096
#define BUFFER_SIZE 8192

// --- Forward Declarations ---
void copy_entry(const char *source, const char *dest, bool interactive, bool recursive);
void copy_file(const char *source, const char *dest, bool interactive);
void copy_directory(const char *source, const char *dest, bool interactive, bool recursive);

// --- Main Function ---
int tool_mycp_main(int argc, char **argv) {
    bool recursive = false;
    bool interactive = false;
    int opt;
    char *sources[argc];
    int source_count = 0;

    // --- Argument Parsing ---
    int first_path_index = 1;
    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '-') {
            // It's an option
            if (strcmp(argv[i], "--recursive") == 0) {
                recursive = true;
            } else if (strcmp(argv[i], "--interactive") == 0) {
                interactive = true;
            } else {
                for (int j = 1; argv[i][j] != '\0'; j++) {
                    if (argv[i][j] == 'r') {
                        recursive = true;
                    } else if (argv[i][j] == 'i') {
                        interactive = true;
                    } else {
                        fprintf(stderr, "mycp: invalid option -- '%c'\n", argv[i][j]);
                        return 1;
                    }
                }
            }
            first_path_index++;
        } else {
            // It's a source or destination path
            sources[source_count++] = argv[i];
        }
    }

    if (source_count < 2) {
        fprintf(stderr, "mycp: missing destination file operand\n");
        return 1;
    }

    char *destination = sources[source_count - 1];
    source_count--; // The last argument is the destination

    // --- Main Logic ---
    // Handle case where multiple sources are provided
    if (source_count > 1) {
        struct stat dest_stat;
        if (stat(destination, &dest_stat) != 0 || !S_ISDIR(dest_stat.st_mode)) {
            fprintf(stderr, "mycp: target '%s' is not a directory\n", destination);
            return 1;
        }
    }

    // Process each source (glob expansion already done by shell)
    for (int i = 0; i < source_count; i++) {
        copy_entry(sources[i], destination, interactive, recursive);
    }

    return 0;
}


/**
 * @brief Determines if the source is a file or directory and calls the appropriate copy function.
 *
 * @param source The source path.
 * @param dest The destination path.
 * @param interactive If true, prompt before overwrite.
 * @param recursive If true, allow directory copying.
 */
void copy_entry(const char *source, const char *dest, bool interactive, bool recursive) {
    struct stat source_stat;
    if (lstat(source, &source_stat) != 0) {
        perror("mycp: lstat");
        return;
    }

    if (S_ISDIR(source_stat.st_mode)) {
        if (!recursive) {
            fprintf(stderr, "mycp: -r not specified; omitting directory '%s'\n", source);
        } else {
            copy_directory(source, dest, interactive, recursive);
        }
    } else if (S_ISREG(source_stat.st_mode)) {
        copy_file(source, dest, interactive);
    } else {
        fprintf(stderr, "mycp: cannot copy '%s': Not a regular file or directory\n", source);
    }
}


/**
 * @brief Copies a single file to a destination.
 *
 * @param source The source file path.
 * @param dest The destination path (can be a file or directory).
 * @param interactive If true, prompt before overwriting an existing file.
 */
void copy_file(const char *source, const char *dest, bool interactive) {
    struct stat dest_stat;
    char final_dest[MAX_PATH_LEN];
    
    // Determine the final destination path
    if (stat(dest, &dest_stat) == 0 && S_ISDIR(dest_stat.st_mode)) {
        char *source_basename = basename((char *)source);
        snprintf(final_dest, sizeof(final_dest), "%s/%s", dest, source_basename);
    } else {
        strncpy(final_dest, dest, sizeof(final_dest) - 1);
        final_dest[sizeof(final_dest) - 1] = '\0';
    }

    // Interactive prompt if destination exists
    if (interactive && access(final_dest, F_OK) == 0) {
        printf("overwrite '%s'? ", final_dest);
        fflush(stdout); // Make sure the prompt is shown
        int c = getchar();
        if (c != 'y' && c != 'Y') {
             // Consume rest of the line
            while (c != '\n' && c != EOF) {
                c = getchar();
            }
            printf("not overwritten\n");
            return;
        }
        // Consume rest of the line
        while (c != '\n' && c != EOF) {
            c = getchar();
        }
    }

    // Open source and destination files
    int fd_from = open(source, O_RDONLY);
    if (fd_from == -1) {
        perror("mycp: open (source)");
        return;
    }

    struct stat source_stat;
    fstat(fd_from, &source_stat);

    int fd_to = open(final_dest, O_WRONLY | O_CREAT | O_TRUNC, source_stat.st_mode);
    if (fd_to == -1) {
        perror("mycp: open (destination)");
        close(fd_from);
        return;
    }
    
    // Copy data
    char buffer[BUFFER_SIZE];
    ssize_t nread;
    while ((nread = read(fd_from, buffer, sizeof(buffer))) > 0) {
        if (write(fd_to, buffer, nread) != nread) {
            perror("mycp: write");
            break;
        }
    }
    
    if (nread == -1) {
        perror("mycp: read");
    }

    close(fd_from);
    close(fd_to);
}

/**
 * @brief Recursively copies a directory and its contents.
 *
 * @param source The source directory path.
 * @param dest The destination path.
 * @param interactive If true, prompt before overwrite.
 * @param recursive Must be true.
 */
void copy_directory(const char *source, const char *dest, bool interactive, bool recursive) {
    DIR *d = opendir(source);
    if (!d) {
        perror("mycp: opendir");
        return;
    }

    struct stat source_stat, dest_stat;
    lstat(source, &source_stat);
    char new_dest[MAX_PATH_LEN];

    // Create destination directory
    if (stat(dest, &dest_stat) == 0 && S_ISDIR(dest_stat.st_mode)) {
        char *source_basename = basename((char *)source);
        snprintf(new_dest, sizeof(new_dest), "%s/%s", dest, source_basename);
    } else {
        strncpy(new_dest, dest, sizeof(new_dest) -1);
        new_dest[sizeof(new_dest)-1] = '\0';
    }

    if (mkdir(new_dest, source_stat.st_mode) != 0 && errno != EEXIST) {
        perror("mycp: mkdir");
        closedir(d);
        return;
    }
    
    // Loop through directory entries
    struct dirent *entry;
    while ((entry = readdir(d)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        char source_path[MAX_PATH_LEN];
        
        // Check for potential path length overflow before creating full source path
        if (strlen(source) + strlen(entry->d_name) + 2 > sizeof(source_path)) {
            fprintf(stderr, "mycp: source path is too long: %s/%s\n", source, entry->d_name);
            continue;
        }
        snprintf(source_path, sizeof(source_path), "%s/%s", source, entry->d_name);
        
        copy_entry(source_path, new_dest, interactive, recursive);
    }
    
    closedir(d);
}
/**

### How to Compile and Run

1.  **Save:** Save the code into a file named `mycp.c`.
2.  **Compile:** Use GCC to compile the program in your terminal.
    ```sh
    gcc mycp.c -o mycp
    ```
3.  **Run:** You can now use `mycp` similarly to the standard `cp` command.

    **Setup for Examples:**
    ```sh
    # Create some files and directories for testing
    mkdir src_dir
    touch src_dir/file1.txt src_dir/file2.log
    echo "hello" > file_a.txt
    mkdir dest_dir
    touch dest_dir/existing_file.txt
    ```

    **Example Usages:**
    * **Copy a single file:**
        ```sh
        ./mycp file_a.txt dest_dir/
        ```

    * **Copy with interactive prompt (file exists):**
        ```sh
        ./mycp file_a.txt dest_dir/existing_file.txt
        # You will be prompted: overwrite 'dest_dir/existing_file.txt'?
        # Press 'y' and Enter to confirm, or any other key to cancel.
        ```

    * **Copy a directory recursively:**
        ```sh
        ./mycp -r src_dir dest_dir/
        # This will create dest_dir/src_dir/ with all its contents.
        ```

    * **Use combined options:**
        ```sh
        ./mycp -ir src_dir dest_dir/
        ```

    * **Copy files matching a pattern:**
        ```sh
        ./mycp "src_dir/*.txt" dest_dir/
        # This copies src_dir/file1.txt to dest_dir/.
        # Note: Quoting the pattern is important so the shell doesn't expand it.
        
*/
