#include "completion.h"
#include "builtins.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

static Env *completion_env = NULL;

// Built-in command names
static const char *builtin_commands[] = {
    "cd", "pwd", "echo", "export", "exit", "set", "unset", 
    "env", "help", "version", "history", "edi",
    "myls", "mycat", "mycp", "mymv", "myrm", 
    "mymkdir", "myrmdir", "mytouch", "mystat", "myfd",
    NULL
};

void completion_init(Env *env) {
    completion_env = env;
}

void completion_free(char **completions) {
    if (completions == NULL) {
        return;
    }
    
    for (int i = 0; completions[i] != NULL; i++) {
        free(completions[i]);
    }
    free(completions);
}

char** completion_get_commands(int *count) {
    int capacity = 50;
    int cnt = 0;
    char **commands = malloc(capacity * sizeof(char*));
    
    if (commands == NULL) {
        *count = 0;
        return NULL;
    }
    
    // Add built-in commands
    for (int i = 0; builtin_commands[i] != NULL; i++) {
        if (cnt >= capacity - 1) {
            capacity *= 2;
            char **new_cmds = realloc(commands, capacity * sizeof(char*));
            if (new_cmds == NULL) {
                completion_free(commands);
                *count = 0;
                return NULL;
            }
            commands = new_cmds;
        }
        commands[cnt++] = strdup(builtin_commands[i]);
    }
    
    commands[cnt] = NULL;
    *count = cnt;
    return commands;
}

char** completion_get_files(const char *prefix, int *count) {
    DIR *dir = opendir(".");
    if (dir == NULL) {
        *count = 0;
        return NULL;
    }
    
    int capacity = 20;
    int cnt = 0;
    char **files = malloc(capacity * sizeof(char*));
    
    if (files == NULL) {
        closedir(dir);
        *count = 0;
        return NULL;
    }
    
    size_t prefix_len = prefix ? strlen(prefix) : 0;
    struct dirent *entry;
    
    while ((entry = readdir(dir)) != NULL) {
        // Skip . and ..
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        
        // Check prefix match
        if (prefix && prefix_len > 0) {
            if (strncmp(entry->d_name, prefix, prefix_len) != 0) {
                continue;
            }
        }
        
        if (cnt >= capacity - 1) {
            capacity *= 2;
            char **new_files = realloc(files, capacity * sizeof(char*));
            if (new_files == NULL) {
                completion_free(files);
                closedir(dir);
                *count = 0;
                return NULL;
            }
            files = new_files;
        }
        
        files[cnt++] = strdup(entry->d_name);
    }
    
    closedir(dir);
    files[cnt] = NULL;
    *count = cnt;
    return files;
}

char** completion_get_variables(const char *prefix, int *count) {
    if (completion_env == NULL) {
        *count = 0;
        return NULL;
    }
    
    int capacity = 20;
    int cnt = 0;
    char **vars = malloc(capacity * sizeof(char*));
    
    if (vars == NULL) {
        *count = 0;
        return NULL;
    }
    
    size_t prefix_len = prefix ? strlen(prefix) : 0;
    
    // Get variables from environment
    for (int i = 0; i < completion_env->count; i++) {
        const char *name = completion_env->bindings[i].name;
        if (name == NULL) {
            continue;
        }
        
        // Check prefix match
        if (prefix && prefix_len > 0) {
            if (strncmp(name, prefix, prefix_len) != 0) {
                continue;
            }
        }
        
        if (cnt >= capacity - 1) {
            capacity *= 2;
            char **new_vars = realloc(vars, capacity * sizeof(char*));
            if (new_vars == NULL) {
                completion_free(vars);
                *count = 0;
                return NULL;
            }
            vars = new_vars;
        }
        
        vars[cnt++] = strdup(name);
    }
    
    vars[cnt] = NULL;
    *count = cnt;
    return vars;
}

char** completion_generate(const char *text, int *count) {
    if (text == NULL || strlen(text) == 0) {
        *count = 0;
        return NULL;
    }
    
    // Check if we're completing a command (first word) or filename
    const char *space = strchr(text, ' ');
    int is_command = (space == NULL);
    
    if (is_command) {
        // Command completion
        int cmd_count = 0;
        char **commands = completion_get_commands(&cmd_count);
        
        if (commands == NULL) {
            *count = 0;
            return NULL;
        }
        
        // Filter commands by prefix
        int capacity = 20;
        int cnt = 0;
        char **matches = malloc(capacity * sizeof(char*));
        
        if (matches == NULL) {
            completion_free(commands);
            *count = 0;
            return NULL;
        }
        
        size_t text_len = strlen(text);
        
        for (int i = 0; commands[i] != NULL; i++) {
            if (strncmp(commands[i], text, text_len) == 0) {
                if (cnt >= capacity - 1) {
                    capacity *= 2;
                    char **new_matches = realloc(matches, capacity * sizeof(char*));
                    if (new_matches == NULL) {
                        completion_free(matches);
                        completion_free(commands);
                        *count = 0;
                        return NULL;
                    }
                    matches = new_matches;
                }
                matches[cnt++] = strdup(commands[i]);
            }
        }
        
        completion_free(commands);
        matches[cnt] = NULL;
        *count = cnt;
        return matches;
    } else {
        // Filename completion for arguments
        // Extract the last word
        const char *last_word = text;
        for (const char *p = text; *p; p++) {
            if (*p == ' ') {
                last_word = p + 1;
            }
        }
        
        // Get matching files
        char **files = completion_get_files(last_word, count);
        if (files == NULL || *count == 0) {
            return files;
        }
        
        // Build full completions with command prefix
        size_t prefix_len = last_word - text;
        char **full_completions = malloc((*count + 1) * sizeof(char*));
        if (full_completions == NULL) {
            completion_free(files);
            *count = 0;
            return NULL;
        }
        
        for (int i = 0; i < *count; i++) {
            // Allocate space for prefix + filename + null
            size_t len = prefix_len + strlen(files[i]) + 1;
            full_completions[i] = malloc(len);
            if (full_completions[i] == NULL) {
                // Cleanup on allocation failure
                for (int j = 0; j < i; j++) {
                    free(full_completions[j]);
                }
                free(full_completions);
                completion_free(files);
                *count = 0;
                return NULL;
            }
            
            // Copy prefix and append filename
            memcpy(full_completions[i], text, prefix_len);
            strcpy(full_completions[i] + prefix_len, files[i]);
        }
        
        full_completions[*count] = NULL;
        completion_free(files);
        return full_completions;
    }
}
