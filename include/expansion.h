#ifndef EXPANSION_H
#define EXPANSION_H

#include "environment.h"

// Function to expand variables in a new allocated string
// Caller must free the returned string
char* expand_variables(const char *input, Env *env);

// Function to expand variables in-place in an existing buffer
// Safer for fixed-size buffers
void expand_variables_inplace(char *input, Env *env, size_t bufsize);

#endif // EXPANSION_H
