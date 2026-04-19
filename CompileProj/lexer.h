#ifndef LEXER_H
#define LEXER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "grammar.h"
#include "error.h"

typedef struct {
	char* input;
	int index;

	token* data;
	int size;
	int count;
} lexer;

typedef enum INPUT {
	UNKNOWN,
	WHITESPACE,
	NEWLINE,
	CF_COMMA,
	CF_OP_SQRBRACKET,
	CF_CL_SQRBRACKET,
	CF_OP_CRLBRACKET,
	CF_CL_CRLBRACKET,
	CF_OP_RNDBRACKET,
	CF_CL_RNDBRACKET,
	CF_SEMICOLON,
	CF_BREAK,
	CF_RETURN,
	CF_PASS,
	CF_LOOP,
	SQUOTE, DQUOTE,
	HASHTAG, 
	PLUS, MULT, MOD, OR, TILDE, AND, LEFT, RIGHT, NOT, EQUALS, COLON, //ordinary operators
	MINUS, DIVIDE, DOT, BACKSLASH, //token altering operators
	DIGIT,
	LETTER,
	A, B, C, D, E, F, G, H, I, K, L, N, O, P, R, S, T, U, V, X,
	UNDERLINE,
	PRINTABLE
} INPUT;


void tokenize(lexer *lxr);

#endif