#include "stack.h"

stack init()
{
	stack s = NULL;
	return s;
}

int isEmpty(stack s)
{
	return s == NULL;
}

void add_node(Symbol* new_item, stack* old_s)
{
	stack s = (stack)malloc(sizeof(stack_node));
	s->next = old_s;
	s->data = new_item;
}
void push(stack* s, Symbol* sb)
{
	if (isEmpty(*s))
		add_node(sb, NULL);
	else
		add_node(sb, s);
}

Symbol* pop(stack* s)
{
	Symbol* sb = NULL;
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