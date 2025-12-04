#ifndef ARGTABLE_DEFS_H
#define ARGTABLE_DEFS_H

#include "argtable3.h"

// Wrapper functions for argument parsing in shell

// Initialize argtable for basic shell command parsing
void* argtable_init_basic(void);

// Free argtable resources
void argtable_free(void **argtable, int count);

// Parse command arguments using argtable3
int argtable_parse(int argc, char **argv, void **argtable, int count);

#endif // ARGTABLE_DEFS_H
