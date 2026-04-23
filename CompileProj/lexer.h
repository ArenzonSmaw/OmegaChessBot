
#ifndef LEXER_H

#define LEXER_H

#include "error.h"
#include "grammar.h"

typedef struct {
	char lexeme[TOKEN_MAX_LENGTH];
	double value;
	symbol type;

	int line, col;
} token;

typedef struct {
	char* input;
	int index;

	token* data;
	int size;
	int count;

	int line, col;

	error_list err_list;
} lexer;

typedef enum INPUT {
	CH_UNKNOWN,
	CH_WHITESPACE,
	CH_NEWLINE,
	CH_COMMA,
	CH_OP_SQRBRACKET,
	CH_CL_SQRBRACKET,
	CH_OP_CRLBRACKET,
	CH_CL_CRLBRACKET,
	CH_OP_RNDBRACKET,
	CH_CL_RNDBRACKET,
	CH_SEMICOLON,
	CH_SQUOTE, CH_DQUOTE,
	CH_HASHTAG,
	CH_PLUS, CH_MULT, CH_MOD, CH_OR, CH_TILDE, CH_AND, CH_LEFT, CH_RIGHT, CH_NOT, CH_EQUALS, CH_COLON, //ordinary operators
	CH_MINUS, CH_DIVIDE, CH_DOT, CH_BACKSLASH, //token altering operators
	CH_DIGIT,
	CH_LETTER,
	A, B, C, D, E, F, G, H, I, K, L, N, O, P, R, S, T, U, V, X,
	CH_UNDERLINE,
	CH_PRINTABLE
} INPUT;


void tokenize(lexer *lxr);

#endif