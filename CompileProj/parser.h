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

	int recovering;
	error_list err_lst;
} parser;

typedef enum {
	REDUCE = -1,
	ACCEPT = 0,
	SHIFT = 1,
	ERROR = 2
} action;

parser* init_parser(lexer* lexer, char* error_file);
/*
	GETS: pointer to lexer struct and error file name
	RETS: parser structure with initiated values
*/

void parse(parser* prsr, char* parser_name);
/*
	GETS: pointer to parser struct and name of the parser file (to load/write the parser from/to)
	RETS: Abstract Syntax Tree root via prsr->ast
*/
#endif