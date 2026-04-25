#include "slrgenerator.h"

typedef struct
{
	items_arr items;
	int count;
} state;

typedef struct {
	int** GOTO;
	int** ACTION;
	int states_count;
} generator;

generator gnrtr;

state* states;
int next_state;
int FIRST[SYMBOLS_COUNT - TERMINALS_COUNT][TERMINALS_COUNT];
int FOLLOW[SYMBOLS_COUNT - TERMINALS_COUNT][TERMINALS_COUNT];

void alloc_tables(int*** ACTION, int*** GOTO)
{
	//allocate dynamically a symbols_count X 10 goto and action tables

	int i, j;
	gnrtr.states_count = 20;
	next_state = 1;
	gnrtr.ACTION = *ACTION;
	gnrtr.GOTO = *GOTO;

	states = (state*)malloc(gnrtr.states_count * sizeof(state));
	gnrtr.ACTION = (int**)malloc(TERMINALS_COUNT * sizeof(int*));
	gnrtr.GOTO = (int**)malloc(SYMBOLS_COUNT * sizeof(int*));

	if (gnrtr.ACTION == NULL || gnrtr.GOTO == NULL || states == NULL)
	{
		memory_error();
	}
	for (i = 0; i < TERMINALS_COUNT; i++)
	{
		gnrtr.ACTION[i] = (int*)malloc(gnrtr.states_count * sizeof(int));
		if (gnrtr.ACTION[i] == NULL)
			memory_error();
		for (j = 0; j < gnrtr.states_count; j++)
		{
			gnrtr.ACTION[i][j] = ERROR;
		}
	}
	for (i = 0; i < SYMBOLS_COUNT; i++)
	{
		gnrtr.GOTO[i] = (int*)malloc(gnrtr.states_count * sizeof(int));

		if (gnrtr.GOTO[i] == NULL)
			memory_error();
		for (j = 0; j < gnrtr.states_count; j++)
		{
			gnrtr.GOTO[i][j] = -1;
		}
	}

	for (i = 0; i < gnrtr.states_count; i++)
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
	int states_add = (gnrtr.states_count >= 80) ? 50 : gnrtr.states_count;
	
	for (i = 0; i < TERMINALS_COUNT; i++)
	{
		gnrtr.ACTION[i] = (int*)realloc(gnrtr.ACTION[i], (gnrtr.states_count + states_add) * sizeof(int));
		if (gnrtr.ACTION[i] == NULL)
			memory_error();
		for (j = 0; j < gnrtr.states_count; j++)
		{
			gnrtr.ACTION[i][j] = ERROR;
		}
	}
	for (i = 0; i < SYMBOLS_COUNT; i++)
	{
		gnrtr.GOTO[i] = (int*)realloc(gnrtr.GOTO[i], (gnrtr.states_count + states_add) * sizeof(int));
		if (gnrtr.GOTO[i] == NULL)
			memory_error();
		for (j = gnrtr.states_count; j < gnrtr.states_count+states_add; j++)
		{
			gnrtr.GOTO[i][j] = -1;
		}
	}
	states = (state*)realloc(states, (gnrtr.states_count + states_add) * sizeof(state));
	for (i = gnrtr.states_count; i < gnrtr.states_count + states_add; i++)
	{
		states[i].items = NULL;
		states[i].count = 0;
	}
	gnrtr.states_count += states_add;
}

int rules_isEqual(item_set rule1, item_set rule2)
{
	return rule1.rule_num == rule2.rule_num && rule1.pos == rule2.pos;
}
int sets_isEqual(items_arr set1, int size1, items_arr set2, int size2)
{
	int idx, jdx, found;
	if (size1 != size2) return 0;
	for (idx = 0; idx < size1; idx++)
	{
		found = 0;
		for (jdx = 0; jdx < size2 && !found; jdx++)
			if (rules_isEqual(set1[idx], set2[jdx]))
				found = 1;
		if (!found) return 0;
	}
	return 1;
}

int add_to_set(items_arr* arr, item_set set, int* arr_size, int* arr_count)
{
	int index, found = 0, dot, equal;
	item_set rule;
	items_arr temp;
	for (index = 0; index < *arr_size && !found; index++)
	{
		rule = (*arr)[index];
		found = rules_isEqual(rule, set);
	}
	if (!found)
	{
		if (*arr_size == 0)
		{
			*arr = (items_arr)malloc(5 * sizeof(item_set));
			
			*arr_size = 5;
			*arr_count = 0;
		}
		else if (*arr_count >= *arr_size)
		{
			temp = (items_arr)realloc(*arr, *arr_size * 2 * sizeof(item_set));
			if (temp)
				*arr = temp;
			*arr_size *= 2;
		}
		if (*arr == NULL)
			memory_error();
		
		(*arr)[(*arr_count)++] = set;
		return 1;
	}
	return 0;
}
void closure(items_arr* set, int* set_size, int* set_count, items_arr rules, int length)
{
	int changed = 1;
	int rule_num, index;
	item_set rule;
	while (changed)
	{
		changed = 0;
		for (rule_num = 0; rule_num < *set_count; rule_num++)
		{
			rule = (*set)[rule_num];
			if (rule.pos < rule.length && !rule.rhs[rule.pos].isTerminal)
			{
				for (index = 0; index < length; index++)
				{
					if (rules[index].lhs.symbol == rule.rhs[rule.pos].symbol)
						changed = changed || add_to_set(set, rules[index], set_size, set_count);
				}
			}
		}
	}
}

int get_state_index(items_arr set, int set_size, int *is_new)
{
	int stt;
	*is_new = 1;
	for (stt = 0; stt < next_state; stt++)
	{
		if (sets_isEqual(states[stt].items, states[stt].count, set, set_size))
		{
			*is_new = 0;
			return stt;
		}
	}

	if (gnrtr.states_count == next_state)
		expand();
	states[next_state].items = set;
	states[next_state].count = set_size;

	return next_state++;
}

int calc_goto(int curr_state, symbol X, items_arr rules, int rules_count)
{
	items_arr rule_set = NULL;
	int set_size = 0, set_count = 0;
	item_set curr_rule;
	int next_state;
	int i, is_new;

	if ((gnrtr.GOTO)[X][curr_state] != -1)
	{
		return (gnrtr.GOTO)[X][curr_state];
	}
	for (i = 0; i < states[curr_state].count; i++)
	{
		curr_rule = states[curr_state].items[i];
		if (curr_rule.pos < curr_rule.length && curr_rule.rhs[curr_rule.pos].symbol == X)
		{
			curr_rule.pos++;
			add_to_set(&rule_set, curr_rule, &set_size, &set_count);
		}
	}
	if (set_size == 0) return -1;
	closure(&rule_set, &set_size, &set_count, rules, rules_count);
	next_state = get_state_index(rule_set, set_count, &is_new);
	if (!is_new)
		free(rule_set);
	gnrtr.GOTO[X][curr_state] = next_state;
	return next_state;
}

void build_states(items_arr arr, int arr_size)
{
	item_set start = arr[0];
	int state = 0;
	items_arr rule_set = (items_arr)malloc(arr_size*sizeof(item_set));
	if (!rule_set) memory_error();
	int set_size = arr_size, set_count = 0;

	item_set rule;
	int state_index;
	int rule_index;

	start.pos = 0;
	rule_set[0] = start;
	set_count++;

	closure(&rule_set, &set_size, &set_count, arr, arr_size);


	states[0].items = rule_set;
	states[0].count = set_size;

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
	int changed = 1;
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
							B = rule.rhs[dot + 1].symbol;
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

void fill_state_action(int state_num)
{
	state stt = states[state_num];
	int item_idx;
	symbol sym_idx;
	item_set itm;

	for (item_idx = 0; item_idx < stt.count; item_idx++)
	{
		itm = stt.items[item_idx];
		if (itm.lhs.symbol >= 0 && itm.pos >= itm.length)
		{
			for (sym_idx = 0; sym_idx < TERMINALS_COUNT; sym_idx++)
			{
				if (FOLLOW[itm.lhs.symbol][sym_idx])
					gnrtr.ACTION[sym_idx][state_num] = itm.rule_num;
			}
		}
		if (itm.pos < itm.length && itm.rhs[itm.pos].isTerminal)
		{
			gnrtr.ACTION[itm.rhs[itm.pos].symbol][state_num] = SHIFT;
		}
	}
}

void fill_action(items_arr rules, int count)
{
	fill_first(rules, count);
	fill_follow(rules, count);
	int state_idx;
	for (state_idx = 0; state_idx < next_state; state_idx++)
	{
		fill_state_action(state_idx);
	}
}
void free_states()
{
	int i;
	for (i = 0; i < next_state; i++)
	{
		if (states[i].items)
		{
			free(states[i].items);
			states[i].items = NULL;
		}
	}
}
void apply_tables(int*** goto_tbl, int*** action_tbl)
{
	*goto_tbl = gnrtr.GOTO;
	*action_tbl = gnrtr.ACTION;
}


int generate(items_arr rules, int rule_count, int*** goto_tbl, int*** action_tbl)
{
	alloc_tables(action_tbl, goto_tbl);
	build_states(rules, rule_count);
	fill_action(rules, rule_count);
	apply_tables(goto_tbl, action_tbl);
	free_states();

	return next_state;
}