#ifndef EVAL_H
#define EVAL_H

#include <stdio.h>

#include "scar.h"

int start_eval(struct tree* n_tree, int n_index, struct token_index* ti,
    FILE* output, struct ivtable_index* vtable, struct ftable_index* ftable, int out);

#endif