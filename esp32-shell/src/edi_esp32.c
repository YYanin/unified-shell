/**
 * @file edi_esp32.c
 * @brief Minimalist vi-like text editor for ESP32
 * 
 * A simplified port of the edi editor for ESP32 microcontrollers.
 * Works over serial console using VT100 escape sequences.
 * 
 * Features:
 * - Modal editing (NORMAL, INSERT, COMMAND modes)
 * - Basic cursor movement (h, j, k, l)
 * - Character insertion and deletion
 * - Line insertion and deletion
 * - File save/load to SPIFFS
 * - VT100-compatible terminal output
 * 
 * Commands:
 * - :w       - Save file
 * - :w name  - Save as filename
 * - :q       - Quit (if no changes)
 * - :q!      - Quit without saving
 * - :wq      - Save and quit
 * 
 * Keys:
 * - i        - Enter INSERT mode
 * - Escape   - Return to NORMAL mode
 * - hjkl     - Cursor movement in NORMAL mode
 * - x        - Delete character under cursor
 * - Backspace- Delete character before cursor (INSERT mode)
 * - Enter    - New line (INSERT mode)
 * 
 * Memory Limits (ESP32):
 * - Max 50 lines
 * - Max 128 characters per line
 * - Screen size: 80x24 (typical serial terminal)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "shell_config.h"

/* ============================================================================
 * Configuration - Memory limits for ESP32
 * ============================================================================ */

#define EDI_MAX_ROWS        50      /* Maximum number of lines */
#define EDI_MAX_LINE_LEN    128     /* Maximum characters per line */
#define EDI_SCREEN_ROWS     24      /* Terminal rows (common for serial) */
#define EDI_SCREEN_COLS     80      /* Terminal columns */
#define EDI_CMD_BUF_SIZE    64      /* Command buffer size */
#define EDI_STATUS_SIZE     80      /* Status message size */

/* Control key macro */
#define CTRL_KEY(k) ((k) & 0x1f)

/* ============================================================================
 * Data Structures
 * ============================================================================ */

/* Single line of text with static buffer */
typedef struct {
    char chars[EDI_MAX_LINE_LEN + 1];  /* Line content (null-terminated) */
    int size;                           /* Current length */
} edi_row_t;

/* Editor modes */
typedef enum {
    EDI_MODE_NORMAL,
    EDI_MODE_INSERT,
    EDI_MODE_COMMAND
} edi_mode_t;

/* Editor state - all static to avoid heap fragmentation */
typedef struct {
    int cx, cy;                         /* Cursor position */
    int row_offset;                     /* Vertical scroll offset */
    int col_offset;                     /* Horizontal scroll offset */
    edi_mode_t mode;                    /* Current mode */
    int numrows;                        /* Number of lines in buffer */
    edi_row_t rows[EDI_MAX_ROWS];       /* Line buffer (static array) */
    char filename[SHELL_MAX_FILENAME];  /* Current filename */
    char statusmsg[EDI_STATUS_SIZE];    /* Status message */
    char cmdbuf[EDI_CMD_BUF_SIZE];      /* Command buffer */
    int cmdbuf_len;                     /* Command buffer length */
    int quit;                           /* Quit flag */
    int modified;                       /* Buffer modified flag */
} edi_state_t;

/* Global editor state */
static edi_state_t E;

/* ============================================================================
 * Platform I/O - Use serial console
 * ============================================================================ */

/* Read a single character from input (blocking) */
static int edi_getchar(void) {
    return getchar();
}

/* Write a string to output */
static void edi_write(const char *s, int len) {
    for (int i = 0; i < len; i++) {
        putchar(s[i]);
    }
    fflush(stdout);
}

/* Write a null-terminated string */
static void edi_puts(const char *s) {
    edi_write(s, strlen(s));
}

/* ============================================================================
 * VT100 Escape Sequences
 * ============================================================================ */

/* Clear screen */
static void edi_clear_screen(void) {
    edi_puts("\x1b[2J");    /* Clear entire screen */
    edi_puts("\x1b[H");     /* Move cursor to home */
}

/* Move cursor to position (1-based) */
static void edi_move_cursor(int row, int col) {
    char buf[32];
    snprintf(buf, sizeof(buf), "\x1b[%d;%dH", row, col);
    edi_puts(buf);
}

/* Hide cursor */
static void edi_hide_cursor(void) {
    edi_puts("\x1b[?25l");
}

/* Show cursor */
static void edi_show_cursor(void) {
    edi_puts("\x1b[?25h");
}

/* Clear to end of line */
static void edi_clear_eol(void) {
    edi_puts("\x1b[K");
}

/* Set inverse video (for status bar) */
static void edi_inverse_on(void) {
    edi_puts("\x1b[7m");
}

/* Reset video attributes */
static void edi_inverse_off(void) {
    edi_puts("\x1b[m");
}

/* ============================================================================
 * Row Operations
 * ============================================================================ */

/* Insert character into a row */
static void edi_row_insert_char(edi_row_t *row, int at, int c) {
    if (at < 0 || at > row->size) at = row->size;
    if (row->size >= EDI_MAX_LINE_LEN) return;  /* Line full */
    
    /* Shift characters right */
    memmove(&row->chars[at + 1], &row->chars[at], row->size - at + 1);
    row->size++;
    row->chars[at] = c;
    E.modified = 1;
}

/* Delete character from a row */
static void edi_row_delete_char(edi_row_t *row, int at) {
    if (at < 0 || at >= row->size) return;
    
    /* Shift characters left */
    memmove(&row->chars[at], &row->chars[at + 1], row->size - at);
    row->size--;
    E.modified = 1;
}

/* Append string to a row */
static void edi_row_append_string(edi_row_t *row, const char *s, int len) {
    if (row->size + len > EDI_MAX_LINE_LEN) {
        len = EDI_MAX_LINE_LEN - row->size;
    }
    memcpy(&row->chars[row->size], s, len);
    row->size += len;
    row->chars[row->size] = '\0';
    E.modified = 1;
}

/* Insert a new row at index */
static int edi_insert_row(int at, const char *s, int len) {
    if (at < 0 || at > E.numrows) return -1;
    if (E.numrows >= EDI_MAX_ROWS) return -1;  /* Buffer full */
    if (len > EDI_MAX_LINE_LEN) len = EDI_MAX_LINE_LEN;
    
    /* Shift rows down */
    memmove(&E.rows[at + 1], &E.rows[at], sizeof(edi_row_t) * (E.numrows - at));
    
    /* Initialize new row */
    memcpy(E.rows[at].chars, s, len);
    E.rows[at].chars[len] = '\0';
    E.rows[at].size = len;
    E.numrows++;
    E.modified = 1;
    
    return 0;
}

/* Delete a row at index */
static void edi_delete_row(int at) {
    if (at < 0 || at >= E.numrows) return;
    
    /* Shift rows up */
    memmove(&E.rows[at], &E.rows[at + 1], sizeof(edi_row_t) * (E.numrows - at - 1));
    E.numrows--;
    E.modified = 1;
}

/* ============================================================================
 * File Operations
 * ============================================================================ */

/* Open a file for editing */
static int edi_open(const char *filename) {
    if (filename == NULL) return -1;
    
    strncpy(E.filename, filename, SHELL_MAX_FILENAME - 1);
    E.filename[SHELL_MAX_FILENAME - 1] = '\0';
    
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        /* File doesn't exist - that's okay, we'll create on save */
        return 0;
    }
    
    char line[EDI_MAX_LINE_LEN + 2];  /* +2 for newline and null */
    
    while (fgets(line, sizeof(line), fp) != NULL && E.numrows < EDI_MAX_ROWS) {
        int len = strlen(line);
        
        /* Strip newline/carriage return */
        while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r')) {
            len--;
        }
        line[len] = '\0';
        
        edi_insert_row(E.numrows, line, len);
    }
    
    fclose(fp);
    E.modified = 0;  /* Just loaded, not modified */
    return 0;
}

/* Save buffer to file */
static int edi_save(const char *filename) {
    if (filename != NULL) {
        strncpy(E.filename, filename, SHELL_MAX_FILENAME - 1);
        E.filename[SHELL_MAX_FILENAME - 1] = '\0';
    }
    
    if (E.filename[0] == '\0') {
        snprintf(E.statusmsg, sizeof(E.statusmsg), "No filename");
        return -1;
    }
    
    FILE *fp = fopen(E.filename, "w");
    if (!fp) {
        snprintf(E.statusmsg, sizeof(E.statusmsg), "Error: cannot save");
        return -1;
    }
    
    int bytes = 0;
    for (int i = 0; i < E.numrows; i++) {
        fwrite(E.rows[i].chars, 1, E.rows[i].size, fp);
        fwrite("\n", 1, 1, fp);
        bytes += E.rows[i].size + 1;
    }
    
    fclose(fp);
    E.modified = 0;
    snprintf(E.statusmsg, sizeof(E.statusmsg), "\"%.40s\" %d bytes", 
             E.filename, bytes);
    return 0;
}

/* ============================================================================
 * Editor Operations
 * ============================================================================ */

/* Insert character at cursor position */
static void edi_insert_char(int c) {
    /* If cursor is past end, create new row */
    if (E.cy == E.numrows) {
        if (edi_insert_row(E.numrows, "", 0) < 0) return;
    }
    
    edi_row_insert_char(&E.rows[E.cy], E.cx, c);
    E.cx++;
}

/* Insert new line at cursor position */
static void edi_insert_newline(void) {
    if (E.numrows >= EDI_MAX_ROWS) {
        snprintf(E.statusmsg, sizeof(E.statusmsg), "Max lines reached");
        return;
    }
    
    if (E.cx == 0) {
        /* At start of line - insert blank row before */
        edi_insert_row(E.cy, "", 0);
    } else {
        /* Split current line */
        edi_row_t *row = &E.rows[E.cy];
        edi_insert_row(E.cy + 1, &row->chars[E.cx], row->size - E.cx);
        
        /* Truncate current line */
        row = &E.rows[E.cy];  /* Re-reference after insert */
        row->size = E.cx;
        row->chars[row->size] = '\0';
    }
    
    E.cy++;
    E.cx = 0;
}

/* Delete character before cursor */
static void edi_delete_char(void) {
    if (E.cy == E.numrows) return;
    if (E.cx == 0 && E.cy == 0) return;
    
    if (E.cx > 0) {
        /* Delete character before cursor */
        edi_row_delete_char(&E.rows[E.cy], E.cx - 1);
        E.cx--;
    } else {
        /* At start of line - join with previous line */
        E.cx = E.rows[E.cy - 1].size;
        edi_row_append_string(&E.rows[E.cy - 1], E.rows[E.cy].chars, E.rows[E.cy].size);
        edi_delete_row(E.cy);
        E.cy--;
    }
}

/* Move cursor */
static void edi_move(int key) {
    edi_row_t *row = (E.cy < E.numrows) ? &E.rows[E.cy] : NULL;
    
    switch (key) {
        case 'h':
            if (E.cx > 0) E.cx--;
            break;
        case 'j':
            if (E.cy < E.numrows - 1) E.cy++;
            break;
        case 'k':
            if (E.cy > 0) E.cy--;
            break;
        case 'l':
            if (row && E.cx < row->size) E.cx++;
            break;
    }
    
    /* Snap cursor to end of line if line is shorter */
    row = (E.cy < E.numrows) ? &E.rows[E.cy] : NULL;
    int rowlen = row ? row->size : 0;
    if (E.cx > rowlen) {
        E.cx = rowlen;
    }
}

/* ============================================================================
 * Screen Drawing
 * ============================================================================ */

/* Adjust scroll offset to keep cursor visible */
static void edi_scroll(void) {
    /* Vertical scrolling */
    if (E.cy < E.row_offset) {
        E.row_offset = E.cy;
    }
    if (E.cy >= E.row_offset + EDI_SCREEN_ROWS - 2) {  /* -2 for status bar */
        E.row_offset = E.cy - EDI_SCREEN_ROWS + 3;
    }
    
    /* Horizontal scrolling */
    if (E.cx < E.col_offset) {
        E.col_offset = E.cx;
    }
    if (E.cx >= E.col_offset + EDI_SCREEN_COLS) {
        E.col_offset = E.cx - EDI_SCREEN_COLS + 1;
    }
}

/* Refresh the screen */
static void edi_refresh_screen(void) {
    edi_scroll();
    
    edi_hide_cursor();
    edi_move_cursor(1, 1);
    
    /* Draw rows */
    for (int y = 0; y < EDI_SCREEN_ROWS - 2; y++) {  /* -2 for status + message */
        int filerow = y + E.row_offset;
        
        if (filerow >= E.numrows) {
            /* Past end of file - draw tilde */
            putchar('~');
        } else {
            /* Draw line content */
            edi_row_t *row = &E.rows[filerow];
            int len = row->size - E.col_offset;
            if (len < 0) len = 0;
            if (len > EDI_SCREEN_COLS) len = EDI_SCREEN_COLS;
            if (len > 0) {
                edi_write(&row->chars[E.col_offset], len);
            }
        }
        
        edi_clear_eol();
        edi_puts("\r\n");
    }
    
    /* Draw status bar */
    edi_inverse_on();
    
    char status[EDI_SCREEN_COLS + 1];
    char rstatus[32];
    
    const char *modestr = "NORMAL";
    if (E.mode == EDI_MODE_INSERT) modestr = "INSERT";
    else if (E.mode == EDI_MODE_COMMAND) modestr = "COMMAND";
    
    if (E.mode == EDI_MODE_COMMAND) {
        snprintf(status, sizeof(status), ":%.*s", E.cmdbuf_len, E.cmdbuf);
    } else {
        snprintf(status, sizeof(status), "-- %s -- %s%s", 
                 modestr,
                 E.filename[0] ? E.filename : "[No Name]",
                 E.modified ? " [+]" : "");
    }
    
    snprintf(rstatus, sizeof(rstatus), "%d/%d L, %d C", 
             E.cy + 1, E.numrows, E.cx + 1);
    
    int slen = strlen(status);
    int rlen = strlen(rstatus);
    
    edi_puts(status);
    
    /* Pad with spaces */
    for (int i = slen; i < EDI_SCREEN_COLS - rlen; i++) {
        putchar(' ');
    }
    
    if (slen + rlen <= EDI_SCREEN_COLS) {
        edi_puts(rstatus);
    }
    
    edi_inverse_off();
    edi_puts("\r\n");
    
    /* Draw message bar */
    edi_puts(E.statusmsg);
    edi_clear_eol();
    
    /* Position cursor */
    edi_move_cursor(E.cy - E.row_offset + 1, E.cx - E.col_offset + 1);
    edi_show_cursor();
    
    fflush(stdout);
}

/* ============================================================================
 * Command Processing
 * ============================================================================ */

/* Process a : command */
static void edi_process_command(void) {
    E.statusmsg[0] = '\0';
    
    if (E.cmdbuf_len == 0) {
        E.mode = EDI_MODE_NORMAL;
        return;
    }
    
    E.cmdbuf[E.cmdbuf_len] = '\0';
    
    if (strcmp(E.cmdbuf, "q") == 0) {
        if (E.modified) {
            snprintf(E.statusmsg, sizeof(E.statusmsg), 
                     "Unsaved changes! Use :q! to force quit");
        } else {
            E.quit = 1;
        }
    } else if (strcmp(E.cmdbuf, "q!") == 0) {
        E.quit = 1;
    } else if (strcmp(E.cmdbuf, "w") == 0) {
        edi_save(NULL);
    } else if (strncmp(E.cmdbuf, "w ", 2) == 0) {
        edi_save(E.cmdbuf + 2);
    } else if (strcmp(E.cmdbuf, "wq") == 0) {
        if (edi_save(NULL) == 0) {
            E.quit = 1;
        }
    } else {
        snprintf(E.statusmsg, sizeof(E.statusmsg), 
                 "Unknown command: %.50s", E.cmdbuf);
    }
    
    E.mode = EDI_MODE_NORMAL;
}

/* ============================================================================
 * Input Handling
 * ============================================================================ */

/* Process a keypress */
static void edi_process_keypress(void) {
    int c = edi_getchar();
    
    /* Handle escape sequences for arrow keys */
    if (c == 0x1b) {  /* Escape */
        int c2 = edi_getchar();
        if (c2 == '[') {
            int c3 = edi_getchar();
            switch (c3) {
                case 'A': c = 'k'; break;  /* Up -> k */
                case 'B': c = 'j'; break;  /* Down -> j */
                case 'C': c = 'l'; break;  /* Right -> l */
                case 'D': c = 'h'; break;  /* Left -> h */
                default:
                    /* Unknown sequence - treat as Escape */
                    if (E.mode == EDI_MODE_INSERT || E.mode == EDI_MODE_COMMAND) {
                        E.mode = EDI_MODE_NORMAL;
                    }
                    return;
            }
            /* In INSERT mode, arrow keys move cursor */
            if (E.mode == EDI_MODE_INSERT) {
                edi_move(c);
                return;
            }
        } else {
            /* Plain Escape - switch to NORMAL mode */
            if (E.mode != EDI_MODE_NORMAL) {
                E.mode = EDI_MODE_NORMAL;
                E.cmdbuf_len = 0;
            }
            return;
        }
    }
    
    if (E.mode == EDI_MODE_NORMAL) {
        switch (c) {
            case ':':
                E.mode = EDI_MODE_COMMAND;
                E.cmdbuf_len = 0;
                E.cmdbuf[0] = '\0';
                break;
            
            case 'i':
                E.mode = EDI_MODE_INSERT;
                break;
            
            case 'h':
            case 'j':
            case 'k':
            case 'l':
                edi_move(c);
                break;
            
            case 'x':
                /* Delete character under cursor */
                if (E.cy < E.numrows) {
                    edi_row_t *row = &E.rows[E.cy];
                    if (E.cx < row->size) {
                        edi_row_delete_char(row, E.cx);
                        if (E.cx >= row->size && row->size > 0) {
                            E.cx = row->size - 1;
                        }
                    }
                }
                break;
            
            case 'o':
                /* Open new line below */
                if (E.numrows < EDI_MAX_ROWS) {
                    edi_insert_row(E.cy + 1, "", 0);
                    E.cy++;
                    E.cx = 0;
                    E.mode = EDI_MODE_INSERT;
                }
                break;
            
            case 'O':
                /* Open new line above */
                if (E.numrows < EDI_MAX_ROWS) {
                    edi_insert_row(E.cy, "", 0);
                    E.cx = 0;
                    E.mode = EDI_MODE_INSERT;
                }
                break;
            
            case 'A':
                /* Append at end of line */
                if (E.cy < E.numrows) {
                    E.cx = E.rows[E.cy].size;
                }
                E.mode = EDI_MODE_INSERT;
                break;
            
            case 'G':
                /* Go to last line */
                if (E.numrows > 0) {
                    E.cy = E.numrows - 1;
                }
                break;
            
            case 'g':
                /* Wait for second 'g' to go to first line */
                {
                    int c2 = edi_getchar();
                    if (c2 == 'g') {
                        E.cy = 0;
                        E.cx = 0;
                    }
                }
                break;
            
            case '0':
                /* Go to start of line */
                E.cx = 0;
                break;
            
            case '$':
                /* Go to end of line */
                if (E.cy < E.numrows) {
                    E.cx = E.rows[E.cy].size;
                    if (E.cx > 0) E.cx--;
                }
                break;
            
            case 'd':
                /* Wait for second 'd' to delete line */
                {
                    int c2 = edi_getchar();
                    if (c2 == 'd' && E.cy < E.numrows) {
                        edi_delete_row(E.cy);
                        if (E.cy >= E.numrows && E.cy > 0) {
                            E.cy--;
                        }
                        E.cx = 0;
                    }
                }
                break;
        }
    } else if (E.mode == EDI_MODE_INSERT) {
        switch (c) {
            case 127:  /* Backspace (DEL) */
            case 0x08: /* Backspace (BS) / Ctrl-H */
                edi_delete_char();
                break;
            
            case '\r':  /* Enter */
            case '\n':
                edi_insert_newline();
                break;
            
            default:
                if (isprint(c) || c == '\t') {
                    edi_insert_char(c);
                }
                break;
        }
    } else if (E.mode == EDI_MODE_COMMAND) {
        switch (c) {
            case '\r':  /* Enter */
            case '\n':
                edi_process_command();
                break;
            
            case 127:  /* Backspace */
            case 0x08: /* Backspace (BS) / Ctrl-H */
                if (E.cmdbuf_len > 0) {
                    E.cmdbuf_len--;
                } else {
                    E.mode = EDI_MODE_NORMAL;
                }
                break;
            
            default:
                if (isprint(c) && E.cmdbuf_len < EDI_CMD_BUF_SIZE - 1) {
                    E.cmdbuf[E.cmdbuf_len++] = c;
                }
                break;
        }
    }
}

/* ============================================================================
 * Editor Entry Point
 * ============================================================================ */

/**
 * @brief Initialize and run the edi editor
 * 
 * @param argc Argument count
 * @param argv Argument vector (argv[1] = optional filename)
 * @return 0 on success
 */
int cmd_edi(int argc, char **argv) {
    /* Initialize editor state */
    memset(&E, 0, sizeof(E));
    E.mode = EDI_MODE_NORMAL;
    
    /* Open file if specified */
    if (argc >= 2) {
        edi_open(argv[1]);
    }
    
    /* If no rows, start with one empty row */
    if (E.numrows == 0) {
        edi_insert_row(0, "", 0);
        E.modified = 0;
    }
    
    /* Clear screen */
    edi_clear_screen();
    
    /* Main loop */
    while (!E.quit) {
        edi_refresh_screen();
        edi_process_keypress();
    }
    
    /* Clear screen on exit */
    edi_clear_screen();
    
    return 0;
}
