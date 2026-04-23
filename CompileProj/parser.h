#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"
#include "stack.h"

typedef struct ast_node
{
	token* data;
	struct ast_node** child;
	int child_count, child_size;
} syntax_node, * AST;

AST init_ast(token* tkn);
int add_son(AST ast, token* tkn);

typedef struct {
	stack stck;
	AST ast;
	int state;
	token* input;
	int input_size;
	int index;

	error_list err_lst;
} parser;

void init_slr_tables();
#endif