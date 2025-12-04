#ifndef BUILTINS_H
#define BUILTINS_H

#include "environment.h"

/**
 * Built-in command function type
 * @param argv NULL-terminated argument array (argv[0] is command name)
 * @param env Shell environment
 * @return Exit status (0 for success, non-zero for failure)
 */
typedef int (*builtin_func)(char **argv, Env *env);

/**
 * Built-in command structure
 */
typedef struct {
    const char *name;
    builtin_func func;
} Builtin;

/**
 * Find a built-in command by name
 * @param name Command name to search for
 * @return Function pointer if found, NULL otherwise
 */
builtin_func find_builtin(const char *name);

/**
 * Built-in command implementations
 */
int builtin_cd(char **argv, Env *env);
int builtin_pwd(char **argv, Env *env);
int builtin_echo(char **argv, Env *env);
int builtin_export(char **argv, Env *env);
int builtin_exit(char **argv, Env *env);
int builtin_set(char **argv, Env *env);
int builtin_unset(char **argv, Env *env);
int builtin_env(char **argv, Env *env);
int builtin_help(char **argv, Env *env);
int builtin_version(char **argv, Env *env);
int builtin_history(char **argv, Env *env);
int builtin_edi(char **argv, Env *env);
int builtin_apt(char **argv, Env *env);
int builtin_jobs(char **argv, Env *env);
int builtin_fg(char **argv, Env *env);
int builtin_bg(char **argv, Env *env);
int builtin_commands(char **argv, Env *env);

#endif /* BUILTINS_H */
