/**
 * terminal.c - Advanced terminal input handling with line editing
 * 
 * This module provides readline-like functionality for the shell:
 * - Raw terminal mode for character-by-character input
 * - Line editing with arrow keys and backspace
 * - Command history navigation (UP/DOWN arrows)
 * - Tab completion for commands and filenames
 * - Cursor movement (LEFT/RIGHT arrows)
 * - Special key handling (Ctrl+C, Ctrl+D)
 * 
 * Architecture:
 * - Operates in raw terminal mode (disables line buffering and echo)
 * - Uses ANSI escape sequences for cursor control
 * - Maintains local line buffer for editing
 * - Callbacks to history and completion modules
 * - Redraws line on every change for visual feedback
 * 
 * Terminal modes:
 * - NORMAL: Canonical mode with line buffering (default)
 * - RAW: Character mode for advanced input handling (used during readline)
 * 
 * Key bindings:
 * - Arrow UP/DOWN: Navigate command history
 * - Arrow LEFT/RIGHT: Move cursor within line
 * - TAB: Trigger command/filename completion
 * - Backspace/Delete: Remove characters
 * - Enter: Submit command
 * - Ctrl+C: Cancel current input
 * - Ctrl+D: EOF (exit if line empty)
 */

#include <sys/ioctl.h>

#include "terminal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <ctype.h>

// Original terminal settings (saved for restoration)
static struct termios orig_termios;
static int term_mode = TERM_MODE_NORMAL;

// Callback function pointers for extensibility
// These are set by main() to connect terminal module with history/completion
static char** (*completion_callback)(const char *text, int *count) = NULL;
static const char* (*history_prev_callback)(void) = NULL;
static const char* (*history_next_callback)(void) = NULL;

// History navigation state
// Tracks position in history list when using UP/DOWN arrows
static int history_position = -1;        // -1 means not navigating, >=0 is index
static char saved_line[1024] = {0};      // Saves current input when entering history

/**
 * terminal_set_completion_callback - Register tab completion handler
 * @callback: Function that generates completions for given text
 * 
 * The callback receives the current line text and returns an array
 * of possible completions. Used for command and filename completion.
 */
void terminal_set_completion_callback(char** (*callback)(const char *text, int *count)) {
    completion_callback = callback;
}

/**
 * terminal_set_history_callbacks - Register history navigation handlers
 * @get_prev: Function to get previous command from history
 * @get_next: Function to get next command from history
 * 
 * These callbacks allow UP/DOWN arrows to navigate command history.
 * The history module implements the actual storage and retrieval.
 */
void terminal_set_history_callbacks(
    const char* (*get_prev)(void),
    const char* (*get_next)(void)
) {
    history_prev_callback = get_prev;
    history_next_callback = get_next;
}

/**
 * terminal_raw_mode - Enable raw terminal mode for character-by-character input
 * 
 * Raw mode configuration:
 * - Disables canonical mode (no line buffering - read chars immediately)
 * - Disables echo (we'll handle display ourselves)
 * - Disables signal generation (Ctrl+C won't send SIGINT)
 * - Disables special character processing (Ctrl+S/Q flow control, etc.)
 * - Sets VMIN=1, VTIME=0 (read returns after 1 char, no timeout)
 * 
 * This allows us to:
 * - Handle arrow keys and special keys
 * - Implement custom line editing
 * - Control exactly what appears on screen
 * 
 * Returns: 0 on success, -1 on error
 */
int terminal_raw_mode(void) {
    // Already in raw mode? Nothing to do
    if (term_mode == TERM_MODE_RAW) {
        return 0;
    }
    
    // Save original terminal settings for later restoration
    if (tcgetattr(STDIN_FILENO, &orig_termios) == -1) {
        perror("tcgetattr");
        return -1;
    }
    
    // Copy original settings and modify for raw mode
    struct termios raw = orig_termios;
    
    // Disable canonical mode (line buffering) and echo
    // Also disable signal generation (Ctrl+C, Ctrl+Z)
    raw.c_lflag &= ~(ECHO | ICANON | ISIG);
    
    // Disable flow control (Ctrl+S, Ctrl+Q) and CR-to-NL translation
    raw.c_iflag &= ~(IXON | ICRNL);
    
    // Set read() to return after 1 character with no timeout
    // VMIN=1: minimum number of characters for read() to return
    // VTIME=0: no timeout (wait indefinitely for input)
    raw.c_cc[VMIN] = 1;
    raw.c_cc[VTIME] = 0;
    
    // Apply new terminal settings
    // TCSAFLUSH: change occurs after all output written, discard unread input
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) {
        perror("tcsetattr");
        return -1;
    }
    
    term_mode = TERM_MODE_RAW;
    return 0;
}

/**
 * terminal_normal_mode - Restore normal terminal mode
 * 
 * Restores original terminal settings saved by terminal_raw_mode().
 * This re-enables canonical mode, echo, and signal generation.
 * 
 * Should be called before exiting raw mode operations to ensure
 * terminal is in usable state.
 * 
 * Returns: 0 on success, -1 on error
 */
int terminal_normal_mode(void) {
    // Already in normal mode? Nothing to do
    if (term_mode == TERM_MODE_NORMAL) {
        return 0;
    }
    
    // Restore original terminal settings
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios) == -1) {
        perror("tcsetattr");
        return -1;
    }
    
    term_mode = TERM_MODE_NORMAL;
    return 0;
}

/**
 * move_cursor_left - Move cursor left by n positions using ANSI escape
 * @n: Number of positions to move
 * 
 * Sends CSI n D escape sequence (\033[nD) to move cursor left.
 * Used when positioning cursor after redrawing line.
 */
static void move_cursor_left(int n) {
    char buf[32];
    snprintf(buf, sizeof(buf), "\033[%dD", n);
    write(STDOUT_FILENO, buf, strlen(buf));
}

/**
 * move_cursor_right - Move cursor right by n positions using ANSI escape
 * @n: Number of positions to move
 * 
 * Sends CSI n C escape sequence (\033[nC) to move cursor right.
 * Used for cursor movement with arrow keys.
 */
static void move_cursor_right(int n) {
    char buf[32];
    snprintf(buf, sizeof(buf), "\033[%dC", n);
    write(STDOUT_FILENO, buf, strlen(buf));
}

/**
 * Line redraw state tracking
 * These variables remember the previous line's length to properly clear it
 * before redrawing with new content.
 */
static int last_prompt_len = 0;  // Length of previous prompt
static int last_line_len = 0;    // Length of previous input line

/**
 * redraw_line - Redraw the input line with cursor at specified position
 * @prompt: The prompt string (e.g., "user@host:~> ")
 * @line: The current input line being edited
 * @cursor_pos: Position where cursor should be (0 = start of line)
 * 
 * Strategy:
 * 1. Move to start of line with \r
 * 2. Overwrite old content with spaces (based on saved lengths)
 * 3. Return to start
 * 4. Write new prompt and line
 * 5. Position cursor correctly
 * 
 * This simple approach avoids complex ANSI escape sequences and works
 * reliably even when lines wrap to multiple terminal rows. The terminal
 * handles wrapping automatically.
 * 
 * Note: Limits clearing to 200 chars to avoid excessive writes on very
 * long lines. Longer lines may leave remnants but this is rare.
 */
/**
 * redraw_line - Redraw the input line with cursor at specified position
 * * FIXES:
 * 1. Gets terminal width to handle wrapping correctly.
 * 2. Moves cursor UP if the previous line wrapped to multiple rows.
 * 3. Uses ANSI 'Clear to End of Screen' (\033[J) instead of writing spaces.
 */
static void redraw_line(const char *prompt, const char *line, int cursor_pos) {
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    int width = (w.ws_col > 0) ? w.ws_col : 80;

    int prompt_len = strlen(prompt);
    int line_len = strlen(line);
    
    int old_total = last_prompt_len + last_line_len;
    int new_total = prompt_len + line_len;

    // Calculate how many rows the content occupies
    // (integer division: 0-79 = 0 rows, 80-159 = 1 row, etc.)
    int old_rows = old_total / width;
    int new_rows = new_total / width;

    // === KEY FIX START ===
    // If we've grown into a new row, we might be at the bottom of the screen.
    // We print a newline to force the terminal to scroll up (making room),
    // then immediately move the cursor back up so we don't leave a gap.
    if (new_rows > old_rows) {
        char scroll_seq[] = "\n\x1b[A";
        write(STDOUT_FILENO, scroll_seq, 4);
    }
    // === KEY FIX END ===

    // Now proceed with the standard redraw logic...

    // 1. Move cursor UP to the start of the existing prompt
    // Use old_rows to know where we started relative to current position
    if (old_rows > 0) {
        char buf[32];
        snprintf(buf, sizeof(buf), "\x1b[%dA", old_rows);
        write(STDOUT_FILENO, buf, strlen(buf));
    }

    // 2. Move to column 0
    write(STDOUT_FILENO, "\r", 1);
    
    // 3. Clear everything down (clean slate)
    write(STDOUT_FILENO, "\x1b[J", 3);

    // 4. Print the new content
    write(STDOUT_FILENO, prompt, prompt_len);
    write(STDOUT_FILENO, line, line_len);

    // 5. Update state
    last_prompt_len = prompt_len;
    last_line_len = line_len;

    // 6. Position cursor correctly
    // We calculate the exact row/col to move to
    int total_pos = prompt_len + cursor_pos;
    int target_row = total_pos / width;     // How many rows down from start
    int target_col = total_pos % width;     // Which column
    
    // If the input spans multiple lines, we need to move the cursor
    // relative to the END of the printed text.
    // Current cursor is at the end of the text (new_rows down).
    // We need to move it to (target_row, target_col).
    
    // Move up from the bottom if necessary
    int rows_from_bottom = new_rows - target_row;
    if (rows_from_bottom > 0) {
        char buf[32];
        snprintf(buf, sizeof(buf), "\x1b[%dA", rows_from_bottom);
        write(STDOUT_FILENO, buf, strlen(buf));
    }

    // Move to the correct column
    // The simplified "\r" + right move is most reliable
    write(STDOUT_FILENO, "\r", 1);
    if (target_col > 0) {
        char buf[32];
        snprintf(buf, sizeof(buf), "\x1b[%dC", target_col);
        write(STDOUT_FILENO, buf, strlen(buf));
    }
}


/**
 * show_completions - Display list of possible completions
 * @completions: Array of completion strings
 * @count: Number of completions in array
 * 
 * Displays up to 20 completions in a horizontal list.
 * If more than 20, shows "... and N more" message.
 * 
 * Called when TAB pressed and multiple matches found.
 */
static void show_completions(char **completions, int count) {
    if (count == 0 || completions == NULL) {
        return;
    }
    
    // Start on new line
    write(STDOUT_FILENO, "\n", 1);
    
    // Show up to 20 completions
    for (int i = 0; i < count && i < 20; i++) {
        write(STDOUT_FILENO, completions[i], strlen(completions[i]));
        write(STDOUT_FILENO, "  ", 2);  // Two spaces between items
    }
    
    // If more than 20, show count of additional matches
    if (count > 20) {
        char buf[64];
        snprintf(buf, sizeof(buf), "\n... and %d more", count - 20);
        write(STDOUT_FILENO, buf, strlen(buf));
    }
    
    write(STDOUT_FILENO, "\n", 1);
}

/**
 * terminal_readline - Read a line of input with advanced editing features
 * @prompt: Prompt string to display
 * 
 * Provides readline-like line editing with:
 * - Arrow keys for cursor movement and history navigation
 * - Backspace for character deletion
 * - Tab for command/filename completion  
 * - Insert characters at cursor position
 * - Ctrl+C to cancel current input
 * - Ctrl+D for EOF
 * 
 * Key handling:
 * - Printable chars: Insert at cursor, advance cursor
 * - Backspace (127/8): Delete char before cursor
 * - Tab (9): Trigger completion
 * - Enter (10/13): Submit line
 * - Arrow keys: Escape sequences [A/B/C/D for up/down/left/right
 * - Ctrl+C (3): Cancel input, return empty string
 * - Ctrl+D (4): EOF on empty line, return NULL
 * 
 * Returns: Dynamically allocated string with input line (caller must free)
 *          NULL on EOF or error
 */
char* terminal_readline(const char *prompt) {
    static char line[1024];
    int cursor_pos = 0;
    int line_len = 0;
    char c;
    
    /* Check if stdin is a terminal - if not, use simple fgets */
    if (!isatty(STDIN_FILENO)) {
        /* Print prompt to stdout anyway for output */
        printf("%s", prompt);
        fflush(stdout);
        
        /* Use simple fgets for non-interactive input */
        if (fgets(line, sizeof(line), stdin) == NULL) {
            return NULL;  /* EOF or error */
        }
        
        /* Remove trailing newline */
        size_t len = strlen(line);
        if (len > 0 && line[len - 1] == '\n') {
            line[len - 1] = '\0';
        }
        
        return strdup(line);
    }
    
    // Reset history position
    history_position = -1;
    saved_line[0] = '\0';
    
    // Reset redraw state
    last_prompt_len = 0;
    last_line_len = 0;
    
    memset(line, 0, sizeof(line));
    
    // Enable raw mode
    if (terminal_raw_mode() == -1) {
        return NULL;
    }
    
    // Print prompt
    write(STDOUT_FILENO, prompt, strlen(prompt));
    
    while (1) {
        if (read(STDIN_FILENO, &c, 1) != 1) {
            terminal_normal_mode();
            return NULL;
        }
        
        // Handle Ctrl+D (EOF)
        if (c == 4 && line_len == 0) {
            terminal_normal_mode();
            return NULL;
        }
        
        // Handle Ctrl+C
        if (c == 3) {
            write(STDOUT_FILENO, "^C\n", 3);
            terminal_normal_mode();
            line[0] = '\0';
            return strdup(line);
        }
        
        // Handle Enter
        if (c == '\n' || c == '\r') {
            write(STDOUT_FILENO, "\n", 1);
            terminal_normal_mode();
            line[line_len] = '\0';
            return strdup(line);
        }
        
        // Handle Backspace
        if (c == 127 || c == 8) {
            if (cursor_pos > 0) {
                memmove(&line[cursor_pos - 1], &line[cursor_pos], line_len - cursor_pos + 1);
                cursor_pos--;
                line_len--;
                redraw_line(prompt, line, cursor_pos);
            }
            continue;
        }
        
        // Handle Tab
        if (c == '\t') {
            if (completion_callback != NULL && cursor_pos == line_len) {
                int count = 0;
                char **completions = completion_callback(line, &count);
                
                if (count == 1 && completions != NULL) {
                    // Single completion - auto-complete
                    int comp_len = strlen(completions[0]);
                    if (comp_len < (int)sizeof(line) - 1) {
                        strcpy(line, completions[0]);
                        line_len = comp_len;
                        cursor_pos = comp_len;
                        redraw_line(prompt, line, cursor_pos);
                    }
                } else if (count > 1) {
                    // Multiple completions - show list
                    show_completions(completions, count);
                    redraw_line(prompt, line, cursor_pos);
                }
                
                // Free completions
                if (completions != NULL) {
                    for (int i = 0; i < count; i++) {
                        free(completions[i]);
                    }
                    free(completions);
                }
            }
            continue;
        }
        
        // Handle Escape sequences (arrow keys, etc.)
        if (c == 27) {
            char seq[3];
            
            if (read(STDIN_FILENO, &seq[0], 1) != 1) continue;
            if (read(STDIN_FILENO, &seq[1], 1) != 1) continue;
            
            if (seq[0] == '[') {
                // Arrow keys
                if (seq[1] == 'A') {
                    // Up arrow - previous history
                    if (history_prev_callback != NULL) {
                        if (history_position == -1) {
                            // Save current line
                            strncpy(saved_line, line, sizeof(saved_line) - 1);
                            history_position = 0;
                        } else {
                            history_position++;
                        }
                        
                        const char *prev = history_prev_callback();
                        if (prev != NULL) {
                            strncpy(line, prev, sizeof(line) - 1);
                            line[sizeof(line) - 1] = '\0';
                            line_len = strlen(line);
                            cursor_pos = line_len;
                            redraw_line(prompt, line, cursor_pos);
                        } else {
                            // No more history
                            history_position--;
                            if (history_position < 0) history_position = 0;
                        }
                    }
                } else if (seq[1] == 'B') {
                    // Down arrow - next history
                    if (history_next_callback != NULL && history_position >= 0) {
                        if (history_position == 0) {
                            // Restore saved line
                            strncpy(line, saved_line, sizeof(line) - 1);
                            line[sizeof(line) - 1] = '\0';
                            history_position = -1;
                        } else {
                            history_position--;
                            const char *next = history_next_callback();
                            if (next != NULL) {
                                strncpy(line, next, sizeof(line) - 1);
                                line[sizeof(line) - 1] = '\0';
                            }
                        }
                        line_len = strlen(line);
                        cursor_pos = line_len;
                        redraw_line(prompt, line, cursor_pos);
                    }
                } else if (seq[1] == 'C') {
                    // Right arrow
                    if (cursor_pos < line_len) {
                        cursor_pos++;
                        move_cursor_right(1);
                    }
                } else if (seq[1] == 'D') {
                    // Left arrow
                    if (cursor_pos > 0) {
                        cursor_pos--;
                        move_cursor_left(1);
                    }
                }
            }
            continue;
        }
        
        // Regular character
        if (isprint(c)) {
            if (line_len < (int)sizeof(line) - 1) {
                // Insert character at cursor position
                memmove(&line[cursor_pos + 1], &line[cursor_pos], line_len - cursor_pos + 1);
                line[cursor_pos] = c;
                cursor_pos++;
                line_len++;
                redraw_line(prompt, line, cursor_pos);
            }
        }
    }
    
    return NULL;
}
