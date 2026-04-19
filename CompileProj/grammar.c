#include "grammar.h"

typedef enum {
	ID,
	LITERAL,
	TYPE,
	FACTOR,
	TERM,
	EXPRESSION,
	STATEMENT,
	PROGRAM,
	NON_TERMINALS_COUNT
} non_terminal;



grammar_rule* rule(Symbol s, Symbol* arr)
{
	grammar_rule* gr = (grammar_rule*)malloc(sizeof(grammar_rule));
	gr->lhs = s;
	gr->rhs = arr;
}