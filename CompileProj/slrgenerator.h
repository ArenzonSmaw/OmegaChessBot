#ifndef GENERATOR_H
#define GENERATOR_H

#include <stdlib.h>
#include "parser.h"

void alloc_tables(int*** ACTION, int*** GOTO);
/*
		GETS: pointers to action and goto table
		DOES: allocates memory for both tables by number of current states
		RETS: allocated tables via pointers
*/

int generate(items_arr, int, int*** GOTO, int*** ACTION);
/*
		GETS: array of items (rule+position), rules array size, pointer to goto and action tables
		DOES: generates and applies the goto and action tables based on the items array
		RETS: generated tables via pointers
*/

#endif
#ifndef GENERATOR_H
#define GENERATOR_H

#include <stdlib.h>
#include "parser.h"

void alloc_tables(int*** ACTION, int*** GOTO);
/*
		GETS: pointers to action and goto table
		DOES: allocates memory for both tables by number of current states
		RETS: allocated tables via pointers
*/

int generate(items_arr, int, int*** GOTO, int*** ACTION);
/*
		GETS: array of items (rule+position), rules array size, pointer to goto and action tables
		DOES: generates and applies the goto and action tables based on the items array
		RETS: generated tables via pointers
*/

#endif
