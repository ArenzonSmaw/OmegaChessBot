#include "codegen.h"
#include <string.h>
#include <stdlib.h>
#pragma warning (disable:4996)

typedef void (*gen_func)(context*, AST);
static gen_func GENERATOR[KIND_COUNT];   /* defined at bottom after all gen_ fns */


char* new_label(context* ctx, const char* prefix)
{
    /* returns a heap-allocated string like "L_loop_3" */
    char buf[64];
    sprintf(buf, "L_%s_%d", prefix, ctx->label_count++);
    char* label = (char*)malloc(strlen(buf) + 1);
    strcpy(label, buf);
    return label;
}

void emit_line(context* ctx, const char* line)
{
    fprintf(ctx->out, "\t%s\n", line);
}
void emit_label(context* ctx, const char* label)
{
    fprintf(ctx->out, "%s:\n", label);
}
void emit_jmp(context* ctx, const char* label)
{
    fprintf(ctx->out, "\tjmp %s\n", label);
}

// conditional jumps – all take (context*, const char* label) 
void emit_je(context* ctx, const char* label)
{
    fprintf(ctx->out, "\tje %s\n", label);
}
void emit_jne(context* ctx, const char* label)
{
    fprintf(ctx->out, "\tjne %s\n", label);
}
void emit_jl(context* ctx, const char* label)
{
    fprintf(ctx->out, "\tjl %s\n", label);
}
void emit_jg(context* ctx, const char* label)
{
    fprintf(ctx->out, "\tjg %s\n", label);
}
void emit_jle(context* ctx, const char* label)
{
    fprintf(ctx->out, "\tjle %s\n", label);
}
void emit_jge(context* ctx, const char* label)
{
    fprintf(ctx->out, "\tjge %s\n", label);
}

void emit_mov_ax_imm(context* ctx, int value)
{
    fprintf(ctx->out, "\tmov ax, %d\n", value);
}
void emit_mov_ax_var(context* ctx, symbol_link* sym)
{
    if (sym->is_global)
        fprintf(ctx->out, "\tmov ax, [%s]\n", sym->name);
    else
        fprintf(ctx->out, "\tmov ax, [bp-%d]\n", sym->offset);
}
void emit_mov_var_ax(context* ctx, symbol_link* sym)
{
    if (sym->is_global)
        fprintf(ctx->out, "\tmov [%s], ax\n", sym->name);
    else
        fprintf(ctx->out, "\tmov [bp-%d], ax\n", sym->offset);
}

void emit_push_ax(context* ctx) { emit_line(ctx, "push ax"); }
void emit_pop_ax(context* ctx) { emit_line(ctx, "pop ax"); }
void emit_pop_bx(context* ctx) { emit_line(ctx, "pop bx"); }
void emit_mov_bx_ax(context* ctx) { emit_line(ctx, "mov bx, ax"); }

void emit_add_ax_bx(context* ctx) { emit_line(ctx, "add ax, bx"); }
void emit_sub_ax_bx(context* ctx) { emit_line(ctx, "sub ax, bx"); }
void emit_mul_bx(context* ctx) { emit_line(ctx, "imul bx"); }
void emit_div_bx(context* ctx)
{
    emit_line(ctx, "cwd");
    emit_line(ctx, "idiv bx");
}
void emit_mod_bx(context* ctx)
{
    emit_line(ctx, "cwd");
    emit_line(ctx, "idiv bx");
    emit_line(ctx, "mov ax, dx");
}

void emit_cmp_ax_bx(context* ctx) { emit_line(ctx, "cmp ax, bx"); }
void emit_cmp_ax_0(context* ctx) { emit_line(ctx, "cmp ax, 0"); }



typedef void (*emit_noarg_func)(context*);
static emit_noarg_func ARITH_CASE[6] = {
    emit_add_ax_bx, /* NODE_ADD */
    emit_sub_ax_bx, /* NODE_SUB */
    emit_mul_bx,    /* NODE_MUL */
    emit_div_bx,    /* NODE_DIV */
    emit_mod_bx,    /* NODE_MOD */
    emit_div_bx,    /* NODE_QUO  – integer quotient same as idiv result in ax */
};

/* conditional jumps: indexed by (node->kind - NODE_LOG_EQUAL) */
typedef void (*emit_label_func)(context*, const char*);
static emit_label_func LOGIC_CASE[6] = {
    emit_je,  /* NODE_LOG_EQUAL    */
    emit_jne, /* NODE_LOG_DIFFERENT*/
    emit_jg,  /* NODE_GREAT        */
    emit_jge, /* NODE_GREAT_EQUAL  */
    emit_jl,  /* NODE_LESS         */
    emit_jle, /* NODE_LESS_EQUAL   */
};


static void gen_globals(context* ctx)
{
    scope scp = ctx->current_scope;
    while (scp->parent != NULL)
        scp = scp->parent;

    for (int i = 0; i < TABLE_ROWS; i++)
    {
        symbol_link* sym = scp->table[i];
        while (sym)
        {
            if (sym->kind == NODE_STRING)
                fprintf(ctx->out, "\t%s db 255 dup(0)\n", sym->name);
            else if (sym->kind == NODE_CHAR)
                fprintf(ctx->out, "\t%s db 0\n", sym->name);
            else
                fprintf(ctx->out, "\t%s dw 0\n", sym->name);

            sym = sym->next;
        }
    }
}


static void gen_expr(context* ctx, AST node)
{
    GENERATOR[node->kind](ctx, node);
}

void gen_literal(context* ctx, AST node)
{
    emit_mov_ax_imm(ctx, (int)node->data.value);
}

void gen_char(context* ctx, AST node)
{
    emit_mov_ax_imm(ctx, (int)node->data.value);
}

void gen_string(context* ctx, AST node)
{
    fprintf(ctx->out, "\tlea ax, [%s]\n", node->data.name);
}

void gen_ident(context* ctx, AST node)
{
    symbol_link* sym = get_symbol(ctx->current_scope, node->data.name);
    emit_mov_ax_var(ctx, sym);          
}

void gen_last_value(context* ctx, AST node)
{
    /* NODE_UNDERLINE: load the UNDERLINE variable (last expression result) */
    fprintf(ctx->out, "\tmov ax, [UNDERLINE]\n");
}

//arithmetic binary (ADD SUB MUL DIV MOD QUO) 
void gen_arith(context* ctx, AST node)
{
    gen_expr(ctx, node->children[0]);
    emit_push_ax(ctx);

    gen_expr(ctx, node->children[1]);
    emit_mov_bx_ax(ctx);    /* bx = right operand  */
    emit_pop_ax(ctx);       /* ax = left operand   */

    ARITH_CASE[node->kind - NODE_ADD](ctx);
}


void gen_logic_cmpr(context* ctx, AST node)
{
    gen_expr(ctx, node->children[0]);
    emit_push_ax(ctx);

    gen_expr(ctx, node->children[1]);
    emit_mov_bx_ax(ctx);
    emit_pop_ax(ctx);

    emit_cmp_ax_bx(ctx);

    char* l_true = new_label(ctx, "true");
    char* l_end = new_label(ctx, "end");

    LOGIC_CASE[node->kind - NODE_LOG_EQUAL](ctx, l_true);  /* FIX: was using ARITH_CASE */

    emit_mov_ax_imm(ctx, 0);
    emit_jmp(ctx, l_end);

    emit_label(ctx, l_true);
    emit_mov_ax_imm(ctx, 1);

    emit_label(ctx, l_end);

    free(l_true);
    free(l_end);
}

/* ?? logical AND / OR / NOT ?? */
void gen_logic(context* ctx, AST node)
{
    char* l_end = new_label(ctx, "end");

    if (node->kind == NODE_LOG_NOT)
    {
        gen_expr(ctx, node->children[0]);
        emit_cmp_ax_0(ctx);
        char* l_true = new_label(ctx, "true");
        emit_je(ctx, l_true);
        emit_mov_ax_imm(ctx, 0);
        emit_jmp(ctx, l_end);
        emit_label(ctx, l_true);
        emit_mov_ax_imm(ctx, 1);
        emit_label(ctx, l_end);
        free(l_true);
    }
    else if (node->kind == NODE_LOG_AND)
    {
        /* short-circuit: if left == 0, result is 0 */
        gen_expr(ctx, node->children[0]);
        emit_cmp_ax_0(ctx);
        emit_je(ctx, l_end);            /* ax is already 0 */
        gen_expr(ctx, node->children[1]);
        emit_cmp_ax_0(ctx);
        char* l_false = new_label(ctx, "false");
        emit_je(ctx, l_false);
        emit_mov_ax_imm(ctx, 1);
        emit_jmp(ctx, l_end);
        emit_label(ctx, l_false);
        emit_mov_ax_imm(ctx, 0);
        emit_label(ctx, l_end);
        free(l_false);
    }
    else /* NODE_LOG_OR */
    {
        /* short-circuit: if left != 0, result is 1 */
        char* l_true = new_label(ctx, "true");
        gen_expr(ctx, node->children[0]);
        emit_cmp_ax_0(ctx);
        emit_jne(ctx, l_true);
        gen_expr(ctx, node->children[1]);
        emit_cmp_ax_0(ctx);
        emit_jne(ctx, l_true);
        emit_mov_ax_imm(ctx, 0);
        emit_jmp(ctx, l_end);
        emit_label(ctx, l_true);
        emit_mov_ax_imm(ctx, 1);
        emit_label(ctx, l_end);
        free(l_true);
    }

    free(l_end);
}

/* ?? bitwise binary (AND OR XOR SHL SHR NOT) ?? */
void gen_bit(context* ctx, AST node)
{
    if (node->kind == NODE_BIT_NOT)
    {
        gen_expr(ctx, node->children[0]);
        emit_line(ctx, "not ax");
        return;
    }

    gen_expr(ctx, node->children[0]);
    emit_push_ax(ctx);
    gen_expr(ctx, node->children[1]);
    emit_mov_bx_ax(ctx);
    emit_pop_ax(ctx);

    switch (node->kind)
    {
    case NODE_BIT_AND:   
        emit_line(ctx, "and ax, bx");  
        break;
    case NODE_BIT_OR:    
        emit_line(ctx, "or ax, bx");   
        break;
    case NODE_XOR:       
        emit_line(ctx, "xor ax, bx");  
        break;
    default: 
        break;
    }
}

void gen_bit_or(context* ctx, AST node) { gen_bit(ctx, node); }

void gen_bit_shl(context* ctx, AST node)
{
    gen_expr(ctx, node->children[0]);
    emit_push_ax(ctx);
    gen_expr(ctx, node->children[1]);
    /* shift count must be in CL */
    emit_line(ctx, "mov cx, ax");
    emit_pop_ax(ctx);
    emit_line(ctx, "shl ax, cl");
}

void gen_bit_shr(context* ctx, AST node)
{
    gen_expr(ctx, node->children[0]);
    emit_push_ax(ctx);
    gen_expr(ctx, node->children[1]);
    emit_line(ctx, "mov cx, ax");
    emit_pop_ax(ctx);
    emit_line(ctx, "sar ax, cl");   /* arithmetic shift right to preserve sign */
}

/* ?? type cast ?? */
void gen_type_cast(context* ctx, AST node)
{
    /* children[0] = type node (kind encodes target type)
       children[1] = expression to cast                   */
    gen_expr(ctx, node->children[1]);
    /* For int<->char the value is already in ax.
       Extend/truncate as needed based on target type. */
    node_kind target = node->children[0]->kind;
    if (target == NODE_CHAR)
        emit_line(ctx, "and ax, 00FFh");   /* keep low byte */
    /* int / natural / rational: ax already holds the value */
}

/* ?? scan (runtime read) ?? */
void gen_scan(context* ctx, AST node)
{
    /*  node->kind was set to the type being scanned (see reduce_scan).
        Call the appropriate runtime routine; result comes back in ax. */
    switch (node->kind)
    {
    case NODE_CHAR:
        emit_line(ctx, "call scan_char");
        break;
    case NODE_STRING:
        emit_line(ctx, "call scan_str");
        break;
    default:
        emit_line(ctx, "call scan_int");
        break;
    }
}

/* ?? print ?? */
void gen_print(context* ctx, AST node)
{
    gen_expr(ctx, node->children[0]);

    /* choose runtime routine by the child's resolved type */
    node_kind k = node->children[0]->kind;
    if (k == NODE_STRING)
        emit_line(ctx, "call print_str");
    else if (k == NODE_CHAR)
        emit_line(ctx, "call print_char");
    else
        emit_line(ctx, "call print_int");

    /* store result into UNDERLINE for '_' references */
    emit_line(ctx, "mov [UNDERLINE], ax");
}

/* ?? function call ?? */
void gen_func_call(context* ctx, AST node)
{
    /*  children[0] = NODE_IDENT (function name)
        children[1] = arg list (NODE_STMT_LIST / NODE_BLOCK)
        Push arguments right-to-left (cdecl-style). */
    AST arg_list = node->children[1];
    for (int i = arg_list->children_count - 1; i >= 0; i--)
    {
        gen_expr(ctx, arg_list->children[i]);
        emit_push_ax(ctx);
    }

    fprintf(ctx->out, "\tcall %s\n", node->children[0]->data.name);

    /* clean up the stack (cdecl caller cleanup) */
    if (arg_list->children_count > 0)
        fprintf(ctx->out, "\tadd sp, %d\n", arg_list->children_count * 2);
}

/* ?? parameter node (used only as declaration metadata, no code) ?? */
void gen_parameter(context* ctx, AST node)
{
    (void)ctx; (void)node;
    /* nothing to emit for a bare parameter node */
}


void gen_var_declare(context* ctx, AST node)
{
    AST id = node->children[0];
    symbol_link* sym = get_symbol(ctx->current_scope, id->data.name);

    if (node->children_count == 2 && node->children[1])
    {
        gen_expr(ctx, node->children[1]);
        emit_mov_var_ax(ctx, sym);
    }
    else
    {
        /* zero-initialise */
        emit_mov_ax_imm(ctx, 0);
        emit_mov_var_ax(ctx, sym);
    }
}

void gen_assignment(context* ctx, AST node)
{
    gen_expr(ctx, node->children[1]);                           
    symbol_link* sym = get_symbol(ctx->current_scope,
        node->children[0]->data.name);
    emit_mov_var_ax(ctx, sym);
    emit_line(ctx, "mov [UNDERLINE], ax");
}

void gen_if(context* ctx, AST node)
{
    char* l_else = new_label(ctx, "else");
    char* l_end = new_label(ctx, "endif");

    gen_expr(ctx, node->children[0]);   /* condition */
    emit_cmp_ax_0(ctx);
    emit_je(ctx, l_else);

    GENERATOR[node->children[1]->kind](ctx, node->children[1]); /* then */

    emit_jmp(ctx, l_end);
    emit_label(ctx, l_else);

    if (node->children_count > 2 && node->children[2])
        GENERATOR[node->children[2]->kind](ctx, node->children[2]); /* else */

    emit_label(ctx, l_end);

    free(l_else);
    free(l_end);
}

void gen_loop(context* ctx, AST node)
{
    /* while-style:  children[0]=condition  children[1]=body
       for-style:    children[0]=iterator   children[1]=step  children[2]=body */
    char* l_start = new_label(ctx, "loop");
    char* l_end = new_label(ctx, "endloop");

    char* prev_break = ctx->break_label;
    ctx->break_label = l_end;

    if (node->children_count == 3)
    {
        /* for-loop: emit iterator init once before the loop */
        GENERATOR[node->children[0]->kind](ctx, node->children[0]);
    }

    emit_label(ctx, l_start);           

    /* condition */
    if (node->children_count == 3)
    {
        /* re-check: compare iterator variable against limit in children[1] */
        gen_expr(ctx, node->children[1]);
        emit_cmp_ax_0(ctx);
        emit_je(ctx, l_end);
        GENERATOR[node->children[2]->kind](ctx, node->children[2]); /* body */
    }
    else
    {
        gen_expr(ctx, node->children[0]);
        emit_cmp_ax_0(ctx);
        emit_je(ctx, l_end);
        GENERATOR[node->children[1]->kind](ctx, node->children[1]); /* body */
    }

    emit_jmp(ctx, l_start);
    emit_label(ctx, l_end);

    ctx->break_label = prev_break;

    free(l_start);
    free(l_end);
}

void gen_break(context* ctx, AST node)
{
    (void)node;
    emit_jmp(ctx, ctx->break_label);
}

void gen_pass(context* ctx, AST node)
{
    (void)node;
    emit_line(ctx, "nop");
}

void gen_return(context* ctx, AST node)
{
    if (node->children_count > 0 && node->children[0])
        gen_expr(ctx, node->children[0]);   /* return value ? ax */

    /* standard 16-bit function epilogue */
    emit_line(ctx, "pop bp");
    emit_line(ctx, "ret");
}

void gen_block(context* ctx, AST node)
{
    for (int i = 0; i < node->children_count; i++)
        GENERATOR[node->children[i]->kind](ctx, node->children[i]);
}

void gen_stmt_list(context* ctx, AST node)
{
    for (int i = 0; i < node->children_count; i++)
        GENERATOR[node->children[i]->kind](ctx, node->children[i]);
}


   /* USE declaration: e.g. "use : expr" – evaluate and discard */
void gen_use(context* ctx, AST node)
{
    gen_expr(ctx, node->children[0]);
}

/* Function declaration */
void gen_func_declare(context* ctx, AST node)
{
    /*  children[0] = return param (NODE_PARAMETER ? name + type)
        children[1] = parameter list
        children[2] = body block                                  */
    AST name_param = node->children[0];
    const char* fname = name_param->children[1]->data.name;   /* id child */

    fprintf(ctx->out, "%s:\n", fname);

    /* standard prologue */
    emit_line(ctx, "push bp");
    emit_line(ctx, "mov bp, sp");

    /* allocate locals: walk scope for this function */
    int local_size = ctx->current_scope->offset_next;
    if (local_size > 0)
        fprintf(ctx->out, "\tsub sp, %d\n", local_size);

    /* save caller registers */
    emit_line(ctx, "push bx");
    emit_line(ctx, "push cx");
    emit_line(ctx, "push dx");
    emit_line(ctx, "push si");
    emit_line(ctx, "push di");

    GENERATOR[node->children[2]->kind](ctx, node->children[2]); /* body */

    /* epilogue (also emitted by gen_return; this is the fall-through path) */
    emit_line(ctx, "pop di");
    emit_line(ctx, "pop si");
    emit_line(ctx, "pop dx");
    emit_line(ctx, "pop cx");
    emit_line(ctx, "pop bx");
    emit_line(ctx, "mov sp, bp");
    emit_line(ctx, "pop bp");
    emit_line(ctx, "ret");
}

/* Program node: emit each top-level declaration */
void gen_program(context* ctx, AST node)
{
    for (int i = 0; i < node->children_count; i++)
        GENERATOR[node->children[i]->kind](ctx, node->children[i]);
}

/* Start / entry-point node */
void gen_start(context* ctx, AST node)
{
    fprintf(ctx->out, ".model small\n");
    fprintf(ctx->out, ".stack 100h\n\n");

    fprintf(ctx->out, ".data\n");
    gen_globals(ctx);
    fprintf(ctx->out, "\tUNDERLINE dw 0\n\n");

    fprintf(ctx->out, ".code\n");
    fprintf(ctx->out, "main:\n");

    /* push ds setup */
    emit_line(ctx, "mov ax, @data");
    emit_line(ctx, "mov ds, ax");

    GENERATOR[node->children[0]->kind](ctx, node->children[0]);

    emit_line(ctx, "mov ax, 4c00h");
    emit_line(ctx, "int 21h");
    fprintf(ctx->out, "end main\n");
}

static gen_func GENERATOR_TABLE[KIND_COUNT] = {
    gen_start,        /* NODE_START         */
    gen_program,      /* NODE_PROGRAM       */
    gen_use,          /* NODE_USE_DECLARE   */
    gen_func_declare, /* NODE_FUNC_DECLARE  */
    gen_var_declare,  /* NODE_VAR_DECLARE   */
    gen_assignment,   /* NODE_ASSIGNMENT    */ 

    gen_if,           /* NODE_IF            */
    gen_loop,         /* NODE_LOOP          */
    gen_return,       /* NODE_RETURN        */
    gen_break,        /* NODE_BREAK         */
    gen_pass,         /* NODE_PASS          */

    gen_ident,        /* NODE_IDENT         */
    gen_literal,      /* NODE_LITERAL       */
    gen_char,         /* NODE_CHAR          */
    gen_string,       /* NODE_STRING        */
    gen_last_value,   /* NODE_UNDERLINE     */
    gen_scan,         /* NODE_SCAN          */
    gen_print,        /* NODE_PRINT         */

    gen_arith,        /* NODE_ADD           */
    gen_arith,        /* NODE_SUB           */
    gen_arith,        /* NODE_MUL           */
    gen_arith,        /* NODE_DIV           */
    gen_arith,        /* NODE_MOD           */
    gen_arith,        /* NODE_QUO           */
    gen_logic,        /* NODE_LOG_OR        */
    gen_bit_or,       /* NODE_BIT_OR        */
    gen_logic,        /* NODE_LOG_AND       */
    gen_bit,          /* NODE_BIT_AND       */
    gen_logic,        /* NODE_LOG_NOT       */
    gen_bit,          /* NODE_BIT_NOT       */
    gen_bit_shr,      /* NODE_BIT_RIGHT     */
    gen_bit_shl,      /* NODE_BIT_LEFT      */
    gen_logic_cmpr,   /* NODE_LOG_EQUAL     */
    gen_logic_cmpr,   /* NODE_LOG_DIFFERENT */
    gen_logic_cmpr,   /* NODE_GREAT         */
    gen_logic_cmpr,   /* NODE_GREAT_EQUAL   */
    gen_logic_cmpr,   /* NODE_LESS          */
    gen_logic_cmpr,   /* NODE_LESS_EQUAL    */
    gen_bit,          /* NODE_XOR           */

    gen_func_call,    /* NODE_FUNC_CALL     */
    gen_block,        /* NODE_BLOCK         */
    gen_stmt_list,    /* NODE_STMT_LIST     */
    gen_type_cast,    /* NODE_TYPE_CAST     */  
    gen_parameter,    /* NODE_PARAMETER     */
};


context* init_context(FILE* out, scope global)
{
    context* ctx = (context*)malloc(sizeof(context));
    if (ctx)
    {
        ctx->out = out;
        ctx->current_scope = global;
        ctx->label_count = 0;
        ctx->temp_count = 0;
        ctx->break_label = NULL;
    }
    else
        memory_error();
    return ctx;
}

void code_generate(context* ctx, AST root)
{
    if (root)
        GENERATOR[root->kind](ctx, root);
}