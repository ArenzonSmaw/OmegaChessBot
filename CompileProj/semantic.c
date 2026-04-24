#include "semantic.h"
#include <string.h>

void init_semanticer(semanticer* smt)
{
	smt->current_scope = NULL;
	smt->current_scope_level = -1;
	smt->current_offset = 0;
	smt->error = err_list();
}

symbol create_symbol(char* name, semantic_kind kind, semantic_type type, int parameter_count)
{
	symbol sym = (symbol)malloc(sizeof(symbol_link));
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

void enter_symbol(scope scp, symbol sym)
{
	int index = hash(sym->name);
	symbol temp = scp->table[index];
	sym->next = temp;
	scp->table[index] = sym;
}

symbol get_symbol(scope scp, char* name)
{
	symbol temp;
	int index = hash(name);

	temp = scp->table[index];

	while (temp != NULL && strcmp(temp->name, name))
		temp = temp->next;
	return temp;
}
symbol extract_symbol(scope scp, char* name)
{
	symbol sym, prev;
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
	smt.current_scope_level = (*smt.current_scope)->level;

	for (i = 0; i < TABLE_ROWS; i++)
	{
		if (temp->table[i])
			free(temp->table[i]);
	}
	free(temp);
}