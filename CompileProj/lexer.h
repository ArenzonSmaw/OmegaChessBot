#ifndef LEXER_H
#define LEXER_H

#include <stdlib.h>
#include "common.h"
#include "error.h"

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

token** tokenize(lexer *lxr);

#endif