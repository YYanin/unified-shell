#ifndef TERMINAL_H
#define TERMINAL_H

#include <stddef.h>

// Terminal input modes
typedef enum {
    TERM_MODE_NORMAL,
    TERM_MODE_RAW
} TermMode;

// Initialize terminal for raw input
int terminal_raw_mode(void);

// Restore terminal to normal mode
int terminal_normal_mode(void);

// Read a line with arrow key support and tab completion
// Returns NULL on EOF, allocated string otherwise (caller must free)
char* terminal_readline(const char *prompt);

// Set completion function callback
void terminal_set_completion_callback(char** (*callback)(const char *text, int *count));

// Set history navigation callbacks
void terminal_set_history_callbacks(
    const char* (*get_prev)(void),
    const char* (*get_next)(void)
);

#endif // TERMINAL_H
