#ifndef AST_H
#define AST_H

#include "lexer.h"
#include "grammar.h"

typedef struct ast_node
{
	token* data;
	symbol type;
	struct ast_node** child;
	int child_count, child_size;
} syntax_node, * AST;

AST init_ast(token* tkn, symbol type);
int add_son(AST ast, AST son);
void alloc_children(AST ast, int children);

#endif