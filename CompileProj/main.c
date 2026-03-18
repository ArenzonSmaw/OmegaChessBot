#include <stdio.h>
#include "common.h"
#include "lexer.h"
#include "parser.h"


int main() {

	token_list lst;
	lst = tokenize("declare mispar: int = 57;\n declare str:string = \"blah blah\";");
	//lst = tokenize("\n \'t\' return");
	token_list temp;
	char types[15][20] = { "HEADER", "INT", "CONTROLFLOW", "CHAR", "FLOAT", "OPERATOR", "STRING", "NATURAL", "RATIONAL", "BOOL", "CONDITIONAL", "IDENTIFIER", "VARTYPE", "ERROR_HANDLER", "DECLARE" };
	while (lst)
	{
		switch (lst->type)
		{
		case IDENTIFIER:
		case STRING:
		case CONTROLFLOW:
		case OPERATOR:
		case VARTYPE:
		case DECLARE:
			printf("%s: %s", lst->info.str_val, types[lst->type]);
			break;
		case NATURAL:
		case BOOL:
			printf("%d: %s", lst->info.num_val, types[lst->type]);
			break;
		}
		printf("\n");
		temp = lst;
		lst = lst->next;
		free(temp);
	}

	return 0;
}