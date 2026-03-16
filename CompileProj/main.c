#include <stdio.h>
#include "common.h"
#include "lexer.h"
#include "parser.h"

char getInput() {
	return getchar();
}

int main() {

	printTokenType(getNextToken());
	
}