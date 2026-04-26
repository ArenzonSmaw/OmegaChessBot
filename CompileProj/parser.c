#include "parser.h"
#include "grammar.h"
#include "slrgenerator.h"
#include <stdlib.h>
#include <string.h>

#pragma warning (disable:4996)

#define TERMINAL 1
#define NON_TERMINAL 0

#define START_STATE 0

#define RULES_NUM 73

items_arr rules;
int rule_index;
action** SLR_ACTION;
int** SLR_GOTO;
int states_count;

node_kind TERMINAL_TO_KIND[TERMINALS_COUNT];
void (*REDUCTION[RULES_NUM])(parser*, symbol);

void fill_terminal_to_kind()
{
	symbol i;
	for (i = 0; i < KIND_COUNT; i++)
	{
		TERMINAL_TO_KIND[i] = -1;
	}
	TERMINAL_TO_KIND[ID] = NODE_IDENT;
	TERMINAL_TO_KIND[UNDERLINE] = NODE_UNDERLINE;
	for (i = INT_LITERAL; i <= BOOL_LITERAL; i++)
	{
		TERMINAL_TO_KIND[i] = NODE_LITERAL;
	}
	TERMINAL_TO_KIND[CHR_LITERAL] = NODE_CHAR;
	TERMINAL_TO_KIND[STR_LITERAL] = NODE_STRING;

	TERMINAL_TO_KIND[AND] = NODE_BIT_AND;
	TERMINAL_TO_KIND[ANDAND] = NODE_LOG_AND;
	TERMINAL_TO_KIND[OR] = NODE_BIT_OR;
	TERMINAL_TO_KIND[OROR] = NODE_LOG_OR;
	TERMINAL_TO_KIND[TILDE_OR] = NODE_XOR;
	TERMINAL_TO_KIND[DBL_EQUALS] = NODE_LOG_EQUAL;
	TERMINAL_TO_KIND[RIGHT] = NODE_GREAT;
	TERMINAL_TO_KIND[DBL_RIGHT] = NODE_BIT_RIGHT;
	TERMINAL_TO_KIND[RIGHT_EQUALS] = NODE_GREAT_EQUAL;
	TERMINAL_TO_KIND[LEFT] = NODE_LESS;
	TERMINAL_TO_KIND[DBL_LEFT] = NODE_BIT_LEFT;
	TERMINAL_TO_KIND[LEFT_EQUALS] = NODE_LESS_EQUAL;
	TERMINAL_TO_KIND[NOT_EQUALS] = NODE_LOG_DIFFERENT;
	TERMINAL_TO_KIND[PLUS] = NODE_ADD;
	TERMINAL_TO_KIND[DBL_PLUS] = NODE_ADD;
	TERMINAL_TO_KIND[MINUS] = NODE_SUB;
	TERMINAL_TO_KIND[DBL_MINUS] = NODE_SUB;
	TERMINAL_TO_KIND[MULT] = NODE_MUL;
	TERMINAL_TO_KIND[DBL_MULT] = NODE_MUL;
	TERMINAL_TO_KIND[DIVIDE] = NODE_DIV;
	TERMINAL_TO_KIND[DBL_DIVIDE] = NODE_DIV;
	TERMINAL_TO_KIND[MOD] = NODE_MOD;
	TERMINAL_TO_KIND[DBL_MOD] = NODE_QUO;
	TERMINAL_TO_KIND[NOT] = NODE_LOG_NOT;
	TERMINAL_TO_KIND[NOTNOT] = NODE_BIT_NOT;
}

void free_all(info* ptr)
{
	free(ptr->node);
	free(ptr);
}
void syntax_error(parser* prsr) 
{
	token tkn = prsr->input[prsr->index];
	char lexeme[TOKEN_MAX_LENGTH] = "'";
	strcat(lexeme, tkn.lexeme);
	strcat(lexeme, "' unexpected");
	err_append(prsr->err_lst, error("Syntax Error", lexeme, tkn.line, tkn.col));
}
void shift(parser* prsr) 
{
	token* tkn = &(prsr->input[prsr->index]);
	node_kind kind = TERMINAL_TO_KIND[tkn->type];
	AST node = create_leaf(tkn, kind);
	prsr->state = SLR_GOTO[tkn->type][prsr->state];
	push(&(prsr->stck), tkn, node, prsr->state);
	prsr->index++;
}

void reduce_parenthesized_expr(parser* prsr, symbol lhs)
{
	//FACTOR -> '(' + EXPRESSION + ')'
	info* opened_bracket, * closed_bracket, * expr;
	int prev_state;
	closed_bracket = pop(&(prsr->stck));
	expr = pop(&(prsr->stck));
	opened_bracket = pop(&(prsr->stck));
	prev_state = top(&(prsr->stck))->state;

	prsr->state = SLR_GOTO[lhs][prev_state];
	push(&(prsr->stck), expr->tkn, expr->node, prsr->state);


	free_all(closed_bracket);
	free_all(opened_bracket);
	free(expr);
}
void reduce_function_call(parser* prsr, symbol lhs)
{
	//FACTOR -> id + '(' + arg_list + ')'
	info* cl_bracket, * exprs, * op_bracket, * id;
	int prev_state;
	AST node;
	cl_bracket = pop(&(prsr->stck));
	exprs = pop(&(prsr->stck));
	op_bracket = pop(&(prsr->stck));
	id = pop(&(prsr->stck));
	prev_state = top(&(prsr->stck))->state;
	
	node = init_ast(NULL, NODE_FUNC_CALL, 2);
	insert_son(node, id, 0);
	insert_son(node, exprs, 1);

	prsr->state = SLR_GOTO[lhs][prev_state];
	push(&(prsr->stck), NULL, node, prsr->state);

	free_all(cl_bracket);
	free_all(op_bracket);
	free(id);
	free(exprs);
}
void reduce_cast(parser* prsr, symbol lhs)
{
	//FACTOR -> ':' + type + ':' + id
	info* factor, * r_col, * type, * l_col;
	int prev_state;
	AST node;
	factor = pop(&(prsr->stck));
	r_col = pop(&(prsr->stck));
	type = pop(&(prsr->stck));
	l_col = pop(&(prsr->stck));
	prev_state = top(&(prsr->stck))->state;

	node = init_ast(NULL, NODE_TYPE_CAST, 2);
	insert_son(node, type, 0);
	insert_son(node, factor, 1);

	prsr->state = SLR_GOTO[lhs][prev_state];

	push(&(prsr->stck), NULL, node, prsr->state);

	free_all(r_col);
	free_all(l_col);
	free(type);
	free(factor);
}
void reduce_prefix_arith(parser* prsr, symbol lhs)
{
	//FACTOR -> '-' + FACTOR
	info* factor, * operator;
	int prev_state;
	factor = pop(&(prsr->stck));
	operator = pop(&(prsr->stck));
	prev_state = top(&(prsr->stck))->state;

	alloc_children(operator->node, 1);
	add_son(operator->node, factor->node);
	prsr->state = SLR_GOTO[lhs][prev_state];
	push(&(prsr->stck), operator->tkn, operator->node, prsr->state);

	free(factor);
	free(operator);
}
void reduce_postfix_arith(parser* prsr, symbol lhs)
{
	//FACTOR -> FACTOR + '!!'
	info* operator,* factor;
	int prev_state;
	operator = pop(&(prsr->stck));
	factor = pop(&(prsr->stck));
	prev_state = top(&(prsr->stck))->state;

	alloc_children(operator->node, 1);
	add_son(operator->node, factor->node);

	prsr->state = SLR_GOTO[lhs][prev_state];
	push(&(prsr->stck), operator->tkn, operator->node, prsr->state);

	free(factor);
	free(operator);
}
void reduce_basic_rec_case(parser* prsr, symbol lhs)
{
	//STATEMENT -> EXPRESSION
	info* rhs = pop(&(prsr->stck));
	int prev_state = top(&(prsr->stck))->state;

	prsr->state = SLR_GOTO[lhs][prev_state];
	push(&(prsr->stck), rhs->tkn, rhs->node, prsr->state);

	free(rhs);
}
void reduce_binary(parser* prsr, symbol lhs)
{
	//EXPRESSION -> EXPRESSION + TERM
	info* opnd1, * opnd2, * oprt;
	int prev_state;
	opnd2 = pop(&(prsr->stck));
	oprt = pop(&(prsr->stck));
	opnd1 = pop(&(prsr->stck));
	prev_state = top(&(prsr->stck))->state;

	alloc_children(oprt->node, 2);
	insert_son(oprt->node, opnd1->node, 0);
	insert_son(oprt->node, opnd2->node, 1);

	prsr->state = SLR_GOTO[lhs][prev_state];
	push(&(prsr->stck), oprt->tkn, oprt->node, prsr->state);

	free(opnd1);
	free(opnd2);
	free(oprt);
}
void reduce_list(parser* prsr, symbol lhs)
{
	//PARAM_LIST -> PARAM_LIST + ',' + PARAM
	info* item, * comma, * list;
	int prev_state;
	item = pop(&(prsr->stck));
	comma = pop(&(prsr->stck));
	list = pop(&(prsr->stck));
	prev_state = top(&(prsr->stck))->state;

	realloc_children(list->node, list->node->children_size + 1);
	insert_son(list->node, item, list->node->children_count++);

	prsr->state = SLR_GOTO[lhs][prev_state];
	push(&(prsr->stck), list->tkn, list->node, prsr->state);

	free(item);
	free(list);
	free_all(comma);
}
void reduce_param(parser* prsr, symbol lhs)
{
	// PARAM -> id + ':' + type
	info* type, * colon, * id;
	int prev_state;
	AST node;
	type = pop(&(prsr->stck));
	colon = pop(&(prsr->stck));
	id = pop(&(prsr->stck));
	prev_state = top(&(prsr->stck))->state;
	node = init_ast(NULL, NODE_PARAMETER, 2);
	insert_son(node, type, 0);
	insert_son(node, id, 1);

	prsr->state = SLR_GOTO[lhs][prev_state];
	push(&(prsr->stck), NULL, node, prsr->state);

	free_all(colon);
	free(type);
	free(id);
}
void reduce_use(parser* prsr, symbol lhs)
{
	//USE -> USE + ':' + STATEMENT
	info* semcol, * stmt, * colon, * use;
	int prev_state;
	AST node;
	semcol = pop(&(prsr->stck));
	stmt = pop(&(prsr->stck));
	colon = pop(&(prsr->stck)); 
	use = pop(&(prsr->stck));
	prev_state = top(&(prsr->stck))->state;

	node = init_ast(NULL, NODE_USE_DECLARE, 1);
	add_son(node, stmt->node);

	prsr->state = SLR_GOTO[lhs][prev_state];
	push(&(prsr->stck), NULL, node, prsr->state);

	free_all(semcol);
	free_all(colon);
	free_all(use);
	free(stmt);
}
void reduce_func_declare(parser* prsr, symbol lhs)
{
	//func_declare -> declare + PARAM + '(' + PARAM_LIST + ')'
	info* semcol = pop(&(prsr->stck)),
		* block = pop(&(prsr->stck)),
		* cl_bracket = pop(&(prsr->stck)),
		* param_list = pop(&(prsr->stck)),
		* op_bracket = pop(&(prsr->stck)),
		* param = pop(&(prsr->stck)),
		* declare = pop(&(prsr->stck));
	int prev_state = top(&(prsr->stck))->state;

	AST node = init_ast(NULL, NODE_FUNC_DECLARE, 3);
	insert_son(node, param, 0);
	insert_son(node, param_list, 1);
	insert_son(node, block, 2);

	prsr->state = SLR_GOTO[lhs][prev_state];
	push(&(prsr->stck), NULL, node, prsr->state);
	
	free_all(semcol);
	free_all(cl_bracket);
	free_all(op_bracket);
	free_all(declare);
	free(block);
	free(param_list);
	free(param);
}
void reduce_var_declare_full(parser* prsr, symbol lhs)
{
	//full declare -> declare + PARAM + '=' + EXPRESSION
	info* semcol = pop(&(prsr->stck)),
		* expr = pop(&(prsr->stck)),
		* eq = pop(&(prsr->stck)),
		* param = pop(&(prsr->stck)),
		* declare = pop(&(prsr->stck));
	int prev_state = top(&(prsr->stck))->state;

	AST node = init_ast(NULL, NODE_VAR_DECLARE, 2);
	insert_son(node, param, 0);
	insert_son(node, expr, 1);

	prsr->state = SLR_GOTO[lhs][prev_state];
	push(&(prsr->stck), NULL, node, prsr->state);

	free_all(semcol);
	free_all(eq);
	free_all(declare);
	free(param);
	free(expr);
}
void reduce_var_declare_half(parser* prsr, symbol lhs)
{
	//half declare -> declare + id + '=' + EXPRESSION
	info* semcol = pop(&(prsr->stck)),
		* expr = pop(&(prsr->stck)),
		* eq = pop(&(prsr->stck)),
		* id = pop(&(prsr->stck)),
		* declare = pop(&(prsr->stck));
	int prev_state = top(&(prsr->stck))->state;
	AST node = init_ast(NULL, NODE_VAR_DECLARE, 2);
	insert_son(node, id, 0);
	insert_son(node, expr, 1);

	prsr->state = SLR_GOTO[lhs][prev_state];
	push(&(prsr->stck), NULL, node, prsr->state);

	free_all(declare);
	free_all(eq);
	free_all(semcol);
	free(expr);
	free(id);
}
void reduce_var_declare_type(parser* prsr, symbol lhs)
{
	//declare type -> declare + PARAM
	info* semcol = pop(&(prsr->stck)),
		* param = pop(&(prsr->stck)),
		* declare = pop(&(prsr->stck));
	int prev_state = top(&(prsr->stck))->state;
	AST node = init_ast(NULL, NODE_VAR_DECLARE, 2);
	insert_son(node, param, 0);

	prsr->state = SLR_GOTO[lhs][prev_state];
	push(&(prsr->stck), NULL, node, prsr->state);

	free_all(semcol);
	free_all(declare);
	free(param);
}
void reduce_var_declare_empty(parser* prsr, symbol lhs)
{
	//declare empty -> declare + id
	info* semcol = pop(&(prsr->stck)),
		* id = pop(&(prsr->stck)),
		* declare = pop(&(prsr->stck));
	int prev_state = top(&(prsr->stck))->state;
	AST node = init_ast(NULL, NODE_VAR_DECLARE, 2);
	insert_son(node, id, 0);

	prsr->state = SLR_GOTO[lhs][prev_state];
	push(&(prsr->stck), NULL, node, prsr->state);

	free_all(semcol);
	free_all(declare);
	free(id);
}

void reduce_loop_while(parser* prsr, symbol lhs)
{
	//loop -> loop + FACTOR + BLOCK 
	info* block = pop(&(prsr->stck)),
		* factor = pop(&(prsr->stck)),
		* loop = pop(&(prsr->stck));
	int prev_state = top(&(prsr->stck))->state;
	loop->node = init_ast(loop->tkn, NODE_LOOP, 2);
	insert_son(loop->node, factor, 0);
	insert_son(loop->node, block, 1);

	prsr->state = SLR_GOTO[lhs][prev_state];
	push(&(prsr->stck), loop->tkn, loop->node, prsr->state);

	free(loop);
	free(factor);
	free(block);
}
void reduce_loop_for(parser* prsr, symbol lhs)
{
	//loop -> loop + '(' + declare + PARAM + '->' + EXPRESSION + ';' + STATEMENT + ')' + BLOCK
	info* block = pop(&(prsr->stck)),
		* clbrck = pop(&(prsr->stck)),
		* stmt = pop(&(prsr->stck)),
		* semcol = pop(&(prsr->stck)),
		* expr = pop(&(prsr->stck)),
		* arrow = pop(&(prsr->stck)),
		* param = pop(&(prsr->stck)),
		* declare = pop(&(prsr->stck)), //
		* opbrck = pop(&(prsr->stck)),
		* loop = pop(&(prsr->stck));
	int prev_state = top(&(prsr->stck))->state;
	AST iterator = init_ast(NULL, NODE_VAR_DECLARE, 2);
	insert_son(iterator, param, 0);
	insert_son(iterator, expr, 1);
	loop->node = init_ast(loop->tkn, NODE_LOOP, 3);
	insert_son(loop->node, iterator, 0);
	insert_son(loop->node, stmt, 1);
	insert_son(loop->node, block, 2);

	prsr->state = SLR_GOTO[lhs][prev_state];
	push(&(prsr->stck), loop->tkn, loop->node, prsr->state);

	free_all(opbrck);
	free_all(declare);
	free_all(arrow);
	free_all(semcol);
	free_all(clbrck);
	free(loop);
	free(param);
	free(expr);
	free(stmt);
	free(block);
}
void reduce_block(parser* prsr, symbol lhs)
{
	// BLOCK -> '{' + STMT_LIST + '}'
	info* clbrck = pop(&(prsr->stck)),
		* stmt_lst = pop(&(prsr->stck)),
		* opbrck = pop(&(prsr->stck));
	int prev_state = top(&(prsr->stck))->state;

	prsr->state = SLR_GOTO[lhs][prev_state];
	push(&(prsr->stck), stmt_lst->tkn, stmt_lst->node, prsr->state);

	free_all(clbrck);
	free_all(opbrck);
	free(stmt_lst);
}

void reduce_if(parser* prsr, symbol lhs)
{
	// IF -> if + FACTOR + BLOCK
	info* block = pop(&(prsr->stck)),
		* factor = pop(&(prsr->stck)),
		* ifkw = pop(&(prsr->stck));
	int prev_state = top(&(prsr->stck))->state;
	alloc_children(ifkw->node, 3);
	insert_son(ifkw->node, factor->node, 0);
	insert_son(ifkw->node, block->node, 1);

	prsr->state = SLR_GOTO[lhs][prev_state];
	push(&(prsr->stck), ifkw->tkn, ifkw->node, prsr->state);

	free(ifkw);
	free(factor);
	free(block);
}
void reduce_else_if(parser* prsr, symbol lhs)
{
	//ELSE IF -> IF + else + FACTOR + BLOCK
	info* block = pop(&(prsr->stck)),
		* factor = pop(&(prsr->stck)),
		* elskw = pop(&(prsr->stck)),
		* ifkw = pop(&(prsr->stck));
	int prev_state = top(&(prsr->stck))->state;
	AST ifstmt = ifkw->node;

	while (ifstmt->children[2] != NULL)
		ifstmt = ifstmt->children[2];

	alloc_children(elskw->node, 3);
	insert_son(elskw->node, factor->node, 0);
	insert_son(elskw->node, block->node, 1);
	insert_son(ifstmt, elskw->node, 2);

	prsr->state = SLR_GOTO[lhs][prev_state];
	push(&(prsr->stck), elskw->tkn, elskw->node, prsr->state);

	free(elskw);
	free(factor);
	free(block);
	free(ifkw);
}
void reduce_else(parser* prsr, symbol lhs)
{
	//ELSE -> IF + else + BLOCK
	info* block = pop(&(prsr->stck)),
		* elskw = pop(&(prsr->stck)),
		* ifkw = pop(&(prsr->stck));
	int prev_state = top(&(prsr->stck))->state;
	AST ifstmt = ifkw->node;
	while (ifstmt->children[2] != NULL)
		ifstmt = ifstmt->children[2];
	insert_son(ifstmt, block->node, 2);

	prsr->state = SLR_GOTO[lhs][prev_state];
	push(&(prsr->stck), ifkw->tkn, ifstmt, prsr->state);

	free_all(elskw);
	free(block);
	free(ifkw);
}

void reduce_assign_eq(parser* prsr, symbol lhs)
{
	//assign eq = id + '=' + EXPRESSION
	info* semcol = pop(&(prsr->stck)),
		* expr = pop(&(prsr->stck)),
		* eq = pop(&(prsr->stck)),
		* param = pop(&(prsr));
	int prev_state = top(&(prsr->stck))->state;
	AST node = init_ast(NULL, NODE_ASSIGNMENT, 2);
	insert_son(node, param->node, 0);
	insert_son(node, expr->node, 1);

	prsr->state = SLR_GOTO[lhs][prev_state];
	push(&(prsr->stck), NULL, node, prsr->state);

	free_all(semcol);
	free_all(eq);
	free(expr);
	free(param);
}
void reduce_assign_inc_bin(parser* prsr, symbol lhs)
{
	//assign inc bin -> id + '++' + EXPRESSION + ';'
	info* semcol = pop(&(prsr->stck)),
		* expr = pop(&(prsr->stck)),
		* oprt = pop(&(prsr->stck)),
		* id = pop(&(prsr->stck));
	int prev_state = top(&(prsr->stck))->state;

	AST node = init_ast(NULL, NODE_ASSIGNMENT, 2);
	AST incr = init_ast(NULL, TERMINAL_TO_KIND[oprt->tkn->type], 2);
	insert_son(incr, id, 0);
	insert_son(incr, expr, 1);
	insert_son(node, id, 0);
	insert_son(node, incr, 1);

	prsr->state = SLR_GOTO[lhs][prev_state];
	push(&(prsr->stck), NULL, node, prsr->state);

	free_all(expr);
	free_all(semcol);
	free(id);
	free(expr);
}
void reduce_assign_inc_unary(parser* prsr, symbol lhs)
{
	// assign inc unary -> id + '**' + ';' 
	info* semcol = pop(&(prsr->stck)),
		* oprt = pop((prsr->stck)),
		* id = pop(&(prsr->stck));
	int prev_state = top(&(prsr->stck))->state;
	node_kind kind = TERMINAL_TO_KIND[oprt->tkn->type];
	AST node = init_ast(NULL, NODE_ASSIGNMENT, 2);
	AST incr = init_ast(NULL, kind, 2);
	token* one = (token*)malloc(sizeof(token));
	if (kind == NODE_ADD || kind == NODE_SUB)
		*one = (token){ "1", 1, INT_LITERAL, oprt->tkn->line, oprt->tkn->col };
	else
		*one = (token){ "2", 2, INT_LITERAL, oprt->tkn->line, oprt->tkn->col };
	insert_son(incr, id, 0);
	insert_son(incr, create_leaf(one, LITERAL), 1);
	insert_son(node, id, 0);
	insert_son(node, incr, 1);

	prsr->state = SLR_GOTO[lhs][prev_state];
	push(&(prsr->stck), NULL, node, prsr->state);

	free_all(semcol);
	free_all(oprt);
	free(id);
	free(one);

}

void reduce_stmt(parser* prsr, symbol lhs)
{
	//STATEMENT -> EXPRESSION + ';'
	info* semcol = pop(&(prsr->stck)),
		* expr = pop(&(prsr->stck));
	int prev_state = top(&(prsr->stck))->state;

	prsr->state = SLR_GOTO[lhs][prev_state];
	push(&(prsr->stck), expr->tkn, expr->node, prsr->state);

	free_all(semcol);
	free(expr);
}
void reduce_stmt_list(parser* prsr, symbol lhs)
{
	//STMT_LIST -> STMT_LIST + STATEMENT
	info* stmt = pop(&(prsr->stck)),
		* stmt_lst = pop(&(prsr->stck));
	int prev_state = top(&(prsr->stck))->state;

	realloc_children(stmt_lst, stmt_lst->node->children_count + 1);
	add_son(stmt_lst->node, stmt->node);

	prsr->state = SLR_GOTO[lhs][prev_state];
	push(&(prsr->stck), stmt_lst->tkn, stmt_lst->node, prsr->state);

	free(stmt);
	free(stmt_lst);
}
void reduce_return(parser* prsr, symbol lhs)
{
	//RETURN -> return + EXPRESSION + ';'
	info* semcol = pop(&(prsr->stck)),
		* expr = pop(&(prsr->stck)),
		* ret = pop(&(prsr->stck));
	int prev_state = top(&(prsr->stck))->state;
	AST node = init_ast(NULL, NODE_RETURN, 1);
	insert_son(node, expr->node, 0);

	prsr->state = SLR_GOTO[lhs][prev_state];
	push(&(prsr->stck), NULL, node, prsr->state);

	free_all(semcol);
	free_all(ret);
	free(expr);
}
void reduce_check(parser* prsr, symbol lhs)
{
	//CHECK -> check + '(' + EXPRESSION + ',' + id + ')' + BLOCK
	info* block = pop(&(prsr->stck)),
		* clbrck = pop(&(prsr->stck)),
		* id = pop(&(prsr->stck)),
		* comma = pop(&(prsr->stck)),
		* expr = pop(&(prsr->stck)),
		* opbrck = pop(&(prsr->stck)),
		* check = pop(&(prsr->stck));
	int prev_state = top(&(prsr->stck))->state;

	alloc_children(check->node, 4);
	insert_son(check->node, expr->node, 0);
	insert_son(check->node, id->node, 1);
	insert_son(check->node, block, 2);

	prsr->state = SLR_GOTO[lhs][prev_state];
	push(&(prsr->stck), check->tkn, check->node, prsr->state);

	free_all(opbrck);
	free_all(comma);
	free_all(clbrck);
	free(check);
	free(id);
	free(expr);
	free(block);
}

void fill_reduction_tbl()
{
	int i;
	//single term reduction
	for (i = 0; i <= 9; i++)
		REDUCTION[i] = reduce_basic_rec_case;
	REDUCTION[17] = REDUCTION[18] = REDUCTION[28] = REDUCTION[31] = REDUCTION[38] = REDUCTION[41] = reduce_basic_rec_case;
	for (i = 43; i <= 46; i++)
		REDUCTION[i] = reduce_basic_rec_case;
	REDUCTION[61] = REDUCTION[65] = REDUCTION[72] = reduce_basic_rec_case;

	// operand operator operand 
	for (i = 19; i <= 27; i++)
		REDUCTION[i] = reduce_binary;
	for (i = 32; i <= 37; i++)
		REDUCTION[i] = reduce_binary;
	REDUCTION[29] = REDUCTION[30] = REDUCTION[39] = REDUCTION[40] = reduce_binary;

	//prefix to factor
	REDUCTION[13] = REDUCTION[14] = reduce_prefix_arith;
	//postfix to factor
	REDUCTION[15] = reduce_postfix_arith;
	//bracketed expression
	REDUCTION[11] = reduce_parenthesized_expr;
	//casting
	REDUCTION[12] = reduce_cast;

	//param
	REDUCTION[63] = reduce_param;

	//something_list
	REDUCTION[16] = REDUCTION[64] = reduce_list;
	//stmt_list
	REDUCTION[60] = reduce_stmt_list;

	//function call
	REDUCTION[10] = reduce_function_call;

	//declaration
	REDUCTION[66] = reduce_use;
	REDUCTION[67] = reduce_var_declare_type;
	REDUCTION[68] = reduce_var_declare_full;
	REDUCTION[69] = reduce_var_declare_half;
	REDUCTION[70] = reduce_var_declare_empty;
	REDUCTION[71] = reduce_func_declare;

	// conditionals
	REDUCTION[55] = reduce_if;
	REDUCTION[56] = reduce_else_if;
	REDUCTION[57] = reduce_else;

	//one word statements
	REDUCTION[42] = REDUCTION[51] = REDUCTION[52] = reduce_stmt;

	//loop
	REDUCTION[58] = reduce_loop_for;
	REDUCTION[59] = reduce_loop_while;

	//assignments
	REDUCTION[47] = REDUCTION[48] = reduce_assign_eq;
	REDUCTION[49] = reduce_assign_inc_bin;
	REDUCTION[50] = reduce_assign_inc_unary;

	//block
	REDUCTION[62] = reduce_block;

	//return
	REDUCTION[53] = reduce_return;

	//check
	REDUCTION[54] = reduce_check;
	
}

void reduce(parser* prsr, int rule_num) 
{
	int rule_idx = -1 * rule_num - 1;
	symbol lhs = rules[rule_idx].lhs.symbol;
	void (*reduce_func)(parser*, symbol) = REDUCTION[rule_idx];
	
	reduce_func(prsr, lhs);
}

void init_rules_arr(int rules_count)
{
	rules = (items_arr)malloc(sizeof(item_set) * rules_count);
	rule_index = 0;
}
void add_rule(item lhs, item rhs[], int length)
{
	int i;
	rules[rule_index].lhs = lhs;

	rules[rule_index].rhs = (item*)malloc(length * sizeof(item));
	for (i = 0; i < length; i++)
		rules[rule_index].rhs[i] = rhs[i];

	rules[rule_index].length = length;
	rules[rule_index].pos = 0;
	rules[rule_index].rule_num = -1 * (rule_index + 1);
	rule_index++;
}

void fill_rules_arr()
{
	const item
		literal = {LITERAL, NON_TERMINAL},
		param = {PARAMETER, NON_TERMINAL },
		param_list = { PARAM_LIST, NON_TERMINAL },
		factor = {FACTOR, NON_TERMINAL},
		arg_list = {ARG_LIST, NON_TERMINAL},
		term = {TERM, NON_TERMINAL},
		arith = {ARITH_EXPR, NON_TERMINAL},
		compr = {CMPR_EXPR, NON_TERMINAL},
		logic = {LOGIC_EXPR, NON_TERMINAL},
		expr = {EXPRESSION, NON_TERMINAL},
		incr = {INCREMENTAL, NON_TERMINAL},
		stmt = {STATEMENT, NON_TERMINAL },
		stmt_list = {STMT_LIST, NON_TERMINAL },
		block = {BLOCK, NON_TERMINAL },
		prgrm = {PROGRAM, NON_TERMINAL},

		bool_lit = {BOOL_LITERAL, TERMINAL},
		int_lit = {INT_LITERAL, TERMINAL},
		flt_lit = {FLOAT_LITERAL, TERMINAL },
		rat_lit = {RAT_LITERAL, TERMINAL },
		chr_lit = {CHR_LITERAL, TERMINAL },
		str_lit = {STR_LITERAL, TERMINAL },
		underline = {UNDERLINE, TERMINAL },
		id = {ID, TERMINAL },
		type = {TYPE, TERMINAL },
		unknwn = {UNKNOWN_OPERATOR, TERMINAL },
		and = {AND, TERMINAL },
		andand = {ANDAND, TERMINAL },
		or = {OR, TERMINAL },
		oror = {OROR, TERMINAL },
		tilde_or = {TILDE_OR, TERMINAL },
		eq = {EQUALS, TERMINAL },
		eqeq = {DBL_EQUALS, TERMINAL },
		right = {RIGHT, TERMINAL},
		rgtrgt = {DBL_RIGHT, TERMINAL },
		rgt_eq = {RIGHT_EQUALS, TERMINAL },
		left = {LEFT, TERMINAL },
		lftlft = {DBL_LEFT, TERMINAL },
		lft_eq = {LEFT_EQUALS, TERMINAL },
		exclam = {NOT, TERMINAL },
		notnot = {NOTNOT, TERMINAL },
		not_eq = {NOT_EQUALS, TERMINAL },
		plus = {PLUS, TERMINAL },
		plspls = {DBL_PLUS, TERMINAL },
		minus = {MINUS, TERMINAL },
		mnsmns = {DBL_MINUS, TERMINAL },
		mult = {MULT, TERMINAL },
		mltmlt = {DBL_MULT, TERMINAL },
		divide = {DIVIDE, TERMINAL },
		divdiv = {DBL_DIVIDE, TERMINAL },
		mod = {MOD, TERMINAL },
		modmod = {DBL_MOD, TERMINAL },
		colon = {COLON, TERMINAL },
		arrow = {ARROW, TERMINAL },

		semcol = {SEMICOLON, TERMINAL },
		comma = {COMMA, TERMINAL },
		opbrck = {OP_RNDBRACKET, TERMINAL },
		clbrck = {CL_RNDBRACKET, TERMINAL },
		opcurl = {OP_CRLBRACKET, TERMINAL },
		clcurl = {CL_CRLBRACKET, TERMINAL },
		opsqr = {OP_SQRBRACKET, TERMINAL },
		clsqr = {CL_SQRBRACKET, TERMINAL },
		ret = {RETURN, TERMINAL },
		brk = {BREAK, TERMINAL },
		pss = {PASS, TERMINAL },
		loop = {LOOP, TERMINAL },
		decl = {DECLARE, TERMINAL },
		use = {USE, TERMINAL },

		ifky = {IF, TERMINAL },
		elsky = {ELSE, TERMINAL},
		chck = {CHECK,TERMINAL },
		excp = {EXCEPTION, TERMINAL },
		grammar_start = {S_TAG, NON_TERMINAL}
	;
	{
		init_rules_arr(RULES_NUM);
		add_rule(grammar_start, (item[1]) { prgrm }, 1);
		add_rule(literal, (item[1]) { bool_lit }, 1);
		add_rule(literal, (item[1]) { int_lit }, 1);
		add_rule(literal, (item[1]) { flt_lit }, 1);
		add_rule(literal, (item[1]) { rat_lit }, 1);
		add_rule(literal, (item[1]) { chr_lit }, 1);
		add_rule(literal, (item[1]) { str_lit }, 1);
		add_rule(factor, (item[1]) { literal }, 1);
		add_rule(factor, (item[1]) { underline }, 1);
		add_rule(factor, (item[1]) { id }, 1);
		add_rule(factor, (item[4]) { id, opbrck, arg_list, clbrck }, 4);
		add_rule(factor, (item[3]) { opbrck, expr, clbrck }, 3);
		add_rule(factor, (item[4]) { colon, type, colon, factor }, 4);
		add_rule(factor, (item[2]) { exclam, factor }, 2);
		add_rule(factor, (item[2]) { minus, factor }, 2);
		add_rule(factor, (item[2]) { factor, notnot }, 2);
		add_rule(arg_list, (item[3]) { arg_list, comma, expr }, 3);
		add_rule(arg_list, (item[1]) { expr }, 1);
		add_rule(term, (item[1]) { factor }, 1);
		add_rule(term, (item[3]) { term, mult, factor }, 3);
		add_rule(term, (item[3]) { term, divide, factor }, 3);
		add_rule(term, (item[3]) { term, mod, factor }, 3);
		add_rule(term, (item[3]) { term, modmod, factor }, 3);
		add_rule(term, (item[3]) { term, rgtrgt, factor }, 3);
		add_rule(term, (item[3]) { term, lftlft, factor }, 3);
		add_rule(term, (item[3]) { term, and, factor }, 3);
		add_rule(term, (item[3]) { term, tilde_or, factor }, 3);
		add_rule(term, (item[3]) { term, or , factor }, 3);
		add_rule(arith, (item[1]) { term }, 1);
		add_rule(arith, (item[3]) { arith, plus, term }, 3);
		add_rule(arith, (item[3]) { arith, minus, term }, 3);
		add_rule(compr, (item[1]) { arith }, 1);
		add_rule(compr, (item[3]) { compr, right, arith }, 3);
		add_rule(compr, (item[3]) { compr, rgt_eq, arith }, 3);
		add_rule(compr, (item[3]) { compr, left, arith }, 3);
		add_rule(compr, (item[3]) { compr, lft_eq, arith }, 3);
		add_rule(compr, (item[3]) { compr, eqeq, arith }, 3);
		add_rule(compr, (item[3]) { compr, not_eq , arith }, 3);
		add_rule(logic, (item[1]) { compr }, 1);
		add_rule(logic, (item[3]) { logic, andand, compr }, 3);
		add_rule(logic, (item[3]) { logic, oror, compr }, 3);
		add_rule(expr, (item[1]) { logic }, 1);
		add_rule(stmt, (item[2]) { expr, semcol }, 2);
		add_rule(incr, (item[1]) { plspls }, 1);
		add_rule(incr, (item[1]) { mnsmns }, 1);
		add_rule(incr, (item[1]) { mltmlt }, 1);
		add_rule(incr, (item[1]) { divdiv }, 1);
		add_rule(stmt, (item[4]) { id, eq, expr, semcol }, 4);
		add_rule(stmt, (item[7]) { colon, type, colon, id, eq, expr, semcol }, 7);
		add_rule(stmt, (item[4]) { id, incr, expr, semcol }, 4);
		add_rule(stmt, (item[3]) { id, incr, semcol }, 3);
		add_rule(stmt, (item[2]) { brk, semcol }, 2);
		add_rule(stmt, (item[2]) { pss, semcol }, 2);
		add_rule(stmt, (item[3]) { ret, expr, semcol }, 3);
		add_rule(stmt, (item[7]) { chck, opbrck, expr, comma, id, clbrck, block }, 7);
		add_rule(stmt, (item[3]) { ifky, factor, block }, 3);
		add_rule(stmt, (item[3]) { elsky, factor, block }, 3);
		add_rule(stmt, (item[2]) { elsky, block }, 2);
		add_rule(stmt, (item[10]) { loop, opbrck, decl, param, arrow, expr, semcol, stmt, clbrck, block }, 10);
		add_rule(stmt, (item[3]) { loop, factor, block }, 3);
		add_rule(stmt_list, (item[2]) { stmt_list, stmt }, 2);
		add_rule(stmt_list, (item[1]) { stmt }, 1);
		add_rule(block, (item[3]) { opbrck, stmt_list, clbrck }, 3);
		add_rule(param, (item[3]) { id, colon, type }, 3);
		add_rule(param_list, (item[3]) { param_list, comma, param }, 3);
		add_rule(param_list, (item[1]) { param }, 1);
		add_rule(stmt, (item[4]) { use, colon, stmt, semcol }, 4);
		add_rule(stmt, (item[3]) { decl, param, semcol }, 3);
		add_rule(stmt, (item[5]) { decl, param, eq, expr, semcol }, 5);
		add_rule(stmt, (item[5]) { decl, id, eq, expr, semcol }, 5);
		add_rule(stmt, (item[3]) { decl, id, semcol }, 3);
		add_rule(stmt, (item[6]) { decl, param, opbrck, param_list, clbrck, block }, 6);
		add_rule(prgrm, (item[1]) { stmt_list }, 1);
	}
}

void create_slr(FILE* fp)
{
	int i, j;
	states_count = generate(rules, RULES_NUM, &SLR_GOTO, &SLR_ACTION);


	fprintf(fp, "%d %d\n", states_count, SYMBOLS_COUNT);
	//save goto table to file
	for (i = 0; i < states_count; i++)
	{
		for (j = 0; j < SYMBOLS_COUNT; j++)
		{
			fprintf(fp, "%d ", SLR_GOTO[j][i]);
		}
		fprintf(fp, "\n");
	}
	//save action table to file
	for (i = 0; i < states_count; i++)
	{
		for (j = 0; j < TERMINALS_COUNT; j++)
		{
			fprintf(fp, "%d ", SLR_ACTION[j][i]);
		}
		fprintf(fp, "\n");
	}

	fclose(fp);
}
int load_slr(FILE* fp)
{
	int i, j;
	int symbols;

	fscanf(fp, "%d %d\n", &states_count, &symbols);
	if (symbols != SYMBOLS_COUNT)
	{
		return 0;
	}
	else {
		//allocate tables
		alloc_tables(&SLR_ACTION, &SLR_GOTO);
		fill_rules_arr();
		//load goto table
		for (i = 0; i < states_count; i++)
		{
			for (j = 0; j < SYMBOLS_COUNT; j++)
			{
				fscanf(fp, "%d ", &(SLR_GOTO[j][i]));
			}
		}
		//load action table
		for (i = 0; i < states_count; i++)
		{
			for (j = 0; j < TERMINALS_COUNT; j++)
			{
				fscanf(fp, "%d", &(SLR_ACTION[j][i]));
			}
		}
	}
	fclose(fp);
	return 1;
}

void parse(parser* prsr, char* parser_name)
{
	
	FILE* fp;
	token lookahead;
	action act;
	int accepted = 0;
	int succeeded = 0;
	prsr->state = START_STATE;

	fp = fopen(parser_name, "r");

	if (fp != NULL)
	{
		succeeded = load_slr(fp);
	}
	if (!succeeded)
	{
		fp = fopen(parser_name, "w");
		fill_rules_arr();
		create_slr(fp);
	}
	
	fill_reduction_tbl();
	fill_terminal_to_kind();
	fill_kind_to_data();

	prsr->stck = init_stack();
	prsr->err_lst = err_list();
	prsr->index = 0;


	do
	{
		lookahead = prsr->input[prsr->index];
		act = SLR_ACTION[lookahead.type][prsr->state];
		if (act < 0)
			reduce(prsr, act);
		else if (act == SHIFT)
			shift(prsr);
		else if (act == ACCEPT)
			accepted = 1;
		else
			syntax_error(prsr);

	} while (!accepted);
}