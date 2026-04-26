#include "semantic.h"
#include <string.h>
#pragma warning (disable:4996)

typedef void (*handle)(semanticer*);

handle HANDLE_DISPATCH[KIND_COUNT];

void prod_error(semanticer* smt, char title[21], char message[32], int line, int col) 
{
	err_append(smt->error, error(title, message, line, col));
}


void analyze(semanticer* smt, AST ast)
{

}

void start_handler(semanticer* smt) 
{
	AST ast = smt->ast;
	if (ast->children == NULL)
		prod_error(smt, "MISSING PROGRAM", "program is missing", ast->line, ast->col);
	else if (ast->children[0]->kind != NODE_PROGRAM)
		prod_error(smt, "EXPECTED PROGRAM", "expected program", ast->line, ast->col);
	else
		analyze(smt, ast->children[0]);
}
void program_handler(semanticer* smt) 
{
	int i;
	AST ast = smt->ast;

	for (i = 0; i < ast->children_count; i++)
	{
		analyze(smt, ast->children[i]);
	}
}
void func_declare_handler(semanticer* smt) 
{
	AST ast = smt->ast;
	int i;
	for (i = 0; i < ast->children_count; i++)
	{
		analyze(smt, ast->children[i]);
	}

}
void var_declare_handler(semanticer* smt) {}
void assignment_handler(semanticer* smt) {}
void if_handler(semanticer* smt) {}
void while_handler(semanticer* smt) {}
void return_handler(semanticer* smt) {}
void break_handler(semanticer* smt) {}
void pass_handler(semanticer* smt) {}
void ident_handler(semanticer* smt) {}
void literal_handler(semanticer* smt) {}
void underline_handler(semanticer* smt) {}
void arithmetic_handler(semanticer* smt) {}
void inc_unary_handler(semanticer* smt) {}
void inc_binary_handler(semanticer* smt) {}
void dec_unary_handler(semanticer* smt) {}
void dec_binary_handler(semanticer* smt) {}
void magnify_unary_handler(semanticer* smt) {}
void magnify_binary_handler(semanticer* smt) {}
void diminish_unary_handler(semanticer* smt) {}
void diminish_binary_handler(semanticer* smt) {}
void logical_handler(semanticer* smt) {}
void bitwise_handler(semanticer* smt) {}
void logical_not_handler(semanticer* smt) {}
void bitwise_not_handler(semanticer* smt) {}
void comparison_handler(semanticer* smt) {}
void range_handler(semanticer* smt) {}
void func_call_handler(semanticer* smt) {}
void block_handler(semanticer* smt) {}

void init_dispatch_table()
{
	HANDLE_DISPATCH[NODE_START] = start_handler;
	HANDLE_DISPATCH[NODE_PROGRAM] = program_handler;

	HANDLE_DISPATCH[NODE_FUNC_DECLARE] = func_declare_handler;
	HANDLE_DISPATCH[NODE_VAR_DECLARE] = var_declare_handler;
	HANDLE_DISPATCH[NODE_ASSIGNMENT] = assignment_handler;

	HANDLE_DISPATCH[NODE_IF] = if_handler;
	HANDLE_DISPATCH[NODE_LOOP] = while_handler;
	HANDLE_DISPATCH[NODE_RETURN] = return_handler;
	HANDLE_DISPATCH[NODE_BREAK] = break_handler;
	HANDLE_DISPATCH[NODE_PASS] = pass_handler;

	HANDLE_DISPATCH[NODE_IDENT] = ident_handler;
	HANDLE_DISPATCH[NODE_LITERAL] = literal_handler;
	HANDLE_DISPATCH[NODE_UNDERLINE] = underline_handler;

	HANDLE_DISPATCH[NODE_ADD] = arithmetic_handler;
	HANDLE_DISPATCH[NODE_SUB] = arithmetic_handler;
	HANDLE_DISPATCH[NODE_MUL] = arithmetic_handler;
	HANDLE_DISPATCH[NODE_DIV] = arithmetic_handler;
	HANDLE_DISPATCH[NODE_MOD] = arithmetic_handler;

	HANDLE_DISPATCH[NODE_INC_UNARY] = inc_unary_handler;
	HANDLE_DISPATCH[NODE_INC_BINARY] = inc_binary_handler;
	HANDLE_DISPATCH[NODE_DEC_UNARY] = dec_unary_handler;
	HANDLE_DISPATCH[NODE_DEC_BINARY] = dec_binary_handler;

	HANDLE_DISPATCH[NODE_MAG_UNARY] = magnify_unary_handler;
	HANDLE_DISPATCH[NODE_MAG_BINARY] = magnify_binary_handler;
	HANDLE_DISPATCH[NODE_DIM_UNARY] = diminish_unary_handler;
	HANDLE_DISPATCH[NODE_DIM_BINARY] = diminish_binary_handler;

	HANDLE_DISPATCH[NODE_LOG_OR] = logical_handler;
	HANDLE_DISPATCH[NODE_LOG_AND] = logical_handler;
	HANDLE_DISPATCH[NODE_LOG_NOT] = logical_not_handler;

	HANDLE_DISPATCH[NODE_BIT_OR] = bitwise_handler;
	HANDLE_DISPATCH[NODE_BIT_AND] = bitwise_handler;
	HANDLE_DISPATCH[NODE_BIT_NOT] = bitwise_not_handler;
	HANDLE_DISPATCH[NODE_XOR] = bitwise_handler;

	HANDLE_DISPATCH[NODE_LOG_EQUAL] = comparison_handler;
	HANDLE_DISPATCH[NODE_LOG_DIFFERENT] = comparison_handler;
	HANDLE_DISPATCH[NODE_GREAT] = comparison_handler;
	HANDLE_DISPATCH[NODE_GREAT_EQUAL] = comparison_handler;
	HANDLE_DISPATCH[NODE_LESS] = comparison_handler;
	HANDLE_DISPATCH[NODE_LESS_EQUAL] = comparison_handler;

	HANDLE_DISPATCH[NODE_RANGE] = range_handler;

	HANDLE_DISPATCH[NODE_FUNC_CALL] = func_call_handler;
	HANDLE_DISPATCH[NODE_BLOCK] = block_handler;
}
void init_semanticer(semanticer* smt)
{
	smt->current_scope = NULL;
	smt->current_scope_level = -1;
	smt->current_offset = 0;
	smt->error = err_list();
}

symbol_link* create_symbol(char* name, semantic_kind kind, semantic_type type, int parameter_count)
{
	symbol_link* sym = (symbol_link*)malloc(sizeof(symbol_link));
	strcpy(sym->name, name);
	sym->kind = kind;
	sym->type = type;
	sym->init_col = sym->init_line = -1;
	sym->next = NULL;
	sym->parameter_count = parameter_count;

	return sym;
}

scope init_scope(int level, scope parent)
{
	int i;
	scope scp = (scope)malloc(sizeof(scope_node));
	scp->level = level;
	scp->parent = parent;

	for (i = 0; i < TABLE_ROWS; i++)
		scp->table[i] = NULL;

	return scp;
}

unsigned int hash(char* symbolname)
{
	int hashed = 0;
	while (symbolname)
	{
		hashed += (symbolname[0] - 'a') * 37;
		symbolname++;
	}
	return hashed % TABLE_ROWS;
}

void enter_symbol(scope scp, symbol_link* sym)
{
	int index = hash(sym->name);
	symbol_link* temp = scp->table[index];
	sym->next = temp;
	scp->table[index] = sym;
}

int symbol_exist(scope scp, char* name)
{
	return get_symbol(scp, name) != NULL;
}
symbol_link* get_symbol(scope scp, char* name)
{
	symbol_link* temp;
	int index = hash(name);

	temp = scp->table[index];

	while (temp != NULL && strcmp(temp->name, name))
		temp = temp->next;
	return temp;
}
symbol_link* extract_symbol(scope scp, char* name)
{
	symbol_link* sym, *prev;
	int index = hash(name);
	
	prev = scp->table[index];
	if (prev == NULL || !strcmp(prev->name, name)) return prev;
	while (prev->next != NULL && strcmp(prev->next->name, name))
		prev = prev->next;
	sym = prev->next;
	prev->next = sym == NULL ? NULL : sym->next;
	return sym;
}

int enter_scope(semanticer smt)
{
	init_scope(++(smt.current_scope_level), smt.current_scope);
}

void exit_scope(semanticer smt)
{
	int i;
	scope temp = smt.current_scope;
	smt.current_scope = temp->parent;
	smt.current_scope_level = (smt.current_scope)->level;

	for (i = 0; i < TABLE_ROWS; i++)
	{
		if (temp->table[i])
			free(temp->table[i]);
	}
	free(temp);
}

void semanticize(semanticer* smt)
{
	init_semanticer(smt);
	analyze(smt, smt->ast);
}