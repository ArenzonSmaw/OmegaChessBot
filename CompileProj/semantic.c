#include "semantic.h"
#include <string.h>
#pragma warning (disable:4996)

typedef type_kind (*handle)(semanticer*, AST);

handle HANDLE_DISPATCH[KIND_COUNT];
int IS_PRINTABLE[TYPE_ERROR + 1] = {
	/*INT*/1, /*FLOAT*/1, /*NATURAL*/1, /*RATIONAL*/1, /*BOOL*/1,
	/*CHAR*/1, /*STRING*/1, /*VOID*/0, /*POINTER*/0, /*EXCEPTION*/1, /*ERROR*/0
};
int IS_INTEGER[TYPE_ERROR + 1] = {
	/*INT*/1, /*FLOAT*/0, /*NATURAL*/1, /*RATIONAL*/0, /*BOOL*/0,
	/*CHAR*/1, /*STRING*/0, /*VOID*/0, /*POINTER*/0, /*EXCEPTION*/0, /*ERROR*/0
};
int IS_NUMERIC[TYPE_ERROR + 1] = {
	/*INT*/1, /*FLOAT*/1, /*NATURAL*/1, /*RATIONAL*/1, /*BOOL*/1,
	/*CHAR*/1, /*STRING*/0, /*VOID*/0, /*POINTER*/0, /*EXCEPTION*/0, /*ERROR*/0
};
int IS_SCALAR[TYPE_ERROR + 1] = {
	/*INT*/1, /*FLOAT*/1, /*NATURAL*/1, /*RATIONAL*/1, /*BOOL*/1,
	/*CHAR*/1, /*STRING*/0, /*VOID*/0, /*POINTER*/1, /*EXCEPTION*/0, /*ERROR*/0
};
int TYPE_WIDTH[TYPE_ERROR + 1] = {
	/*INT*/1, /*FLOAT*/3, /*NATURAL*/0, /*RATIONAL*/2, /*BOOL*/0,
	/*CHAR*/1, /*STRING*/-1, /*VOID*/-1, /*POINTER*/-1, /*EXCEPTION*/4, /*ERROR*/-1
};


void prod_error(semanticer* smt, char title[TITLE_MAX_LENGTH], char message[MESSAGE_MAX_LENGTH], char ident[TOKEN_MAX_LENGTH], int line, int col) 
{
	/*
		GETS: pointer to semanticer struct, error title, message, and additional var name, line and col.
		DOES: creates a new error and appends to smt->error. if ident not empty, concats it to error message
	*/
	int len = strlen(ident);
	char err_message[MESSAGE_MAX_LENGTH] = "\0";
	strcat(err_message, message);
	if (len > 0) {
		strcat(err_message, ident);
		strcat(err_message, "'");
	}
	
	err_append(smt->error, error(title, err_message, line, col));
}

type_kind wider_type(type_kind type1, type_kind type2)
{
	// error propagation — silently pass through
	if (type1 == TYPE_ERROR || type2 == TYPE_ERROR) return TYPE_ERROR;

	// non-numeric types have no width — can't widen
	if (TYPE_WIDTH[type1] < 0 || TYPE_WIDTH[type2] < 0) return TYPE_ERROR;

	// CHAR widens to INT
	if (type1 == TYPE_CHAR) type2 = TYPE_INT;
	if (type2 == TYPE_CHAR) type2 = TYPE_INT;

	return TYPE_WIDTH[type1] >= TYPE_WIDTH[type2] ? type1 : type2;
}
int type_can_contain(type_kind container, type_kind value)
{
	if (container == TYPE_ERROR || value == TYPE_ERROR) return 1;
	if (container == value) return 1;

	if (container == TYPE_EXCEPTION) return 1;

	// CHAR -> INT widening
	if (value == TYPE_CHAR && container == TYPE_INT) return 1;

	// BOOL -> numeric widening
	if (value == TYPE_BOOL) {
		if (TYPE_WIDTH[container] >= 0) return 1;
	}

	if (TYPE_WIDTH[container] < 0 || TYPE_WIDTH[value] < 0) return 0;

	return TYPE_WIDTH[container] >= TYPE_WIDTH[value];
}
int is_comparable(type_kind type1, type_kind type2)
{
	if (type1 == TYPE_ERROR || type2 == TYPE_ERROR) return 1;
	if (type1 == type2) return 1;
	if (IS_NUMERIC[type1] && IS_NUMERIC[type2]) return 1;
	return 0;
}
int is_printable(type_kind type)
{
	return IS_PRINTABLE[type];
}

symbol_link* create_symbol(char* name, semantic_kind kind, type_kind type, AST initializer, int scope_level, int line, int col, int offset)
{
	symbol_link* sym = (symbol_link*)malloc(sizeof(symbol_link));
	sym->name = strdup(name);

	sym->kind = kind;
	sym->type = type;

	sym->scope_level = scope_level;
	sym->is_global = scope_level == 0;

	sym->decl_line = line;
	sym->decl_col = col;
	sym->offset = offset;

	sym->is_param = 0;

	if (initializer)
	{
		sym->is_initialized = 1;
		sym->initializer = initializer;
		if (initializer->kind == NODE_LITERAL)
		{
			sym->init_const_val1 = initializer->value1;
			sym->init_const_val2 = initializer->value2;
			sym->is_init_const = 1;
		}
		else
			sym->is_init_const = 0;
	}
	else
	{
		sym->is_initialized = 0;
		sym->initializer = NULL;
	}
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
	else
	{
		scp->parent = NULL;
		scp->offset_next = 0;
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

void enter_scope(semanticer* smt)
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

void analyze(semanticer* smt, AST ast)
{
	//analyzes the current ast node via dipatching handlers
	handle handler = HANDLE_DISPATCH[ast->kind];
	handler(smt, ast);
}
void start_handler(semanticer* smt, AST ast)
{
	//handles proram start, calls for analysis of child
	analyze(smt, ast->children[0]);
}

void func_declare_handler(semanticer* smt, AST ast)
{
	//handles function declaration
	int i;
	int prev_infunc;
	type_kind prev_rettype;

	AST name_param = ast->children[0];   
	AST param_list = ast->children[1];  
	AST block = ast->children[2];   
	AST param;

	char* fname = name_param->children[1]->name;
	type_kind return_type = name_param->children[0]->type;

	if (symbol_exist(smt->current_scope, fname)) {
		prod_error(smt, "DECLARATION ERROR", "redeclaration of function '", fname, ast->line, ast->col);
	}

	symbol_link* fsym = create_symbol(fname, FUNCTION, return_type, NULL,
		smt->current_scope->level,
		ast->line, ast->col, 0);

	if (!param_list)
		fsym->param_count = 0;
	else
	{
		fsym->param_count = param_list->children_count;
		fsym->params = (param_info*)malloc(fsym->param_count * sizeof(param_info));
		for (i = 0; i < param_list->children_count; i++)
		{
			param = param_list->children[i];
			fsym->params[i].type = param->children[0]->type;
			fsym->params[i].name = param->children[1]->name;
		}
	}
	enter_symbol(smt->current_scope, fsym);
	enter_scope(smt);
	prev_infunc = smt->in_function;
	smt->in_function = 1;
	prev_rettype = smt->current_return_type;
	smt->current_return_type = return_type;
	for (i = 0; i < fsym->param_count; i++)
	{
		param = param_list->children[i];
		analyze(smt, param);
	}

	analyze(smt, block);

	smt->current_return_type = prev_rettype;
	smt->in_function = prev_infunc;
	exit_scope(smt);

	ast->type = return_type;
}
void var_declare_handler(semanticer* smt, AST ast)
{
	//handles variable declaration

	char* name;
	type_kind declared_type;
	type_kind init_type;
	AST decl = ast->children[0];
	AST initializer = NULL;

	if (decl->kind == NODE_PARAMETER) {
		declared_type = decl->children[0]->type;
		name = decl->children[1]->name;
	}
	else {
		name = decl->name;
		declared_type = ast->type;
	}

	if (symbol_exist(smt->current_scope, name)) 
	{
		prod_error(smt, "DECLARATION ERROR", "redeclaration of '", name, ast->line, ast->col);
	}
	init_type = TYPE_VOID;
	if (ast->children_count > 1 && ast->children[1] != NULL) {
		analyze(smt, ast->children[1]);
		initializer = ast->children[1];
		init_type = ast->children[1]->type;

		if (!type_can_contain(declared_type, init_type)) {
			prod_error(smt, "TYPE ERROR", "incompatible declare and initiation types for '", name, ast->line, ast->col);
		}
	}
	symbol_link* sym = create_symbol(name, VARIABLE, declared_type, initializer, smt->current_scope->level,
		ast->line, ast->col, smt->current_scope->offset_next);
	enter_symbol(smt->current_scope, sym);

	ast->type = declared_type;
}

void assignment_handler(semanticer* smt, AST ast)
{
	//handles assignment nodes 

	AST ident = ast->children[0];
	AST rhs = ast->children[1];
	type_kind rhs_type;

	symbol_link* sym = get_symbol(smt->current_scope, ident->name);
	if (!sym) {
		prod_error(smt, "ASSIGNMENT ERROR", "assignment to undeclared variable '", ident->name, ast->line, ast->col);
		ast->type = TYPE_ERROR;
	}
	else if (sym->kind == CONSTANT) {
		prod_error(smt, "ASSIGNMENT ERROR", "assignement to constant '", sym->name, sym->decl_line, sym->decl_col);
	}

	analyze(smt, rhs);
	rhs_type = rhs->type;

	if (!type_can_contain(sym->type, rhs_type)) {
		prod_error(smt, "TYPE ERROR", "invalid assignment type for variable '", sym->name, ast->line, ast->col);
		ast->type = TYPE_ERROR;
	}

	ast->type = rhs->type;  
}
void parameter_handler(semanticer* smt, AST ast)
{
	//handles parameter nodes

	AST ident = ast->children[1];
	AST type_node = ast->children[0];

	char* name = ident->name;
	type_kind ptype = type_node->type;

	if (symbol_exist(smt->current_scope, name)) {
		prod_error(smt, "SIGNATURE ERROR", "duplicate parameter name '", name, ast->line, ast->col);
		ast->type = TYPE_ERROR;
	}
	else {
		symbol_link* sym = create_symbol(name, PARAM, ptype, NULL,
			smt->current_scope->level,
			ast->line, ast->col,
			smt->current_scope->offset_next++);
		enter_symbol(smt->current_scope, sym);
		ast->type = type_node->type;
	}
}
void type_cast_handler(semanticer* smt, AST ast) 
{
	//handles type casting
	AST type = ast->children[0];
	AST expr = ast->children[1];
	
	analyze(smt, expr);

	if (!type_can_contain(type->type, expr->type))
	{
		prod_error(smt, "TYPE ERROR", "incompatible types conversion", "\0", ast->line, ast->col);
		ast->type = TYPE_ERROR;
	}
	else
	{
		ast->type = type->type;
	}
	return ast->type;
}
void stmt_list_handler(semanticer* smt, AST ast)
{
	//handles statement list node
	int i;
	for (i = 0; i < ast->children_count; i++)
	{
		if (ast->children[i])
			analyze(smt, ast->children[i]);
	}
}

void if_handler(semanticer* smt, AST ast)
{
	//handles if node

	AST cond = ast->children[0];
	AST then = ast->children[1];
	AST othrwise = ast->children[2];

	type_kind cond_type;

	analyze(smt, cond);
	cond_type = cond->type;
	if (!IS_SCALAR[cond_type]) {
		prod_error(smt, "TYPE ERROR", "condition must be a scalar type", "\0", ast->line, ast->col);
	}

	//block
	enter_scope(smt);
	analyze(smt, then);
	exit_scope(smt);

	//else
	if (othrwise != NULL) {
		enter_scope(smt);
		analyze(smt, othrwise); 
		exit_scope(smt);
	}
	ast->type = TYPE_VOID;
}
void loop_handler(semanticer* smt, AST ast)
{
	//handles loop node

	enter_scope(smt);
	smt->loop_depth++;

	if (ast->children_count == 2) { 
		AST cond = ast->children[0];
		AST block = ast->children[1];

		analyze(smt, cond);
		if (!IS_SCALAR[cond->type]) {
			prod_error(smt, "TYPE ERROR", "condition must be of scalar type", "\0", ast->line, ast->col);
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
			prod_error(smt, "TYPE ERROR", "loop bound must be an integer type", "\0", ast->line, ast->col);
		}

		analyze(smt, step);
		analyze(smt, block);
	}

	exit_scope(smt);
	smt->loop_depth--;
	ast->type = TYPE_VOID;
}

void return_handler(semanticer* smt, AST ast)
{
	//handles return node
	AST expr = ast->children[0];

	analyze(smt, expr);
	if (!type_can_contain(smt->current_return_type, expr->type))
		prod_error(smt, "TYPE ERROR", "incompatible return type", "\0", ast->line, ast->col);
	else
		ast->type = expr->type;
}
void break_handler(semanticer* smt, AST ast)
{
	//handles break node
	if (smt->loop_depth == 0){
		prod_error(smt, "CONTROLFLOW ERROR", "invalid break outside of a loop", "\0", ast->line, ast->col);
		ast->type = TYPE_ERROR;
	}
	else
		smt->ast->type = TYPE_VOID;
}
void pass_handler(semanticer* smt, AST ast)
{
	//handles pass node
	if (smt->loop_depth == 0)
	{
		prod_error(smt, "CONTROLFLOW ERROR", "invalid pass outside of a loop", "\0", ast->line, ast->col);
		ast->type = TYPE_ERROR;
	}
	else
		ast->type = TYPE_VOID;
}
void ident_handler(semanticer* smt, AST ast)
{
	//handles identifier node
	symbol_link* sym = get_symbol(smt->current_scope, ast->name);

	if (!sym) {
		prod_error(smt, "DECLARATION ERROR", "undefined identifier '", ast->name, ast->line, ast->col);
		ast->type = TYPE_ERROR;
	}
	else
		ast->type = sym->type;
}
void literal_handler(semanticer* smt, AST ast)
{
	//handles literal nodes
	if (ast->type == TYPE_ERROR) {
		prod_error(smt, "TYPE ERROR", "unknown literal type", "\0", ast->line, ast->col);
	}
}
void underline_handler(semanticer* smt, AST ast)
{

	smt->ast->type = TYPE_VOID;
}
void arithmetic_handler(semanticer* smt, AST ast)
{
	//handles arithmetic nodes
	AST left = ast->children[0];
	AST right = ast->children[1];
	
	analyze(smt, left);
	if (right)
	{
		analyze(smt, right);

		if (!IS_INTEGER[left->type] || !IS_INTEGER[right->type]) {
			prod_error(smt, "TYPE ERROR", "arithmetic operands must be integer types", "\0", ast->line, ast->col);

			ast->type = TYPE_ERROR;
		}
		else {
			ast->type = wider_type(left->type, right->type);
		}
	}
	else
	{
		ast->type = TYPE_INT;
	}
	
}

void logical_handler(semanticer* smt, AST ast)
{
	//handles logical nodes
	AST left = ast->children[0];
	AST right = ast->children[1];

	analyze(smt, left);
	analyze(smt, right);

	if (!IS_SCALAR[left->type] || !IS_SCALAR[right->type]) {
		prod_error(smt, "TYPE ERROR", "logical operators require scalar operands", "\0", ast->line, ast->col);
		ast->type = TYPE_ERROR;
	}

	ast->type = TYPE_BOOL;
}
void bitwise_handler(semanticer* smt, AST ast)
{
	//handles bitwise operations nodes
	AST left = ast->children[0];
	AST right = ast->children[1];

	analyze(smt, left);
	analyze(smt, right);

	if (!IS_INTEGER[left->type] || !IS_INTEGER[right->type]) {
		prod_error(smt, "TYPE ERROR", "bitwise operators require interer operands", "\0", ast->line, ast->col);
		ast->type = TYPE_ERROR;
	}
	else
		ast->type = wider_type(left->type, right->type);
}
void logical_not_handler(semanticer* smt, AST ast)
{
	//handles logical not (!)
	AST operand = ast->children[0];

	analyze(smt, operand);

	if (!IS_SCALAR[operand->type]) {
		prod_error(smt, "TYPE ERROR", "logical '!' requires a scalar operand", "\0", ast->line, ast->col);
		ast->type = TYPE_ERROR;
	}

	ast->type = TYPE_INT;
}
void bitwise_not_handler(semanticer* smt, AST ast)
{
	//handles bitwise not (!!)
	AST operand = ast->children[0];

	analyze(smt, operand);

	if (!IS_INTEGER[operand->type]) {
		prod_error(smt, "TYPE ERROR", "bitwise '!' requires an integer operand", "\0", ast->line, ast->col);
		ast->type = TYPE_ERROR;
	}
	else
		ast->type = operand->type;
}
void comparison_handler(semanticer* smt, AST ast)
{
	//handles comparisson nodes
	AST left = ast->children[0];
	AST right = ast->children[1];

	analyze(smt, left);
	analyze(smt, right);

	if (!is_comparable(left->type, right->type)) {
		prod_error(smt, "TYPE ERROR", "comparison between incompatible types", "\0", ast->line, ast->col);
		ast->type = TYPE_ERROR;
	}

	ast->type = TYPE_BOOL;  
}
void check_args(semanticer* smt, AST arg_list, symbol_link* func)
{
	int i, arg_count;
	AST arg;
	arg_count = arg_list ? arg_list->children_count: 0;
	if (arg_count != func->param_count) {
		prod_error(smt, "SIGNATURE ERROR", "argument number does not match", "\0", arg_list->line, arg_list->col);
		arg_list->type = TYPE_ERROR;
	}
	for (i = 0; i < arg_count; i++) {
		arg = arg_list->children[i];
		analyze(smt, arg);
		if (arg->kind == NODE_IDENT)
		{
			arg = arg->children[1];
		}

		if (arg->type != func->params[i].type)
			prod_error(smt, "TYPE ERROR", "argument type does not match for ",
				arg->children[0]->name, arg->line, arg->col);
	}
}
void func_call_handler(semanticer* smt, AST ast)
{
	//handles function call nod
	AST ident = ast->children[0];
	AST arg_list = ast->children[1];
	symbol_link* sym = get_symbol(smt->current_scope, ident->name);
	if (!sym) {
		prod_error(smt, "DECLARATION ERROR", "call to undeclared function '", ident->name, ast->line, ast->col);
		ast->type = TYPE_ERROR;
	}
	else if (sym->kind != FUNCTION) {
		prod_error(smt, "SIGNATUME ERROR", "function call with invalid function identifier '", ident->name, ast->line, ast->col);
		ast->type = TYPE_ERROR;
	}
	else {
		check_args(smt, arg_list, sym);
	}
	ast->type = sym->type;
}
void block_handler(semanticer* smt, AST ast)
{
	//handles block node
	for (int i = 0; i < ast->children_count; i++) {
		analyze(smt, ast->children[i]);
	}

	ast->type = TYPE_VOID;
}

void scan_handler(semanticer* smt, AST ast) 
{
	if (!is_printable(ast->type))
	{
		prod_error(smt, "TYPE ERROR", "scan type must be a scannable type- a printable type", "\0", ast->line, ast->col);
	}
}
void print_handler(semanticer* smt, AST ast) 
{
	AST expr = ast->children[0];
	analyze(smt, expr);
	if (!is_printable(expr->type))
	{
		prod_error(smt, "TYPE ERROR", "expr inside 'print' must be of a printable type", "\0", ast->line, ast->col);
		ast->type = TYPE_ERROR;
	}
	else
		ast->type = expr->type;
}

void init_dispatch_table()
{
	//fills the HANDLER DISPATCH lookup table
	HANDLE_DISPATCH[NODE_START] = start_handler;

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
	HANDLE_DISPATCH[NODE_SCAN] = scan_handler;
	HANDLE_DISPATCH[NODE_PRINT] = print_handler;

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



semanticer* init_semanticer(AST ast, char* error_file)
{
	semanticer* smt = (semanticer*)malloc(sizeof(semanticer));
	FILE* err = fopen(error_file, "w");
	if (smt)
	{
		smt->ast = ast;
		smt->error = err_list(err);
		smt->current_scope = init_scope(0, NULL);
		smt->in_function = 0;
		smt->loop_depth = 0;
		smt->current_return_type = TYPE_VOID;
	}
	else
		memory_error();
	return smt;
}

void semanticize(semanticer* smt)
{
	//semanticizes the input ast, calls for the recursive analysis of ast
	init_dispatch_table();
	analyze(smt, smt->ast);
}