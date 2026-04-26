#ifndef GENERATOR_H
#define GENERATOR_H

#include <stdlib.h>
#include "parser.h"

void alloc_tables(int*** ACTION, int*** GOTO);
int generate(items_arr, int, int*** GOTO, int*** ACTION);

#endif