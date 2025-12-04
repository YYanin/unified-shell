#include "arithmetic.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/**
 * Parser state for arithmetic evaluation
 */
typedef struct {
    const char *expr;
    int pos;
    Env *env;
} Parser;

/**
 * Forward declarations
 */
static int parse_expr(Parser *p);
static int parse_term(Parser *p);
static int parse_factor(Parser *p);

/**
 * Skip whitespace
 */
static void skip_whitespace(Parser *p) {
    while (p->expr[p->pos] && isspace(p->expr[p->pos])) {
        p->pos++;
    }
}

/**
 * Parse a number
 */
static int parse_number(Parser *p) {
    int value = 0;
    int negative = 0;
    
    skip_whitespace(p);
    
    if (p->expr[p->pos] == '-') {
        negative = 1;
        p->pos++;
    } else if (p->expr[p->pos] == '+') {
        p->pos++;
    }
    
    if (!isdigit(p->expr[p->pos])) {
        return 0;
    }
    
    while (isdigit(p->expr[p->pos])) {
        value = value * 10 + (p->expr[p->pos] - '0');
        p->pos++;
    }
    
    return negative ? -value : value;
}

/**
 * Parse a variable name and get its value
 */
static int parse_variable(Parser *p) {
    skip_whitespace(p);
    
    // Skip optional $
    if (p->expr[p->pos] == '$') {
        p->pos++;
    }
    
    // Extract variable name
    char varname[256];
    int i = 0;
    
    while (p->expr[p->pos] && (isalnum(p->expr[p->pos]) || p->expr[p->pos] == '_')) {
        if (i < 255) {
            varname[i++] = p->expr[p->pos];
        }
        p->pos++;
    }
    varname[i] = '\0';
    
    if (i == 0) {
        fprintf(stderr, "arithmetic: expected variable name\n");
        return 0;
    }
    
    // Look up variable value
    const char *value = env_get(p->env, varname);
    if (value == NULL) {
        fprintf(stderr, "arithmetic: undefined variable: %s\n", varname);
        return 0;
    }
    
    return atoi(value);
}

/**
 * Parse a factor: number, variable, or (expression)
 */
static int parse_factor(Parser *p) {
    skip_whitespace(p);
    
    // Check for parentheses
    if (p->expr[p->pos] == '(') {
        p->pos++;
        int value = parse_expr(p);
        skip_whitespace(p);
        if (p->expr[p->pos] == ')') {
            p->pos++;
        } else {
            fprintf(stderr, "arithmetic: expected ')'\n");
        }
        return value;
    }
    
    // Check for variable ($ or letter)
    if (p->expr[p->pos] == '$' || isalpha(p->expr[p->pos]) || p->expr[p->pos] == '_') {
        return parse_variable(p);
    }
    
    // Must be a number
    return parse_number(p);
}

/**
 * Parse a term: factor [* / % factor]*
 */
static int parse_term(Parser *p) {
    int left = parse_factor(p);
    
    while (1) {
        skip_whitespace(p);
        char op = p->expr[p->pos];
        
        if (op == '*') {
            p->pos++;
            int right = parse_factor(p);
            left = left * right;
        } else if (op == '/') {
            p->pos++;
            int right = parse_factor(p);
            if (right == 0) {
                fprintf(stderr, "arithmetic: division by zero\n");
                return 0;
            }
            left = left / right;
        } else if (op == '%') {
            p->pos++;
            int right = parse_factor(p);
            if (right == 0) {
                fprintf(stderr, "arithmetic: modulo by zero\n");
                return 0;
            }
            left = left % right;
        } else {
            break;
        }
    }
    
    return left;
}

/**
 * Parse an expression: term [+ - term]*
 */
static int parse_expr(Parser *p) {
    int left = parse_term(p);
    
    while (1) {
        skip_whitespace(p);
        char op = p->expr[p->pos];
        
        if (op == '+') {
            p->pos++;
            int right = parse_term(p);
            left = left + right;
        } else if (op == '-') {
            p->pos++;
            int right = parse_term(p);
            left = left - right;
        } else {
            break;
        }
    }
    
    return left;
}

/**
 * Evaluate an arithmetic expression
 */
int eval_arithmetic(const char *expr, Env *env) {
    if (expr == NULL || env == NULL) {
        return 0;
    }
    
    Parser parser;
    parser.expr = expr;
    parser.pos = 0;
    parser.env = env;
    
    int result = parse_expr(&parser);
    
    skip_whitespace(&parser);
    if (parser.expr[parser.pos] != '\0') {
        fprintf(stderr, "arithmetic: unexpected character at position %d: '%c'\n", 
                parser.pos, parser.expr[parser.pos]);
    }
    
    return result;
}
