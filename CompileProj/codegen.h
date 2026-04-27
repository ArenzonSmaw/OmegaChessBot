#ifndef CODEGEN_H
#define CODEGEN_H

#include "ast.h"
#include "semantic.h"
#include <stdio.h>

typedef struct {
    FILE* out;
    scope current_scope;
    int temp_count;
    int label_count;  
    char* break_label;
} context;

void init_context(context* ctx, FILE* out, scope global);

void code_generate(context* ctx, AST root);


#endif