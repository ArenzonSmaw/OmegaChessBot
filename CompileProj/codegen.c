#include "codegen.h"
#include <string.h>

typedef void (*emit_func)(context*);
emit_func ARITH_CASE[4] = {
    emit_add_ax_bx,//ADD
    emit_sub_ax_bx,//SUB
    emit_mul_bx,//MUL
    emit_div_bx//DIV
    
};
emit_func LOGIC_CASE[4] = {
    emit_je,//LOGIC_EQUAL
    emit_jl,//LESS
    emit_jg//GREATER
};

typedef void (*gen_func)(context*, AST);
gen_func GENERATOR[KIND_COUNT] = {
    gen_start,//NODE_START
    gen_program,//NODE_PROGRAM
    gen_use,//NODE_USE_DECLARE
    gen_func_declare,//NODE_FUNC_DECLARE
    gen_var_declare//NODE_VAR_DECLARE
    gen_assign,//NODE_ASSIGNMENT

    gen_if,//NODE_IF
    gen_loop,//NODE_LOOP
    gen_return,//NODE_RETURN
    gen_break,//NODE_BREAK
    gen_pass,//NODE_PASS

    gen_ident,//NODE_IDENT
    gen_literal,//NODE_LITERAL
    gen_char,//NODE_CHAR
    gen_string,//NODE_STRING
    gen_last_value,//NODE_UNDERLINE
    gen_scan,//NODE_SCAN
    gen_print,//NODE_PRINT

    gen_expr_arith,//NODE_ADD
    gen_expr_arith,//NODE_SUB
    gen_term_arith,//NODE_MUL
    gen_term_arith,//NODE_DIV
    gen_term_arith,//NODE_MOD
    gen_term_arith,//NODE_QUO
    gen_logic,//NODE_LOG_OR
    gen_bit_or,//NODE_BIT_OR
    gen_logic,//NODE_LOG_AND
    gen_bit,//NODE_BIT_AND
    gen_logic,//NODE_LOG_NOT
    gen_bit,//NODE_BIT_NOT
    gen_bit_shr,//NODE_BIT_RIGHT
    gen_bit_shl,//NODE_BIT_LEFT
    gen_logic_cmpr,//NODE_LOG_EQUAL
    gen_logic_cmpr,//NODE_LOG_DIFFERENT
    gen_logic_cmpr,//NODE_GREAT
    gen_logic_cmpr,//NODE_GREAT_EQUAL
    gen_logic_cmpr,//NODE_LESS
    gen_logic_cmpr,//NODE_LESS_EQUAL
    gen_bit,//NODE_XOR

    gen_func_call,//NODE_FUNC_CALL
    gen_block,//NODE_BLOCK
    gen_block,//NODE_STMT_LIST
    gen_,//NODE_TYPE_CAST
    gen_parameter//NODE_PARAMETER
};




void emit_line(context* ctx, const char* line)
{
	fprintf(ctx->out, "%s\n", line);
}
void emit_label(context* ctx, const char* label)
{
	fprintf(ctx->out, "%s:\n", label);
}

void emit_jmp(context* ctx, const char* label)
{
	fprintf(ctx->out, "jmp %s\n", label);
}

void emit_je(context* ctx, const char* label)
{
	fprintf(ctx->out, "je %s\n", label);
}
void emit_jl(context* ctx, char* label)
{
    fprintf(ctx->out, "jl %s\n", label);
}
void emit_jg(context* ctx, char* label)
{
    fprintf(ctx->out, "jg %s\n", label);
}

void emit_mov_ax_imm(context* ctx, int value)
{
    fprintf(ctx->out, "mov ax, %d\n", value);
}

void emit_mov_ax_var(context* ctx, symbol_link* sym)
{
    if (sym->is_global)
        fprintf(ctx->out, "mov ax, [%s]\n", sym->name);
    else
        fprintf(ctx->out, "mov ax, [bp-%d]\n", sym->offset);
}

void emit_mov_var_ax(context* ctx, symbol_link* sym)
{
    if (sym->is_global)
        fprintf(ctx->out, "mov [%s], ax\n", sym->name);
    else
        fprintf(ctx->out, "mov [bp-%d], ax\n", sym->offset);
}

void emit_push_ax(context* ctx)
{
    emit_line(ctx, "push ax");
}

void emit_pop_ax(context* ctx)
{
    emit_line(ctx, "pop ax");
}

void emit_pop_bx(context* ctx)
{
    emit_line(ctx, "pop bx");
}

void emit_mov_bx_ax(context* ctx)
{
    emit_line(ctx, "mov bx, ax");
}
void emit_add_ax_bx(context* ctx)
{
    emit_line(ctx, "add ax, bx");
}

void emit_sub_ax_bx(context* ctx)
{
    emit_line(ctx, "sub ax, bx");
}

void emit_mul_bx(context* ctx)
{
    emit_line(ctx, "imul bx");
}

void emit_div_bx(context* ctx)
{
    emit_line(ctx, "cwd");
    emit_line(ctx, "idiv bx");
}

void emit_cmp_ax_bx(context* ctx)
{
    emit_line(ctx, "cmp ax, bx");
}

void emit_cmp_ax_0(context* ctx)
{
    emit_line(ctx, "cmp ax, 0");
}



void gen_globals(context* ctx)
{
    scope scp = ctx->current_scope;
    symbol_link* sym;
    char line[30];
    while (scp->parent != NULL)
        scp = scp->parent;

    for (int i = 0; i < TABLE_ROWS; i++)
    {
        sym = scp->table[i];

        while (sym)
        {
            strcpy(line, sym->name);

            if (!sym->is_initialized) 
            {
                if (sym->kind == NODE_STRING)
                    strcat(line, " db 255 dup(0)");
                else if (sym->kind == NODE_CHAR)
                    strcat(line, " db 0");
                else
                    strcat(line, " dw 0");
            }
            else
            {
                if (sym->kind == NODE_STRING)
                {
                    strcat(line, " db \"");
                    strcat(line, sym->name);
                    strcat(line, "\", $");
                }
                else if (sym->kind == NODE_CHAR)
                {
                    strcat(line, " db \'");
                    strcat(line, (char[2]) { sym->name, '\0' });
                    strcat(line, "\'");
                }
                else
                {

                }
            }
            emit_line(ctx, line);
            
            sym = sym->next;
        }
    }
}

void gen_start(context* ctx, AST node)
{
    emit_line(ctx, ".model small");
    emit_line(ctx, ".stack 100h");

    emit_line(ctx, ".data");
    gen_globals(ctx);   // optional helper

    emit_line(ctx, ".code");
    emit_line(ctx, "main:");

    gen_stmt_list(ctx, node->children[0]);

    emit_line(ctx, "mov ax, 4c00h");
    emit_line(ctx, "int 21h");
    emit_line(ctx, "end main");
}
void gen_stmt_list(context* ctx, AST node)
{
    int i;
    for (i = 0; i < node->children_count; i++)
    {
        GENERATOR[node->children[i]->type](ctx, node->children[i]);
    }
}

void gen_literal(context* ctx, AST node)
{
    emit_mov_ax_imm(ctx, (int)node->data.value);
}
void gen_ident(context* ctx, AST node)
{
    symbol_link* sym = get_symbol(ctx->current_scope, node->data.name);
    emit_mov_ax_var(ctx, sym->name);
}

void gen_term_arith(context* ctx, AST node)
{
    gen_expr(ctx, node->children[0]);
    emit_push_ax(ctx);
    
    gen_expr(ctx, node->children[1]);
    emit_mov_bx_ax(ctx);

    emit_pop_ax(ctx);

    ARITH_CASE[node->type-NODE_ADD](ctx);
}
void gen_expr_arith(context* ctx, AST node)
{
    gen_expr(ctx, node->children[0]);
    emit(ctx, "push ax");

    gen_expr(ctx, node->children[1]);
    emit(ctx, "mov bx, ax");

    emit(ctx, "pop ax");
    emit(ctx, "cmp ax, bx");

    char* l_true = new_label(ctx, "true");
    char* l_end = new_label(ctx, "end");

    ARITH_CASE[node->type](ctx);
}
void gen_logic_cmpr(context* ctx, AST node)
{
    gen_expr(ctx, node->children[0]);
    emit(ctx, "push ax");

    gen_expr(ctx, node->children[1]);
    emit(ctx, "mov bx, ax");

    emit(ctx, "pop ax");
    emit(ctx, "cmp ax, bx");

    char* l_true = new_label(ctx, "true");
    char* l_end = new_label(ctx, "end");

    LOGIC_CASE[node->type - NODE_LOG_EQUAL](ctx);

    emit(ctx, "mov ax, 0");
    emit(ctx, "jmp %s", l_end);

    emit(ctx, "%s:", l_true);
    emit(ctx, "mov ax, 1");

    emit(ctx, "%s:", l_end);
}

void gen_var_declare(context* ctx, AST node)
{
    AST id = node->children[0];

    symbol_link* sym = get_symbol(ctx->current_scope, id->data.name);

    if (node->children_count == 2)
    {
        // has initializer
        AST expr = node->children[1];

        GENERATOR[expr->type](ctx, expr);   // AX = value

        emit_mov_var_ax(ctx, sym);
    }
    else
    {
        // no initializer ? default value
        emit_mov_ax_imm(ctx, 0);

        emit_mov_var_ax(ctx, sym);
    }
}

void gen_assignment(context* ctx, AST node)
{
    AST lhs = node->children[0];
    AST rhs = node->children[1];

    gen_expr(ctx, rhs);

    symbol_link* sym = get_symbol(ctx->current_scope, lhs->data.name);

    emit_mov_var_ax(ctx, sym);
}

void gen_print(context* ctx, AST node)
{
    GENERATOR[node->children[0]->type](ctx, node->children[0]);
    emit_line(ctx, "call print_int"); // you implement runtime
}

void gen_if(context* ctx, AST node)
{
    char* l_else = new_label(ctx, "else");
    char* l_end = new_label(ctx, "endif");

    gen_expr(ctx, node->children[0]);
    emit_cmp_ax_0(ctx);
    emit_je(ctx, l_else);

    GENERATOR[node->children[1]->type](ctx, node->children[1]); // then

    emit_jmp(ctx, l_end);
    emit_label(ctx, l_else);

    if (node->children_count > 2 && node->children[2])
        GENERATOR[node->children[2]->type](ctx, node->children[2]);

    emit_label(ctx, l_end);
}

void gen_loop(context* ctx, AST node)
{
    char* l_start = new_label(ctx, "loop");
    char* l_end = new_label(ctx, "endloop");

    char* prev_break = ctx->break_label;
    ctx->break_label = l_end;

    emit_label(ctx, "%s:", l_start);

    gen_expr(ctx, node->children[0]);
    emit_cmp_ax_0(ctx);
    emit_je(ctx, l_end);

    GENERATOR[node->children[1]->type](ctx, node->children[1]);

    emit_jmp(ctx, l_start);
    emit_label(ctx, l_end);

    ctx->break_label = prev_break;
}

void gen_break(context* ctx, AST node)
{
    emit_jmp(ctx, ctx->break_label);
}

void gen_block(context* ctx, AST node)
{
    for (int i = 0; i < node->children_count; i++)
        GENERATOR[node->children[i]->type](ctx, node->children[i]);
}



void init_context(context* ctx, FILE* out, scope global)
{
	ctx->out = out;
	ctx->current_scope = global;
	ctx->label_count = ctx->temp_count = 0;
}


void code_generate(context* ctx, AST root)
{
    if (root) 
    {
        for (int i = 0; i < root->children_count; i++)
        {
           GENERATOR[root->children[i]->type](ctx, root->children[i]);
        }
    }
}