#ifndef PARSER_H
#define PARSER_H

#include "common.h"

typedef struct ast_node
{
	token* data;
	struct ast_node** child;
	int child_count, child_size;
} syntax_node, * AST;

AST init_ast(token* tkn);
int add_son(AST ast, token* tkn);

typedef struct {
	token* input;
	int input_size;
	AST ast;
} parser;

typedef enum {
	FACTOR,
	TERM,
	ARITH_EXPR,
	CMPR_EXPR,
	LOGIC_EXPR,
	DECLARE_STMT,
	CONDITION_STMT,
	LOOP_STMT,
	CONTROLFLOW_STMT,
	STATEMENT,
	STATEMENT_LIST,
	PROGRAM
} precedence;

#endif