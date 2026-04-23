#include "parser.h"
#include "grammar.h"
#include <stdlib.h>
#include <string.h>

#define TERMINAL 1
#define NON_TERMINAL 0

#define START_STATE 0

typedef enum {
	ACCEPT = 0,
	SHIFT = 1,
	REDUCE = 2,
	ERROR = -1
} action;
action** ACTION;
int** GOTO;
typedef struct
{
	items_arr items;
	int count;
} state;
state* states;
int states_count;
int next_state;
int FIRST[SYMBOLS_COUNT - TERMINALS_COUNT][TERMINALS_COUNT];
int FOLLOW[SYMBOLS_COUNT - TERMINALS_COUNT][TERMINALS_COUNT];

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
	push(prsr->stck, prsr->input[prsr->index].type, prsr->state);
}
void reduce(parser* prsr, item_set rule) 
{
	
}
void accept(parser* prsr) {}

void alloc_tables()
{
	//allocate dynamically a symbols_count X 10 goto and action tables

	int i, j;
	states_count = 20; 
	next_state = 1;

	ACTION = (action**)malloc(SYMBOLS_COUNT * sizeof(action*));
	states = (state*)malloc(states_count * sizeof(state));
	GOTO = (int**)malloc(SYMBOLS_COUNT * sizeof(int*));

	if (ACTION == NULL || GOTO == NULL || states == NULL)
	{
		memory_error();
	}

	for (i = 0; i < SYMBOLS_COUNT; i++)
	{
		ACTION[i] = (action*)malloc(states_count*sizeof(action));
		GOTO[i] = (int*)malloc(states_count*sizeof(int));

		if (ACTION[i] == NULL || GOTO[i] == NULL)
			memory_error();
		for (j = 0; j < states_count; j++)
		{
			ACTION[i][j] = ERROR;
			GOTO[i][j] = -1;
		}
	}

	for (i = 0; i < states_count; i++)
	{
		states[i].items = NULL;
		states[i].count = 0;
	}

	for (i = 0; i < SYMBOLS_COUNT - TERMINALS_COUNT; i++)
	{
		for (j = 0; j < TERMINALS_COUNT; j++)
		{
			FIRST[i][j] = 0;
			FOLLOW[i][j] = 0;
		}
	}
}
void expand()
{
	//double the size of both tables (add states) if states_num < 80. else add 50 states
	int i, j;
	int states_add = (states_count >= 80) ? 50 : states_count;
	
	for (i = 0; i < SYMBOLS_COUNT; i++)
	{
		ACTION[i] = (action*)realloc(ACTION[i], (states_count + states_add) * sizeof(action));
		GOTO[i] = (int*)realloc(GOTO[i], (states_count + states_add) * sizeof(int));
		if (GOTO[i] == NULL || ACTION[i] == NULL)
			memory_error();
		states_add += states_count;
		for (j = states_count; j < states_add; j++)
		{
			ACTION[i][j] = ERROR;
			GOTO[i][j] = -1;
		}
		states_count = states_add;
	}
}

int rules_isEqual(item_set rule1, item_set rule2)
{
	int equal = 0, dot;
	if (rule1.lhs.symbol == rule2.lhs.symbol && rule1.length == rule2.length && rule1.pos == rule2.pos)
	{
		equal = 1;
		for (dot = 0; dot + rule1.pos < rule1.length; dot++)
		{
			if (rule1.rhs[rule1.pos + dot].symbol != rule2.rhs[rule2.pos + dot].symbol)
				equal = 0;
		}
	}
	return equal;
}
int sets_isEqual(items_arr set1, int size1, items_arr set2, int size2)
{
	int idx;
	if (size1 != size2) return 0;
	for (idx = 0; idx < size1; idx++)
	{
		if (!rules_isEqual(set1[idx], set2[idx]))
			return 0;
	}
	return 1;
}
int add_to_set(items_arr* arr, item_set set, int* arr_size)
{
	int index, found = 0, dot, equal;
	item_set rule;
	for (index = 0; index < *arr_size && !found; index++)
	{
		rule = (*arr)[index];
		found = rules_isEqual(rule, set);
	}
	if (!found)
	{
		*arr = (items_arr)realloc(*arr, ++(*arr_size)*sizeof(item_set));
		(*arr)[*arr_size - 1] = set;
		return 1;
	}
	return 0;
}
void closure(items_arr* set,int* set_size, items_arr rules, int length)
{
	int changed = 1;
	int rule_num, index;
	item_set rule;
	while (changed)
	{
		changed = 0;
		for (rule_num = 0; rule_num < *set_size; rule_num++)
		{
			rule = (*set)[rule_num];
			if (rule.pos < rule.length && !rule.rhs[rule.pos].isTerminal)
			{
				for (index = 0; index < length; index++) 
				{
					if (rules[index].lhs.symbol == rule.rhs[rule.pos].symbol)
						changed = changed || add_to_set(set, rules[index], set_size);
				}
			}
		}
	}
}

int get_state_index(items_arr set, int set_size)
{
	int stt;
	for (stt = 0; stt < states_count; stt++)
	{
		if (sets_isEqual(states[stt].items, states[stt].count, set, set_size))
			return stt;
	}

	if (states_count == next_state)
		expand();
	states[next_state].items = set;
	states[next_state].count = set_size;
	
	return next_state++;
}

int calc_goto(int curr_state, symbol X, items_arr rules, int rules_count)
{
	items_arr rule_set = NULL;
	int set_size = 0;
	item_set curr_rule;
	int next_state;
	int i;
	if (GOTO[X][curr_state] != -1)
	{
		return GOTO[X][curr_state];
	}
	for (i = 0; i < states[curr_state].count; i++)
	{
		curr_rule = states[curr_state].items[i];
		if (curr_rule.pos < curr_rule.length && curr_rule.rhs[curr_rule.pos].symbol == X)
		{
			curr_rule.pos++;
			add_to_set(&rule_set, curr_rule, &set_size);
		}
	}
	if (set_size == 0) return -1;
	closure(&rule_set, &set_size, rules, rules_count);
	next_state = get_state_index(rule_set, set_size);
	GOTO[X][curr_state] = next_state;
	return next_state;
}

void build_states(items_arr arr, int arr_size)
{
	item_set start = arr[0];
	int state = START_STATE;
	items_arr rule_set = (items_arr)malloc(sizeof(item_set));
	int set_size = 1;

	item_set rule;
	int state_index;
	int rule_index;
	int goto_state;

	start.pos = 0;
	rule_set[0] = start;

	closure(&rule_set, &set_size, arr, arr_size);

	states[0].items = rule_set;
	states[0].count = set_size;
	next_state = 1;

	for (state_index = 0; state_index < next_state; state_index++)
	{
		for (rule_index = 0; rule_index < states[state_index].count; rule_index++)
		{
			rule = states[state_index].items[rule_index];
			if (rule.pos < rule.length)
				calc_goto(state_index, rule.rhs[rule.pos].symbol, arr, arr_size);
		}
	}
}

void fill_first(items_arr rules, int count)
{
	int changed = 1;
	int index;
	item_set item;
	symbol l, r, temp;
	while (changed)
	{
		changed = 0;

		for (index = 0; index < count; index++)
		{
			item = rules[index];
			l = item.lhs.symbol;
			r = item.rhs[0].symbol;
			if (item.rhs[0].isTerminal && FIRST[l][r] == 0)
			{
				FIRST[l][item.rhs[0].symbol] = 1;
				changed = 1;
			}
			else if (!item.rhs[0].isTerminal)
			{
				for (temp = 0; temp < TERMINALS_COUNT; temp++)
				{
					if (FIRST[r][temp] && !FIRST[l][temp])
					{
						FIRST[l][temp] = 1;
						changed = 1;
					}
				}
			}
		}
	}
}
void fill_follow(items_arr rules, int count)
{
	int changed;
	int dot, index, i;
	item_set rule;
	symbol A, B;
	FOLLOW[S_TAG][END_TOKEN] = 1;
	while (changed) {
		changed = 0;
		for (index = 0; index < count; index++)
		{
			rule = rules[index];
			for (dot = 0; dot < rule.length; dot++)
			{
				if (!rule.rhs[dot].isTerminal)
				{
					A = rule.rhs[dot].symbol;

					if (dot + 1 < rule.length)
					{
						if (rule.rhs[dot + 1].isTerminal)
						{
							if (!FOLLOW[A][rule.rhs[dot + 1].symbol])
							{
								FOLLOW[A][rule.rhs[dot + 1].symbol] = 1; 
								changed = 1;
							}
						}
						else 
						{
							symbol B = rule.rhs[dot + 1].symbol;
							for (i = 0; i < TERMINALS_COUNT; i++)
							{
								if (FIRST[B][i] && !FOLLOW[A][i])
								{
									FOLLOW[A][i] = 1;
									changed = 1;
								}
							}
						}
					}
					else
					{
						for (i = 0; i < TERMINALS_COUNT; i++)
						{
							if (!FOLLOW[A][i] && FOLLOW[rule.lhs.symbol][i])
							{
								FOLLOW[A][i] = 1;
								changed = 1;
							}
						}
					}
				}
			}
		}
	}
}

void build_tables(items_arr arr, int length)
{	
	int set_num;
	alloc_tables();

	for (set_num = 0; set_num < length; set_num++)
	{
		add_rule(&arr[set_num], arr, length);
	}
}
#define RULES_NUM 74
void init_slr_tables()
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


	items_arr rules = (item_set[RULES_NUM]){

		{grammar_start, (item[1]){prgrm}, 0, 1},

		{literal, (item[1]) { bool_lit }, 0, 1},
		{literal, (item[1]) { int_lit }, 0, 1},
		{literal, (item[1]) { flt_lit }, 0, 1},
		{literal, (item[1]) { rat_lit }, 0, 1},
		{literal, (item[1]) { chr_lit }, 0, 1},
		{literal, (item[1]) { str_lit }, 0, 1},

		{factor, (item[1]){literal}, 0, 1},
		{factor, (item[1]){underline}, 0, 1},
		{factor, (item[1]){id}, 0, 1},
		{factor, (item[4]){id, opbrck, arg_list, clbrck}, 0, 4},
		{factor, (item[3]){opbrck, expr, clbrck}, 0, 3},
		{factor,  (item[4]){colon, type, colon, factor}, 0, 4},
		{factor,  (item[2]){exclam, factor}, 0, 2},
		{factor,  (item[2]){minus, factor}, 0, 2},
		{factor,  (item[3]) { factor, notnot}, 0, 3},

		{arg_list, (item[3]) { arg_list, comma, expr}, 0, 3},
		{arg_list, (item[1]) {expr}, 0, 1},

		{term, 	(item[1]){ factor }, 0, 1},
		{term, 	(item[3]){ term, mult, factor}, 0, 3},
		{term, 	(item[3]){ term, divide, factor }, 0, 3},
		{term, 	(item[3]){ term, mod, factor }, 0, 3},
		{term, 	(item[3]){ term, modmod, factor }, 0, 3},
		{term, 	(item[3]){ term, rgtrgt, factor }, 0, 3},
		{term, 	(item[3]){ term, lftlft, factor }, 0, 3},
		{term, 	(item[3]){ term, and, factor }, 0, 3},
		{term, 	(item[3]){ term, tilde_or, factor }, 0, 3},
		{term, 	(item[3]){ term, or, factor }, 0, 3},

		{arith, (item[1]){term}, 0, 1},
		{arith, (item[3]){arith, plus, term }, 0, 3},
		{arith, (item[3]){arith, minus, term }, 0, 3},

		{compr,  (item[1])  { arith }, 0, 1},
		{compr,  (item[3]) { compr, right, arith }, 0, 3},
		{compr,  (item[3]) { compr, rgt_eq, arith }, 0, 3},
		{compr,  (item[3]) { compr, left, arith }, 0, 3},
		{compr,  (item[3]) { compr, lft_eq, arith }, 0, 3},
		{compr,  (item[3]) { compr, eqeq, arith }, 0, 3},
		{compr,  (item[3]) { compr, not_eq, arith }, 0, 3},

		{logic, (item[1]){compr}, 0, 1},
		{logic, (item[3]){ logic, andand, compr }, 0, 3},
		{logic, (item[3]){ logic, oror, compr }, 0, 3},
		
		{expr,  (item[1]){ logic }, 0, 1},

		{stmt,  (item[2]){ expr, semcol}, 0, 2},

		{incr,  (item[1]){plspls}, 0, 1},
		{incr,  (item[1]){mnsmns}, 0, 1},
		{incr,  (item[1]){mltmlt}, 0, 1},
		{incr,  (item[1]){divdiv}, 0, 1},

		{stmt,  (item[4]){id,eq,expr, semcol}, 0, 4},
		{stmt,  (item[7]){colon,type,colon,id,eq,expr, semcol }, 0, 7},
		{stmt,  (item[4]){ id,incr, expr,semcol }, 0, 4},
		{stmt,  (item[3]){ id,incr,semcol }, 0, 3},

		{stmt,  (item[2]) { brk,semcol }, 0, 2},
		{stmt,  (item[2]) { pss, semcol }, 0, 2},
		{stmt,  (item[3]){ret,expr,semcol }, 0, 3},

		{stmt,  (item[7]){chck,opbrck,expr,comma,id,clbrck,block}, 0, 7},
		{stmt,  (item[5]){ifky,opbrck,expr,clbrck,block}, 0, 5},
		{stmt,  (item[5]){elsky,opbrck,expr,clbrck,block}, 0, 5},
		{stmt,  (item[2]){elsky, block}, 0, 2},

		{stmt,  (item[9]){loop,opbrck, decl,arrow,expr,semcol,stmt,clbrck,block}, 0, 9},
		{stmt,  (item[5]){loop,opbrck,expr,clbrck,block}, 0, 5},

		{stmt_list,  (item[2]) { stmt_list, stmt } , 0, 2},
		{stmt_list,  (item[1]){stmt}, 0, 1},
		{block,  (item[3]){opbrck, stmt_list, clbrck}, 0, 3},

		{param,  (item[3]){ id,colon,type}, 0, 3},
		{param_list,  (item[3]){param_list,comma,param}, 0, 3},
		{param_list,  (item[1]){param}, 0, 1},

		{stmt,  (item[4]) { use,colon,stmt,semcol }, 0, 4},
		{stmt,  (item[3]){decl, param, semcol}, 0, 3},
		{stmt,  (item[5]){ decl, param, eq, expr, semcol }, 0, 5},
		{stmt,  (item[5]){ decl, id, eq, expr, semcol }, 0, 5},
		{stmt,  (item[3]){ decl, id, semcol}, 0, 3},
		{stmt,  (item[6]){ decl, param, opbrck, param_list, clbrck, block}, 0, 6},
		
		{prgrm, (item[1]){stmt_list}, 0, 1}
	};

	build_tables(rules, RULES_NUM);
	
}


void parse(parser* prsr)
{
	action act;
	prsr->state = START_STATE;
	
	token token_data;

	init_slr_tables();

	prsr->stck = init_stack();
	prsr->err_lst = err_list();

	for (prsr->index = 0; prsr->index < prsr->input_size; (prsr->index)++)
	{
		token_data = prsr->input[prsr->index];
		act = ACTION[token_data.type][prsr->state];
		prsr->state = GOTO[token_data.type][prsr->state];

		switch (act) {
		case SHIFT:
			shift(prsr, token_data.type);
			break;
		case REDUCE:
			break;
		case ERROR:
			break;
		}
	}
}