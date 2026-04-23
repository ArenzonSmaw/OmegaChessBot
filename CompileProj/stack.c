#include "stack.h"

stack init_stack()
{
	stack s = NULL;
	return s;
}

int isEmpty(stack s)
{
	return s == NULL;
}

void add_node(item* new_item, stack* old_s)
{
	stack s = (stack)malloc(sizeof(stack_node));
	s->next = old_s;
	s->data = new_item;
}
void push(stack* s, item* sb, int stt)
{
	if (isEmpty(*s))
		add_node(sb, NULL);
	else
		add_node(sb, s);
}

info* pop(stack* s)
{
	info* sb = NULL;
	stack temp;
	if (!isEmpty(*s))
	{
		sb = (*s)->data;
		temp = *s;
		*s = (*s)->next;
		free(temp);
	}
	return sb;
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