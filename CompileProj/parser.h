#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"
#include "stack.h"
#include "ast.h"

typedef struct {
	stack stck;
	AST ast;
	int state;
	token* input;
	int input_size;
	int index;

	error_list err_lst;
} parser;

typedef enum {
	REDUCE = -1,
	ACCEPT = 0,
	SHIFT = 1,
	ERROR = 2
} action;


void parse(parser* prsr, char* parser_name);
#endif