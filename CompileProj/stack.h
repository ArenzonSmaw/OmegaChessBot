#ifndef STACK_H
#define STACK_H

#include "parser.h"
#include "grammar.h"

typedef struct stack_node
{
	Symbol *data;
	struct stack_node* next;

} stack_node, *stack;

stack init();
void push(stack*, Symbol*);
Symbol* pop(stack*);
int isEmpty(stack);
void free_stack(stack*);

#endif
