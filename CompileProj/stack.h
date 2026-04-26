#ifndef STACK_H
#define STACK_H

#include <stdlib.h>
#include "grammar.h"
#include "ast.h"

typedef struct {
	token* tkn;
	AST node;
	int state;
}info;

typedef struct stack_node
{
	info *data;
	struct stack_node* next;

} stack_node, *stack;

stack init_stack();
void push(stack*, token* tkn, AST node, int state);
info* pop(stack*);
info* top(stack*);
int isEmpty(stack);
void free_stack(stack*);

#endif
