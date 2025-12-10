/**
 * help.h - Centralized Help System for Built-in Commands
 * 
 * This header defines the help system infrastructure for providing
 * comprehensive documentation for all built-in shell commands.
 * 
 * Features:
 *   - Structured help entries with consistent format
 *   - Support for --help flag on all built-ins
 *   - Detailed usage, description, options, and examples
 *   - Searchable help database
 */

#ifndef HELP_H
#define HELP_H

/**
 * HelpEntry - Structure representing help information for a command
 * 
 * Each built-in command has a HelpEntry that contains all documentation
 * needed for users to understand and use the command effectively.
 * 
 * Fields:
 *   - name: Command name (e.g., "cd", "echo", "apt")
 *   - summary: One-line description of what the command does
 *   - usage: Syntax showing how to invoke the command with arguments
 *   - description: Detailed explanation of command functionality
 *   - options: Description of available flags and options
 *   - examples: Practical usage examples demonstrating common cases
 */
typedef struct {
    const char *name;         /* Command name */
    const char *summary;      /* One-line summary */
    const char *usage;        /* Usage syntax */
    const char *description;  /* Detailed description */
    const char *options;      /* Options description */
    const char *examples;     /* Usage examples */
} HelpEntry;

/**
 * get_help_entry - Retrieve help entry for a specific command
 * 
 * Searches the help database for the specified command name and returns
 * a pointer to its HelpEntry structure.
 * 
 * @param cmd_name: Name of the command to look up (e.g., "cd", "pwd")
 * 
 * Returns: Pointer to HelpEntry if found, NULL if command not found
 * 
 * Example:
 *   const HelpEntry *help = get_help_entry("cd");
 *   if (help) {
 *       printf("%s - %s\n", help->name, help->summary);
 *   }
 */
const HelpEntry* get_help_entry(const char *cmd_name);

/**
 * print_help - Display formatted help text for a command
 * 
 * Prints the help entry in a consistent, readable format with sections:
 * NAME, USAGE, DESCRIPTION, OPTIONS, and EXAMPLES.
 * 
 * @param entry: Pointer to HelpEntry to display (must not be NULL)
 * 
 * Format:
 *   NAME
 *       command - summary
 *   
 *   USAGE
 *       usage
 *   
 *   DESCRIPTION
 *       description
 *   
 *   OPTIONS
 *       options
 *   
 *   EXAMPLES
 *       examples
 * 
 * Example:
 *   const HelpEntry *help = get_help_entry("pwd");
 *   if (help) {
 *       print_help(help);
 *   }
 */
void print_help(const HelpEntry *entry);

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
 * 
 * Example:
 *   if (check_help_flag(argc, argv)) {
 *       const HelpEntry *help = get_help_entry("mycommand");
 *       if (help) {
 *           print_help(help);
 *           return 0;
 *       }
 *   }
 *   // Normal command execution continues...
 */
int check_help_flag(int argc, char **argv);

#endif /* HELP_H */
