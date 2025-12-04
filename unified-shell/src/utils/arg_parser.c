#include <stdio.h>
#include <stdlib.h>
#include "argtable_defs.h"

void* argtable_init_basic(void) {
    // Basic initialization placeholder
    return NULL;
}

void argtable_free(void **argtable, int count) {
    if (argtable != NULL) {
        arg_freetable(argtable, count);
    }
}

int argtable_parse(int argc, char **argv, void **argtable, int count) {
    if (argtable == NULL || argc == 0 || argv == NULL) {
        return -1;
    }
    
    int nerrors = arg_parse(argc, argv, argtable);
    
    if (nerrors > 0) {
        arg_print_errors(stderr, argtable[count-1], argv[0]);
        return -1;
    }
    
    return 0;
}
