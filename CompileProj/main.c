#include <stdio.h>
#include "common.h"
#include "lexer.h"
#include "parser.h"


int main() {

	token_list lst = tokenize("int mispar = 5;");
	
	while (lst)
	{
		switch (lst->type)
		{
		case INT:
		case CHAR:
		case FLOAT:
		case STRING:
		case RATIONAL:
		case BOOL:
		case IDENTIFIER:
			printf("%s: %s", lst->info.str_val, lst->type);
			break;
		case NATURAL:
			printf("%d: %s", lst->info.num_val, lst->type);
			break;
		}
		printf('\n');
	}

	return 0;
}