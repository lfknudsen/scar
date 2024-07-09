#ifndef PARSER_H
#define PARSER_H

#include <stdio.h>

#include "scar.h"

struct tree* parse(FILE *f, struct token_index *ti);

#endif