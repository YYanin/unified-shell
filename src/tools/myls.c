/**
 * @file myls.c
 * @brief A simple implementation of the 'ls' command in C.
 *
 * This program lists the contents of a directory with support for long format (-l),
 * showing all files (-a), and respecting .gitignore files.
 *
 * Compilation:
 * gcc myls.c -o myls
 *
 * Example Usages:
 * ./myls
 * ./myls /path/to/directory
 * ./myls -l
 * ./myls -a
 * ./myls -la src
 * ./myls "*.c"
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <fnmatch.h>
#include <stdbool.h>
#include <libgen.h>

#define MAX_IGNORE_PATTERNS 100
#define MAX_PATH_LEN 1024

// --- Forward Declarations ---

void list_directory(const char *path, bool show_all, bool long_format, const char *pattern);
void print_long_format(const char *filepath, const char *name);
void load_gitignore(const char *dir_path, char ignore_patterns[MAX_IGNORE_PATTERNS][256], int *ignore_count);
bool should_ignore(const char *name, char ignore_patterns[MAX_IGNORE_PATTERNS][256], int ignore_count);

// --- Main Function ---

int tool_myls_main(int argc, char **argv) {
    bool show_all = false;
    bool long_format = false;
    const char *path = ".";
    const char *pattern = "*"; // Default pattern: match all

    int path_arg_count = 0;

    // --- Argument Parsing ---
    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '-') {
            // It's an option
            for (int j = 1; argv[i][j] != '\0'; j++) {
                if (argv[i][j] == 'l') {
                    long_format = true;
                } else if (argv[i][j] == 'a') {
                    show_all = true;
                } else {
                    fprintf(stderr, "myls: invalid option -- '%c'\n", argv[i][j]);
                    return 1;
                }
            }
        } else {
            // It's a path or pattern
            path_arg_count++;
            // A simple approach: first non-option is path, second is pattern
            // A more robust ls handles multiple paths and complex patterns
            if (path_arg_count == 1) {
                path = argv[i];
            } else if (path_arg_count == 2) {
                pattern = argv[i];
            } else {
                 fprintf(stderr, "myls: too many arguments. Provide at most one path and one pattern.\n");
                 return 1;
            }
        }
    }
    
    // If the provided path might be a pattern, separate directory from pattern
    char path_copy[MAX_PATH_LEN];
    strncpy(path_copy, path, sizeof(path_copy) - 1);
    path_copy[sizeof(path_copy) - 1] = '\0';
    
    char *dir = dirname(path_copy);
    
    // If a pattern was not the second argument, it might be part of the path argument
    if (path_arg_count < 2) {
        // Check if the base part of the path contains a wildcard
        char base_copy[MAX_PATH_LEN];
        strncpy(base_copy, path, sizeof(base_copy) -1);
        base_copy[sizeof(base_copy)-1] = '\0';
        char *base = basename(base_copy);

        if (strchr(base, '*') != NULL || strchr(base, '?') != NULL || strchr(base, '[') != NULL) {
            pattern = base;
            path = dir;
        }
    }


    list_directory(path, show_all, long_format, pattern);

    return 0;
}

/**
 * @brief Lists the contents of a given directory.
 *
 * @param path The path to the directory.
 * @param show_all If true, includes hidden files (starting with '.').
 * @param long_format If true, prints in long format.
 * @param pattern A glob pattern to filter file names.
 */
void list_directory(const char *path, bool show_all, bool long_format, const char *pattern) {
    DIR *d = opendir(path);
    if (d == NULL) {
        perror("myls: cannot open directory");
        return;
    }

    char ignore_patterns[MAX_IGNORE_PATTERNS][256];
    int ignore_count = 0;
    load_gitignore(path, ignore_patterns, &ignore_count);

    struct dirent *dir_entry;
    while ((dir_entry = readdir(d)) != NULL) {
        const char *name = dir_entry->d_name;

        // --- Filtering Logic ---
        // 1. Ignore '.' and '..'
        if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0) {
            continue;
        }

        // 2. Ignore hidden files if -a is not specified
        if (!show_all && name[0] == '.') {
            continue;
        }

        // 3. Ignore files based on .gitignore patterns
        if (should_ignore(name, ignore_patterns, ignore_count)) {
            continue;
        }

        // 4. Match against user-provided pattern
        if (fnmatch(pattern, name, 0) != 0) {
            continue;
        }

        // --- Printing Logic ---
        if (long_format) {
            char full_path[MAX_PATH_LEN];
            snprintf(full_path, sizeof(full_path), "%s/%s", path, name);
            print_long_format(full_path, name);
        } else {
            printf("%s\n", name);
        }
    }

    closedir(d);
}


/**
 * @brief Prints detailed information about a file (long format).
 *
 * @param filepath The full path to the file.
 * @param name The name of the file to display.
 */
void print_long_format(const char *filepath, const char *name) {
    struct stat file_stat;
    // Use lstat to get info about the link itself, not the file it points to
    if (lstat(filepath, &file_stat) == -1) {
        perror("myls: lstat");
        return;
    }

    // 1. Permissions
    printf((S_ISDIR(file_stat.st_mode)) ? "d" : "-");
    printf((S_ISLNK(file_stat.st_mode)) ? "l" : "-");
    printf((file_stat.st_mode & S_IRUSR) ? "r" : "-");
    printf((file_stat.st_mode & S_IWUSR) ? "w" : "-");
    printf((file_stat.st_mode & S_IXUSR) ? "x" : "-");
    printf((file_stat.st_mode & S_IRGRP) ? "r" : "-");
    printf((file_stat.st_mode & S_IWGRP) ? "w" : "-");
    printf((file_stat.st_mode & S_IXGRP) ? "x" : "-");
    printf((file_stat.st_mode & S_IROTH) ? "r" : "-");
    printf((file_stat.st_mode & S_IWOTH) ? "w" : "-");
    printf((file_stat.st_mode & S_IXOTH) ? "x" : "-");

    // 2. Number of hard links
    printf(" %2lu", file_stat.st_nlink);

    // 3. Owner and Group
    struct passwd *pw = getpwuid(file_stat.st_uid);
    struct group *gr = getgrgid(file_stat.st_gid);
    printf(" %-8s %-8s", pw ? pw->pw_name : "unknown", gr ? gr->gr_name : "unknown");

    // 4. Size
    printf(" %8lld", (long long)file_stat.st_size);

    // 5. Modification time
    char time_buf[20];
    strftime(time_buf, sizeof(time_buf), "%b %d %H:%M", localtime(&file_stat.st_mtime));
    printf(" %s", time_buf);

    // 6. Name and symbolic link target
    printf(" %s", name);
    if (S_ISLNK(file_stat.st_mode)) {
        char link_target[MAX_PATH_LEN];
        ssize_t len = readlink(filepath, link_target, sizeof(link_target) - 1);
        if (len != -1) {
            link_target[len] = '\0';
            printf(" -> %s", link_target);
        }
    }
    printf("\n");
}


/**
 * @brief Loads patterns from a .gitignore file in the specified directory.
 * NOTE: This is a simplified implementation. It does not search parent directories.
 *
 * @param dir_path The directory containing the .gitignore file.
 * @param ignore_patterns A 2D array to store the loaded patterns.
 * @param ignore_count A pointer to an integer to store the number of patterns loaded.
 */
void load_gitignore(const char *dir_path, char ignore_patterns[MAX_IGNORE_PATTERNS][256], int *ignore_count) {
    char gitignore_path[MAX_PATH_LEN];
    snprintf(gitignore_path, sizeof(gitignore_path), "%s/.gitignore", dir_path);

    FILE *file = fopen(gitignore_path, "r");
    if (file == NULL) {
        return; // No .gitignore file found, which is fine
    }

    char line[256];
    while (fgets(line, sizeof(line), file) && *ignore_count < MAX_IGNORE_PATTERNS) {
        // Remove trailing newline
        line[strcspn(line, "\n")] = 0;
        
        // Ignore empty lines and comments
        if (line[0] == '\0' || line[0] == '#') {
            continue;
        }

        // Store the pattern
        strncpy(ignore_patterns[*ignore_count], line, 255);
        ignore_patterns[*ignore_count][255] = '\0';
        (*ignore_count)++;
    }

    fclose(file);
}

/**
 * @brief Checks if a filename matches any of the .gitignore patterns.
 *
 * @param name The filename to check.
 * @param ignore_patterns The array of patterns from .gitignore.
 * @param ignore_count The number of patterns in the array.
 * @return true if the file should be ignored, false otherwise.
 */
bool should_ignore(const char *name, char ignore_patterns[MAX_IGNORE_PATTERNS][256], int ignore_count) {
    for (int i = 0; i < ignore_count; i++) {
        if (fnmatch(ignore_patterns[i], name, 0) == 0) {
            return true;
        }
    }
    return false;
}

