#include "parser.h"
#include "grammar.h"
#include "slrgenerator.h"
#include <stdlib.h>
#include <string.h>

#pragma warning (disable:4996)

#define TERMINAL 1
#define NON_TERMINAL 0

#define START_STATE 0

#define RULES_NUM 7

items_arr rules;
int rule_index;
action** ACTION;
int** GOTO;
int states_count;

node_kind TERMINAL_TO_KIND[TERMINALS_COUNT];

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

	TERMINAL_TO_KIND[PLUS] = NODE_ADD;
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
	TERMINAL_TO_KIND[DBL_PLUS] = NODE_INC_BINARY;
	TERMINAL_TO_KIND[MINUS] = NODE_SUB;
	TERMINAL_TO_KIND[DBL_MINUS] = NODE_DEC_BINARY;
	TERMINAL_TO_KIND[MULT] = NODE_MUL;
	TERMINAL_TO_KIND[DBL_MULT] = NODE_MAG_BINARY;
	TERMINAL_TO_KIND[DIVIDE] = NODE_DIV;
	TERMINAL_TO_KIND[DBL_DIVIDE] = NODE_DIM_BINARY;
	TERMINAL_TO_KIND[MOD] = NODE_MOD;
	TERMINAL_TO_KIND[DBL_MOD] = NODE_QUO;
	TERMINAL_TO_KIND[NOT] = NODE_LOG_NOT;
	TERMINAL_TO_KIND[NOTNOT] = NODE_BIT_NOT;
}

void syntax_error(parser* prsr) 
{
	token tkn = prsr->input[prsr->index];
	char* lexeme = "'";
	strcat(lexeme, tkn.lexeme);
	strcat(lexeme, "' unexpected");
	err_append(prsr->err_lst, error("Syntax Error", lexeme, tkn.line, tkn.col));
}
void shift(parser* prsr) 
{
	token tkn = prsr->input[prsr->index];
	node_kind kind = TERMINAL_TO_KIND[tkn.type];
	AST node = create_leaf(tkn, kind);
	push(prsr->stck, prsr->input[prsr->index].type, prsr->state, node);
}
void reduce(parser* prsr, item_set rule) 
{
	int i;
	AST father = init_ast(NULL, TERMINAL_TO_KIND[rule.lhs.symbol], rule.length);
	info data;
	alloc_children(father, rule.length);

	for (i = rule.length -1; i >= 0; i--)
	{
		data = *(pop(prsr->stck));
		father->children[i] = data.node;
		father->children_count++;
	}

	push(prsr->stck, rule.lhs.symbol, father, GOTO[rule.lhs.symbol][top(prsr->stck)->state]);
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
	/*{
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
		add_rule(factor, (item[3]) { factor, notnot }, 2);
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
		add_rule(stmt, (item[5]) { ifky, opbrck, expr, clbrck, block }, 5);
		add_rule(stmt, (item[5]) { elsky, opbrck, expr, clbrck, block }, 5);
		add_rule(stmt, (item[2]) { elsky, block }, 2);
		add_rule(stmt, (item[9]) { loop, opbrck, decl, arrow, expr, semcol, stmt, clbrck, block }, 9);
		add_rule(stmt, (item[5]) { loop, opbrck, expr, clbrck, block }, 5);
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
	}*/
		init_rules_arr(7);
		add_rule(grammar_start, (item[]) { stmt }, 1);
		add_rule(stmt, (item[]) { expr, semcol }, 2);
		add_rule(expr, (item[]) { expr, mult, term }, 3);
		add_rule(expr, (item[]) { term }, 1);
		add_rule(term, (item[]) { term, plus, factor }, 3);
		add_rule(term, (item[]) { factor }, 1);
		add_rule(factor, (item[]) { int_lit }, 1);
}

void create_slr(FILE* fp)
{
	int i, j;
	states_count = generate(rules, RULES_NUM, &GOTO, &ACTION);


	fprintf(fp, "%d %s\n", states_count, SYMBOLS_COUNT);
	//save goto table to file
	for (i = 0; i < states_count; i++)
	{
		for (j = 0; j < SYMBOLS_COUNT; j++)
		{
			fprintf(fp, "%d ", GOTO[j][i]);
		}
		fprintf(fp, "\n");
	}
	//save action table to file
	for (i = 0; i < states_count; i++)
	{
		for (j = 0; j < TERMINALS_COUNT; j++)
		{
			fprintf(fp, "%d ", ACTION[j][i]);
		}
		fprintf(fp, "\n");
	}

	fclose(fp);
}
void load_slr(FILE* fp)
{
	int i, j;
	int symbols;

	fscanf(fp, "%d %d\n", &states_count, &symbols);
	if (symbols != SYMBOLS_COUNT)
	{
		fill_rules_arr();
		create_slr(fp);
	}
	else {
		//load goto table
		for (i = 0; i < states_count; i++)
		{
			for (j = 0; j < SYMBOLS_COUNT; j++)
			{
				fscanf(fp, "%d ", &(GOTO[j][i]));
			}
		}
		//load action table
		for (i = 0; i < states_count; i++)
		{
			for (j = 0; j < SYMBOLS_COUNT; j++)
			{
				fscanf(fp, "%d", &(ACTION[j][i]));
			}
		}
	}
	fclose(fp);
}

void parse(parser* prsr, char* parser_name)
{
	
	FILE* fp;
	token token_data;
	action act;
	int accepted = 0;
	prsr->state = START_STATE;

	fp = fopen(parser_name, "r");

	if (fp == NULL && (fp = fopen(parser_name, "w")))
	{
		fill_rules_arr();
		create_slr(fp);
	}
	else
		load_slr(fp);
	

	prsr->stck = init_stack();
	prsr->err_lst = err_list();
	prsr->index = 0;

	do
	{
		token_data = prsr->input[prsr->index];
		act = ACTION[token_data.type][prsr->state];
		if (act < 0)
			reduce(prsr, rules[(-1 * act) - 1]);
		else if (act == SHIFT)
			shift(prsr);
		else if (act == ACCEPT)
			accepted = 1;
		else
			syntax_error(prsr);

	} while (!accepted);
}