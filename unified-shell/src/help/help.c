/**
 * help.c - Centralized Help System Implementation
 * 
 * This module provides comprehensive help documentation for all built-in
 * shell commands. It maintains a database of help entries and provides
 * functions to retrieve and display help information.
 * 
 * Key features:
 *   - Structured help database for all built-ins
 *   - Consistent help formatting
 *   - Support for --help flag detection
 *   - Easy to extend with new commands
 */

#include "help.h"
#include <stdio.h>
#include <string.h>

/* ============================================================================
 * Help Database - All Built-in Commands
 * ============================================================================ */

/**
 * help_entries - Static array of help information for all built-in commands
 * 
 * This array contains detailed help for every built-in command in the shell.
 * To add a new command, simply add a new HelpEntry to this array.
 */
static const HelpEntry help_entries[] = {
    /* cd - Change Directory */
    {
        .name = "cd",
        .summary = "Change the current working directory",
        .usage = "cd [directory]",
        .description = 
            "Changes the current working directory to the specified path.\n"
            "If no directory is specified, changes to the user's home directory.\n"
            "Supports absolute paths, relative paths, and special directories:\n"
            "  .   Current directory\n"
            "  ..  Parent directory\n"
            "  ~   Home directory\n"
            "  -   Previous directory",
        .options = 
            "directory    Directory path to change to (optional)\n"
            "             If omitted, changes to $HOME",
        .examples =
            "cd /tmp          Change to /tmp directory\n"
            "cd ..            Move to parent directory\n"
            "cd               Change to home directory\n"
            "cd -             Return to previous directory"
    },

    /* pwd - Print Working Directory */
    {
        .name = "pwd",
        .summary = "Print the current working directory",
        .usage = "pwd",
        .description =
            "Displays the absolute path of the current working directory.\n"
            "This shows your current location in the filesystem.",
        .options = "(none)",
        .examples =
            "pwd              Display current directory\n"
            "                 Example output: /home/user/projects"
    },

    /* echo - Print Arguments */
    {
        .name = "echo",
        .summary = "Print arguments to standard output",
        .usage = "echo [args...]",
        .description =
            "Prints all arguments separated by spaces, followed by a newline.\n"
            "Variable expansion is performed on arguments.\n"
            "Use quotes to preserve spaces and special characters.",
        .options =
            "args...          One or more arguments to print\n"
            "                 Variables like $VAR are expanded",
        .examples =
            "echo Hello World        Print multiple words\n"
            "echo \"Path: $PATH\"      Print with variable expansion\n"
            "echo test               Print single word"
    },

    /* export - Set Environment Variable */
    {
        .name = "export",
        .summary = "Set or export environment variables",
        .usage = "export NAME=value",
        .description =
            "Sets an environment variable that will be available to child processes.\n"
            "Variables set with export are inherited by programs launched from the shell.\n"
            "Use 'env' command to view all exported variables.",
        .options =
            "NAME=value       Variable assignment (name and value)\n"
            "                 No spaces around '=' sign",
        .examples =
            "export PATH=/usr/bin    Set PATH variable\n"
            "export USER=john        Set USER variable\n"
            "export DEBUG=1          Enable debug mode"
    },

    /* set - Set Shell Variable */
    {
        .name = "set",
        .summary = "Set shell variables",
        .usage = "set NAME=value",
        .description =
            "Sets a shell variable (local to the shell, not exported to child processes).\n"
            "Unlike 'export', these variables are only available within the shell.\n"
            "Use 'env' to see exported variables.",
        .options =
            "NAME=value       Variable assignment (name and value)\n"
            "                 No spaces around '=' sign",
        .examples =
            "set counter=0           Set local variable\n"
            "set temp=test           Set temporary variable\n"
            "set x=hello             Set variable x"
    },

    /* unset - Remove Variable */
    {
        .name = "unset",
        .summary = "Remove environment or shell variables",
        .usage = "unset NAME",
        .description =
            "Removes the specified environment or shell variable.\n"
            "The variable will no longer be defined in the shell or its child processes.",
        .options =
            "NAME             Name of variable to remove",
        .examples =
            "unset PATH              Remove PATH variable\n"
            "unset DEBUG             Remove DEBUG variable\n"
            "unset temp              Remove temp variable"
    },

    /* env - Display Environment */
    {
        .name = "env",
        .summary = "Display all environment variables",
        .usage = "env",
        .description =
            "Prints all environment variables and their values.\n"
            "Shows variables set with 'export' that are available to child processes.\n"
            "Output format: NAME=value (one per line)",
        .options = "(none)",
        .examples =
            "env                     List all environment variables\n"
            "env | grep PATH         Find PATH-related variables\n"
            "env | wc -l             Count environment variables"
    },

    /* help - Show Help */
    {
        .name = "help",
        .summary = "Display help information",
        .usage = "help [command]",
        .description =
            "Shows help information for built-in commands.\n"
            "If a command name is provided, shows detailed help for that command.\n"
            "If no command is specified, shows general help and lists all commands.",
        .options =
            "command          Name of command to get help for (optional)\n"
            "                 If omitted, shows general help",
        .examples =
            "help                    Show general help\n"
            "help cd                 Show help for cd command\n"
            "help echo               Show help for echo command"
    },

    /* version - Show Version */
    {
        .name = "version",
        .summary = "Display shell version information",
        .usage = "version",
        .description =
            "Displays the version number and build information of the shell.\n"
            "Useful for debugging and compatibility checks.",
        .options = "(none)",
        .examples =
            "version                 Display version information"
    },

    /* history - Command History */
    {
        .name = "history",
        .summary = "Display command history",
        .usage = "history",
        .description =
            "Shows a numbered list of previously executed commands.\n"
            "Commands are saved across shell sessions.\n"
            "Use UP/DOWN arrow keys to navigate history interactively.",
        .options = "(none)",
        .examples =
            "history                 Show all command history\n"
            "history | grep cd       Find cd commands in history\n"
            "history | tail -20      Show last 20 commands"
    },

    /* edi - File Editor */
    {
        .name = "edi",
        .summary = "Interactive line-based file editor",
        .usage = "edi <filename>",
        .description =
            "Opens an interactive editor for viewing and modifying text files.\n"
            "Provides line-by-line editing capabilities.\n"
            "Simpler than full-screen editors like vi or nano.",
        .options =
            "filename         Name of file to edit (required)",
        .examples =
            "edi config.txt          Edit config.txt file\n"
            "edi notes.txt           Edit notes.txt file"
    },

    /* apt - Package Manager */
    {
        .name = "apt",
        .summary = "Shell package manager",
        .usage = "apt <subcommand> [args...]",
        .description =
            "Manages software packages in the shell environment.\n"
            "Provides package installation, removal, and search capabilities.\n"
            "\n"
            "Subcommands:\n"
            "  init        Initialize package repository\n"
            "  update      Update package index\n"
            "  list        List available packages\n"
            "  search      Search for packages\n"
            "  show        Show package details\n"
            "  install     Install a package\n"
            "  remove      Remove a package\n"
            "  depends     Show package dependencies",
        .options =
            "subcommand       Package management operation (required)\n"
            "args...          Additional arguments for subcommand\n"
            "\n"
            "Use 'apt <subcommand> --help' for subcommand-specific help",
        .examples =
            "apt init                Initialize repository\n"
            "apt list                List all packages\n"
            "apt search math         Search for math-related packages\n"
            "apt install hello       Install hello package\n"
            "apt remove hello        Remove hello package"
    },

    /* jobs - Job Control */
    {
        .name = "jobs",
        .summary = "List background jobs",
        .usage = "jobs",
        .description =
            "Displays all background jobs with their job IDs, PIDs, and status.\n"
            "Shows running, stopped, and completed jobs.\n"
            "Use 'fg' and 'bg' commands to control jobs.",
        .options = "(none)",
        .examples =
            "jobs                    List all background jobs\n"
            "sleep 10 &              Start background job\n"
            "jobs                    See the job listed"
    },

    /* fg - Foreground Job */
    {
        .name = "fg",
        .summary = "Bring job to foreground",
        .usage = "fg [%job_id]",
        .description =
            "Brings a background job to the foreground.\n"
            "If no job ID is specified, brings the most recent job.\n"
            "The job will receive keyboard input and control terminal.",
        .options =
            "%job_id          Job ID to bring to foreground (optional)\n"
            "                 Use 'jobs' to see job IDs\n"
            "                 If omitted, uses most recent job",
        .examples =
            "fg                      Foreground most recent job\n"
            "fg %1                   Foreground job 1\n"
            "jobs                    List jobs to get IDs\n"
            "fg %2                   Foreground job 2"
    },

    /* bg - Background Job */
    {
        .name = "bg",
        .summary = "Resume job in background",
        .usage = "bg [%job_id]",
        .description =
            "Resumes a stopped job in the background.\n"
            "If no job ID is specified, resumes the most recent stopped job.\n"
            "The job will continue running without terminal control.",
        .options =
            "%job_id          Job ID to resume (optional)\n"
            "                 Use 'jobs' to see job IDs\n"
            "                 If omitted, uses most recent job",
        .examples =
            "bg                      Resume recent job in background\n"
            "bg %1                   Resume job 1 in background"
    },

    /* commands - List Commands */
    {
        .name = "commands",
        .summary = "List all available commands",
        .usage = "commands [--json]",
        .description =
            "Lists all built-in commands available in the shell.\n"
            "With --json flag, outputs structured JSON format for parsing.\n"
            "Used by AI integration system for command discovery.",
        .options =
            "--json           Output in JSON format (optional)\n"
            "                 Default: human-readable format",
        .examples =
            "commands                List all commands\n"
            "commands --json         List in JSON format\n"
            "commands | grep apt     Find apt-related commands"
    },

    /* exit - Exit Shell */
    {
        .name = "exit",
        .summary = "Exit the shell",
        .usage = "exit [status]",
        .description =
            "Exits the shell with optional exit status code.\n"
            "If no status is provided, exits with status 0 (success).\n"
            "All background jobs are terminated on exit.",
        .options =
            "status           Exit status code (optional, default: 0)\n"
            "                 0 = success, non-zero = error",
        .examples =
            "exit                    Exit with status 0\n"
            "exit 0                  Exit with success status\n"
            "exit 1                  Exit with error status"
    },

    /* Sentinel - marks end of array */
    { NULL, NULL, NULL, NULL, NULL, NULL }
};

/* ============================================================================
 * Help System Functions
 * ============================================================================ */

/**
 * get_help_entry - Retrieve help entry for a specific command
 * 
 * Searches the help database for the specified command name and returns
 * a pointer to its HelpEntry structure.
 * 
 * @param cmd_name: Name of the command to look up
 * 
 * Returns: Pointer to HelpEntry if found, NULL if command not found
 */
const HelpEntry* get_help_entry(const char *cmd_name) {
    if (cmd_name == NULL) {
        return NULL;
    }

    /* Linear search through help entries array */
    for (int i = 0; help_entries[i].name != NULL; i++) {
        if (strcmp(help_entries[i].name, cmd_name) == 0) {
            return &help_entries[i];
        }
    }

    /* Command not found in help database */
    return NULL;
}

/**
 * print_help - Display formatted help text for a command
 * 
 * Prints the help entry in a consistent, readable format with sections:
 * NAME, USAGE, DESCRIPTION, OPTIONS, and EXAMPLES.
 * 
 * @param entry: Pointer to HelpEntry to display (must not be NULL)
 */
void print_help(const HelpEntry *entry) {
    if (entry == NULL) {
        fprintf(stderr, "Error: NULL help entry\n");
        return;
    }

    /* Print NAME section */
    printf("NAME\n");
    printf("    %s - %s\n\n", entry->name, entry->summary);

    /* Print USAGE section */
    printf("USAGE\n");
    printf("    %s\n\n", entry->usage);

    /* Print DESCRIPTION section */
    printf("DESCRIPTION\n");
    /* Indent each line of description */
    const char *desc = entry->description;
    const char *line_start = desc;
    while (*desc) {
        if (*desc == '\n') {
            printf("    %.*s\n", (int)(desc - line_start), line_start);
            line_start = desc + 1;
        }
        desc++;
    }
    /* Print last line if no trailing newline */
    if (line_start < desc) {
        printf("    %.*s\n", (int)(desc - line_start), line_start);
    }
    printf("\n");

    /* Print OPTIONS section */
    printf("OPTIONS\n");
    const char *opts = entry->options;
    line_start = opts;
    while (*opts) {
        if (*opts == '\n') {
            printf("    %.*s\n", (int)(opts - line_start), line_start);
            line_start = opts + 1;
        }
        opts++;
    }
    if (line_start < opts) {
        printf("    %.*s\n", (int)(opts - line_start), line_start);
    }
    printf("\n");

    /* Print EXAMPLES section */
    printf("EXAMPLES\n");
    const char *exmp = entry->examples;
    line_start = exmp;
    while (*exmp) {
        if (*exmp == '\n') {
            printf("    %.*s\n", (int)(exmp - line_start), line_start);
            line_start = exmp + 1;
        }
        exmp++;
    }
    if (line_start < exmp) {
        printf("    %.*s\n", (int)(exmp - line_start), line_start);
    }
    printf("\n");
}

/**
 * check_help_flag - Check if --help or -h flag is present in arguments
 * 
 * Scans the argument array for the help flag (--help or -h) at any position.
 * This allows built-in commands to detect when the user is requesting help
 * instead of executing the command normally.
 * 
 * @param argc: Number of arguments (excluding NULL terminator)
 * @param argv: Argument array (NULL-terminated)
 * 
 * Returns: 1 if help flag found, 0 otherwise
 */
int check_help_flag(int argc, char **argv) {
    /* Scan all arguments for help flags */
    for (int i = 0; i < argc; i++) {
        if (argv[i] == NULL) {
            break;
        }
        
        /* Check for --help or -h */
        if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            return 1;
        }
    }

    /* No help flag found */
    return 0;
}
