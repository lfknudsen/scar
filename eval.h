#ifndef EVAL_H
#define EVAL_H

#include <stdio.h>

#include "scar.h"

struct Val start_eval(struct vtable_index* vtable, struct ftable_index* ftable, struct state* state);

#endif