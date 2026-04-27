#include "semantic.h"
#include <string.h>
#pragma warning (disable:4996)

typedef type_kind (*handle)(semanticer*, AST);

handle HANDLE_DISPATCH[KIND_COUNT];
int IS_INTEGER[TYPE_ERROR + 1] = {
	/*INT*/1, /*FLOAT*/0, /*NATURAL*/1, /*RATIONAL*/0, /*BOOL*/0,
	/*CHAR*/0, /*STRING*/0, /*VOID*/0, /*POINTER*/0, /*EXCEPTION*/0, /*ERROR*/0
};
int IS_NUMERIC[TYPE_ERROR + 1] = {
	/*INT*/1, /*FLOAT*/1, /*NATURAL*/1, /*RATIONAL*/1, /*BOOL*/1,
	/*CHAR*/0, /*STRING*/0, /*VOID*/0, /*POINTER*/0, /*EXCEPTION*/0, /*ERROR*/0
};
int IS_SCALAR[TYPE_ERROR + 1] = {
	/*INT*/1, /*FLOAT*/1, /*NATURAL*/1, /*RATIONAL*/1, /*BOOL*/1,
	/*CHAR*/1, /*STRING*/0, /*VOID*/0, /*POINTER*/1, /*EXCEPTION*/0, /*ERROR*/0
};
int TYPE_WIDTH[TYPE_ERROR + 1] = {
	/*INT*/4, /*FLOAT*/3, /*NATURAL*/1, /*RATIONAL*/2, /*BOOL*/4,
	/*CHAR*/1, /*STRING*/2, /*VOID*/0, /*POINTER*/1, /*EXCEPTION*/5, /*ERROR*/6
};


void type_error() {}
void declaration_error() {}
void assignment_error() {}
void signature_error() {}
void controlflow_error() {}
void prod_error(semanticer* smt, char title[21], char message[32], int line, int col) 
{
	err_append(smt->error, error(title, message, line, col));
}

type_kind wider_type(type_kind type1, type_kind type2)
{
	if (TYPE_WIDTH[type1] >= TYPE_WIDTH[type2])
		return type1;
	return type2;
}
int is_comparable(type_kind type1, type_kind type2)
{
	if (type1 == TYPE_ERROR || type2 == TYPE_ERROR) return 1;
	if (type1 == type2) return 1;
	if (IS_NUMERIC[type1] && IS_NUMERIC[type2]) return 1;
	return 0;
}

void init_semanticer(semanticer* smt)
{
	smt->current_scope = init_scope(0, NULL);
	smt->in_function = 0;
	smt->loop_depth = 0;
	
	smt->error = err_list();
}

symbol_link* create_symbol(char* name, semantic_kind kind, type_kind type, int scope_level, int line, int col, int offset)
{
	symbol_link* sym = (symbol_link*)malloc(sizeof(symbol_link));
	sym->name = strdup(name);

	sym->kind = kind;
	sym->type = type;

	sym->scope_level = scope_level;

	sym->decl_line = line;
	sym->decl_col = col;
	sym->offset = offset;

	sym->next = NULL;

	return sym;
}

scope init_scope(int level, scope parent)
{
	int i;
	scope scp = (scope)malloc(sizeof(scope_node));
	scp->level = level;
	if (parent) {
		scp->parent = parent;
		scp->offset_next = parent->offset_next;
	}

	for (i = 0; i < TABLE_ROWS; i++)
		scp->table[i] = NULL;

	return scp;
}

unsigned int hash(char* symbolname)
{
	int hashed = 0;
	while (symbolname[0])
	{
		hashed += (symbolname[0] - 'a') * 37;
		symbolname++;
	}
	return hashed % TABLE_ROWS;
}

void enter_symbol(scope scp, symbol_link* sym)
{
	int index = hash(sym->name) % TABLE_ROWS;
	symbol_link* temp = scp->table[index];
	sym->next = temp;
	scp->table[index] = sym;
}

symbol_link* get_symbol(scope scp, char* name)
{
	symbol_link* temp;
	int index;
	if (!scp) return NULL;

	index = hash(name) % TABLE_ROWS;
	temp = scp->table[index];

	while (temp != NULL && strcmp(temp->name, name))
		temp = temp->next;
	
	if (temp == NULL)
		temp = get_symbol(scp->parent, name);
	return temp;
}
int symbol_exist(scope scp, char* name)
{
	symbol_link* temp;
	int index = hash(name) % TABLE_ROWS;

	temp = scp->table[index];

	while (temp != NULL && strcmp(temp->name, name))
		temp = temp->next;
	return temp != NULL;
}

symbol_link* extract_symbol(scope scp, char* name)
{
	symbol_link* sym, *prev;
	int index = hash(name) % TABLE_ROWS;
	
	prev = scp->table[index];
	if (prev == NULL || !strcmp(prev->name, name)) return prev;
	while (prev->next != NULL && strcmp(prev->next->name, name))
		prev = prev->next;
	sym = prev->next;
	prev->next = sym == NULL ? NULL : sym->next;
	return sym;
}

int enter_scope(semanticer* smt)
{
	scope scp = smt->current_scope;
	smt->current_scope = init_scope(scp->level+1, scp);
}

void exit_scope(semanticer* smt)
{
	int i;
	symbol_link* sym, *next;
	scope temp = smt->current_scope;
	smt->current_scope = temp->parent; 

	if (smt->current_scope && temp->offset_next > smt->current_scope->offset_next)
		smt->current_scope->offset_next = temp->offset_next;

	for (i = 0; i < TABLE_ROWS; i++) {
		sym = temp->table[i];
		while (sym) {
			next = sym->next;
			free(sym->name);
			free(sym);
			sym = next;
		}
	}

	free(temp);
}

type_kind start_handler(semanticer* smt)
{
	return analyze(smt, smt->ast->children[0]);
}

type_kind program_handler(semanticer* smt)
{
	int i;
	AST ast = smt->ast;

	for (i = 0; i < ast->children_count; i++)
		analyze(smt, ast->children[i]);

	return type_void();
}
type_kind func_declare_handler(semanticer* smt, AST ast)
{
	int i;
	AST ast = smt->ast;
	AST name_param = ast->children[0];   
	AST param_list = ast->children[1];  
	AST block = ast->children[2];   
	AST param;

	char* fname = name_param->children[0]->data.name;
	type_kind return_type = node_type_to_semantic(name_param->children[1]->type);

	if (scope_lookup_current(smt, fname)) {
		declaration_error(smt, ast->line, ast->col,
			"redeclaration of function '%s'", fname);
	}

	symbol_link* fsym = create_symbol(fname, FUNCTION, return_type,
		smt->current_scope->level,
		ast->line, ast->col, 0);
	fsym->param_count = param_list->children_count;
	fsym->params = (param_info*)malloc(fsym->param_count * sizeof(param_info));
	for (i = 0; i < param_list->children_count; i++)
	{
		fsym->params[i].type = param_list->children[i]->type;
		fsym->params[i].name = param_list->children[i]->data.name;
	}

	enter_symbol(smt->current_scope, fsym);
	enter_scope(smt);

	for (int i = 0; i < param_list->children_count; i++) {
		param = param_list->children[i];  
		analyze(smt, param);                  
	}

	analyze(smt, block);

	exit_scope(smt);

	ast->type = return_type;
}
type_kind var_declare_handler(semanticer* smt, AST ast)
{
	/*
		GETS: pointer to semanticer struct
		DOES: 
	*/
	char* name;
	type_kind declared_type;
	type_kind init_type;
	AST ast = smt->ast;
	AST decl = ast->children[0];

	if (decl->kind == NODE_PARAMETER) {
		name = decl->children[0]->data.name;
		declared_type = decl->children[1]->type;
	}
	else {
		name = decl->data.name;
		declared_type = ast->type;
	}

	if (symbol_exist(smt->current_scope, name)) 
	{
		declaration_error(smt, ast->line, ast->col, "redeclaration of '%s'", name);
	}
	init_type = TYPE_VOID;
	if (ast->children_count > 1 && ast->children[1] != NULL) {
		analyze(smt, ast->children[1]);
		init_type = ast->children[1]->type;

		if (declared_type != init_type) {
				declaration_error(); //
		}
	}
	symbol_link* sym = create_symbol(name, VARIABLE, declared_type, smt->current_scope->level,
		ast->line, ast->col, smt->current_scope->offset_next++);
	enter_symbol(smt, sym);

	ast->type = declared_type;
}

type_kind assignment_handler(semanticer* smt, AST ast)
{
	AST ast = smt->ast;
	AST ident = ast->children[0];
	AST rhs = ast->children[1];
	type_kind rhs_type;

	symbol_link* sym = get_symbol(smt, ident->data.name);
	if (!sym) {
		assignment_error(smt, ident->line, ident->col,
			"assignment to undeclared variable '%s'", ident->data.name); //
		ast->type = TYPE_ERROR;
	}
	else if (sym->kind == CONSTANT) {
		assignment_error(smt, ident->line, ident->col,
			"assignment to constant '%s'", ident->data.name);
	}

	analyze(smt, rhs);
	rhs_type = rhs->type;

	if (sym->type != rhs_type) {
		type_error(smt, ast->line, ast->col, "type mismatch: cannot assign %d to '%s' of type %d",
			rhs_type, sym->name, sym->type);
		ast->type = TYPE_ERROR;
	}

	ast->type = rhs->type;  
}
type_kind parameter_handler(semanticer* smt, AST ast)
{
	AST ast = smt->ast;
	AST ident = ast->children[0];
	AST type_node = ast->children[1];

	char* name = ident->data.name;
	type_kind ptype = type_node->type;

	if (symbol_exist(smt, name)) {
		signature_error(smt, ast->line, ast->col,//
			"duplicate parameter name '%s'", name);
		ast->type = TYPE_ERROR;
	}
	else {
		symbol_link* sym = create_symbol(name, PARAM, ptype,
			smt->current_scope->level,
			ast->line, ast->col,
			smt->current_scope->offset_next++);
		scope_insert(smt, sym);
		ast->type = type_node->type;
	}
}
type_kind type_cast_handler(semanticer* smt, AST ast) 
{
	AST type = ast->children[0];
	AST expr = ast->children[1];
	
	analyze(smt, expr);

	if (!IS_SCALAR[type->type] || !IS_SCALAR[expr->type])
	{
		type_error();
		ast->type = TYPE_ERROR;
	}
	else
	{
		ast->type = type->type;
	}
	return ast->type;
}
type_kind stmt_list_handler(semanticer* smt, AST ast)
{
	int i;
	for (i = 0; i < ast->children_count; i++)
	{
		if (ast->children[i])
			analyze(smt, ast->children[i]);
	}
}

type_kind if_handler(semanticer* smt, AST ast)
{
	AST ast = smt->ast;
	AST cond = ast->children[0];
	AST block = ast->children[1];
	type_kind cond_type;

	analyze(smt, cond);
	cond_type = cond->type;
	if (cond_type == TYPE_VOID || cond_type == TYPE_STRING) {
		type_error(smt, cond->line, cond->col,
			"condition must be a scalar type");
	}

	//block
	enter_scope(smt);
	analyze(smt, block);
	exit_scope(smt);

	//else
	if (ast->children_count > 2 && ast->children[2] != NULL) {
		enter_scope(smt);
		analyze(smt, ast->children[2]); 
		exit_scope(smt);
	}
	ast->type = TYPE_VOID;
}
type_kind loop_handler(semanticer* smt, AST ast)
{
	AST ast = smt->ast;

	enter_scope(smt);
	smt->loop_depth++;

	if (ast->children_count == 2) { 
		AST cond = ast->children[0];
		AST block = ast->children[1];

		analyze(smt, cond);
		if (cond->type == TYPE_VOID || cond->type == TYPE_STRING) {
			emit_error(smt, cond->line, cond->col,
				"loop condition must be a scalar type");
		}
		analyze(smt, block);
	}
	else { 
		AST param = ast->children[0];
		AST end_val = ast->children[1];
		AST step = ast->children[2];
		AST block = ast->children[3];

		analyze(smt, param);

		analyze(smt, end_val);
		if (end_val->type != TYPE_INT && end_val->type != TYPE_NATURAL) {
			type_error(smt, end_val->line, end_val->col,
				"loop bound must be an integer type");
		}

		analyze(smt, step);
		analyze(smt, block);
	}

	exit_scope(smt);
	smt->loop_depth--;
	ast->type = TYPE_VOID;
}

type_kind return_handler(semanticer* smt, AST ast)
{
	AST ast = smt->ast;
	AST expr = ast->children[0];

	analyze(smt, expr);
	if (expr->type != smt->current_return_type)
		type_error();
	else
		ast->type = expr->type;
}
type_kind break_handler(semanticer* smt, AST ast)
{
	if (smt->loop_depth == 0){
		controlflow_error();
	}
	else
		smt->ast->type = TYPE_VOID;
}
type_kind pass_handler(semanticer* smt, AST ast)
{
	if (smt->loop_depth == 0)
	{
		controlflow_error();
		smt->ast->type = TYPE_ERROR;
	}
	else
		smt->ast->type = TYPE_VOID;
}
type_kind ident_handler(semanticer* smt, AST ast)
{
	AST ast = smt->ast;
	symbol_link* sym = get_symbol(smt->current_scope, ast->data.name);

	if (!sym) {
		declaration_error(smt, ast->line, ast->col,
			"undefined identifier '%s'", ast->data.name);
		ast->type = TYPE_ERROR;
		return;
	}

	ast->type = ast_type_from_semantic(sym->type);
}
type_kind literal_handler(semanticer* smt, AST ast)
{
	if (ast->type == TYPE_ERROR) {
		type_error(smt, ast->line, ast->col, "unknown literal type");
	}
}
type_kind underline_handler(semanticer* smt, AST ast)
{
	smt->ast->type = TYPE_VOID;
}
type_kind arithmetic_handler(semanticer* smt, AST ast)
{
	AST ast = smt->ast;
	AST left = ast->children[0];
	AST right = ast->children[1];

	analyze(smt, left);
	analyze(smt, right);

	if (!IS_NUMERIC[left->type] || !IS_NUMERIC[right->type]) {
		type_error(smt, ast->line, ast->col,
			"arithmetic operands must be numeric types");
		ast->type = TYPE_ERROR;
	}
	else {
		ast->type = wider_type(left->type, right->type);
	}
}

type_kind logical_handler(semanticer* smt, AST ast)
{
	AST ast = smt->ast;
	AST left = ast->children[0];
	AST right = ast->children[1];

	analyze(smt, left);
	analyze(smt, right);

	if (!IS_SCALAR[left->type] || !IS_SCALAR[right->type]) {
		type_error(smt, ast->line, ast->col,
			"logical operators require scalar operands");
	}

	ast->type = TYPE_BOOL;
}
type_kind bitwise_handler(semanticer* smt, AST ast)
{
	AST ast = smt->ast;
	AST left = ast->children[0];
	AST right = ast->children[1];

	analyze(smt, left);
	analyze(smt, right);

	if (!IS_INTEGER[left->type] || !IS_INTEGER[right->type]) {
		type_error(smt, ast->line, ast->col,
			"bitwise operators require integer operands");
		ast->type = TYPE_ERROR;
	}
	else
		ast->type = wider_type(left->type, right->type);
}
type_kind logical_not_handler(semanticer* smt, AST ast)
{
	AST ast = smt->ast;
	AST operand = ast->children[0];

	analyze(smt, operand);

	if (!IS_SCALAR[operand->type]) {
		type_error(smt, ast->line, ast->col,
			"logical not requires a scalar operand");
	}

	ast->type = TYPE_INT;
}
type_kind bitwise_not_handler(semanticer* smt, AST ast)
{
	AST ast = smt->ast;
	AST operand = ast->children[0];

	analyze(smt, operand);

	if (!IS_INTEGER[operand->type]) {
		type_error(smt, ast->line, ast->col,
			"bitwise not requires an integer operand");
		ast->type = TYPE_ERROR;
	}
	else
		ast->type = operand->type;
}
type_kind comparison_handler(semanticer* smt, AST ast)
{
	AST ast = smt->ast;
	AST left = ast->children[0];
	AST right = ast->children[1];

	analyze(smt, left);
	analyze(smt, right);

	/*if (!is_comparable(left->type, right->type)) {
		type_error(smt, ast->line, ast->col,
			"comparison between incompatible types");
	}*/

	ast->type = TYPE_BOOL;  
}
type_kind func_call_handler(semanticer* smt, AST ast)
{
	AST ast = smt->ast;
	AST ident = ast->children[0];
	type_kind temp;
	symbol_link* sym = get_symbol(smt->current_scope, ident->data.name);
	if (!sym) {
		declaration_error(smt, ident->line, ident->col,
			"call to undeclared function '%s'", ident->data.name);
		ast->type = TYPE_ERROR;
	}
	else if (sym->kind != FUNCTION) {
		signature_error(smt, ident->line, ident->col,
			"'%s' is not a function", ident->data.name);
		ast->type = TYPE_ERROR;
	}
	else {
		int arg_count = ast->children_count - 1;
		if (arg_count != sym->param_count) {
			signature_error(smt, ast->line, ast->col,
				"'%s' expects %d arguments but got %d",
				sym->name, sym->param_count, arg_count);
		}
		temp = smt->current_return_type;
		smt->current_return_type = ast->kind;
		for (int i = 1; i < ast->children_count; i++) {
			analyze(smt, ast->children[i]);
		}
		ast->type = sym->type;  // return type of the function
	}
}
type_kind block_handler(semanticer* smt, AST ast)
{
	AST ast = smt->ast;

	for (int i = 0; i < ast->children_count; i++) {
		analyze(smt, ast->children[i]);
	}

	ast->type = TYPE_VOID;
}

type_kind init_dispatch_table()
{
	HANDLE_DISPATCH[NODE_START] = start_handler;
	HANDLE_DISPATCH[NODE_PROGRAM] = program_handler;

	HANDLE_DISPATCH[NODE_FUNC_DECLARE] = func_declare_handler;
	HANDLE_DISPATCH[NODE_VAR_DECLARE] = var_declare_handler;
	HANDLE_DISPATCH[NODE_ASSIGNMENT] = assignment_handler;
	HANDLE_DISPATCH[NODE_PARAMETER] = parameter_handler;
	HANDLE_DISPATCH[NODE_TYPE_CAST] = type_cast_handler;
	HANDLE_DISPATCH[NODE_STMT_LIST] = stmt_list_handler;

	HANDLE_DISPATCH[NODE_IF] = if_handler;
	HANDLE_DISPATCH[NODE_LOOP] = loop_handler;
	HANDLE_DISPATCH[NODE_RETURN] = return_handler;
	HANDLE_DISPATCH[NODE_BREAK] = break_handler;
	HANDLE_DISPATCH[NODE_PASS] = pass_handler;

	HANDLE_DISPATCH[NODE_IDENT] = ident_handler;
	HANDLE_DISPATCH[NODE_LITERAL] = literal_handler;
	HANDLE_DISPATCH[NODE_CHAR] = literal_handler;
	HANDLE_DISPATCH[NODE_STRING] = literal_handler;
	HANDLE_DISPATCH[NODE_UNDERLINE] = underline_handler;

	HANDLE_DISPATCH[NODE_ADD] = arithmetic_handler;
	HANDLE_DISPATCH[NODE_SUB] = arithmetic_handler;
	HANDLE_DISPATCH[NODE_MUL] = arithmetic_handler;
	HANDLE_DISPATCH[NODE_DIV] = arithmetic_handler;
	HANDLE_DISPATCH[NODE_MOD] = arithmetic_handler;

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

	HANDLE_DISPATCH[NODE_FUNC_CALL] = func_call_handler;
	HANDLE_DISPATCH[NODE_BLOCK] = block_handler;
}

type_kind analyze(semanticer* smt, AST ast)
{
	handle handler = HANDLE_DISPATCH[ast->kind];
	return handler(smt, ast);
}

void semanticize(semanticer* smt)
{
	init_semanticer(smt);
	analyze(smt, smt->ast);
}