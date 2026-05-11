#ifndef SEMANTIC_H
#define SEMANTIC_H

#include "ast.h"
#include "lexer.h"

#define TABLE_ROWS 256

static int IS_FLT_OR_RAT[TYPE_ERROR + 1] = {
	/*INT*/0, /*FLOAT*/1, /*NATURAL*/0, /*RATIONAL*/1, /*BOOL*/0,
	/*CHAR*/0, /*STRING*/0, /*VOID*/0, /*POINTER*/0, /*EXCEPTION*/0, /*ERROR*/0
};

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

	int is_param;

	int is_initialized; 
	AST initializer;
	int is_init_const;
	int init_const_val1, init_const_val2;

	struct symbol_node* next;
} symbol_link;


typedef struct scope_node {
	symbol_link *table[TABLE_ROWS];
	struct scope_node* parent;
	int level;
	int param_offset_next;
	int local_offset_next;
} scope_node, *scope;

typedef struct {
	AST ast;
	scope current_scope;
	type_kind current_return_type;
	int loop_depth;
	int in_function;
	int locals_count;
	error_list error;
} semanticer;

semanticer* init_semanticer(AST ast, char* error_out);

symbol_link* create_symbol(char* name, semantic_kind, type_kind, AST initializer, int scope_level, int line, int col, int offset);
void enter_symbol(scope, symbol_link*);
symbol_link* get_symbol(scope, char* name);
symbol_link* extract_symbol(scope, char* name);
int symbol_exist(scope, char* name);

int type_size(type_kind);


scope init_scope(int level, scope parent, int is_function);
int hash(char* symbolname);

void enter_scope(semanticer*);

void exit_scope(semanticer*);

void semanticize(semanticer*);

#endif
