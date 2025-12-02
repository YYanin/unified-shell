#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "environment.h"

/**
 * env_new - Allocate and initialize an empty environment
 * Returns: Pointer to newly allocated Env struct
 */
Env* env_new(void) {
    Env *env;
    int i;
    
    env = (Env *)malloc(sizeof(Env));
    if (!env) {
        fprintf(stderr, "env_new: malloc failed\n");
        exit(1);
    }
    
    env->count = 0;
    
    // Initialize all bindings to NULL for safety
    for (i = 0; i < MAX_VARS; i++) {
        env->bindings[i].name = NULL;
        env->bindings[i].value = NULL;
    }
    
    return env;
}

/**
 * env_free - Free environment and all allocated strings
 * @env: Environment to free
 */
void env_free(Env *env) {
    int i;
    
    if (!env) {
        return;
    }
    
    // Free all name and value strings
    for (i = 0; i < env->count; i++) {
        if (env->bindings[i].name) {
            free(env->bindings[i].name);
        }
        if (env->bindings[i].value) {
            free(env->bindings[i].value);
        }
    }
    
    free(env);
}

/**
 * env_get - Get the value of a variable from the environment
 * @env: Environment to search
 * @name: Variable name to look up
 * Returns: Variable value (string) or NULL if not found
 * 
 * First checks internal environment, then falls back to system getenv()
 */
char* env_get(Env *env, const char *name) {
    int i;
    
    if (!env || !name) {
        return NULL;
    }
    
    // Search internal environment first
    for (i = 0; i < env->count; i++) {
        if (env->bindings[i].name && strcmp(env->bindings[i].name, name) == 0) {
            return env->bindings[i].value;
        }
    }
    
    // Fall back to system environment
    return getenv(name);
}

/**
 * env_set - Set a variable in the environment
 * @env: Environment to modify
 * @name: Variable name
 * @value: Variable value
 * 
 * Updates existing variable or creates new one
 */
void env_set(Env *env, const char *name, const char *value) {
    int i;
    
    if (!env || !name || !value) {
        return;
    }
    
    // Check if variable already exists - update it
    for (i = 0; i < env->count; i++) {
        if (env->bindings[i].name && strcmp(env->bindings[i].name, name) == 0) {
            // Free old value and set new one
            free(env->bindings[i].value);
            env->bindings[i].value = strdup(value);
            if (!env->bindings[i].value) {
                fprintf(stderr, "env_set: strdup failed\n");
                exit(1);
            }
            return;
        }
    }
    
    // Variable doesn't exist - create new binding
    if (env->count >= MAX_VARS) {
        fprintf(stderr, "env_set: maximum number of variables (%d) reached\n", MAX_VARS);
        return;
    }
    
    env->bindings[env->count].name = strdup(name);
    env->bindings[env->count].value = strdup(value);
    
    if (!env->bindings[env->count].name || !env->bindings[env->count].value) {
        fprintf(stderr, "env_set: strdup failed\n");
        exit(1);
    }
    
    env->count++;
}

/**
 * env_unset - Remove a variable from the environment
 * @env: Environment to modify
 * @name: Variable name to remove
 */
void env_unset(Env *env, const char *name) {
    int i, j;
    
    if (!env || !name) {
        return;
    }
    
    // Find the variable
    for (i = 0; i < env->count; i++) {
        if (env->bindings[i].name && strcmp(env->bindings[i].name, name) == 0) {
            // Free the strings
            free(env->bindings[i].name);
            free(env->bindings[i].value);
            
            // Shift remaining bindings down
            for (j = i; j < env->count - 1; j++) {
                env->bindings[j] = env->bindings[j + 1];
            }
            
            // Clear the last binding
            env->bindings[env->count - 1].name = NULL;
            env->bindings[env->count - 1].value = NULL;
            
            env->count--;
            return;
        }
    }
}

/**
 * env_print - Print all variables in the environment (for debugging)
 * @env: Environment to print
 */
void env_print(Env *env) {
    int i;
    
    if (!env) {
        return;
    }
    
    printf("=== Environment (%d variables) ===\n", env->count);
    for (i = 0; i < env->count; i++) {
        if (env->bindings[i].name && env->bindings[i].value) {
            printf("%s=%s\n", env->bindings[i].name, env->bindings[i].value);
        }
    }
    printf("=================================\n");
}
