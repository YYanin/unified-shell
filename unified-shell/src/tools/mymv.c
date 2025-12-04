/**
 * @file mymv.c
 * @brief A simple implementation of the 'mv' command in C.
 *
 * This program moves or renames files and directories. It supports an
 * interactive mode to confirm overwrites.
 *
 * Compilation:
 * gcc mymv.c -o mymv
 *
 * Example Usages:
 *
 * # Rename file1.txt to file2.txt
 * ./mymv file1.txt file2.txt
 *
 * # Move file1.txt into the existing 'documents' directory
 * ./mymv file1.txt documents/
 *
 * # Move multiple files into the 'documents' directory
 * ./mymv report.docx notes.txt documents/
 *
 * # Interactively move a file, which will prompt if 'archive/photo.jpg' exists
 * ./mymv -i photo.jpg archive/
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <unistd.h>
#include <libgen.h>
#include <errno.h>

#define MAX_PATH_LEN 4096

// --- Forward Declaration ---
void move_or_rename(const char *source, const char *dest, bool interactive);

// --- Main Function ---
int tool_mymv_main(int argc, char **argv) {
    bool interactive = false;
    char *paths[argc];
    int path_count = 0;

    // --- Argument Parsing ---
    // Separate options from paths
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-i") == 0 || strcmp(argv[i], "--interactive") == 0) {
            interactive = true;
        } else if (argv[i][0] == '-') {
            fprintf(stderr, "mymv: invalid option '%s'\n", argv[i]);
            return 1;
        } else {
            paths[path_count++] = argv[i];
        }
    }

    // Check for correct number of path arguments
    if (path_count < 2) {
        fprintf(stderr, "mymv: missing destination file operand\n");
        fprintf(stderr, "Usage: ./mymv [-i] SOURCE DEST\n");
        fprintf(stderr, "   or: ./mymv [-i] SOURCE... DIRECTORY\n");
        return 1;
    }

    // --- Core Logic ---
    char *destination = paths[path_count - 1];
    int source_count = path_count - 1;

    // Case 1: Moving multiple files/dirs into a directory
    if (source_count > 1) {
        struct stat dest_stat;
        if (stat(destination, &dest_stat) != 0) {
            fprintf(stderr, "mymv: target '%s': %s\n", destination, strerror(errno));
            return 1;
        }
        if (!S_ISDIR(dest_stat.st_mode)) {
            fprintf(stderr, "mymv: target '%s' is not a directory\n", destination);
            return 1;
        }

        // Loop through all sources and move them into the destination directory
        for (int i = 0; i < source_count; i++) {
            char *source = paths[i];
            
            char source_copy[MAX_PATH_LEN];
            strncpy(source_copy, source, sizeof(source_copy) - 1);
            source_copy[sizeof(source_copy) - 1] = '\0';
            char *source_basename = basename(source_copy);
            
            char final_dest_path[MAX_PATH_LEN];
            snprintf(final_dest_path, sizeof(final_dest_path), "%s/%s", destination, source_basename);

            move_or_rename(source, final_dest_path, interactive);
        }
    } 
    // Case 2: Renaming a single file/dir OR moving it into a directory
    else {
        char *source = paths[0];
        struct stat dest_stat;

        // Check if destination exists and is a directory.
        if (stat(destination, &dest_stat) == 0 && S_ISDIR(dest_stat.st_mode)) {
            // It's a directory, so construct the path to move the source *inside* it.
            char source_copy[MAX_PATH_LEN];
            strncpy(source_copy, source, sizeof(source_copy) - 1);
            source_copy[sizeof(source_copy) - 1] = '\0';
            char *source_basename = basename(source_copy);
            
            char final_dest_path[MAX_PATH_LEN];
            snprintf(final_dest_path, sizeof(final_dest_path), "%s/%s", destination, source_basename);
            
            move_or_rename(source, final_dest_path, interactive);
        } else {
            // Destination is not a directory (or doesn't exist), so treat as a simple rename.
            move_or_rename(source, destination, interactive);
        }
    }

    return 0;
}

/**
 * @brief Handles the core logic of renaming a single entry.
 *
 * @param source The source path.
 * @param dest The destination path.
 * @param interactive If true, prompt before overwriting an existing file.
 */
void move_or_rename(const char *source, const char *dest, bool interactive) {
    if (access(source, F_OK) != 0) {
        fprintf(stderr, "mymv: cannot stat '%s': %s\n", source, strerror(errno));
        return;
    }
    
    if (interactive && access(dest, F_OK) == 0) {
        printf("mymv: overwrite '%s'? ", dest);
        fflush(stdout);
        int c = getchar();
        if (c != 'y' && c != 'Y') {
            while (c != '\n' && c != EOF) { c = getchar(); }
            fprintf(stderr, "not overwritten\n");
            return;
        }
        while (c != '\n' && c != EOF) { c = getchar(); }
    }

    if (rename(source, dest) != 0) {
        fprintf(stderr, "mymv: cannot move '%s' to '%s': %s\n", source, dest, strerror(errno));
    }
}


