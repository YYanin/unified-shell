#include "conditional.h"
#include "executor.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/**
 * Global variable to track last command exit status
 */
int last_exit_status = 0;

/**
 * Helper: Skip whitespace
 */
static char* skip_whitespace(char *str) {
    while (*str && isspace(*str)) {
        str++;
    }
    return str;
}

/**
 * Helper: Find keyword in string (case-sensitive, word boundary)
 */
static char* find_keyword(char *str, const char *keyword) {
    size_t kw_len = strlen(keyword);
    char *ptr = str;
    
    while (*ptr) {
        // Check if keyword matches
        if (strncmp(ptr, keyword, kw_len) == 0) {
            // Check word boundary (space or end of string after keyword)
            char after = ptr[kw_len];
            if (after == '\0' || isspace(after)) {
                return ptr;
            }
        }
        ptr++;
    }
    return NULL;
}

/**
 * Parse a conditional statement
 * Syntax: if <condition> then <commands> [else <commands>] fi
 */
int parse_conditional(char *line, char **condition, char **then_block, char **else_block) {
    if (line == NULL || condition == NULL || then_block == NULL || else_block == NULL) {
        return -1;
    }

    *condition = NULL;
    *then_block = NULL;
    *else_block = NULL;

    // Check if line starts with "if"
    char *ptr = skip_whitespace(line);
    if (strncmp(ptr, "if", 2) != 0 || (!isspace(ptr[2]) && ptr[2] != '\0')) {
        return 0;  // Not a conditional
    }
    ptr += 2;
    ptr = skip_whitespace(ptr);

    // Find "then" keyword
    char *then_ptr = find_keyword(ptr, "then");
    if (then_ptr == NULL) {
        fprintf(stderr, "ushell: syntax error: expected 'then'\n");
        return -1;
    }

    // Extract condition (between "if" and "then")
    size_t cond_len = then_ptr - ptr;
    *condition = malloc(cond_len + 1);
    if (*condition == NULL) {
        perror("malloc");
        return -1;
    }
    strncpy(*condition, ptr, cond_len);
    (*condition)[cond_len] = '\0';
    
    // Trim trailing whitespace from condition
    char *cond_end = *condition + strlen(*condition) - 1;
    while (cond_end >= *condition && isspace(*cond_end)) {
        *cond_end = '\0';
        cond_end--;
    }

    // Move past "then"
    ptr = then_ptr + 4;
    ptr = skip_whitespace(ptr);

    // Find "else" or "fi"
    char *else_ptr = find_keyword(ptr, "else");
    char *fi_ptr = find_keyword(ptr, "fi");

    if (fi_ptr == NULL) {
        fprintf(stderr, "ushell: syntax error: expected 'fi'\n");
        free(*condition);
        *condition = NULL;
        return -1;
    }

    if (else_ptr != NULL && else_ptr < fi_ptr) {
        // Has else block
        
        // Extract then block (between "then" and "else")
        size_t then_len = else_ptr - ptr;
        *then_block = malloc(then_len + 1);
        if (*then_block == NULL) {
            perror("malloc");
            free(*condition);
            *condition = NULL;
            return -1;
        }
        strncpy(*then_block, ptr, then_len);
        (*then_block)[then_len] = '\0';
        
        // Trim trailing whitespace
        char *then_end = *then_block + strlen(*then_block) - 1;
        while (then_end >= *then_block && isspace(*then_end)) {
            *then_end = '\0';
            then_end--;
        }

        // Move past "else"
        ptr = else_ptr + 4;
        ptr = skip_whitespace(ptr);

        // Extract else block (between "else" and "fi")
        size_t else_len = fi_ptr - ptr;
        *else_block = malloc(else_len + 1);
        if (*else_block == NULL) {
            perror("malloc");
            free(*condition);
            free(*then_block);
            *condition = NULL;
            *then_block = NULL;
            return -1;
        }
        strncpy(*else_block, ptr, else_len);
        (*else_block)[else_len] = '\0';
        
        // Trim trailing whitespace
        char *else_end = *else_block + strlen(*else_block) - 1;
        while (else_end >= *else_block && isspace(*else_end)) {
            *else_end = '\0';
            else_end--;
        }
    } else {
        // No else block
        
        // Extract then block (between "then" and "fi")
        size_t then_len = fi_ptr - ptr;
        *then_block = malloc(then_len + 1);
        if (*then_block == NULL) {
            perror("malloc");
            free(*condition);
            *condition = NULL;
            return -1;
        }
        strncpy(*then_block, ptr, then_len);
        (*then_block)[then_len] = '\0';
        
        // Trim trailing whitespace
        char *then_end = *then_block + strlen(*then_block) - 1;
        while (then_end >= *then_block && isspace(*then_end)) {
            *then_end = '\0';
            then_end--;
        }

        *else_block = NULL;
    }

    return 1;  // Successfully parsed conditional
}

/**
 * Execute a conditional statement
 */
int execute_conditional(char *condition, char *then_block, char *else_block, Env *env) {
    if (condition == NULL || then_block == NULL || env == NULL) {
        return -1;
    }

    // Parse and execute condition command
    Command *cond_commands = NULL;
    int cond_count = 0;
    
    if (parse_pipeline(condition, &cond_commands, &cond_count) < 0) {
        fprintf(stderr, "ushell: failed to parse condition\n");
        return -1;
    }

    int cond_status = -1;
    if (cond_count > 0 && cond_commands != NULL) {
        cond_status = execute_pipeline(cond_commands, cond_count, env);
        free_pipeline(cond_commands, cond_count);
    }

    // Update global last exit status
    last_exit_status = cond_status;

    // Execute appropriate block based on condition result
    char *block_to_execute = NULL;
    if (cond_status == 0) {
        // Condition succeeded - execute then block
        block_to_execute = then_block;
    } else {
        // Condition failed - execute else block (if present)
        if (else_block != NULL && strlen(else_block) > 0) {
            block_to_execute = else_block;
        } else {
            // No else block, return condition status
            return cond_status;
        }
    }

    // Parse and execute the selected block
    Command *block_commands = NULL;
    int block_count = 0;
    
    if (parse_pipeline(block_to_execute, &block_commands, &block_count) < 0) {
        fprintf(stderr, "ushell: failed to parse command block\n");
        return -1;
    }

    int block_status = 0;
    if (block_count > 0 && block_commands != NULL) {
        block_status = execute_pipeline(block_commands, block_count, env);
        free_pipeline(block_commands, block_count);
    }

    // Update global last exit status
    last_exit_status = block_status;

    return block_status;
}
