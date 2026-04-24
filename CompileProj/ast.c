#include "ast.h"
#include <stdlib.h>

AST init_ast(token* tkn, symbol tok_type)
{
	AST syntax_tree = (AST)malloc(sizeof(syntax_node));
	if (syntax_tree != NULL)
	{
		syntax_tree->child = NULL;
		syntax_tree->child_size = 0;
		syntax_tree->child_count = 0;
		syntax_tree->data = tkn;
		syntax_tree->type = tok_type;
	}
	return syntax_tree;
}

void alloc_children(AST ast, int children)
{
	ast->child = (AST*)malloc(children * sizeof(AST));
	if (!ast->child) memory_error();
	ast->child_size = children;
}

int add_son(AST ast, AST son)
{
	int succeeded = 1;
	if (ast->child_count == ast->child_size)
	{
		ast->child = (AST*)realloc(ast->child, (ast->child_size*2) * sizeof(AST));
		ast->child_size *= 2;
		succeeded = ast->child != NULL;
	}
	if (!succeeded)
		memory_error();

	ast->child[ast->child_count++] = son;

	return succeeded;
}

