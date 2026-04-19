#ifndef GRAMMAR_H
#define GRAMMAR_H

#include "common.h"

typedef enum {
	
	FACTOR,
	TERM,
	EXPRESSION,
	STATEMENT,
	PROGRAM,
	NON_TERMINALS_COUNT
} non_terminal;

typedef enum {
	ID,
	NUM_LITERAL,
	RAT_LITERAL,
	TXT_LITERAL,
	BOOL_LITERAL,
	LITERAL,

	TYPE,

	AND,
	ANDAND,
	OR,
	OROR,
	TILDE_OR,
	EQUALS,
	DBL_EQUALS,
	RIGHT,
	DBL_RIGHT,
	RIGHT_EQUALS,
	LEFT,
	DBL_LEFT,
	LEFT_EQUALS,
	NOT_EQUALS,
	PLUS,
	DBL_PLUS,
	MINUS,
	DBL_MINUS,
	MULT,
	DBL_MULT,
	DIVIDE,
	DBL_DIVIDE,
	MODULUS,
	DBL_MOD,
	NOT,
	NOTNOT,
	COLON,

	SEMICOLON,
	COMMA,
	OP_RNDBRACKET,
	CL_RNDBRACKET,
	OP_CRLBRACKET,
	CL_CRLBRACKET,
	OP_SQRBRACKET,
	CL_SQRBRACKET,
	RETURN,
	BREAK,
	PASS,
	LOOP,
	DECLARE,

	CHECK,
	UNDERLINE,
	TERMINALS_COUNT
} terminal, type;

typedef struct {
	char lexeme[TOKEN_MAX_LENGTH];
	double value;
	terminal type;
} token;

typedef struct
{
	union {
		non_terminal nonterm;
		terminal terminal;
	};
	int isTerminal;
} Symbol;

typedef struct
{
	Symbol lhs;
	Symbol* rhs;

} grammar_rule, * grammar_rules;

grammar_rule* rule(Symbol, Symbol*);
#endif
