#ifndef CODEGEN_H
#define CODEGEN_H

#include "ast.h"
#include "semantic.h"
#include <stdio.h>

typedef struct context {
    FILE* out;
    scope current_scope;
    int temp_count;
    int label_count;  
    char break_label[32];
    char step_label[32];

} context;


context* init_context(FILE* out, scope global);

void code_generate(context* ctx, AST root);


#endif