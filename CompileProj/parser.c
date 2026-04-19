#include "parser.h"
#include "grammar.h"

void (*(ACTION[1][TERMINALS_COUNT]))(parser* prsr);


void parse(parser* prsr)
{
	void (*action)(parser*);
	int state = 0;


}
