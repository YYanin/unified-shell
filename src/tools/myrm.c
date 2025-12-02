/**
 * @file myrm.c
 * @brief A simple implementation of the 'rm' command in C.
 *
 * This program removes files or directories. It supports interactive mode (-i)
 * to confirm removals and recursive mode (-r) to remove directories.
 *
 * Compilation:
 * gcc myrm.c -o myrm
 *
 * Example Usages:
 *
 * # Remove a file
 * ./myrm file1.txt
 *
 * # Interactively remove a file
 * ./myrm -i file_to_delete.log
 *
 * # Recursively remove a directory and its contents
 * ./myrm -r old_project/
 *
 * # Interactively and recursively remove a directory
 * ./myrm -ri sensitive_data/
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>

#define MAX_PATH_LEN 4096

// --- Forward Declarations ---
void remove_entry(const char *path, bool interactive, bool recursive);
void remove_directory_recursively(const char *path, bool interactive);
bool get_confirmation(const char *prompt_type, const char *path);

// --- Main Function ---
int tool_myrm_main(int argc, char **argv) {
    bool interactive = false;
    bool recursive = false;
    char *paths[argc];
    int path_count = 0;

    // --- Argument Parsing ---
    if (argc < 2) {
        fprintf(stderr, "myrm: missing operand\n");
        return 1;
    }
    
    // Separate options from paths
    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '-') {
            // It's an option, e.g., -i, -r, -ri, -ir
            for (int j = 1; argv[i][j] != '\0'; j++) {
                if (argv[i][j] == 'i') {
                    interactive = true;
                } else if (argv[i][j] == 'r') {
                    recursive = true;
                } else {
                    fprintf(stderr, "myrm: invalid option -- '%c'\n", argv[i][j]);
                    return 1;
                }
            }
        } else {
            // It's a path
            paths[path_count++] = argv[i];
        }
    }

    if (path_count == 0) {
        fprintf(stderr, "myrm: missing operand\n");
        return 1;
    }
    
    // --- Core Logic ---
    // Process each path provided
    for (int i = 0; i < path_count; i++) {
        remove_entry(paths[i], interactive, recursive);
    }

    return 0;
}

/**
 * @brief Determines the type of the entry at a given path (file, dir, etc.)
 * and calls the appropriate removal function.
 * * @param path The path to the file or directory.
 * @param interactive If true, prompt user before removing.
 * @param recursive If true, allow directory removal.
 */
void remove_entry(const char *path, bool interactive, bool recursive) {
    struct stat path_stat;
    // Use lstat to get info without following symbolic links
    if (lstat(path, &path_stat) != 0) {
        fprintf(stderr, "myrm: cannot remove '%s': %s\n", path, strerror(errno));
        return;
    }

    // Check if it's a directory
    if (S_ISDIR(path_stat.st_mode)) {
        if (!recursive) {
            fprintf(stderr, "myrm: cannot remove '%s': Is a directory\n", path);
            return;
        }
        remove_directory_recursively(path, interactive);
    } 
    // It's a file, symbolic link, or other non-directory entry
    else {
        const char* type_str = S_ISLNK(path_stat.st_mode) ? "symbolic link" : "regular file";
        if (interactive && !get_confirmation(type_str, path)) {
            return; // User said no
        }
        
        if (unlink(path) != 0) {
            fprintf(stderr, "myrm: cannot remove '%s': %s\n", path, strerror(errno));
        }
    }
}

/**
 * @brief Recursively removes a directory and all its contents.
 * * @param path The path to the directory.
 * @param interactive If true, prompt for each entry within the directory.
 */
void remove_directory_recursively(const char *path, bool interactive) {
    DIR *dir = opendir(path);
    if (!dir) {
        fprintf(stderr, "myrm: cannot open directory '%s': %s\n", path, strerror(errno));
        return;
    }

    struct dirent *entry;
    // Iterate over all entries in the directory
    while ((entry = readdir(dir)) != NULL) {
        // Skip special directories '.' and '..'
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        // Construct the full path for the entry
        char full_path[MAX_PATH_LEN];
        snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);

        // Recursively call the main removal function for the entry
        remove_entry(full_path, interactive, true); // recursive must be true here
    }
    closedir(dir);

    // After removing contents, remove the directory itself
    if (interactive && !get_confirmation("directory", path)) {
        return; // User said no
    }

    if (rmdir(path) != 0) {
        fprintf(stderr, "myrm: cannot remove directory '%s': %s\n", path, strerror(errno));
    }
}

/**
 * @brief Prompts the user for confirmation and returns their choice.
 * * @param prompt_type A string describing the type of file (e.g., "regular file").
 * @param path The path of the file being considered for removal.
 * @return true if the user confirms, false otherwise.
 */
bool get_confirmation(const char *prompt_type, const char *path) {
    printf("myrm: remove %s '%s'? ", prompt_type, path);
    fflush(stdout); // Ensure the prompt is shown

    int c = getchar();
    bool confirmed = (c == 'y' || c == 'Y');

    // Consume the rest of the line (e.g., the Enter key)
    while (c != '\n' && c != EOF) {
        c = getchar();
    }

    return confirmed;
}

