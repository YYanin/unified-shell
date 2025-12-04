#include "history.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static char **history_entries = NULL;
static int history_current_count = 0;
static int history_max_size = HISTORY_SIZE;

void history_init(void) {
    if (history_entries == NULL) {
        history_entries = calloc(history_max_size, sizeof(char*));
        if (history_entries == NULL) {
            perror("history_init: calloc failed");
            return;
        }
        history_current_count = 0;
    }
}

void history_load(const char *filename) {
    if (filename == NULL || history_entries == NULL) {
        return;
    }
    
    char filepath[1024];
    const char *home = getenv("HOME");
    if (home == NULL) {
        return;
    }
    
    snprintf(filepath, sizeof(filepath), "%s/%s", home, filename);
    
    FILE *fp = fopen(filepath, "r");
    if (fp == NULL) {
        return;
    }
    
    char line[1024];
    while (fgets(line, sizeof(line), fp) != NULL && history_current_count < history_max_size) {
        // Remove trailing newline
        size_t len = strlen(line);
        if (len > 0 && line[len-1] == '\n') {
            line[len-1] = '\0';
        }
        
        if (strlen(line) > 0) {
            history_entries[history_current_count] = strdup(line);
            if (history_entries[history_current_count] != NULL) {
                history_current_count++;
            }
        }
    }
    
    fclose(fp);
}

void history_save(const char *filename) {
    if (filename == NULL || history_entries == NULL) {
        return;
    }
    
    char filepath[1024];
    const char *home = getenv("HOME");
    if (home == NULL) {
        return;
    }
    
    snprintf(filepath, sizeof(filepath), "%s/%s", home, filename);
    
    FILE *fp = fopen(filepath, "w");
    if (fp == NULL) {
        return;
    }
    
    for (int i = 0; i < history_current_count; i++) {
        if (history_entries[i] != NULL) {
            fprintf(fp, "%s\n", history_entries[i]);
        }
    }
    
    fclose(fp);
}

void history_add(const char *line) {
    if (line == NULL || strlen(line) == 0 || history_entries == NULL) {
        return;
    }
    
    // Don't add duplicate of most recent entry
    if (history_current_count > 0) {
        if (history_entries[history_current_count - 1] != NULL &&
            strcmp(history_entries[history_current_count - 1], line) == 0) {
            return;
        }
    }
    
    // If at max capacity, shift everything down
    if (history_current_count >= history_max_size) {
        free(history_entries[0]);
        for (int i = 1; i < history_max_size; i++) {
            history_entries[i-1] = history_entries[i];
        }
        history_current_count = history_max_size - 1;
    }
    
    history_entries[history_current_count] = strdup(line);
    if (history_entries[history_current_count] != NULL) {
        history_current_count++;
    }
}

const char* history_get(int index) {
    if (history_entries == NULL || index < 0 || index >= history_current_count) {
        return NULL;
    }
    
    // Index 0 is most recent, so reverse the lookup
    return history_entries[history_current_count - 1 - index];
}

int history_count(void) {
    return history_current_count;
}

void history_clear(void) {
    if (history_entries == NULL) {
        return;
    }
    
    for (int i = 0; i < history_current_count; i++) {
        free(history_entries[i]);
        history_entries[i] = NULL;
    }
    history_current_count = 0;
}

void history_free(void) {
    if (history_entries == NULL) {
        return;
    }
    
    history_clear();
    free(history_entries);
    history_entries = NULL;
}

// Navigation state for terminal integration
static int nav_position = -1;

const char* history_get_prev(void) {
    if (history_entries == NULL || history_current_count == 0) {
        return NULL;
    }
    
    if (nav_position == -1) {
        nav_position = history_current_count - 1;
    } else if (nav_position > 0) {
        nav_position--;
    } else {
        return NULL;
    }
    
    return history_entries[nav_position];
}

const char* history_get_next(void) {
    if (history_entries == NULL || nav_position < 0) {
        return NULL;
    }
    
    if (nav_position < history_current_count - 1) {
        nav_position++;
        return history_entries[nav_position];
    } else {
        nav_position = -1;
        return "";
    }
}

void history_reset_position(void) {
    nav_position = -1;
}
