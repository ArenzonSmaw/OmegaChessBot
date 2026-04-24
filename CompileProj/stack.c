#include "stack.h"
#include "error.h"

stack init_stack()
{
	stack s = NULL;
	return s;
}

int isEmpty(stack s)
{
	return s == NULL;
}


void push(stack* s, symbol sb, AST ast_node, int stt)
{
	stack s_node = (stack)malloc(sizeof(stack_node));
	if (s_node) {
		s_node->next = s;
		s_node->data->item = sb;
		s_node->data->state = stt;
		s_node->data->node = ast_node;
		*s = s_node;
	}
	else memory_error();
}

info* pop(stack* s)
{
	info* inf = NULL;
	stack temp;
	if (!isEmpty(*s))
	{
		inf = (*s)->data;
		temp = *s;
		*s = (*s)->next;
		free(temp);
	}
	return inf;
}

info* top(stack* s)
{
	return (*s)->data;
}
void free_stack(stack* s)
{
	stack temp;
	while (!isEmpty(*s))
	{
		temp = *s;
		*s = (*s)->next;
		free(temp);
	}
}