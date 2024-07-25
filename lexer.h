#ifndef LEXER_H
#define LEXER_H

#include "scar.h"

int lex(FILE *f, FILE *output, struct token_index *ti, int out);

#endif