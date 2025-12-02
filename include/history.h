#ifndef HISTORY_H
#define HISTORY_H

#define HISTORY_SIZE 1000
#define HISTORY_FILE ".ushell_history"

// Initialize history system
void history_init(void);

// Load history from file
void history_load(const char *filename);

// Save history to file
void history_save(const char *filename);

// Add command to history
void history_add(const char *line);

// Get history entry by index (0 = most recent)
const char* history_get(int index);

// Get total history count
int history_count(void);

// Clear all history
void history_clear(void);

// Free history resources
void history_free(void);

// Navigation functions for terminal integration
const char* history_get_prev(void);
const char* history_get_next(void);
void history_reset_position(void);

#endif // HISTORY_H
