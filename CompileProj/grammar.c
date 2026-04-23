#include "grammar.h"
//
//typedef enum {
//	ID,
//	LITERAL,
//	TYPE,
//	FACTOR,
//	TERM,
//	EXPRESSION,
//	STATEMENT,
//	PROGRAM,
//	NON_TERMINALS_COUNT
//} non_terminal;



item_set* rule(item s, item* arr)
{
	item_set* gr = (item_set*)malloc(sizeof(item_set));
	gr->lhs = s;
	gr->rhs = arr;
}