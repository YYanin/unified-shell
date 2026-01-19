/**
 * @file esp_shell.c
 * @brief ESP32 Shell - Shell Implementation
 * 
 * This file implements the ESP32-specific shell functionality.
 * It provides a simple command-line interface that reads input from
 * the serial console and executes built-in commands.
 * 
 * File operations use the VFS abstraction layer (shell_vfs.h) which
 * provides a portable interface for both ESP32 and Linux.
 * 
 * Execution uses direct function calls (no fork/exec) with I/O
 * redirection support via executor_esp32.h.
 * 
 * Terminal I/O is handled by the terminal_esp32 module which provides
 * line editing, history navigation, and escape sequence parsing.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "esp_shell.h"
#include "platform.h"
#include "shell_vfs.h"
#include "executor_esp32.h"
#include "terminal_esp32.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_log.h"

/* Tag for ESP logging */
static const char *TAG = "shell";

/* ============================================================================
 * Shell State Variables
 * ============================================================================ */

/* Current working directory */
static char current_dir[ESP_SHELL_MAX_PATH] = "/spiffs";

/* Command line buffer */
static char line_buffer[ESP_SHELL_MAX_LINE_LEN];

/* Command history buffer */
static char history[ESP_SHELL_HISTORY_SIZE][ESP_SHELL_MAX_LINE_LEN];
static int history_count = 0;
static int history_index = 0;

/* ============================================================================
 * Forward Declarations of Built-in Commands
 * ============================================================================ */
static int cmd_help(int argc, char **argv);
static int cmd_exit(int argc, char **argv);
static int cmd_reboot(int argc, char **argv);
static int cmd_info(int argc, char **argv);
static int cmd_free(int argc, char **argv);
static int cmd_uptime(int argc, char **argv);
static int cmd_pwd(int argc, char **argv);
static int cmd_cd(int argc, char **argv);
static int cmd_ls(int argc, char **argv);
static int cmd_cat(int argc, char **argv);
static int cmd_echo(int argc, char **argv);
static int cmd_touch(int argc, char **argv);
static int cmd_rm(int argc, char **argv);
static int cmd_mkdir(int argc, char **argv);
static int cmd_history(int argc, char **argv);

/* ============================================================================
 * Built-in Command Table
 * ============================================================================ */

/* Array of built-in commands - add new commands here */
static const esp_shell_cmd_t builtin_commands[] = {
    /* Shell control commands */
    {"help",    "Show available commands",       cmd_help},
    {"exit",    "Exit shell (reboot ESP32)",     cmd_exit},
    {"reboot",  "Reboot the ESP32",              cmd_reboot},
    
    /* System information commands */
    {"info",    "Show system information",       cmd_info},
    {"free",    "Show free memory",              cmd_free},
    {"uptime",  "Show system uptime",            cmd_uptime},
    
    /* Directory commands */
    {"pwd",     "Print working directory",       cmd_pwd},
    {"cd",      "Change directory",              cmd_cd},
    {"ls",      "List directory contents",       cmd_ls},
    
    /* File commands */
    {"cat",     "Display file contents",         cmd_cat},
    {"echo",    "Print text",                    cmd_echo},
    {"touch",   "Create empty file",             cmd_touch},
    {"rm",      "Remove file",                   cmd_rm},
    {"mkdir",   "Create directory",              cmd_mkdir},
    
    /* Shell history */
    {"history", "Show command history",          cmd_history},
    
    /* End of command list marker */
    {NULL, NULL, NULL}
};

/* ============================================================================
 * Helper Functions
 * ============================================================================ */

/**
 * @brief Build absolute path from relative path
 * 
 * @param path Input path (may be relative or absolute)
 * @param buf Output buffer for absolute path
 * @param size Size of output buffer
 * @return Pointer to buf on success, NULL on failure
 */
static char* build_path(const char *path, char *buf, size_t size) {
    /* Handle "." - current directory */
    if (strcmp(path, ".") == 0) {
        strncpy(buf, current_dir, size - 1);
        buf[size - 1] = '\0';
        return buf;
    }
    
    /* If path is absolute, use it directly */
    if (path[0] == '/') {
        strncpy(buf, path, size - 1);
        buf[size - 1] = '\0';
        return buf;
    }
    
    /* Build absolute path from current directory */
    int len = snprintf(buf, size, "%s/%s", current_dir, path);
    if (len >= (int)size) {
        return NULL;  /* Path too long */
    }
    
    return buf;
}

/**
 * @brief Parse command line into argc/argv
 * 
 * Splits the command line on whitespace and handles basic quoting.
 * 
 * @param line The command line to parse (will be modified)
 * @param argv Array to store argument pointers
 * @param max_args Maximum number of arguments
 * @return Number of arguments parsed
 */
static int parse_line(char *line, char **argv, int max_args) {
    int argc = 0;
    char *p = line;
    
    while (*p && argc < max_args) {
        /* Skip leading whitespace */
        while (*p && isspace((unsigned char)*p)) {
            p++;
        }
        
        if (!*p) break;
        
        /* Check for quoted string */
        if (*p == '"' || *p == '\'') {
            char quote = *p++;
            argv[argc++] = p;
            
            /* Find closing quote */
            while (*p && *p != quote) {
                p++;
            }
            if (*p) {
                *p++ = '\0';
            }
        } else {
            /* Regular argument - find end */
            argv[argc++] = p;
            while (*p && !isspace((unsigned char)*p)) {
                p++;
            }
            if (*p) {
                *p++ = '\0';
            }
        }
    }
    
    return argc;
}

/**
 * @brief Find command in command table
 * 
 * @param name Command name to find
 * @return Pointer to command structure, or NULL if not found
 */
static const esp_shell_cmd_t* find_command(const char *name) {
    for (int i = 0; builtin_commands[i].name != NULL; i++) {
        if (strcmp(name, builtin_commands[i].name) == 0) {
            return &builtin_commands[i];
        }
    }
    return NULL;
}

/**
 * @brief Add command to history
 * 
 * @param line Command line to add
 */
static void add_to_history(const char *line) {
    if (strlen(line) == 0) return;
    
    /* Don't add duplicates of the last command */
    if (history_count > 0 && 
        strcmp(history[(history_count - 1) % ESP_SHELL_HISTORY_SIZE], line) == 0) {
        return;
    }
    
    strncpy(history[history_count % ESP_SHELL_HISTORY_SIZE], 
            line, ESP_SHELL_MAX_LINE_LEN - 1);
    history[history_count % ESP_SHELL_HISTORY_SIZE][ESP_SHELL_MAX_LINE_LEN - 1] = '\0';
    history_count++;
    
    /* Also add to terminal history for up/down arrow navigation */
    terminal_history_add(line);
}

/* ============================================================================
 * Built-in Command Implementations
 * ============================================================================ */

/**
 * @brief help command - Show available commands
 */
static int cmd_help(int argc, char **argv) {
    (void)argc;
    (void)argv;
    
    printf("Available commands:\n");
    printf("------------------\n");
    for (int i = 0; builtin_commands[i].name != NULL; i++) {
        printf("  %-10s - %s\n", 
               builtin_commands[i].name, 
               builtin_commands[i].help);
    }
    printf("\n");
    return 0;
}

/**
 * @brief exit command - Exit shell (reboots ESP32)
 */
static int cmd_exit(int argc, char **argv) {
    (void)argc;
    (void)argv;
    
    printf("Rebooting ESP32...\n");
    platform_sleep_ms(100);
    esp_restart();
    return 0;  /* Never reached */
}

/**
 * @brief reboot command - Reboot ESP32
 */
static int cmd_reboot(int argc, char **argv) {
    return cmd_exit(argc, argv);
}

/**
 * @brief info command - Show system information
 */
static int cmd_info(int argc, char **argv) {
    (void)argc;
    (void)argv;
    
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);
    
    /* Get flash size using the newer API */
    uint32_t flash_size = 0;
    esp_flash_get_size(NULL, &flash_size);
    
    printf("ESP32 System Information\n");
    printf("------------------------\n");
    printf("Chip:         ESP32 with %d CPU cores\n", chip_info.cores);
    printf("WiFi:         %s\n", (chip_info.features & CHIP_FEATURE_WIFI_BGN) ? "Yes" : "No");
    printf("Bluetooth:    %s\n", (chip_info.features & CHIP_FEATURE_BT) ? "Yes" : "No");
    printf("Flash:        %lu MB %s\n", 
           (unsigned long)(flash_size / (1024 * 1024)),
           (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "(embedded)" : "(external)");
    printf("Free heap:    %lu bytes\n", (unsigned long)esp_get_free_heap_size());
    printf("IDF version:  %s\n", esp_get_idf_version());
    
    return 0;
}

/**
 * @brief free command - Show free memory
 */
static int cmd_free(int argc, char **argv) {
    (void)argc;
    (void)argv;
    
    printf("Free heap memory: %lu bytes\n", (unsigned long)esp_get_free_heap_size());
    printf("Minimum free:     %lu bytes\n", (unsigned long)esp_get_minimum_free_heap_size());
    
    return 0;
}

/**
 * @brief uptime command - Show system uptime
 */
static int cmd_uptime(int argc, char **argv) {
    (void)argc;
    (void)argv;
    
    unsigned long ms = platform_get_time_ms();
    unsigned long secs = ms / 1000;
    unsigned long mins = secs / 60;
    unsigned long hours = mins / 60;
    
    printf("Uptime: %lu:%02lu:%02lu (%lu ms)\n", 
           hours, mins % 60, secs % 60, ms);
    
    return 0;
}

/**
 * @brief pwd command - Print working directory
 */
static int cmd_pwd(int argc, char **argv) {
    (void)argc;
    (void)argv;
    
    char cwd[ESP_SHELL_MAX_PATH];
    if (vfs_getcwd(cwd, sizeof(cwd)) == 0) {
        printf("%s\n", cwd);
    } else {
        printf("%s\n", current_dir);
    }
    return 0;
}

/**
 * @brief cd command - Change directory
 */
static int cmd_cd(int argc, char **argv) {
    if (argc < 2) {
        /* cd with no args goes to root */
        if (vfs_chdir("/spiffs") == 0) {
            strcpy(current_dir, "/spiffs");
        }
        return 0;
    }
    
    /* Use VFS to change directory */
    if (vfs_chdir(argv[1]) == 0) {
        /* Update our local copy of cwd */
        vfs_getcwd(current_dir, ESP_SHELL_MAX_PATH);
        return 0;
    }
    
    printf("cd: %s: No such directory\n", argv[1]);
    return 1;
}

/**
 * @brief ls command - List directory contents
 * 
 * Uses VFS abstraction layer for portable directory listing.
 */
static int cmd_ls(int argc, char **argv) {
    const char *dir = argc > 1 ? argv[1] : ".";
    char path[ESP_SHELL_MAX_PATH];
    
    if (build_path(dir, path, sizeof(path)) == NULL) {
        printf("ls: path too long\n");
        return 1;
    }
    
    /* Use VFS directory functions */
    vfs_dir_t d = vfs_opendir(path);
    if (d == NULL) {
        printf("ls: cannot access '%s': No such file or directory\n", dir);
        return 1;
    }
    
    vfs_dirent_t entry;
    while (vfs_readdir(d, &entry) == 0) {
        /* Print with size info for files - use redir_printf for redirection */
        if (entry.is_dir) {
            redir_printf("%s/\n", entry.name);
        } else {
            redir_printf("%-20s %6lu bytes\n", entry.name, (unsigned long)entry.size);
        }
    }
    
    vfs_closedir(d);
    return 0;
}

/**
 * @brief cat command - Display file contents
 * 
 * Uses VFS abstraction layer for portable file reading.
 */
static int cmd_cat(int argc, char **argv) {
    if (argc < 2) {
        printf("Usage: cat <file>\n");
        return 1;
    }
    
    char path[ESP_SHELL_MAX_PATH];
    if (build_path(argv[1], path, sizeof(path)) == NULL) {
        printf("cat: path too long\n");
        return 1;
    }
    
    vfs_file_t f = vfs_open(path, "r");
    if (f == NULL) {
        printf("cat: %s: No such file or directory\n", argv[1]);
        return 1;
    }
    
    char buf[128];
    size_t bytes_read;
    while ((bytes_read = vfs_read(buf, sizeof(buf) - 1, f)) > 0) {
        buf[bytes_read] = '\0';
        redir_puts(buf);  /* Use redir_puts for redirection support */
    }
    
    vfs_close(f);
    return 0;
}

/**
 * @brief echo command - Print text
 * 
 * Uses redir_printf to support output redirection (> and >>).
 */
static int cmd_echo(int argc, char **argv) {
    for (int i = 1; i < argc; i++) {
        redir_printf("%s", argv[i]);
        if (i < argc - 1) redir_printf(" ");
    }
    redir_printf("\n");
    return 0;
}

/**
 * @brief touch command - Create empty file
 * 
 * Uses VFS abstraction layer for portable file creation.
 */
static int cmd_touch(int argc, char **argv) {
    if (argc < 2) {
        printf("Usage: touch <file>\n");
        return 1;
    }
    
    char path[ESP_SHELL_MAX_PATH];
    if (build_path(argv[1], path, sizeof(path)) == NULL) {
        printf("touch: path too long\n");
        return 1;
    }
    
    vfs_file_t f = vfs_open(path, "a");
    if (f == NULL) {
        printf("touch: cannot create '%s'\n", argv[1]);
        return 1;
    }
    
    vfs_close(f);
    printf("Created: %s\n", argv[1]);
    return 0;
}

/**
 * @brief rm command - Remove file
 * 
 * Uses VFS abstraction layer for portable file removal.
 */
static int cmd_rm(int argc, char **argv) {
    if (argc < 2) {
        printf("Usage: rm <file>\n");
        return 1;
    }
    
    char path[ESP_SHELL_MAX_PATH];
    if (build_path(argv[1], path, sizeof(path)) == NULL) {
        printf("rm: path too long\n");
        return 1;
    }
    
    if (vfs_remove(path) != 0) {
        printf("rm: cannot remove '%s'\n", argv[1]);
        return 1;
    }
    
    printf("Removed: %s\n", argv[1]);
    return 0;
}

/**
 * @brief mkdir command - Create directory
 * 
 * Uses VFS abstraction layer. Note: SPIFFS does not support true 
 * directories, but this is here for future LittleFS support.
 */
static int cmd_mkdir(int argc, char **argv) {
    if (argc < 2) {
        printf("Usage: mkdir <directory>\n");
        return 1;
    }
    
    char path[ESP_SHELL_MAX_PATH];
    if (build_path(argv[1], path, sizeof(path)) == NULL) {
        printf("mkdir: path too long\n");
        return 1;
    }
    
    if (vfs_mkdir(path) != 0) {
        printf("mkdir: cannot create '%s'\n", argv[1]);
        printf("Note: SPIFFS does not support true directories.\n");
        printf("Consider using LittleFS for directory support.\n");
        return 1;
    }
    
    printf("Created directory: %s\n", argv[1]);
    return 0;
}

/**
 * @brief history command - Show command history
 * 
 * Displays the list of recently executed commands.
 * Uses redir_printf for output redirection support.
 */
static int cmd_history(int argc, char **argv) {
    (void)argc;
    (void)argv;
    
    if (history_count == 0) {
        redir_printf("No commands in history.\n");
        return 0;
    }
    
    /* Calculate starting index for display */
    int start = 0;
    int total = history_count;
    
    if (history_count > ESP_SHELL_HISTORY_SIZE) {
        start = history_count - ESP_SHELL_HISTORY_SIZE;
        total = ESP_SHELL_HISTORY_SIZE;
    }
    
    for (int i = 0; i < total; i++) {
        int idx = (start + i) % ESP_SHELL_HISTORY_SIZE;
        redir_printf("%4d  %s\n", start + i + 1, history[idx]);
    }
    
    return 0;
}

/* ============================================================================
 * Shell Public API Implementation
 * ============================================================================ */

/**
 * @brief Initialize the ESP32 shell
 * 
 * Sets up the shell and VFS layer for filesystem operations.
 */
int esp_shell_init(void) {
    ESP_LOGI(TAG, "Initializing shell...");
    
    /* Clear history */
    memset(history, 0, sizeof(history));
    history_count = 0;
    history_index = 0;
    
    /* Initialize VFS layer - this mounts SPIFFS on ESP32 */
    if (vfs_init() != 0) {
        ESP_LOGE(TAG, "Failed to initialize VFS");
        /* Continue anyway - some commands may still work */
    }
    
    /* Set initial directory from VFS */
    if (vfs_getcwd(current_dir, ESP_SHELL_MAX_PATH) != 0) {
        strcpy(current_dir, "/spiffs");
    }
    
    ESP_LOGI(TAG, "Shell initialized with %d built-in commands", 
             (int)(sizeof(builtin_commands) / sizeof(builtin_commands[0]) - 1));
    
    return 0;
}

/**
 * @brief Execute a single command string
 * 
 * Parses the command line, extracts I/O redirection operators,
 * and executes the command using direct function calls.
 * 
 * Supports:
 * - > file  : Redirect stdout to file (truncate)
 * - >> file : Redirect stdout to file (append)
 * - < file  : (Not yet implemented) Redirect stdin from file
 * - |       : (Not supported) Pipelines not available on ESP32
 */
int esp_shell_execute(const char *cmdline) {
    /* Copy to local buffer since we modify it */
    char line[ESP_SHELL_MAX_LINE_LEN];
    strncpy(line, cmdline, sizeof(line) - 1);
    line[sizeof(line) - 1] = '\0';
    
    /* Parse into argc/argv */
    char *argv[ESP_SHELL_MAX_ARGS];
    int argc = parse_line(line, argv, ESP_SHELL_MAX_ARGS);
    
    if (argc == 0) {
        return 0;  /* Empty line */
    }
    
    /* Check for unsupported pipeline operator */
    if (has_pipeline(argc, argv)) {
        printf("error: pipelines not supported on ESP32\n");
        return 1;
    }
    
    /* Parse I/O redirection operators */
    parsed_cmd_t cmd;
    if (parse_redirections(argc, argv, &cmd) != 0) {
        return 1;  /* Syntax error already printed */
    }
    
    /* Find the command */
    const esp_shell_cmd_t *builtin = find_command(cmd.argv[0]);
    if (builtin == NULL) {
        printf("%s: command not found\n", cmd.argv[0]);
        return 127;
    }
    
    /* Execute with redirection support */
    if (cmd.stdout_redir != REDIR_NONE || cmd.stdin_redir != REDIR_NONE) {
        return execute_with_redirection(&cmd, builtin->func);
    }
    
    /* No redirection - execute directly */
    return builtin->func(cmd.argc, cmd.argv);
}

/**
 * @brief Read a line of input from the console
 * 
 * Simple line editor with backspace support.
 * 
 * @param buf Buffer to store the line
 * @param size Size of buffer
 * @return Number of characters read, or -1 on error
 */
static int read_line(char *buf, size_t size) {
    size_t pos = 0;
    
    while (pos < size - 1) {
        int c = platform_read_char();
        
        if (c < 0) {
            /* No character available, yield to other tasks */
            vTaskDelay(pdMS_TO_TICKS(10));
            continue;
        }
        
        if (c == '\n' || c == '\r') {
            /* End of line */
            platform_write_char('\n');
            buf[pos] = '\0';
            return pos;
        }
        
        if (c == 0x7F || c == '\b') {
            /* Backspace */
            if (pos > 0) {
                pos--;
                /* Erase character on screen: backspace, space, backspace */
                platform_write_string("\b \b");
            }
            continue;
        }
        
        if (c == 0x03) {
            /* Ctrl+C - cancel current line */
            platform_write_string("^C\n");
            buf[0] = '\0';
            return 0;
        }
        
        if (c >= 32 && c < 127) {
            /* Printable character */
            buf[pos++] = (char)c;
            platform_write_char((char)c);
        }
    }
    
    buf[pos] = '\0';
    return pos;
}

/**
 * @brief Run the shell main loop
 * 
 * Main loop uses terminal_read_line for enhanced line editing including:
 * - Left/right arrow keys for cursor movement
 * - Up/down arrow keys for history navigation
 * - Ctrl+A/E for home/end
 * - Ctrl+U/K for line kill
 * - Ctrl+L for clear screen
 */
void esp_shell_run(void) {
    ESP_LOGI(TAG, "Entering shell main loop");
    
    /* Initialize terminal subsystem for line editing */
    terminal_init();
    
    while (1) {
        /* Print prompt */
        printf("%s", ESP_SHELL_PROMPT);
        fflush(stdout);
        
        /* Read command line with full editing support */
        int result = terminal_read_line(line_buffer, sizeof(line_buffer));
        
        if (result < 0) {
            /* Cancelled (Ctrl+C) - just continue to next prompt */
            continue;
        }
        
        /* Skip empty lines */
        if (strlen(line_buffer) == 0) {
            continue;
        }
        
        /* Add to history */
        add_to_history(line_buffer);
        
        /* Execute the command */
        esp_shell_execute(line_buffer);
    }
}

/**
 * @brief Get current working directory
 */
char* esp_shell_getcwd(char *buf, size_t size) {
    strncpy(buf, current_dir, size - 1);
    buf[size - 1] = '\0';
    return buf;
}

/**
 * @brief Change current working directory
 */
int esp_shell_chdir(const char *path) {
    char *args[2] = {"cd", (char*)path};
    return cmd_cd(2, args);
}
