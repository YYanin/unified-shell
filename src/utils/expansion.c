#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "expansion.h"
#include "arithmetic.h"

/**
 * expand_variables - Expand $var and $((...)) tokens in a string
 * @input: Input string with potential $var tokens
 * @env: Environment for variable lookup
 * 
 * Returns: Newly allocated string with variables expanded (caller must free)
 * 
 * Supports:
 * - $VAR syntax
 * - ${VAR} syntax
 * - $((arithmetic)) syntax
 * - Undefined variables are replaced with empty string
 */
char* expand_variables(const char *input, Env *env) {
    char *result;
    char *output;
    const char *ptr;
    char var_name[256];
    int var_idx;
    char *var_value;
    size_t result_size;
    size_t result_len;
    
    if (!input) {
        return NULL;
    }
    
    // Allocate initial result buffer (will grow if needed)
    result_size = strlen(input) * 2 + 256;  // Generous initial size
    result = (char *)malloc(result_size);
    if (!result) {
        fprintf(stderr, "expand_variables: malloc failed\n");
        exit(1);
    }
    
    output = result;
    result_len = 0;
    ptr = input;
    
    while (*ptr) {
        if (*ptr == '$') {
            ptr++;  // Skip $
            
            // Check for $((arithmetic)) syntax
            if (*ptr == '(' && *(ptr + 1) == '(') {
                ptr += 2;  // Skip ((
                
                // Extract arithmetic expression (up to ))
                char arith_expr[512];
                int arith_idx = 0;
                int paren_depth = 0;  // Track nested parens within expression
                
                while (*ptr) {
                    // Check for )) ending
                    if (*ptr == ')' && *(ptr + 1) == ')' && paren_depth == 0) {
                        ptr += 2;  // Skip ))
                        break;
                    }
                    
                    // Track nested parentheses
                    if (*ptr == '(') {
                        paren_depth++;
                    } else if (*ptr == ')') {
                        paren_depth--;
                    }
                    
                    // Copy character to expression
                    if (arith_idx < 511) {
                        arith_expr[arith_idx++] = *ptr;
                    }
                    ptr++;
                }
                arith_expr[arith_idx] = '\0';
                
                // Evaluate arithmetic expression
                int arith_result = eval_arithmetic(arith_expr, env);
                
                // Convert result to string
                char result_str[32];
                snprintf(result_str, sizeof(result_str), "%d", arith_result);
                size_t result_str_len = strlen(result_str);
                
                // Ensure we have enough space
                while (result_len + result_str_len + 1 > result_size) {
                    result_size *= 2;
                    result = (char *)realloc(result, result_size);
                    if (!result) {
                        fprintf(stderr, "expand_variables: realloc failed\n");
                        exit(1);
                    }
                    output = result + result_len;
                }
                
                strcpy(output, result_str);
                output += result_str_len;
                result_len += result_str_len;
                
                continue;
            }
            
            // Check for ${VAR} syntax
            int braced = 0;
            if (*ptr == '{') {
                braced = 1;
                ptr++;
            }
            
            // Extract variable name
            var_idx = 0;
            while (*ptr && (isalnum(*ptr) || *ptr == '_')) {
                if (var_idx < 255) {
                    var_name[var_idx++] = *ptr;
                }
                ptr++;
            }
            var_name[var_idx] = '\0';
            
            // Skip closing brace if present
            if (braced && *ptr == '}') {
                ptr++;
            }
            
            // Get variable value
            var_value = env_get(env, var_name);
            
            // Append value to result (or empty string if undefined)
            if (var_value) {
                size_t val_len = strlen(var_value);
                
                // Ensure we have enough space
                while (result_len + val_len + 1 > result_size) {
                    result_size *= 2;
                    result = (char *)realloc(result, result_size);
                    if (!result) {
                        fprintf(stderr, "expand_variables: realloc failed\n");
                        exit(1);
                    }
                    output = result + result_len;
                }
                
                strcpy(output, var_value);
                output += val_len;
                result_len += val_len;
            }
        } else {
            // Regular character - copy it
            *output++ = *ptr++;
            result_len++;
            
            // Ensure we have space
            if (result_len + 1 >= result_size) {
                result_size *= 2;
                result = (char *)realloc(result, result_size);
                if (!result) {
                    fprintf(stderr, "expand_variables: realloc failed\n");
                    exit(1);
                }
                output = result + result_len;
            }
        }
    }
    
    *output = '\0';
    return result;
}

/**
 * expand_variables_inplace - Expand variables in a fixed-size buffer
 * @input: Buffer containing input string (modified in place)
 * @env: Environment for variable lookup
 * @bufsize: Size of the buffer
 * 
 * Performs variable expansion directly in the buffer.
 * Truncates if result would exceed buffer size.
 */
void expand_variables_inplace(char *input, Env *env, size_t bufsize) {
    char *expanded;
    
    if (!input || bufsize == 0) {
        return;
    }
    
    // Use the dynamic version
    expanded = expand_variables(input, env);
    
    if (expanded) {
        // Copy back to input buffer, truncating if necessary
        strncpy(input, expanded, bufsize - 1);
        input[bufsize - 1] = '\0';
        free(expanded);
    }
}
