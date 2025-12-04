/*
 * edi.c - A minimalist vim-like text editor
 * 
 * Compilation:
 *   gcc -o edi edi.c -std=c99 -Wall -Wextra
 * 
 * Usage:
 *   ./edi              # Start with empty buffer
 *   ./edi filename.txt # Open existing file
 */

/*** includes ***/

#define _POSIX_C_SOURCE 200809L

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

// Environment structure forward declaration
typedef struct Environment Env;

/*** defines ***/

#define CTRL_KEY(k) ((k) & 0x1f)
#define EDITOR_VERSION "0.0.1"

/*** data ***/

// Structure to hold a single line of text
typedef struct erow {
  int size;
  char *chars;
} erow;

enum editorMode {
  NORMAL,
  INSERT,
  COMMAND
};

struct editorConfig {
  int cx, cy; // Cursor x and y position
  int screenrows;
  int screencols;
  int numrows;  // Number of rows in the buffer
  erow *row;    // Pointer to an array of erow structs (our buffer)
  int row_offset; // Keep track of which row is at the top of the screen
  int col_offset; // Keep track of which column is at the left of the screen
  enum editorMode mode;
  char *filename; // To store the name of the file
  char statusmsg[80]; // Status message to display
  char cmdbuf[80];    // Command buffer for : commands
  int cmdbuf_len;     // Length of command buffer
  struct termios orig_termios; // To restore terminal on exit
  int quit;           // Flag to signal quit
};

struct editorConfig E;

/*** forward declarations ***/

void editorCleanup();

/*** terminal ***/

// Error handling
void die(const char *s) {
  // Clear screen on exit
  write(STDOUT_FILENO, "\x1b[2J", 4);
  write(STDOUT_FILENO, "\x1b[H", 3);
  
  perror(s); // Print error message
  exit(1);
}

void disableRawMode() {
  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &E.orig_termios) == -1)
    die("tcsetattr");
}

void enableRawMode() {
  if (tcgetattr(STDIN_FILENO, &E.orig_termios) == -1) die("tcgetattr");
  // Don't register atexit in built-in mode - we'll call disableRawMode manually

  struct termios raw = E.orig_termios;
  // Disable echo, canonical mode, Ctrl-V, and Ctrl-C/Z
  raw.c_lflag &= ~(ECHO | ICANON | ISIG | IEXTEN);
  // Disable Ctrl-S/Q (software flow control) and Ctrl-M
  raw.c_iflag &= ~(IXON | ICRNL | BRKINT | INPCK | ISTRIP);
  // Disable output processing (like converting \n to \r\n)
  raw.c_oflag &= ~(OPOST);
  raw.c_cflag |= (CS8);
  
  // Set timeout for read()
  raw.c_cc[VMIN] = 0;
  raw.c_cc[VTIME] = 1;

  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) die("tcsetattr");
}

// Read a single keypress from standard input
int readKey() {
  int nread;
  char c;
  while ((nread = read(STDIN_FILENO, &c, 1)) != 1) {
    if (nread == -1 && errno != EAGAIN) die("read");
  }
  return c;
}

int getWindowSize(int *rows, int *cols) {
  struct winsize ws;

  if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
    return -1;
  } else {
    *cols = ws.ws_col;
    *rows = ws.ws_row;
    return 0;
  }
}

/*** row operations ***/

/*
 * Memory Management Strategy:
 * 
 * The editor maintains a dynamic array of rows (E.row), where each row
 * contains a dynamically allocated character buffer (chars).
 * 
 * Row Array (E.row):
 *   - Initially NULL with E.numrows = 0
 *   - Grows via realloc() when inserting new rows
 *   - Uses memmove() to shift rows when inserting/deleting
 * 
 * Character Buffers (row->chars):
 *   - Each row's text is stored in a malloc'd buffer
 *   - Grows via realloc() when inserting characters
 *   - Always null-terminated for safety
 * 
 * Cleanup:
 *   - editorCleanup() frees all character buffers first
 *   - Then frees the row array itself
 *   - Also frees the filename string
 *   - Called before all exit points to prevent memory leaks
 */

// Insert a character into a row
void editorRowInsertChar(erow *row, int at, int c) {
  if (at < 0 || at > row->size) at = row->size;
  // +2: one for the new char, one for the null terminator
  row->chars = realloc(row->chars, row->size + 2);
  // Make space for the new character
  memmove(&row->chars[at + 1], &row->chars[at], row->size - at + 1);
  row->size++;
  row->chars[at] = c;
}

// Deletes a character from a row
void editorRowDeleteChar(erow *row, int at) {
  if (at < 0 || at >= row->size) return;
  // Overwrite the char at 'at' by moving memory
  memmove(&row->chars[at], &row->chars[at + 1], row->size - at);
  row->size--;
  // We could realloc to shrink, but we won't for simplicity
}

// Appends a string to the end of a row
void editorRowAppendString(erow *row, char *s, size_t len) {
    row->chars = realloc(row->chars, row->size + len + 1);
    memcpy(&row->chars[row->size], s, len);
    row->size += len;
    row->chars[row->size] = '\0';
}

// Insert a new empty row at a specific index
void editorInsertRow(int at, char *s, size_t len) {
  if (at < 0 || at > E.numrows) return;

  // Allocate space for the new row in the E.row array
  E.row = realloc(E.row, sizeof(erow) * (E.numrows + 1));
  // Move all rows from 'at' onwards down by one
  memmove(&E.row[at + 1], &E.row[at], sizeof(erow) * (E.numrows - at));

  // Initialize the new row
  E.row[at].size = len;
  E.row[at].chars = malloc(len + 1);
  memcpy(E.row[at].chars, s, len);
  E.row[at].chars[len] = '\0';
  E.numrows++;
}

// Opens and reads a file into the buffer
// If the file doesn't exist, it will be created when saved
void editorOpen(char *filename) {
  free(E.filename); // Free old filename if any
  E.filename = strdup(filename); // Make a copy of the filename

  FILE *fp = fopen(filename, "r");
  if (!fp) {
    // File doesn't exist - that's okay, we'll create it on save
    // Start with an empty buffer
    return;
  }

  char *line = NULL;
  size_t linecap = 0;
  ssize_t linelen;

  // Read file line by line
  while ((linelen = getline(&line, &linecap, fp)) != -1) {
    // Strip newline or carriage return
    while (linelen > 0 && (line[linelen - 1] == '\n' ||
                           line[linelen - 1] == '\r'))
      linelen--;
    
    // Add the line to our buffer
    editorInsertRow(E.numrows, line, linelen);
  }
  free(line);
  fclose(fp);
}

// Saves the current buffer to disk
// Returns 0 on success, -1 on failure
int editorSave(char *filename) {
  if (filename) {
    free(E.filename);
    E.filename = strdup(filename);
  }
  
  if (E.filename == NULL) {
    return -1; // No filename specified
  }

  FILE *fp = fopen(E.filename, "w");
  if (!fp) {
    return -1; // Failed to open file for writing
  }

  // Write each row to the file
  for (int i = 0; i < E.numrows; i++) {
    fwrite(E.row[i].chars, 1, E.row[i].size, fp);
    fwrite("\n", 1, 1, fp); // Add newline after each row
  }

  fclose(fp);
  return 0; // Success
}


// Deletes a row at index 'at'
void editorDeleteRow(int at) {
    if (at < 0 || at >= E.numrows) return;
    
    // Free the memory for the row's chars
    free(E.row[at].chars);
    
    // Move all rows after 'at' up by one
    memmove(&E.row[at], &E.row[at + 1], sizeof(erow) * (E.numrows - at - 1));
    E.numrows--;
    // We could realloc E.row to shrink, but we won't
}

// Handles pressing 'Enter' in INSERT mode
void editorInsertNewLine() {
  if (E.cx == 0) {
    // If at start of line, just insert a blank row before
    editorInsertRow(E.cy, "", 0);
  } else {
    // Split the current line
    erow *row = &E.row[E.cy];
    // Insert a new row with the content from the cursor position onwards
    editorInsertRow(E.cy + 1, &row->chars[E.cx], row->size - E.cx);
    
    // Truncate the current line
    row = &E.row[E.cy]; // Re-assign, as realloc in editorInsertRow might move it
    row->size = E.cx;
    row->chars[row->size] = '\0';
    // We can realloc to shrink, but not strictly necessary immediately
  }
  // Move cursor to the start of the new line
  E.cy++;
  E.cx = 0;
}

// This handles character insertion in INSERT mode
void editorInsertChar(int c) {
  // If cursor is on the '~' line, append a new row first
  if (E.cy == E.numrows) {
    editorInsertRow(E.numrows, "", 0);
  }
  // Insert the character at the cursor position
  editorRowInsertChar(&E.row[E.cy], E.cx, c);
  E.cx++; // Move cursor forward
}

// Handles 'Backspace' in INSERT mode
void editorDeleteChar() {
  if (E.cy == E.numrows) return; // No deleting on '~' line
  if (E.cx == 0 && E.cy == 0) return; // At start of file

  if (E.cx > 0) {
    // Just delete character before cursor
    editorRowDeleteChar(&E.row[E.cy], E.cx - 1);
    E.cx--;
  } else {
    // At start of line, join with the line above
    E.cx = E.row[E.cy - 1].size; // Move cursor to end of prev line
    // Append current line's content to previous line
    editorRowAppendString(&E.row[E.cy - 1], E.row[E.cy].chars, E.row[E.cy].size);
    // Delete the current row
    editorDeleteRow(E.cy);
    E.cy--;
  }
}

/*** append buffer ***/

// A simple dynamic string buffer for writing to the screen
struct abuf {
  char *b;
  int len;
};

#define ABUF_INIT {NULL, 0}

void abAppend(struct abuf *ab, const char *s, int len) {
  char *new_buf = realloc(ab->b, ab->len + len);

  if (new_buf == NULL) return;
  memcpy(&new_buf[ab->len], s, len);
  ab->b = new_buf;
  ab->len += len;
}

void abFree(struct abuf *ab) {
  free(ab->b);
}


/*** output ***/

// This function adjusts the row_offset and col_offset
// based on the cursor's position (E.cy, E.cx) to ensure
// the cursor is always visible on screen.
void editorScroll() {
  // Vertical scrolling
  if (E.cy < E.row_offset) {
    // Cursor is above the visible window
    E.row_offset = E.cy;
  }
  // -1 for status bar
  if (E.cy >= E.row_offset + E.screenrows - 1) {
    // Cursor is below the visible window
    E.row_offset = E.cy - E.screenrows + 2; 
  }

  // Horizontal scrolling
  if (E.cx < E.col_offset) {
    // Cursor is left of the visible window
    E.col_offset = E.cx;
  }
  if (E.cx >= E.col_offset + E.screencols) {
    // Cursor is right of the visible window
    E.col_offset = E.cx - E.screencols + 1;
  }
}

void editorDrawRows(struct abuf *ab) {
  int y;
  for (y = 0; y < E.screenrows - 1; y++) { // Leave space for status bar
    int filerow = y + E.row_offset;
    if (filerow >= E.numrows) {
      // Draw tildes for lines past the end of the file
      abAppend(ab, "~", 1);
    } else {
      // Draw the actual line from the buffer
      int len = E.row[filerow].size - E.col_offset;
      if (len < 0) len = 0; // Handle scrolling past end of line
      if (len > E.screencols) len = E.screencols; // Truncate to screen width
      abAppend(ab, &E.row[filerow].chars[E.col_offset], len);
    }
    
    // Clear the rest of the line
    abAppend(ab, "\x1b[K", 3);
    abAppend(ab, "\r\n", 2);
  }
}

void editorDrawStatusBar(struct abuf *ab) {
  char status[80];
  char modestr[16];
  
  // Get mode string
  switch(E.mode) {
    case NORMAL:
      snprintf(modestr, sizeof(modestr), "NORMAL");
      break;
    case INSERT:
      snprintf(modestr, sizeof(modestr), "INSERT");
      break;
    case COMMAND:
      snprintf(modestr, sizeof(modestr), "COMMAND");
      break;
  }

  // If in COMMAND mode, show command buffer
  if (E.mode == COMMAND) {
    snprintf(status, sizeof(status), ":%.*s", E.cmdbuf_len, E.cmdbuf);
  } else {
    // Format status bar text: Mode, filename
    snprintf(status, sizeof(status), "-- %s -- %s", 
             modestr, E.filename ? E.filename : "[No Name]");
  }
  
  int len = strlen(status);
  
  // Right-align cursor position (unless in command mode)
  char rstatus[32];
  rstatus[0] = '\0';
  int rlen = 0;
  
  if (E.mode != COMMAND) {
    // Show Line/TotalLines, Column
    snprintf(rstatus, sizeof(rstatus), "%d/%d L, %d C", 
             E.cy + 1, E.numrows, E.cx + 1);
    rlen = strlen(rstatus);
  }
  
  // Style: inverted colors
  abAppend(ab, "\x1b[7m", 4); 
  abAppend(ab, status, len);
  
  // Fill the middle with spaces
  while (len < E.screencols - rlen) {
    abAppend(ab, " ", 1);
    len++;
  }
  
  // Append right-aligned status
  if (rlen > 0 && len + rlen <= E.screencols) {
    abAppend(ab, rstatus, rlen);
  }
  abAppend(ab, "\x1b[m", 3); // Reset styling
  
  // Display status message if available
  if (E.statusmsg[0] != '\0' && E.mode != COMMAND) {
    abAppend(ab, "\r\n", 2);
    abAppend(ab, E.statusmsg, strlen(E.statusmsg));
    abAppend(ab, "\x1b[K", 3); // Clear rest of line
  }
}

void editorRefreshScreen() {
  editorScroll(); // Adjust scrolling before redrawing

  struct abuf ab = ABUF_INIT;

  abAppend(&ab, "\x1b[?25l", 6); // Hide cursor
  abAppend(&ab, "\x1b[H", 3);  // Move cursor to 1,1 (top-left)

  editorDrawRows(&ab);
  editorDrawStatusBar(&ab);

  // Move cursor to its position
  // E.cy and E.cx are FILE coordinates. We need to convert to SCREEN coordinates.
  char buf[32];
  snprintf(buf, sizeof(buf), "\x1b[%d;%dH", (E.cy - E.row_offset) + 1,
                                          (E.cx - E.col_offset) + 1);
  abAppend(&ab, buf, strlen(buf));
  
  abAppend(&ab, "\x1b[?25h", 6); // Show cursor

  write(STDOUT_FILENO, ab.b, ab.len);
  abFree(&ab);
}

/*** input ***/

// Process a command entered in COMMAND mode
void editorProcessCommand() {
  E.statusmsg[0] = '\0'; // Clear status message
  
  if (E.cmdbuf_len == 0) {
    E.mode = NORMAL;
    return;
  }
  
  // Null-terminate the command buffer
  E.cmdbuf[E.cmdbuf_len] = '\0';
  
  // Parse command
  if (strcmp(E.cmdbuf, "q") == 0) {
    // Quit
    editorCleanup();
    write(STDOUT_FILENO, "\x1b[2J", 4);
    write(STDOUT_FILENO, "\x1b[H", 3);
    E.quit = 1;
  } else if (strcmp(E.cmdbuf, "w") == 0) {
    // Save to current filename
    if (editorSave(NULL) == 0) {
      snprintf(E.statusmsg, sizeof(E.statusmsg), "\"%s\" written", 
               E.filename ? E.filename : "[No Name]");
    } else {
      snprintf(E.statusmsg, sizeof(E.statusmsg), "Error: could not save file");
    }
  } else if (strncmp(E.cmdbuf, "w ", 2) == 0) {
    // Save to specified filename
    char *filename = E.cmdbuf + 2;
    if (editorSave(filename) == 0) {
      snprintf(E.statusmsg, sizeof(E.statusmsg), "\"%s\" written", filename);
    } else {
      snprintf(E.statusmsg, sizeof(E.statusmsg), "Error: could not save to %s", filename);
    }
  } else if (strcmp(E.cmdbuf, "wq") == 0) {
    // Save and quit
    if (editorSave(NULL) == 0) {
      editorCleanup();
      write(STDOUT_FILENO, "\x1b[2J", 4);
      write(STDOUT_FILENO, "\x1b[H", 3);
      E.quit = 1;
    } else {
      snprintf(E.statusmsg, sizeof(E.statusmsg), "Error: could not save file");
    }
  } else if (strcmp(E.cmdbuf, "q!") == 0) {
    // Force quit without saving
    editorCleanup();
    write(STDOUT_FILENO, "\x1b[2J", 4);
    write(STDOUT_FILENO, "\x1b[H", 3);
    E.quit = 1;
  } else {
    snprintf(E.statusmsg, sizeof(E.statusmsg), "Unknown command: %.60s", E.cmdbuf);
  }
  
  E.mode = NORMAL;
}

void editorMoveCursor(int key) {
  // Get the row the cursor is currently on
  erow *row = (E.cy >= E.numrows) ? NULL : &E.row[E.cy];

  switch (key) {
    case 'h':
      if (E.cx > 0) E.cx--;
      break;
    case 'j':
      if (E.cy < E.numrows - 1) E.cy++; // Don't move past the last line
      break;
    case 'k':
      if (E.cy > 0) E.cy--;
      break;
    case 'l':
      // Only move right if the row exists and cursor isn't past the end
      if (row && E.cx < row->size) E.cx++; 
      break;
  }
  
  // After moving up or down, snap cursor to the end of the new line
  // if the new line is shorter.
  row = (E.cy >= E.numrows) ? NULL : &E.row[E.cy];
  int rowlen = row ? row->size : 0;
  if (E.cx > rowlen) {
    E.cx = rowlen;
  }
}

void editorProcessKeypress() {
  int c = readKey();

  if (E.mode == NORMAL) {
    switch (c) {
      case 'q':
        // Note: 'q' alone won't quit anymore - use :q instead
        // This is to match vim behavior better
        break;

      case ':':
        // Enter COMMAND mode
        E.mode = COMMAND;
        E.cmdbuf_len = 0;
        E.cmdbuf[0] = '\0';
        break;

      case 'i':
        // Switch to INSERT mode
        E.mode = INSERT;
        break;
      
      // Basic cursor movement
      case 'h':
      case 'j':
      case 'k':
      case 'l':
        editorMoveCursor(c);
        break;
        
      case 'x': { // Delete character under cursor
        if (E.cy >= E.numrows) break; // On tilde line
        erow *row = &E.row[E.cy];
        if (E.cx >= row->size) break; // Cursor is past end of line, nothing to delete

        editorRowDeleteChar(row, E.cx);
        
        // If cursor is now past the end of the line (after deletion),
        // and the line isn't empty, move cursor left.
        if (E.cx >= row->size && row->size > 0) {
           E.cx = row->size - 1;
        }
        break;
      }
    }
  } else if (E.mode == INSERT) {
    switch (c) {
      case 27: // Escape key
        // Switch to NORMAL mode
        E.mode = NORMAL;
        break;
      
      case 127: // Backspace key
      case CTRL_KEY('h'): // Also treat Ctrl-H as backspace
        editorDeleteChar();
        break;
        
      case '\r': // Enter key
        editorInsertNewLine();
        break;
      
      default:
        // Insert character into buffer
        if (isprint(c)) {
          editorInsertChar(c);
        }
        break;
    }
  } else if (E.mode == COMMAND) {
    switch (c) {
      case 27: // Escape key
        // Cancel command mode
        E.mode = NORMAL;
        E.cmdbuf_len = 0;
        break;
      
      case '\r': // Enter key
        // Execute command
        editorProcessCommand();
        break;
      
      case 127: // Backspace key
      case CTRL_KEY('h'):
        // Delete character from command buffer
        if (E.cmdbuf_len > 0) {
          E.cmdbuf_len--;
        }
        break;
      
      default:
        // Add character to command buffer
        if (isprint(c) && E.cmdbuf_len < (int)sizeof(E.cmdbuf) - 1) {
          E.cmdbuf[E.cmdbuf_len++] = c;
        }
        break;
    }
  }
}

/*** cleanup ***/

// Cleanup function to free all allocated memory
// Call this before exiting to prevent memory leaks
void editorCleanup() {
  // Free all rows
  for (int i = 0; i < E.numrows; i++) {
    free(E.row[i].chars);
  }
  free(E.row);
  
  // Free filename
  free(E.filename);
}

/*** init ***/

void initEditor() {
  E.cx = 0;
  E.cy = 0;
  E.numrows = 0;
  E.row = NULL; // Initialize buffer as empty
  E.row_offset = 0;
  E.col_offset = 0;
  E.mode = NORMAL;
  E.filename = NULL; // No file open initially
  E.statusmsg[0] = '\0'; // Clear status message
  E.cmdbuf_len = 0; // Clear command buffer
  E.cmdbuf[0] = '\0';
  E.quit = 0; // Initialize quit flag

  if (getWindowSize(&E.screenrows, &E.screencols) == -1) die("getWindowSize");
}

int builtin_edi(char **argv, Env *env) {
  (void)env; // Unused parameter
  
  // Count arguments
  int argc = 0;
  while (argv[argc] != NULL) {
    argc++;
  }
  
  enableRawMode();
  initEditor();

  // Open file if one is specified
  if (argc >= 2) {
    editorOpen(argv[1]);
  }

  while (!E.quit) {
    editorRefreshScreen();
    editorProcessKeypress();
  }

  disableRawMode();
  return 0;
}

