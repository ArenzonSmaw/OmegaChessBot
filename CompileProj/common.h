#ifndef COMMON_H
#define COMMON_H

#include<stdlib.h>

#define TOKEN_MAX_LENGTH 33

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
	char lexeme[TOKEN_MAX_LENGTH];
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