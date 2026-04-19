#include "parser.h"

AST init_ast(token* tkn)
{
	AST syntax_tree = (AST)malloc(sizeof(syntax_node));
	if (syntax_tree != NULL)
	{
		syntax_tree->child = NULL;
		syntax_tree->child_size = 0;
		syntax_tree->child_count = 0;
		syntax_tree->data = tkn;
	}
	return syntax_tree;
}

int add_son(AST ast, token* tkn)
{
	int succeeded = 1; // 1 - success, 0 - failure
	if (ast->child_count >= ast->child_size)
	{
		ast->child = (AST*)realloc(ast->child, (ast->child_size + 1) * sizeof(AST));
		succeeded = (ast->child != NULL);
	}
	if (succeeded) {
		ast->child[ast->child_count] = init_ast(tkn);
		succeeded = (ast->child[ast->child_count] != NULL) ? 1 : 0;
	}
	return succeeded;
}