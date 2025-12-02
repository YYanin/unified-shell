#ifndef PARSER_WRAPPER_H
#define PARSER_WRAPPER_H

#include <stdio.h>
#include "../parser/Absyn.h"
#include "../parser/Parser.h"
#include "../parser/Printer.h"

// Parser functions from BNFC-generated code
// pProgram parses input from a FILE* and returns the AST root
Program pProgram(FILE *inp);

// showProgram prints the AST to stdout (for debugging)
void showProgram(Program p);

#endif // PARSER_WRAPPER_H
