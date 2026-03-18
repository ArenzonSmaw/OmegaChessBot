#ifndef LEXER_H
#define LEXER_H

#include <stdlib.h>

typedef enum INPUT {
	UNKNOWN,
	WHITESPACE,
	CF,
	SQUOTE, DQUOTE,
	OP,
	MINUS, DIVIDE, DOT,
	DIGIT,
	LETTER,
	A, B, C, D, E, F, G, H, I, K, L, N, O, P, R, S, T, U, V, X,
	PRINTABLE
} INPUT;

typedef enum TYPES {
	HEADER,
	INT,
	CONTROLFLOW,
	CHAR,
	FLOAT,
	OPERATOR,
	STRING,
	NATURAL,
	RATIONAL,
	BOOL,
	CONDITIONAL,
	IDENTIFIER,
	VARTYPE,
	ERROR_HANDLER,
	DECLARE
} TYPES;

typedef union {
	string str_val;
	int num_val;
	bool bool_val;
} data;
typedef struct node {
	data info;
	TYPES type;
	struct node* next;
} token_node, *token_list;

token_list tokenize(char[]);

#endif