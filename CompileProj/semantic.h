#ifndef SEMANTIC_H
#define SEMANTIC_H

#include "ast.h"
#include "lexer.h"

#define TABLE_ROWS 256

typedef enum
{
	VARIABLE,
	CONSTANT,
	PARAMETER,
	FUNCTION,
	LABEL
} semantic_kind;

typedef enum
{
	INT,
	FLOAT,
	RATIONAL,
	NATURAL,
	CHAR,
	STRING,
	VOID,
	POINTER,

} semantic_type;

typedef struct symbol_node {
	char* name;
	semantic_kind kind;
	semantic_type type;
	int scope_level;
	int init_line, init_col;
	int offset;
	int parameter_count;

	struct symbol_node* next;

} symbol_link, *symbol;

typedef struct scope_node {
	symbol table[TABLE_ROWS];
	struct scope_node* parent;
	int level;

} scope_node, *scope;

typedef struct {
	scope* current_scope;
	int current_scope_level;
	int current_offset;
	error_list error;
} semanticer;

symbol create_symbol(char* name, semantic_kind, semantic_type);
void enter_symbol(scope, symbol);
symbol extract_symbol(scope, char* name);
int symbol_exist(scope, char* name);


scope init_scope(int level, scope parent);
int hash(char* symbolname);

void enter_scope(semanticer, int scope);

void exit_scope(semanticer);

#endif