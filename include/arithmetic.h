#ifndef ARITHMETIC_H
#define ARITHMETIC_H

#include "environment.h"

/**
 * Evaluate an arithmetic expression
 * Supports: +, -, *, /, %, parentheses
 * Supports variables ($var or just var name)
 * @param expr Expression to evaluate
 * @param env Environment for variable lookup
 * @return Integer result, or 0 on error
 */
int eval_arithmetic(const char *expr, Env *env);

#endif /* ARITHMETIC_H */
