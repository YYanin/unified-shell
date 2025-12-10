#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "environment.h"

/**
 * env_new - Allocate and initialize an empty environment
 * Returns: Pointer to newly allocated Env struct
 * 
 * Thread safety: Initializes mutex for thread-safe environment access
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
    
    // Initialize mutex for thread-safe access
    if (pthread_mutex_init(&env->env_mutex, NULL) != 0) {
        fprintf(stderr, "env_new: mutex initialization failed\n");
        free(env);
        exit(1);
    }
    
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
 * 
 * Thread safety: Destroys mutex before freeing
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
    
    // Destroy mutex before freeing environment
    pthread_mutex_destroy(&env->env_mutex);
    
    free(env);
}

/**
 * env_get - Get the value of a variable from the environment
 * @env: Environment to search
 * @name: Variable name to look up
 * Returns: Variable value (string) or NULL if not found
 * 
 * First checks internal environment, then falls back to system getenv()
 * Thread safety: Uses mutex to protect environment access
 */
char* env_get(Env *env, const char *name) {
    int i;
    char *result = NULL;
    
    if (!env || !name) {
        return NULL;
    }
    
    // Lock mutex for thread-safe access
    pthread_mutex_lock(&env->env_mutex);
    
    // Search internal environment first
    for (i = 0; i < env->count; i++) {
        if (env->bindings[i].name && strcmp(env->bindings[i].name, name) == 0) {
            result = env->bindings[i].value;
            pthread_mutex_unlock(&env->env_mutex);
            return result;
        }
    }
    
    // Unlock before calling getenv (which may use its own locks)
    pthread_mutex_unlock(&env->env_mutex);
    
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
 * Thread safety: Uses mutex to protect environment modifications
 */
void env_set(Env *env, const char *name, const char *value) {
    int i;
    
    if (!env || !name || !value) {
        return;
    }
    
    // Lock mutex for thread-safe access
    pthread_mutex_lock(&env->env_mutex);
    
    // Check if variable already exists - update it
    for (i = 0; i < env->count; i++) {
        if (env->bindings[i].name && strcmp(env->bindings[i].name, name) == 0) {
            // Free old value and set new one
            free(env->bindings[i].value);
            env->bindings[i].value = strdup(value);
            if (!env->bindings[i].value) {
                fprintf(stderr, "env_set: strdup failed\n");
                pthread_mutex_unlock(&env->env_mutex);
                exit(1);
            }
            pthread_mutex_unlock(&env->env_mutex);
            return;
        }
    }
    
    // Variable doesn't exist - create new binding
    if (env->count >= MAX_VARS) {
        fprintf(stderr, "env_set: maximum number of variables (%d) reached\n", MAX_VARS);
        pthread_mutex_unlock(&env->env_mutex);
        return;
    }
    
    env->bindings[env->count].name = strdup(name);
    env->bindings[env->count].value = strdup(value);
    
    if (!env->bindings[env->count].name || !env->bindings[env->count].value) {
        fprintf(stderr, "env_set: strdup failed\n");
        pthread_mutex_unlock(&env->env_mutex);
        exit(1);
    }
    
    env->count++;
    
    pthread_mutex_unlock(&env->env_mutex);
}

/**
 * env_unset - Remove a variable from the environment
 * @env: Environment to modify
 * @name: Variable name to remove
 * 
 * Thread safety: Uses mutex to protect environment modifications
 */
void env_unset(Env *env, const char *name) {
    int i, j;
    
    if (!env || !name) {
        return;
    }
    
    // Lock mutex for thread-safe access
    pthread_mutex_lock(&env->env_mutex);
    
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
            pthread_mutex_unlock(&env->env_mutex);
            return;
        }
    }
    
    pthread_mutex_unlock(&env->env_mutex);
}

/**
 * env_print - Print all variables in the environment (for debugging)
 * @env: Environment to print
 * 
 * Thread safety: Uses mutex to protect environment access during printing
 */
void env_print(Env *env) {
    int i;
    
    if (!env) {
        return;
    }
    
    // Lock mutex for thread-safe access
    pthread_mutex_lock(&env->env_mutex);
    
    printf("=== Environment (%d variables) ===\n", env->count);
    for (i = 0; i < env->count; i++) {
        if (env->bindings[i].name && env->bindings[i].value) {
            printf("%s=%s\n", env->bindings[i].name, env->bindings[i].value);
        }
    }
    printf("=================================\n");
    
    pthread_mutex_unlock(&env->env_mutex);
}
