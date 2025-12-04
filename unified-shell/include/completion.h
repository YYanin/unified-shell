#ifndef COMPLETION_H
#define COMPLETION_H

#include "environment.h"

// Initialize completion system
void completion_init(Env *env);

// Generate completions for a partial command/filename
// Returns NULL-terminated array of matches (caller must free)
char** completion_generate(const char *text, int *count);

// Free completion results
void completion_free(char **completions);

// Get list of available commands (built-ins + PATH)
char** completion_get_commands(int *count);

// Get list of files in current directory matching prefix
char** completion_get_files(const char *prefix, int *count);

// Get list of variables matching prefix
char** completion_get_variables(const char *prefix, int *count);

#endif // COMPLETION_H
