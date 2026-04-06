#ifndef COMMON_H
#define COMMON_H

#include<stdlib.h>

typedef enum { false = 0, true = 1 } bool;

typedef char *string;

typedef enum {
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
} type;

typedef struct {
	char* lexeme;
	double value;
	type type;
} token;

typedef struct {
	char* input;
	int index;

	token* data;
	int size;
	int count;
} lexer;

#endif