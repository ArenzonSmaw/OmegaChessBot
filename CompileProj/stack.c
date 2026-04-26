#include "stack.h"
#include "error.h"

stack init_stack()
{
	stack s = (stack)malloc(sizeof(stack_node));
	info* data = (info*)malloc(sizeof(info));
	s->data = (info*)malloc(sizeof(info));
	s->next = NULL;
	s->data->node = NULL;
	s->data->tkn = NULL;
	s->data->state = 0;
	return s;
}

int isEmpty(stack s)
{
	return s->data->node == NULL;
}


void push(stack* s, token* tkn, AST ast_node, int stt)
{
	stack s_node = (stack)malloc(sizeof(stack_node));
	info* data = (info*)malloc(sizeof(info));
	if (s_node && data) {
		s_node->next = *s;
		data->tkn = tkn;
		data->state = stt;
		data->node = ast_node;
		s_node->data = data;
		*s = s_node;
	}
	else memory_error();
}

info* pop(stack* s)
{
	stack temp;
	info* inf = (*s)->data;
	if (!isEmpty(*s))
	{
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