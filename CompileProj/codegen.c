#include "codegen.h"
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#pragma warning (disable:4996)

#define MAX_STRINGS 128
#define MAX_STR_RET_BUFFS 64

typedef void (*gen_func)(context*, AST);
gen_func GENERATOR[KIND_COUNT];
char* global_frmts[TYPE_ERROR] = {
    "_%s dw 0\n",    //TYPE_INT
    "_%s dw 0\n_%s_hi dw 0\n",   //TYPE_FLOAT
    "_%s dw 0\n",    //TYPE_NATURAL
    "_%s dw 0\n_%s_hi dw 0\n",   //TYPE_RATIONAL
    "_%s dw 0\n",    //TYPE_BOOL
    "_%s db 0\n",    //TYPE_CHAR
    "_%s db 255 dup(0), '$'\n",  //TYPE_STRING
    "_%s dw 0\n",    //TYPE_VOID
    "_%s dw 0\n",    //TYPE_POINTER
    "_%s dw 0\n"    //TYPE_EXCEPTION
};

char* strings[MAX_STRINGS];
int strings_count = 0;

char* str_ret_buffs[MAX_STR_RET_BUFFS];
int str_retbuff_count = 0;

int underline_emitted = 0; //1 if _ was declared as a global var

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

int new_label(context* ctx)
{
    /* returns the next label number */
    return ctx->label_count++;
}
int add_string(const char* str)
{
    int i, found = -1;
    for (i = 0; i < strings_count; i++)
        if (strcmp(strings[i], str) == 0) 
            found = i;

    if (found != -1)
        return found;
    
    strings[strings_count] = strdup(str);
    return strings_count++;
}
void reg_str_ret_buff(const char* fname)
{
    int i, flag = 1;
    for (i = 0; (i < str_retbuff_count) && flag; i++)
        if (strcmp(str_ret_buffs[i], fname) == 0)
            flag = 0;
    if (flag)
        str_ret_buffs[str_retbuff_count++] = strdup(fname);
}

symbol_link* lookup(context* ctx, char* name)
{
    return get_symbol(ctx->current_scope, name);
}

void enter_ctx_scope(context* ctx)
{
    scope child = init_scope(ctx->current_scope->level + 1, ctx->current_scope);
    ctx->current_scope = child;
}
void exit_ctx_scope(context* ctx)
{
    int i;
    symbol_link* sym, * next;
    scope temp = ctx->current_scope;
    ctx->current_scope = temp->parent;

    if (ctx->current_scope && temp->offset_next > ctx->current_scope->offset_next)
        ctx->current_scope->offset_next = temp->offset_next;

    for (i = 0; i < TABLE_ROWS; i++) {
        sym = temp->table[i];
        while (sym) {
            next = sym->next;
            free(sym->name);
            free(sym);
            sym = next;
        }
    }

    free(temp);
}


void emit_line(context* ctx, const char* frmt, ...)
{
    va_list vals;
    va_start(vals, frmt);
    fprintf(ctx->out, "\t");
    vfprintf(ctx->out, frmt, vals);
    fprintf(ctx->out, "\n");
    va_end(vals, frmt);
}
void emit_string(context* ctx, const char* frmt, ...)
{
    va_list vals;
    va_start(vals, frmt);
    vfprintf(ctx->out, frmt, vals);
    va_end(vals);
}

void emit_addr_of(context* ctx, symbol_link* sym)
{
    if (sym->is_global)
        emit_line(ctx, "lea si, [_%s]", sym->name);
    else
        emit_line(ctx, "lea si, [bp - %d]", sym->offset);
}

void emit_load(context* ctx, symbol_link* sym)
{
    if (sym->is_global) {
        if (sym->type == TYPE_STRING)
            emit_line(ctx, "lea ax, [_%s]", sym->name);
        else
            emit_line(ctx, "mov ax, [_%s]", sym->name);
        if (sym->type == TYPE_RATIONAL || sym->type == TYPE_FLOAT)
            emit_line(ctx, "mov bx, [_%s_hi]", sym->name);
    }
    else {
        if (sym->type == TYPE_STRING)
            emit_line(ctx, "lea ax, [bp - %d]", sym->offset);
        else
            emit_line(ctx, "mov ax, [bp - %d]", sym->offset);
        if (sym->type == TYPE_RATIONAL || sym->type == TYPE_FLOAT)
            emit_line(ctx, "mov bx, [bp - %d]", sym->offset - 2);
    }
}

void emit_store(context* ctx, symbol_link* sym)
{
    if (sym->is_global) {
        emit_line(ctx, "mov [_%s], ax", sym->name);
        if (sym->type == TYPE_RATIONAL || sym->type == TYPE_FLOAT)
            emit_line(ctx, "mov [_%s_hi], bx", sym->name);
    }
    else {
        emit_line(ctx, "mov [bp - %d], ax", sym->offset);
        if (sym->type == TYPE_RATIONAL || sym->type == TYPE_FLOAT)
            emit_line(ctx, "mov [bp - %d], bx", sym->offset - 2);
    }
}

void emit_label(context* ctx, const char* label)
{
    fprintf(ctx->out, "%s:\n", label);
}
void emit_jmp(context* ctx, const char* label)
{
    fprintf(ctx->out, "\tjmp %s\n", label);
}

void emit_global_var(context* ctx, symbol_link* sym)
{
    if (sym->type == TYPE_FLOAT || sym->type == TYPE_RATIONAL)
        emit_string(ctx, global_frmts[sym->type], sym->name, sym->name);
    else
        emit_string(ctx, global_frmts[sym->type], sym->name);
}
//conditional jmps 
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
    if (!sym)
        fprintf(ctx->out, "\tmov ax, 0\n");
    else if (sym->is_global)
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

void emit_print_int_proc(context* ctx)
{
    emit_line(ctx, "print_int proc");
    emit_line(ctx, "\tcmp ax, 0");
    emit_line(ctx, "\tjge pi_positive");
    emit_line(ctx, "\tpush ax");
    emit_line(ctx, "\tmov dl, '-'");
    emit_line(ctx, "\tmov ah, 02h");
    emit_line(ctx, "\tint 21h");
    emit_line(ctx, "\tpop ax");
    emit_line(ctx, "\tneg ax");
    emit_line(ctx, "pi_positive:");
    emit_line(ctx, "\tmov cx, 0");
    emit_line(ctx, "\tmov bx, 10");
    emit_line(ctx, "pi_divide_loop:");
    emit_line(ctx, "\tmov dx, 0");
    emit_line(ctx, "\tdiv bx");
    emit_line(ctx, "\tpush dx");
    emit_line(ctx, "\tinc cx");
    emit_line(ctx, "\tcmp ax, 0");
    emit_line(ctx, "\tjne pi_divide_loop");
    emit_line(ctx, "pi_print_loop:");
    emit_line(ctx, "\tpop dx");
    emit_line(ctx, "\tadd dl, '0'");
    emit_line(ctx, "\tmov ah, 02h");
    emit_line(ctx, "\tint 21h");
    emit_line(ctx, "\tloop pi_print_loop");
    emit_line(ctx, "\tret");
    emit_line(ctx, "print_int endp\n");
}

void emit_print_char_proc(context* ctx)
{
    emit_line(ctx, "print_char proc");
    emit_line(ctx, "\tmov ah, 02h");
    emit_line(ctx, "\tint 21h");
    emit_line(ctx, "\tret");
    emit_line(ctx, "print_char endp\n");
}

void emit_print_string_proc(context* ctx)
{
    emit_line(ctx, "print_string proc");
    emit_line(ctx, "\tmov ah, 09h");
    emit_line(ctx, "\tint 21h");
    emit_line(ctx, "\tret");
    emit_line(ctx, "print_string endp\n");
}

void emit_print_rational_proc(context* ctx)
{
    emit_line(ctx, "print_rational proc");
    emit_line(ctx, "\tpush bx");
    emit_line(ctx, "\tcall print_int");
    emit_line(ctx, "\tpop bx");
    emit_line(ctx, "\tpush bx");
    emit_line(ctx, "\tmov dl, '/'");
    emit_line(ctx, "\tmov ah, 02h");
    emit_line(ctx, "\tint 21h");
    emit_line(ctx, "\tpop ax");
    emit_line(ctx, "\tcall print_int");
    emit_line(ctx, "\tret");
    emit_line(ctx, "print_rational endp\n");
}

void emit_print_float_proc(context* ctx)
{
    emit_line(ctx, "print_float proc");
    emit_line(ctx, "\tcall print_int");
    emit_line(ctx, "\tmov dl, '.'");
    emit_line(ctx, "\tmov ah, 02h");
    emit_line(ctx, "\tint 21h");
    emit_line(ctx, "\tmov ax, bx");
    emit_line(ctx, "\tcmp ax, 10");
    emit_line(ctx, "\tjge pf_print_frac");
    emit_line(ctx, "\tmov dl, '0'");
    emit_line(ctx, "\tmov ah, 02h");
    emit_line(ctx, "\tint 21h");
    emit_line(ctx, "pf_print_frac:");
    emit_line(ctx, "\tcall print_int");
    emit_line(ctx, "\tret");
    emit_line(ctx, "print_float endp\n");
}

void emit_procedures(context* ctx)
{
    emit_print_int_proc(ctx);
    emit_print_char_proc(ctx);
    emit_print_string_proc(ctx);
    emit_print_rational_proc(ctx);
    emit_print_float_proc(ctx);
}


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



void gen_globals(context* ctx)
{
    scope scp = ctx->current_scope;
    while (scp->parent != NULL)
        scp = scp->parent;

    for (int i = 0; i < TABLE_ROWS; i++)
    {
        symbol_link* sym = scp->table[i];
        while (sym)
        {
            emit_global_var(ctx, sym);
        }
    }
}

void gen_stmt_list(context* ctx, AST node)
{
    int i;
    for (i = 0; i < node->children_count; i++)
        gen_node(ctx, node);
}

void gen_block(context* ctx, AST node)
{
    gen_node(ctx, node->children[0]);
}

void gen_var_declare(context* ctx, AST node)
{
    AST param = node->children[0];
    AST id;
    if (param->kind == NODE_PARAMETER)
        id = param->children[1];
    else
        id = param;

    symbol_link* sym = get_symbol(ctx->current_scope, id->data.name);

    if (sym->is_global)
    {
        emit_string(ctx, "\n.DATA\n");
        emit_global_var(ctx, sym);
        emit_string(ctx, ".CODE\n");
    }
    if (node->children_count == 2 && node->children[1])
    {
        gen_expr(ctx, node->children[1]);
        emit_store(ctx, sym);
    }
}

void re_enter_params(context* ctx, AST prm_lst)
{
    AST param;
    symbol_link* psym;
    char* pname;
    int i;
    for (i = 0; i < prm_lst->children_count; i++) {
        param = prm_lst->children[i];
        pname = param->children[0]->data.name;
        psym = lookup(ctx, pname);
        if (!psym)
        {
        }
    }
}
void gen_func_declare(context* ctx, AST node)
{
    AST name_param = node->children[0];
    AST param_list = node->children[1];
    AST body = node->children[2];
    
    char* fname = name_param->children[1]->data.name;   /* id child */
    type_kind rettype = name_param->children[0]->type;
    
    if (rettype == TYPE_STRING)
        reg_str_ret_buff(fname);

    emit_string(ctx, "\n%s PROC\n", fname);

    /* standard prologue */
    emit_line(ctx, "push bp");
    emit_line(ctx, "mov bp, sp");

    /* save caller registers */
    emit_line(ctx, "push bx");
    emit_line(ctx, "push cx");
    emit_line(ctx, "push dx");
    emit_line(ctx, "push si");
    emit_line(ctx, "push di");

    enter_ctx_scope(ctx);

    re_enter_params(ctx, param_list);

    int local_size = ctx->current_scope->offset_next;
    if (local_size > 0)
        emit_line(ctx, "sub sp, %d", local_size);

    gen_node(ctx, body); 

    /* epilogue (also emitted by gen_return; this is the fall-through path) */
    emit_string(ctx, "_%s_epilogue:\n", fname);
    emit_line(ctx, "pop di");
    emit_line(ctx, "pop si");
    emit_line(ctx, "pop dx");
    emit_line(ctx, "pop cx");
    emit_line(ctx, "pop bx");
    emit_line(ctx, "mov sp, bp");
    emit_line(ctx, "pop bp");
    emit_line(ctx, "ret");
    emit_string(ctx, "%s ENDP\n", fname);
}

void gen_assignment(context* ctx, AST node)
{
    char* name = node->children[0]->data.name;
    symbol_link* sym = lookup(ctx, name);

    gen_expr(ctx, node->children[1]);
    
    if (sym->type == TYPE_STRING)
    {
        emit_line(ctx, "push ax");          /* save src ptr */
        emit_addr_of(ctx, sym);             /* SI = dest     */
        emit_line(ctx, "pop bx");           /* BX = src      */
        /* inline strcpy loop */
        int lbl = new_label(ctx);
        emit_string(ctx, "_strcpy_%d:\n", lbl);
        emit_line(ctx, "mov al, [bx]");
        emit_line(ctx, "mov [si], al");
        emit_line(ctx, "cmp al, 0");
        emit_line(ctx, "je _strcpy_%d_done", lbl);
        emit_line(ctx, "inc bx");
        emit_line(ctx, "inc si");
        emit_line(ctx, "jmp _strcpy_%d", lbl);
        emit_string(ctx, "_strcpy_%d_done:\n", lbl);
    }
    else
        emit_store(ctx, sym);

    emit_line(ctx, "mov [_UNDERLINE], ax");
}

void gen_return(context* ctx, AST node)
{
    gen_expr(ctx, node->children[0]);  

    emit_line(ctx, "pop di");
    emit_line(ctx, "pop si");
    emit_line(ctx, "pop dx");
    emit_line(ctx, "pop cx");
    emit_line(ctx, "pop bx");
    emit_line(ctx, "mov sp, bp");
    emit_line(ctx, "pop bp");
    emit_line(ctx, "ret");

}

void gen_break(context* ctx, AST node)
{
    (void)node;
    if (ctx->break_label)
        emit_line(ctx, "jmp %s", ctx->break_label);
}

void gen_pass(context* ctx, AST node)
{
    (void)node;
    emit_line(ctx, "nop");
}

void gen_if(context* ctx, AST node)
{
    char* l_else = new_label(ctx, "else");
    char* l_end = new_label(ctx);

    gen_expr(ctx, node->children[0]);   /* condition */
    emit_cmp_ax_0(ctx);
    emit_je(ctx, l_else);

    GENERATOR[node->children[1]->kind](ctx, node->children[1]); /* then */

    emit_string(ctx, "_lbl_%d:\n", l_end);
    emit_string(ctx, "_lbl_%d:\n", l_else);

    if (node->children_count > 2 && node->children[2])
        GENERATOR[node->children[2]->kind](ctx, node->children[2]); /* else */

    emit_string(ctx, "_lbl_%d:\n", l_end);

    free(l_else);
    free(l_end);
}

void gen_loop(context* ctx, AST node)
{
    /* while-style:  children[0]=condition  children[1]=body
       for-style:    children[0]=iterator   children[1]=step  children[2]=body */
    int l_start = new_label(ctx);
    int l_end = new_label(ctx);
    char* prev_break = ctx->break_label;

    sprintf(ctx->break_label, "_lbl_%d", l_end);

    if (node->children_count == 2 &&
        node->children[1]->kind == NODE_BLOCK) {
        //CHANGE: a while loop not a set rep loop
        int top_lbl = new_label(ctx);
        gen_expr(ctx, node->children[0]);   /* count -> AX */
        emit_line(ctx, "mov cx, ax");
        emit_line(ctx, "cmp cx, 0");
        emit_line(ctx, "je _lbl_%d", l_end);
        emit_string(ctx, "_lbl_%d:\n", top_lbl);
        gen_node(ctx, node->children[1]);
        emit_line(ctx, "loop _lbl_%d", top_lbl);
    }
    else {
        /*--------------------------------------------------------------
         * For loop:  loop( decl var = start -> end ; step ) { block }
         * children[0] = var decl node (NODE_VAR_DECLARE)
         * children[1] = limit expr
         * children[2] = step stmt
         * children[3] = block
         *------------------------------------------------------------*/
        int top_lbl = new_label(ctx);
        int cont_lbl = new_label(ctx);

        gen_var_declare(ctx, node->children[0]);    /* init var */
        char* vname = node->children[0]->children[0]->data.name;
        symbol_link* sym = lookup(ctx, vname);

        emit_string(ctx, "_lbl_%d:\n", top_lbl);
        /* condition: var <= limit */
        gen_expr(ctx, node->children[1]);           /* limit -> AX */
        emit_load(ctx, sym);                        /* var   -> AX (overwrites) */
        /* We need both; save limit first */
        emit_line(ctx, "push ax");                  /* push var */
        gen_expr(ctx, node->children[1]);           /* limit -> AX */
        emit_line(ctx, "pop bx");                   /* BX = var  */
        emit_line(ctx, "cmp bx, ax");
        emit_line(ctx, "jg _lbl_%d", l_end);   /* var > limit -> done */

        gen_node(ctx, node->children[3]);           /* body */

        emit_string(ctx, "_lbl_%d:\n", cont_lbl);
        gen_node(ctx, node->children[2]);           /* step */
        emit_line(ctx, "jmp _lbl_%d", top_lbl);
    }

    emit_string(ctx, "_lbl_%d:\n", l_end);
    ctx->break_label = prev_break;
}

void gen_print(context* ctx, AST node)
{
    gen_expr(ctx, node->children[0]);

    /* choose runtime routine by the child's resolved type */
    type_kind k = node->children[0]->type;
    if (k == TYPE_STRING)
    {
        emit_line(ctx, "mov dx, ax");
        emit_line(ctx, "call print_str");
    }
    else if (k == TYPE_CHAR)
    {
        emit_line(ctx, "mov dl, al");
        emit_line(ctx, "call print_char");
    }
    else if (k == TYPE_INT)
        emit_line(ctx, "call print_int");
    else if (k == TYPE_NATURAL)
        emit_line(ctx, "call print_int");
    else if (k == TYPE_RATIONAL)
        emit_line(ctx, "call print_rat");
    else if (k == TYPE_FLOAT)
        emit_line(ctx, "call print_flt");
    else if (k == TYPE_BOOL)
        emit_line(ctx, "call print_bool");

    /* store result into UNDERLINE for '_' references */
    emit_line(ctx, "mov [_UNDERLINE], ax");
}

void gen_scan(context* ctx, AST node)
{
    type_kind type = node->type;

    if (type == TYPE_STRING)
        emit_scan_str(ctx);
    else if (type == TYPE_INT || TYPE_NATURAL)
        emit_scan_int(ctx);
    else if (type == TYPE_CHAR)
        emit_scan_char(ctx);
    else if (type == TYPE_RATIONAL || TYPE_FLOAT)
    {
    }
    else if (type == TYPE_BOOL);
}

void gen_use(context* ctx, AST node)
{
    gen_node(ctx, node->children[0]);
}

//
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

void gen_bit_or(context* ctx, AST node) 
{ 
    gen_bit(ctx, node); 
}

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


/* ?? print ?? */



void gen_func_call(context* ctx, AST node)
{
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



















   /* USE declaration: e.g. "use : expr" – evaluate and discard */


/* Function declaration */


void gen_node(context* ctx, AST node)
{
    GENERATOR[node->kind](ctx, node);
}

/* Start / entry-point node */
void gen_start(context* ctx, AST node)
{
    emit_string(ctx, ".MODEL SMALL\n");
    emit_string(ctx, ".STACK 100h\n");
    emit_string(ctx, ".DATA\n");
    emit_string(ctx, "; --- global variables ---\n");

    emit_string(ctx, "_UNDERLINE dw 0\n");
    emit_string(ctx, "_UNDERLINE_hi dw 0\n");
    underline_emitted = 1;

    emit_string(ctx, "; --- code section ---\n");
    emit_string(ctx, ".CODE\n");
    emit_string(ctx, "MAIN PROC\n");
    emit_string(ctx, "\tmov ax, @DATA\n");
    emit_string(ctx, "\tmov ds, ax\n");
    emit_string(ctx, "\tmov es, ax\n");

    gen_node(ctx, node->children[0]);

    emit_string(ctx, "\tmov ax, 4C00h\n");
    emit_string(ctx, "\tint 21h\n");
    emit_string(ctx, "MAIN ENDP\n\n");

    emit_procedures(ctx);

    if (strings_count > 0 || str_retbuff_count > 0) {
        emit_string(ctx, "\n.DATA\n");
        int i;
        for (i = 0; i < strings_count; i++)
            emit_string(ctx, "_str_%d db \"%s\", '$'\n", i, strings[i]);
        for (i = 0; i < str_retbuff_count; i++)
            emit_string(ctx, "_ret_%s db 255 dup(0), '$'\n", str_ret_buffs[i]);
    }

    emit_string(ctx, "END MAIN\n");
}

void init_generator_tbl()
{
    int i;
    for (i = 0; i < KIND_COUNT; i++) // default case
        GENERATOR[i] = gen_expr;

    GENERATOR[NODE_START] = gen_start;
    GENERATOR[NODE_USE_DECLARE] = gen_use;
    GENERATOR[NODE_FUNC_DECLARE] = gen_func_declare; 
    GENERATOR[NODE_VAR_DECLARE] = gen_var_declare;  
    GENERATOR[NODE_ASSIGNMENT] = gen_assignment; 

    GENERATOR[NODE_IF] = gen_if; 
    GENERATOR[NODE_LOOP] = gen_loop; 
    GENERATOR[NODE_RETURN] = gen_return;
    GENERATOR[NODE_BREAK] = gen_break; 
    GENERATOR[NODE_PASS] = gen_pass; 

    GENERATOR[NODE_IDENT] = gen_ident;
    GENERATOR[NODE_LITERAL] = gen_literal; 
    GENERATOR[NODE_CHAR] = gen_char; 
    GENERATOR[NODE_STRING] = gen_string; 
    GENERATOR[NODE_UNDERLINE] = gen_last_value;
    GENERATOR[NODE_SCAN] = gen_scan; 
    GENERATOR[NODE_PRINT] = gen_print; 

    for (i = NODE_ADD; i <= NODE_QUO; i++)
        GENERATOR[i] = gen_arith;
    GENERATOR[NODE_LOG_OR] = GENERATOR[NODE_LOG_AND] = GENERATOR[NODE_LOG_NOT] = gen_logic;
    for (i = NODE_LOG_EQUAL; i <= NODE_LESS_EQUAL; i++)
        GENERATOR[i] = gen_logic_cmpr;
    GENERATOR[NODE_BIT_AND] = GENERATOR[NODE_BIT_NOT] = GENERATOR[NODE_XOR] = gen_bit;
    GENERATOR[NODE_BIT_OR] = gen_bit_or;
    GENERATOR[NODE_BIT_RIGHT] = gen_bit_shr;
    GENERATOR[NODE_BIT_LEFT] = gen_bit_shl;

    GENERATOR[NODE_FUNC_CALL] = gen_func_call;
    GENERATOR[NODE_BLOCK] = gen_block;
    GENERATOR[NODE_STMT_LIST] = gen_stmt_list;   
    GENERATOR[NODE_TYPE_CAST] = gen_type_cast;    
    GENERATOR[NODE_PARAMETER] = gen_parameter;    
}



void code_generate(context* ctx, AST root)
{
    init_generator_tbl();
    if (root)
        gen_node(ctx, root);
}