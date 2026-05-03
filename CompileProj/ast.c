#include "ast.h"
#include <stdlib.h>


void fill_kind_to_data()
{
	//fills global array of data_type enum by node_kind enum
	int i;
	for (i = 0; i < KIND_COUNT; i++)
	{
		//initialize with default 'none' value
		KIND_TO_DATA[i] = NONE;
	}
	KIND_TO_DATA[NODE_LITERAL] = KIND_TO_DATA[NODE_UNDERLINE] = NUM;
	KIND_TO_DATA[NODE_IDENT] = NAME;
}

AST init_ast(token* tkn, node_kind kind, int children)
{
	AST syntax_tree = create_leaf(tkn, kind);
	alloc_children(syntax_tree, children);
	syntax_tree->children_count = 0;
	
	return syntax_tree;
}

AST create_leaf(token* tkn, node_kind kind)
{
	AST ast = (AST)malloc(sizeof(syntax_node));
	data_type dattype;
	if (KIND_TO_DATA[0] == 0)
		fill_kind_to_data();
	dattype = KIND_TO_DATA[kind];
	if (!ast) memory_error();
	ast->children = NULL;
	ast->children_count = 0;
	ast->children_size = 0;
	ast->kind = kind;
	if (tkn)
	{
		ast->line = tkn->line;
		ast->col = tkn->col;
		ast->type = dattype;

		if (dattype == NUM)
			ast->data.value = tkn->value;

		else if (dattype == NAME)
			ast->data.name = tkn->lexeme;
	}

	return ast;
}

void alloc_children(AST ast, int children)
{
	int i;
	if (ast->children != NULL)
		free(ast->children);
	ast->children = NULL;
	ast->children = (AST*)malloc(children * sizeof(AST));
	if (!ast->children) memory_error();
	ast->children_size = children;

	for (i = 0; i < children; i++)
	{
		ast->children[i] = NULL;
	}
}
void realloc_children(AST ast, int children)
{
	int i;
	ast->children = (AST*)realloc(ast->children, children * sizeof(AST));
	if (!ast->children) memory_error();
	if (children < ast->children_size)
		ast->children_count = children;
	else
	{
		for (i = ast->children_count; i < children; i++)
			ast->children[i] = NULL;
	}
	ast->children_size = children;
}

int insert_son(AST ast, AST son, int index)
{
	int succeeded = 1;
	if (index >= ast->children_size || index < 0)
		succeeded = 0;
	if (succeeded)
		ast->children[index] = son;
	
	return succeeded;
}

int add_son(AST ast, AST son)
{
	int idx = -1, i;
	for (i = 0; i < ast->children_size && idx == -1; i++)
	{
		if (ast->children[i] == NULL)
			idx = i;
	}
	if (!insert_son(ast, son, ast->children_count))
	{
		ast->children = (AST*)realloc(ast->children, (ast->children_size * 2) * sizeof(AST));
		if (ast->children == NULL) memory_error();
		ast->children_size *= 2;
		ast->children[ast->children_count++] = son;
	}
	else
		ast->children_count++;
	
	return 1;
}

