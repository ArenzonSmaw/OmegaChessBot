#ifndef STACK_H
#define STACK_H

#include <stdlib.h>
#include "grammar.h"

typedef struct {
	item* item;
	int* state;
	int* rule_index;
}info;

typedef struct stack_node
{
	info *data;
	struct stack_node* next;

} stack_node, *stack;

stack init_stack();
void push(stack*, item*, int);
info* pop(stack*);
int isEmpty(stack);
void free_stack(stack*);

#endif
