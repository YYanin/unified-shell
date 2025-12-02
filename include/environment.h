#ifndef ENVIRONMENT_H
#define ENVIRONMENT_H

#define MAX_VARS 100
#define VAR_NAME_MAX 64
#define VAR_VALUE_MAX 256

// Variable binding structure
typedef struct {
    char *name;
    char *value;
} Binding;

// Environment structure for variable storage
typedef struct {
    Binding bindings[MAX_VARS];
    int count;
} Env;

// Environment management functions
Env* env_new(void);
void env_free(Env *env);
char* env_get(Env *env, const char *name);
void env_set(Env *env, const char *name, const char *value);
void env_unset(Env *env, const char *name);
void env_print(Env *env);

#endif // ENVIRONMENT_H
