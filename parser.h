#ifndef PARSER_H
#define PARSER_H

#include <stdio.h>

#include "scar.h"

struct node_tree* parse(FILE *f, struct state* st);

#endif