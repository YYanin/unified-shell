/**
 * @file mystat.c
 * @brief A simple implementation of the 'stat' command in C.
 *
 * This program displays detailed information about one or more files,
 * including size, permissions, ownership, and timestamps.
 *
 * Compilation:
 * gcc mystat.c -o mystat
 *
 * Example Usage:
 * ./mystat /etc/passwd
 */

#define _DEFAULT_SOURCE // For tm_zone
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include <pwd.h>
#include <grp.h>
#include <string.h>
#include <errno.h>

// --- Forward Declarations ---
void display_stat_info(const char *path);
void format_permissions(mode_t mode, char *str);
const char* get_file_type(mode_t mode);

// --- Main Function ---
int tool_mystat_main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "mystat: missing operand\n");
        return 1;
    }

    // Process each file path provided as an argument
    for (int i = 1; i < argc; i++) {
        display_stat_info(argv[i]);
        if (i < argc - 1) {
            printf("\n"); // Add a newline between outputs for multiple files
        }
    }

    return 0;
}

/**
 * @brief Gathers file stats using lstat and prints them in a formatted way.
 *
 * @param path The path to the file to inspect.
 */
void display_stat_info(const char *path) {
    struct stat sb;

    // Use lstat() to get information about the file itself,
    // not what it points to if it's a symbolic link.
    if (lstat(path, &sb) == -1) {
        fprintf(stderr, "mystat: cannot stat '%s': %s\n", path, strerror(errno));
        return;
    }

    // --- Format Data for Display ---

    // 1. Permissions string (-rwxrwxrwx)
    char perms_str[11];
    format_permissions(sb.st_mode, perms_str);

    // 2. User and Group names
    struct passwd *pw = getpwuid(sb.st_uid);
    struct group  *gr = getgrgid(sb.st_gid);
    const char *user_name = pw ? pw->pw_name : "UNKNOWN";
    const char *group_name = gr ? gr->gr_name : "UNKNOWN";

    // 3. Timestamps
    char access_time[100], modify_time[100], change_time[100];
    char time_buffer[64];
    struct tm *tm_info;

    // Access time
    tm_info = localtime(&sb.st_atim.tv_sec);
    strftime(time_buffer, sizeof(time_buffer), "%Y-%m-%d %H:%M:%S", tm_info);
    snprintf(access_time, sizeof(access_time), "%s.%09ld %s", time_buffer, sb.st_atim.tv_nsec, tm_info->tm_zone);

    // Modify time
    tm_info = localtime(&sb.st_mtim.tv_sec);
    strftime(time_buffer, sizeof(time_buffer), "%Y-%m-%d %H:%M:%S", tm_info);
    snprintf(modify_time, sizeof(modify_time), "%s.%09ld %s", time_buffer, sb.st_mtim.tv_nsec, tm_info->tm_zone);

    // Change time
    tm_info = localtime(&sb.st_ctim.tv_sec);
    strftime(time_buffer, sizeof(time_buffer), "%Y-%m-%d %H:%M:%S", tm_info);
    snprintf(change_time, sizeof(change_time), "%s.%09ld %s", time_buffer, sb.st_ctim.tv_nsec, tm_info->tm_zone);


    // --- Print Formatted Output ---

    printf("  File: %s\n", path);
    printf("  Size: %-15ld Blocks: %-10ld IO Block: %-6ld %s\n",
           (long)sb.st_size, (long)sb.st_blocks, (long)sb.st_blksize, get_file_type(sb.st_mode));
    printf("Device: %lxh/%lud\t Inode: %-11lu Links: %lu\n",
           (unsigned long)sb.st_dev, (unsigned long)sb.st_dev, (unsigned long)sb.st_ino, (unsigned long)sb.st_nlink);
    printf("Access: (%04o/%s)  Uid: (%5u/%8s)   Gid: (%5u/%8s)\n",
           sb.st_mode & 07777, perms_str, sb.st_uid, user_name, sb.st_gid, group_name);
    printf("Access: %s\n", access_time);
    printf("Modify: %s\n", modify_time);
    printf("Change: %s\n", change_time);
}

/**
 * @brief Formats the file mode into a symbolic permission string (e.g., -rwxr-xr-x).
 *
 * @param mode The st_mode field from a stat struct.
 * @param str A character buffer (at least 11 chars long) to store the result.
 */
void format_permissions(mode_t mode, char *str) {
    // File type character
    if (S_ISREG(mode)) str[0] = '-';
    else if (S_ISDIR(mode)) str[0] = 'd';
    else if (S_ISLNK(mode)) str[0] = 'l';
    else if (S_ISCHR(mode)) str[0] = 'c';
    else if (S_ISBLK(mode)) str[0] = 'b';
    else if (S_ISFIFO(mode)) str[0] = 'p';
    else if (S_ISSOCK(mode)) str[0] = 's';
    else str[0] = '?';

    // User permissions
    str[1] = (mode & S_IRUSR) ? 'r' : '-';
    str[2] = (mode & S_IWUSR) ? 'w' : '-';
    str[3] = (mode & S_IXUSR) ? 'x' : '-';

    // Group permissions
    str[4] = (mode & S_IRGRP) ? 'r' : '-';
    str[5] = (mode & S_IWGRP) ? 'w' : '-';
    str[6] = (mode & S_IXGRP) ? 'x' : '-';

    // Other permissions
    str[7] = (mode & S_IROTH) ? 'r' : '-';
    str[8] = (mode & S_IWOTH) ? 'w' : '-';
    str[9] = (mode & S_IXOTH) ? 'x' : '-';

    // Special bits (SUID, SGID, Sticky)
    if (mode & S_ISUID) str[3] = (mode & S_IXUSR) ? 's' : 'S';
    if (mode & S_ISGID) str[6] = (mode & S_IXGRP) ? 's' : 'S';
    if (mode & S_ISVTX) str[9] = (mode & S_IXOTH) ? 't' : 'T';

    str[10] = '\0';
}

/**
 * @brief Returns a string representation of the file type.
 *
 * @param mode The st_mode field from a stat struct.
 * @return A constant string describing the file type.
 */
const char* get_file_type(mode_t mode) {
    if (S_ISREG(mode)) return "regular file";
    if (S_ISDIR(mode)) return "directory";
    if (S_ISLNK(mode)) return "symbolic link";
    if (S_ISCHR(mode)) return "character device";
    if (S_ISBLK(mode)) return "block device";
    if (S_ISFIFO(mode)) return "FIFO (named pipe)";
    if (S_ISSOCK(mode)) return "socket";
    return "unknown";
}
/*
```

### How to Compile and Run

1.  **Save the Code:** Save the content above into a file named `mystat.c`.
2.  **Compile:** Open your terminal and use GCC to compile the program. You may need to link the math library for some systems, though it's often not required for this code.
    ```sh
    gcc mystat.c -o mystat
    ```
3.  **Run:** You can now use the program to inspect files.

    **Example Usages:**

    * **Stat a system file:**
        ```sh
        ./mystat /etc/passwd
        ```

    * **Stat your shell executable:**
        ```sh
        ./mystat /bin/bash
        ```

    * **Stat a directory:**
        ```sh
        ./mystat /tmp
        
*/
