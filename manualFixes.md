Documentation: Terminal Input Handling Fixes

1. Initial State & Architecture

The module terminal.c implements a raw-mode shell interface similar to readline.

    Core Functionality: Handles character-by-character input, history navigation (Up/Down), tab completion, and signal handling.

    Original Redraw Logic: Used simple Carriage Return (\r) to return to the start of the line before overwriting old text.

2. Issue #1: The "Staircase" Bug (Line Duplication)

The Problem: When user input exceeded the terminal width, the terminal automatically wrapped the text to a new line. The next keypress triggered a redraw using \r, which only moved the cursor to the start of the current (new) row, causing the prompt and input to be printed again on the line below.

The Fix: We implemented logic to calculate the vertical height of the input and move the cursor up to the starting row.

    Header Added: <sys/ioctl.h> to access window size.

    Logic:

        Get terminal width using ioctl(STDOUT_FILENO, TIOCGWINSZ, &w).

        Calculate how many rows the previous content occupied: old_total / width.

        If the content spanned multiple rows, send ANSI code \x1b[NA (Move Up N lines) before sending \r.

3. Issue #2: The Vertical Overwrite Bug

The Problem: When input grew enough to require a new line (e.g., expanding from 1 row to 2), the redraw logic overwrote the content physically below the cursor instead of pushing the terminal buffer up. This resulted in the bottom line of the terminal being cut off or overwritten incorrectly.

The Fix: We implemented a "Scroll Trick" to force the terminal engine to allocate new space at the bottom of the screen when necessary.

    Logic:

        Compare new_rows (rows needed for new text) vs old_rows (rows used by previous text).

        The Trigger: if (new_rows > old_rows)

        The Action: Write \n followed immediately by \x1b[A (Up).

            If at the bottom of the screen, \n forces a scroll (pushing text up).

            \x1b[A immediately returns the cursor to the correct position so the user doesn't see a jump.

4. Final redraw_line Algorithm

The final, robust version of the redraw function performs these steps in order:

    Measure: specific terminal width via ioctl.

    Detect Growth: Check if the line count has increased. If so, apply the Scroll Trick (\n\x1b[A).

    Reset Cursor: Move the cursor UP (\x1b[A) by the number of rows the previous text occupied.

    Clear: Move to column 0 (\r) and Clear to End of Screen (\x1b[J) to remove artifacts.

    Print: Write the prompt and the full input buffer.

    Position: Calculate the exact target_row and target_col for the cursor based on the text length and terminal width, and move the cursor there using absolute positioning calculations rather than relative steps.

5. Key ANSI Escape Codes Used

Code	Description	Purpose
\r	Carriage Return	Move to column 0 of current row.
\x1b[J	Clear Screen	Clears from cursor to end of screen (prevents "ghost" text).
\x1b[A	Cursor Up	Moves cursor up 1 row.
\x1b[NA	Cursor Up N	Moves cursor up N rows (used for resetting start position).
\x1b[NC	Cursor Right N	Moves cursor right N columns (used for final positioning).