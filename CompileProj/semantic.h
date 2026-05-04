#ifndef SEMANTIC_H
#define SEMANTIC_H

#include "ast.h"
#include "lexer.h"

#define TABLE_ROWS 256

typedef enum
{
	VARIABLE,
	CONSTANT,
	PARAM,
	FUNCTION,
	LABEL
} semantic_kind;


typedef struct {
	char* name;
	type_kind type;
} param_info;

typedef struct symbol_node {
	char* name;
	//double value;
	semantic_kind kind;  
	type_kind type;  

	int decl_line;
	int decl_col;
	int scope_level;

	int offset; 
	int is_global; 

	int param_count;
	param_info* params; 

	int is_initialized; 
	AST initializer;
	int is_init_const;
	int init_const_val;

	struct symbol_node* next;
} symbol_link;


typedef struct scope_node {
	symbol_link *table[TABLE_ROWS];
	struct scope_node* parent;
	int level;
	int offset_next;
} scope_node, *scope;

typedef struct {
	AST ast;
	scope current_scope;
	type_kind current_return_type;
	int loop_depth;
	int in_function;
	error_list error;
} semanticer;

semanticer* init_semanticer(AST ast, char* error_out);

symbol_link* create_symbol(char* name, semantic_kind, type_kind, int scope_level, int line, int col, int offset);
void enter_symbol(scope, symbol);
symbol_link* get_symbol(scope, char* name);
symbol_link* extract_symbol(scope, char* name);
int symbol_exist(scope, char* name);


scope init_scope(int level, scope parent);
int hash(char* symbolname);

int enter_scope(semanticer);

void exit_scope(semanticer);

void semanticize(semanticer*);

#endif
