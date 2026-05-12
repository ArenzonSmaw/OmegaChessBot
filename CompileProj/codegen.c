#include "codegen.h"
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#pragma warning (disable:4996)

#define MAX_STRINGS 128
#define MAX_STR_RET_BUFFS 64

FILE* func_buff;

typedef void (*gen_func)(context*, AST);
gen_func GENERATOR[KIND_COUNT];
char* global_frmts[TYPE_ERROR] = {
    "_%s dw 0\n",    //TYPE_INT
    "_%s dw 0\n_%s_hi dw 0\n",   //TYPE_FLOAT
    "_%s dw 0\n",    //TYPE_NATURAL
    "_%s dw 0\n_%s_hi dw 0\n",   //TYPE_RATIONAL
    "_%s dw 0\n",    //TYPE_BOOL
    "_%s dw 0\n",    //TYPE_CHAR
    "_%s db 255 dup(0), '$'\n",  //TYPE_STRING
    "_%s dw 0\n",    //TYPE_VOID
    "_%s dw 0\n",    //TYPE_POINTER
    "_%s dw 0\n"    //TYPE_EXCEPTION
};

char* strings[MAX_STRINGS];
int strings_count = 0;

char* str_ret_buffs[MAX_STR_RET_BUFFS];
int str_retbuff_count = 0;


context* init_context(FILE* out, scope global)
{
    context* ctx = (context*)malloc(sizeof(context));
    if (ctx)
    {
        ctx->out = out;
        ctx->current_scope = global;
        ctx->current_scope->param_offset_next = ctx->current_scope->local_offset_next = 2;
        ctx->label_count = 0;
        ctx->temp_count = 0;

        strcpy(ctx->break_label, "\0");
        strcpy(ctx->step_label, "\0");
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
    scope child = init_scope(ctx->current_scope->level + 1, ctx->current_scope, 0);
    ctx->current_scope = child;
}
void exit_ctx_scope(context* ctx)
{
    int i;
    symbol_link* sym, * next;
    scope temp = ctx->current_scope;
    ctx->current_scope = temp->parent;

    if (ctx->current_scope)
    {
        ctx->current_scope->param_offset_next = temp->param_offset_next;
        ctx->current_scope->local_offset_next = temp->local_offset_next;
    }

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


void emit_func_buffer(context* ctx)
{
    int ch;
    func_buff = fopen("output/funcbuff.txt", "r");

    while ((ch = fgetc(func_buff)) != EOF) {
        fputc(ch, ctx->out);
    }
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

void emit_string_literal(context* ctx, int i, char* str)
{
    emit_string(ctx, "_str_%d db ", i);
    while (str[0] != '\0')
    {
        if (str[0] == '\\')
        {
            emit_string(ctx, "\", 0Ah, \"");
        }
        else
        {
            emit_string(ctx, "%c", str[0]);
        }
        str++;
    }
    emit_string(ctx, ", '$'\n");
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
    else if (sym->is_param) {
        if (sym->type == TYPE_STRING)
            emit_line(ctx, "lea ax, [bp + %d]", sym->offset);
        else
            emit_line(ctx, "mov ax, [bp + %d]", sym->offset);
        if (sym->type == TYPE_RATIONAL || sym->type == TYPE_FLOAT)
            emit_line(ctx, "mov bx, [bp + %d]", sym->offset + 2);
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

void emit_global_var(context* ctx, symbol_link* sym)
{
    if (sym->type == TYPE_FLOAT || sym->type == TYPE_RATIONAL)
        emit_string(ctx, global_frmts[sym->type], sym->name, sym->name);
    else
        emit_string(ctx, global_frmts[sym->type], sym->name);
}


void emit_print_int_proc(context* ctx)
{
    emit_string(ctx, "print_int proc\n");
    emit_line(ctx, "cmp ax, 0");
    emit_line(ctx, "jge pi_positive");
    emit_line(ctx, "push ax");
    emit_line(ctx, "mov dl, '-'");
    emit_line(ctx, "mov ah, 02h");
    emit_line(ctx, "int 21h");
    emit_line(ctx, "pop ax");
    emit_line(ctx, "neg ax");
    emit_string(ctx, "pi_positive:\n");
    emit_line(ctx, "mov cx, 0");
    emit_line(ctx, "mov bx, 10");
    emit_string(ctx, "pi_divide_loop:\n");
    emit_line(ctx, "mov dx, 0");
    emit_line(ctx, "div bx");
    emit_line(ctx, "push dx");
    emit_line(ctx, "inc cx");
    emit_line(ctx, "cmp ax, 0");
    emit_line(ctx, "jne pi_divide_loop");
    emit_string(ctx, "pi_print_loop:\n");
    emit_line(ctx, "pop dx");
    emit_line(ctx, "add dl, '0'");
    emit_line(ctx, "mov ah, 02h");
    emit_line(ctx, "int 21h");
    emit_line(ctx, "loop pi_print_loop");
    emit_line(ctx, "ret");
    emit_string(ctx, "print_int endp\n\n");
}

void emit_print_char_proc(context* ctx)
{
    emit_string(ctx, "print_char proc\n");
    emit_line(ctx, "cmp al, '\\'");
    emit_line(ctx, "jne _print_char");
    emit_line(ctx, "mov dl, 0Ah");
    emit_string(ctx, "_print_char:\n");
    emit_line(ctx, "mov ah, 02h");
    emit_line(ctx, "int 21h");
    emit_line(ctx, "ret");
    emit_string(ctx, "print_char endp\n\n");
}

void emit_print_string_proc(context* ctx)
{
    emit_string(ctx, "print_string proc\n");
    emit_line(ctx, "mov ah, 09h");
    emit_line(ctx, "int 21h");
    emit_line(ctx, "ret");
    emit_string(ctx, "print_string endp\n\n");
}

void emit_print_rational_proc(context* ctx)
{
    emit_string(ctx, "print_rational proc\n");
    emit_line(ctx, "push bx");
    emit_line(ctx, "call print_int");
    emit_line(ctx, "pop bx");
    emit_line(ctx, "push bx");
    emit_line(ctx, "mov dl, '/'");
    emit_line(ctx, "mov ah, 02h");
    emit_line(ctx, "int 21h");
    emit_line(ctx, "pop ax");
    emit_line(ctx, "call print_int");
    emit_line(ctx, "mov dl, 0Ah");
    emit_line(ctx, "mov ah, 02h");
    emit_line(ctx, "int 21h");
    emit_line(ctx, "ret");
    emit_string(ctx, "print_rational endp\n\n");
}

void emit_print_float_proc(context* ctx)
{
    emit_string(ctx, "print_float proc\n");
    emit_line(ctx, "push bx");
    emit_line(ctx, "call print_int");
    emit_line(ctx, "pop bx");
    emit_line(ctx, "mov dl, '.'");
    emit_line(ctx, "mov ah, 02h");
    emit_line(ctx, "int 21h");
    emit_line(ctx, "mov ax, bx");
    emit_line(ctx, "cmp ax, 10");
    emit_line(ctx, "jge pf_print_frac");
    emit_line(ctx, "mov dl, '0'");
    emit_line(ctx, "mov ah, 02h");
    emit_line(ctx, "int 21h");
    emit_line(ctx, "mov ax, bx");
    emit_string(ctx, "pf_print_frac:\n");
    emit_line(ctx, "call print_int");
    emit_line(ctx, "mov dl, 0Ah");
    emit_line(ctx, "mov ah, 02h");
    emit_line(ctx, "int 21h");
    emit_line(ctx, "ret");
    emit_string(ctx, "print_float endp\n\n");
}

void emit_print_bool_proc(context* ctx)
{
    emit_string(ctx, "print_bool proc\n");
    emit_line(ctx, "cmp al, 0");
    emit_line(ctx, "je print_bool_false");
    emit_line(ctx, "lea dx, true");
    emit_line(ctx, "jmp _print_bool");
    emit_string(ctx, "print_bool_false:\n");
    emit_line(ctx, "lea dx, false");
    emit_string(ctx, "_print_bool:\n");
    emit_line(ctx, "mov ah, 09h");
    emit_line(ctx, "int 21h");
    emit_line(ctx, "mov dl, 0Ah");
    emit_line(ctx, "mov ah, 02h");
    emit_line(ctx, "int 21h");
    emit_line(ctx, "ret");
    emit_string(ctx, "print_bool endp\n\n");
}

void emit_scan_int_proc(context* ctx)
{
    emit_string(ctx, "scan_int PROC\n");
    emit_line(ctx, "mov bx, 0");
    emit_line(ctx, "mov dx, 0");
    emit_line(ctx, "mov ah, 01h");
    emit_line(ctx, "int 21h");

    emit_line(ctx, "cmp al, '-'");
    emit_line(ctx, "jne _scan_int_cont_1");
    emit_line(ctx, "mov dx, 1");
    emit_string(ctx, "_scan_int_cont_1:\n");
    emit_line(ctx, "push dx");
    emit_line(ctx, "jne _scan_int_cont_2");

    emit_string(ctx, "_scan_int:\n");
    emit_line(ctx, "mov ah, 01h");
    emit_line(ctx, "int 21h");

    emit_string(ctx, "_scan_int_cont_2:\n");
    emit_line(ctx, "cmp al, 0Dh");
    emit_line(ctx, "je _scan_int_done");

    emit_line(ctx, "sub al, '0'");
    emit_line(ctx, "cbw");
    emit_line(ctx, "xchg ax, bx");
    emit_line(ctx, "mov cx, 10");
    emit_line(ctx, "mul cx");
    emit_line(ctx, "add ax, bx");
    emit_line(ctx, "mov bx, ax");
    emit_line(ctx, "jmp _scan_int");

    emit_string(ctx, "_scan_int_done:\n");
    emit_line(ctx, "mov ax, bx");
    emit_line(ctx, "pop dx");
    emit_line(ctx, "cmp dx, 0");
    emit_line(ctx, "je _scan_int_ret");
    emit_line(ctx, "neg ax");

    emit_string(ctx, "_scan_int_ret:\n");
    emit_line(ctx, "ret");
    emit_string(ctx, "scan_int ENDP\n\n");

}

void emit_scan_float_proc(context* ctx)
{
    emit_string(ctx, "scan_float PROC\n");
    emit_line(ctx, "mov bx, 0");

    emit_string(ctx, "_scan_ipart:\n");
    emit_line(ctx, "mov ah, 01h");
    emit_line(ctx, "int 21h");
    emit_line(ctx, "cmp al, 0Dh");
    emit_line(ctx, "je _scan_ipart_end");
    emit_line(ctx, "cmp al, '.'");
    emit_line(ctx, "je _scan_fpart_start");

    emit_line(ctx, "sub al, '0'");
    emit_line(ctx, "cbw");
    emit_line(ctx, "xchg ax, bx");
    emit_line(ctx, "mov cx, 10");
    emit_line(ctx, "mul cx");
    emit_line(ctx, "add ax, bx");
    emit_line(ctx, "mov bx, ax");
    emit_line(ctx, "jmp _scan_ipart");

    emit_string(ctx, "_scan_fpart_start:\n");
    emit_line(ctx, "push bx");
    emit_string(ctx, "_scan_fpart:\n");
    emit_line(ctx, "mov ah, 01h");
    emit_line(ctx, "int 21h");
    emit_line(ctx, "cmp al, 0Dh");
    emit_line(ctx, "je _scan_fpart_end");

    emit_line(ctx, "sub al, '0'");
    emit_line(ctx, "cbw");
    emit_line(ctx, "xchg ax, bx");
    emit_line(ctx, "mov cx, 10");
    emit_line(ctx, "mul cx");
    emit_line(ctx, "add ax, bx");
    emit_line(ctx, "mov bx, ax");
    emit_line(ctx, "jmp _scan_fpart");

    emit_string(ctx, "_scan_fpart_end:\n");
    emit_line(ctx, "mov bx, ax");
    emit_line(ctx, "pop ax");
    emit_line(ctx, "jmp _scan_float_done");
    emit_string(ctx, "_scan_ipart_end:\n");
    emit_line(ctx, "mov ax, bx");
    emit_string(ctx, "_scan_float_done:\n");
    emit_line(ctx, "ret");
    emit_string(ctx, "scan_float ENDP\n\n");
}

void emit_scan_rational_proc(context* ctx)
{
    emit_string(ctx, "scan_rational PROC\n");
    emit_line(ctx, "mov bx, 0");

    emit_string(ctx, "_scan_npart:\n");
    emit_line(ctx, "mov ah, 01h");
    emit_line(ctx, "int 21h");
    emit_line(ctx, "cmp al, 0Dh");
    emit_line(ctx, "je _scan_npart_end");
    emit_line(ctx, "cmp al, '/'");
    emit_line(ctx, "je _scan_dpart_start");

    emit_line(ctx, "sub al, '0'");
    emit_line(ctx, "cbw");
    emit_line(ctx, "xchg ax, bx");
    emit_line(ctx, "mov cx, 10");
    emit_line(ctx, "mul cx");
    emit_line(ctx, "add ax, bx");
    emit_line(ctx, "mov bx, ax");
    emit_line(ctx, "jmp _scan_npart");

    emit_string(ctx, "_scan_dpart_start:\n");
    emit_line(ctx, "push bx");
    emit_string(ctx, "_scan_dpart:\n");
    emit_line(ctx, "mov ah, 01h");
    emit_line(ctx, "int 21h");
    emit_line(ctx, "cmp al, 0Dh");
    emit_line(ctx, "je _scan_dpart_end");

    emit_line(ctx, "sub al, '0'");
    emit_line(ctx, "cbw");
    emit_line(ctx, "xchg ax, bx");
    emit_line(ctx, "mov cx, 10");
    emit_line(ctx, "mul cx");
    emit_line(ctx, "add ax, bx");
    emit_line(ctx, "mov bx, ax");
    emit_line(ctx, "jmp _scan_dpart");

    emit_string(ctx, "_scan_dpart_end:\n");
    emit_line(ctx, "mov bx, ax");
    emit_line(ctx, "pop ax");
    emit_line(ctx, "jmp _scan_ratnal_done");
    emit_string(ctx, "_scan_npart_end:\n");
    emit_line(ctx, "mov ax, bx");
    emit_line(ctx, "mov bx, 0");
    emit_string(ctx, "_scan_ratnal_done:\n");
    emit_line(ctx, "ret");
    emit_string(ctx, "scan_rational ENDP\n\n");
}

void emit_scan_bool_proc(context* ctx)
{
    emit_string(ctx, "scan_bool PROC\n");
    emit_line(ctx, "mov ah, 1h");
    emit_line(ctx, "int 21h");
    emit_line(ctx, "cmp al, 'y'");
    emit_line(ctx, "je scan_bool_true");
    emit_line(ctx, "cmp al, 'Y'");
    emit_line(ctx, "je scan_bool_true");
    emit_line(ctx, "cmp al, 't'");
    emit_line(ctx, "je scan_bool_true");
    emit_line(ctx, "cmp al, 'T'");
    emit_line(ctx, "je scan_bool_true");
    emit_line(ctx, "jmp scan_bool_true");
    emit_line(ctx, "mov ax, 0");
    emit_line(ctx, "jmp scan_bool_finish");
    emit_string(ctx, "scan_bool_true:\n");
    emit_line(ctx, "mov ax, 1");
    emit_string(ctx, "scan_bool_finish:\n");
    emit_line(ctx, "ret");
    emit_string(ctx, "scan_bool ENDP\n\n");
}

void emit_f_add_proc(context* ctx) 
{
    emit_string(ctx, "f_add PROC\n");
    emit_line(ctx, "add bx, dx");
    emit_line(ctx, "cmp bx, 100");
    emit_line(ctx, "jl _fadd_no_carry");
    emit_line(ctx, "sub bx, 100");
    emit_line(ctx, "inc ax");
    emit_string(ctx, "_fadd_no_carry:\n");
    emit_line(ctx, "add ax, cx");
    emit_line(ctx, "ret");
    emit_string(ctx, "f_add ENDP\n\n");
}
void emit_f_sub_proc(context* ctx) 
{
    emit_string(ctx, "f_sub PROC\n");
    emit_line(ctx, "sub bx, dx");
    emit_line(ctx, "cmp bx, 0");
    emit_line(ctx, "jge _fsub_no_borrow");
    emit_line(ctx, "add bx, 100");
    emit_line(ctx, "dec ax");
    emit_string(ctx, "_fsub_no_borrow:\n");
    emit_line(ctx, "sub ax, cx");
    emit_line(ctx, "ret");
    emit_string(ctx, "f_sub ENDP\n\n");
}
void emit_f_mul_proc(context* ctx) 
{
    emit_string(ctx, "f_mul PROC\n");
    emit_line(ctx, "push ax");
    emit_line(ctx, "mov ax, bx");
    emit_line(ctx, "mul cx");
    emit_line(ctx, "push cx");
    emit_line(ctx, "mov cx, 100");
    emit_line(ctx, "div cx");
    emit_line(ctx, "xchg ax, dx");
    emit_line(ctx, "pop cx");
    emit_line(ctx, "mov bx, ax");
    emit_line(ctx, "pop ax");
    emit_line(ctx, "push dx");
    emit_line(ctx, "imul cx");
    emit_line(ctx, "pop dx");
    emit_line(ctx, "add ax, dx");
    emit_line(ctx, "ret");
    emit_string(ctx, "f_mul ENDP\n\n");
}
void emit_f_div_proc(context* ctx) 
{
    emit_string(ctx, "f_div PROC\n");
    emit_line(ctx, "mov dx, 0");
    emit_line(ctx, "idiv cx");
    emit_line(ctx, "push ax");
    emit_line(ctx, "mov ax, bx");
    emit_line(ctx, "add ax, dx");
    emit_line(ctx, "mov dx, 0");
    emit_line(ctx, "div cx");
    emit_line(ctx, "mov bx, ax");
    emit_line(ctx, "pop ax");
    emit_line(ctx, "ret");
    emit_string(ctx, "f_div ENDP\n\n");
}

void emit_f_cmpr_proc(context* ctx) 
{
    emit_string(ctx, "f_cmp PROC\n");
    emit_line(ctx, "cmp ax, cx");
    emit_line(ctx, "jne _fcmp_done");
    emit_line(ctx, "cmp bx, dx");
    emit_string(ctx, "_fcmp_done:\n");
    emit_line(ctx, "ret");
    emit_string(ctx, "f_cmp ENDP\n\n");
}

void emit_conv_rat_to_float(context* ctx)
{
    emit_string(ctx, "_conv_flt PROC\n");
    emit_line(ctx, "div bx");
    emit_line(ctx, "push ax");
    emit_line(ctx, "push bx");
    emit_line(ctx, "mov ax, dx");
    emit_line(ctx, "mov bx, 100");
    emit_line(ctx, "mul bx");
    emit_line(ctx, "pop bx");
    emit_line(ctx, "xor dx, dx");
    emit_line(ctx, "div bx");
    emit_line(ctx, "mov bx, ax");
    emit_line(ctx, "pop ax");
    emit_line(ctx, "ret");
    emit_string(ctx, "_conv_flt ENDP\n\n");
}

void emit_conv_float_to_rat(context* ctx)
{
    emit_string(ctx, "_conv_rat PROC\n");
    emit_line(ctx, "mov cx, 100");
    emit_line(ctx, "imul cx");
    emit_line(ctx, "cmp dx, 0");
    emit_line(ctx, "je _conv_rat_cont");
    emit_line(ctx, "mov cx, 10");
    emit_line(ctx, "idiv cx");
    emit_line(ctx, "mov bx, 10");
    emit_line(ctx, "push ax");
    emit_line(ctx, "mov ax, bx");
    emit_line(ctx, "div cx");
    emit_line(ctx, "mov dx, ax");
    emit_line(ctx, "pop ax");
    emit_line(ctx, "add ax, dx");
    emit_line(ctx, "mov bx, 10");
    emit_line(ctx, "jmp _conv_rat_end");
    emit_string(ctx, "_conv_rat_cont:\n");
    emit_line(ctx, "add ax, bx");
    emit_line(ctx, "mov bx, 100");
    emit_string(ctx, "_conv_rat_end:\n");
    emit_line(ctx, "ret");
    emit_string(ctx, "_conv_rat ENDP\n\n");
}


void emit_procedures(context* ctx)
{
    emit_print_int_proc(ctx);
    emit_print_char_proc(ctx);
    emit_print_string_proc(ctx);
    emit_print_rational_proc(ctx);
    emit_print_float_proc(ctx);
    emit_print_bool_proc(ctx);

    emit_scan_int_proc(ctx);
    emit_scan_float_proc(ctx);
    emit_scan_rational_proc(ctx);
    emit_scan_bool_proc(ctx);

    emit_f_add_proc(ctx);
    emit_f_sub_proc(ctx);
    emit_f_mul_proc(ctx);
    emit_f_div_proc(ctx);
    emit_f_cmpr_proc(ctx);

    emit_conv_float_to_rat(ctx);
    emit_conv_rat_to_float(ctx);
}


void emit_scan_char(context* ctx)
{
    emit_line(ctx, "mov ah, 01h");
    emit_line(ctx, "int 21h");
    emit_line(ctx, "cbw");
}

void emit_scan_str(context* ctx)
{
    int sid = strings_count++;

    emit_string(ctx, "\n.DATA\n");
    emit_string(ctx, "_scan_str_%d db 255, 0, 255 dup(0)\n", sid);
    emit_string(ctx, ".CODE\n");

    emit_line(ctx, "lea dx, [_scan_str_%d]", sid);
    emit_line(ctx, "mov ah, 0Ah");
    emit_line(ctx, "int 21h");

    emit_line(ctx, "lea bx, [_scan_str_%d]", sid);
    emit_line(ctx, "mov cl, [bx+1]");
    emit_line(ctx, "mov ch, 0");
    emit_line(ctx, "add bx, 2");
    emit_line(ctx, "add bx, cx");
    emit_line(ctx, "mov byte ptr [bx], '$'");
    emit_line(ctx, "lea ax, [_scan_str_%d + 2]", sid);
}

void emit_scan_int(context* ctx)
{
    emit_line(ctx, "call scan_int");
}

void emit_scan_float(context* ctx)
{
    emit_line(ctx, "call scan_float");
}

void emit_scan_rational(context* ctx)
{
    emit_line(ctx, "call scan_rational");
}

void emit_scan_bool(context* ctx)
{
    emit_line(ctx, "call scan_bool");
}



void conv_rat_to_float(context*);
void conv_float_to_rat(context*);

void gen_node(context* ctx, AST node)
{
    GENERATOR[node->kind](ctx, node);
}

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
            sym = sym->next;
        }
    }
}

void gen_stmt_list(context* ctx, AST node)
{
    int i;
    int offset = ctx->current_scope->local_offset_next;
    enter_ctx_scope(ctx);
    ctx->current_scope->local_offset_next = offset;

    for (i = 0; i < node->children_count; i++)
        gen_node(ctx, node->children[i]);

    exit_ctx_scope(ctx);
}

void gen_block(context* ctx, AST node)
{
    gen_node(ctx, node->children[0]);
}

void gen_expr(context* ctx, AST node)
{
    GENERATOR[node->kind](ctx, node);
}

void gen_var_declare(context* ctx, AST node)
{
    AST param = node->children[0];
    AST id;
    int offset;

    if (param->kind == NODE_PARAMETER)
        id = param->children[1];
    else
        id = param;

    

    symbol_link* sym = get_symbol(ctx->current_scope, id->name);

    if (sym && sym->is_global)
    {
        emit_string(ctx, "\n.DATA\n");
        emit_global_var(ctx, sym);
        emit_string(ctx, ".CODE\n");
    }
    else
    {
        offset = ctx->current_scope->local_offset_next;
        ctx->current_scope->local_offset_next += type_size(param->type);

        enter_symbol(ctx->current_scope, 
            create_symbol(id->name, VARIABLE, param->children[0]->type,
                NULL , ctx->current_scope->level, id->line, id->col, offset));
        sym = get_symbol(ctx->current_scope, id->name);
    }
    if (node->children_count == 2 && node->children[1])
    {
        gen_expr(ctx, node->children[1]);
        if (param->children[0]->type = TYPE_FLOAT && node->children[1]->type == TYPE_RATIONAL)
            conv_rat_to_float(ctx);
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
        pname = param->children[1]->name;
        psym = extract_symbol(ctx->current_scope, pname);
        if (!psym)
        {
            psym = create_symbol(pname, PARAM, param->children[0]->type, NULL, 
                ctx->current_scope->level, param->line, param->col, 
                ctx->current_scope->param_offset_next);
            ctx->current_scope->param_offset_next += type_size(psym->type);
            psym->is_param = 1;
        }
        enter_symbol(ctx->current_scope, psym);
    }
}
void gen_func_declare(context* ctx, AST node)
{
    FILE* holder = ctx->out;

    AST name_param = node->children[0];
    AST param_list = node->children[1];
    AST body = node->children[2];
    
    char* fname = name_param->children[1]->name;   /* id child */
    type_kind rettype = name_param->children[0]->type;
    
    int local_size = ctx->current_scope->local_offset_next;
    ctx->out = func_buff;

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
    emit_line(ctx, "sub sp, %d", local_size);

    enter_ctx_scope(ctx);
 
    if (param_list)
        re_enter_params(ctx, param_list);


    gen_node(ctx, body); 

    /* epilogue (also emitted by gen_return; this is the fall-through path) */
    emit_string(ctx, "_%s_epilogue:\n", fname);
    emit_line(ctx, "add sp, %d", local_size);
    emit_line(ctx, "pop di");
    emit_line(ctx, "pop si");
    emit_line(ctx, "pop dx");
    emit_line(ctx, "pop cx");
    emit_line(ctx, "pop bx");
    emit_line(ctx, "mov sp, bp");
    emit_line(ctx, "pop bp");
    emit_line(ctx, "ret");
    emit_string(ctx, "%s ENDP\n", fname);

    ctx->out = holder;
}

void gen_assignment(context* ctx, AST node)
{
    char* name = node->children[0]->name;
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
}

void gen_break(context* ctx, AST node)
{
    if (strcmp(ctx->break_label, "\0"))
        emit_line(ctx, "jmp %s", ctx->break_label);
}

void gen_pass(context* ctx, AST node)
{
    if (strcmp(ctx->step_label, "\0"))
        emit_line(ctx, "jmp %s", ctx->step_label);
}

void gen_if(context* ctx, AST node)
{
    int l_else = new_label(ctx);
    int l_end = new_label(ctx);

    gen_expr(ctx, node->children[0]);   /* condition */
    emit_line(ctx, "cmp ax, 0");
    emit_line(ctx, "je _lbl_%d ", l_else);

    gen_node(ctx, node->children[1]); /* then */

    emit_line(ctx, "jmp _lbl_%d\n", l_end);
    emit_string(ctx, "_lbl_%d:\n", l_else);

    if (node->children_count > 2 && node->children[2])
        gen_node(ctx, node->children[2]); /* else */

    emit_string(ctx, "_lbl_%d:\n", l_end);

}

void gen_loop(context* ctx, AST node)
{
    /* while-style:  children[0]=condition  children[1]=body
       for-style:    children[0]=iterator   children[1]=limit  children[2]=step children[3] = body */
    int l_start = new_label(ctx);
    int l_end = new_label(ctx);
    char prev_break[32];
    char prev_step[32];
    strcpy(prev_break, ctx->break_label);
    strcpy(prev_step, ctx->step_label);
    sprintf(ctx->break_label, "_lbl_%d", l_end);
    sprintf(ctx->step_label, "_step_%d", l_start); // was l_end, should be l_start

    enter_ctx_scope(ctx);

    if (node->children_count == 2)
    {
        // while-style: condition re-evaluated every iteration
        emit_string(ctx, "_lbl_%d:\n", l_start);
        gen_expr(ctx, node->children[0]);   /* condition -> AX */
        emit_line(ctx, "cmp ax, 0");
        emit_line(ctx, "jne _skip_%d", l_start);
        emit_line(ctx, "jmp near ptr %s", ctx->break_label);
        emit_string(ctx, "_skip_%d:\n", l_start);
        gen_node(ctx, node->children[1]);   /* body */
        emit_line(ctx, "jmp _lbl_%d", l_start);
        emit_string(ctx, "%s:\n", ctx->break_label);
    }
    else
    {
        // for-style: init, condition check, body, step, repeat
        node->children[0]->local_size += 2; // iterator variable
        gen_var_declare(ctx, node->children[0]);    /* init var */
        char* vname = node->children[0]->children[0]->children[1]->name;
        symbol_link* sym = lookup(ctx, vname);
        emit_string(ctx, "_lbl_%d:\n", l_start);
        emit_load(ctx, sym);                        /* AX = var */
        emit_line(ctx, "push ax");                  /* save var */
        gen_expr(ctx, node->children[1]);           /* limit -> AX */
        emit_line(ctx, "mov bx, ax");               /* BX = limit */
        emit_line(ctx, "pop ax");                   /* AX = var */
        emit_line(ctx, "cmp ax, bx");
        emit_line(ctx, "jl _skip_%d", l_start);
        emit_line(ctx, "jmp near ptr %s", ctx->break_label);      /* var > limit -> done */
        emit_string(ctx, "_skip_%d:\n", l_start);

        gen_node(ctx, node->children[3]);           /* body */

        emit_string(ctx, "%s:\n", ctx->step_label);
        gen_node(ctx, node->children[2]);           /* step */
        emit_line(ctx, "jmp _lbl_%d", l_start);

        emit_string(ctx, "%s:\n", ctx->break_label);
    }
    strcpy(ctx->break_label, prev_break);
    strcpy(ctx->step_label, prev_step);
    exit_ctx_scope(ctx);
}

void gen_print(context* ctx, AST node)
{
    gen_expr(ctx, node->children[0]);

    /* choose runtime routine by the child's resolved type */
    type_kind k = node->children[0]->type;
    if (k == TYPE_STRING)
    {
        emit_line(ctx, "mov dx, ax");
        emit_line(ctx, "call print_string");
    }
    else if (k == TYPE_CHAR)
    {
        emit_line(ctx, "mov dl, al");
        emit_line(ctx, "call print_char");
    }
    else if (k == TYPE_INT)
    {
        emit_line(ctx, "call print_int");
        emit_line(ctx, "mov dl, 0Ah");
        emit_line(ctx, "mov ah, 02h");
        emit_line(ctx, "int 21h");
    }
    else if (k == TYPE_NATURAL)
    {
        emit_line(ctx, "call print_int");
        emit_line(ctx, "mov dl, 0Ah");
        emit_line(ctx, "mov ah, 02h");
        emit_line(ctx, "int 21h");
    }
    else if (k == TYPE_RATIONAL)
        emit_line(ctx, "call print_rational");
    else if (k == TYPE_FLOAT)
        emit_line(ctx, "call print_float");
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
    else if (type == TYPE_RATIONAL)
        emit_scan_rational(ctx);
    else if (type == TYPE_FLOAT)
        emit_scan_float(ctx);
    else if (type == TYPE_BOOL)
        emit_scan_bool(ctx);
}

void gen_use(context* ctx, AST node)
{
    gen_node(ctx, node->children[0]);
}

void gen_literal(context* ctx, AST node)
{
    type_kind type = node->type;
    int idx, ipart, fpart;

    if (type == TYPE_STRING)
    {
        idx = add_string(node->name);
        emit_line(ctx, "lea ax, [_str_%d]", idx);
    }
    else if (type == TYPE_CHAR)
    {
        emit_line(ctx, "mov ax, %d", node->name);
    }
    else if (type == TYPE_BOOL || type == TYPE_INT || type == TYPE_NATURAL)
    {
        emit_line(ctx, "mov ax, %d", node->value1);
    }
    else if (type == TYPE_FLOAT || type == TYPE_RATIONAL)
    {
        ipart = node->value1;
        fpart = node->value2;
        emit_line(ctx, "mov ax, %d", ipart);
        emit_line(ctx, "mov bx, %d", fpart);
    }
}

void gen_ident(context* ctx, AST node)
{
    symbol_link* sym = lookup(ctx, node->name);
    if (sym)
        emit_load(ctx, sym);          
}

static void gen_underline(context* ctx, AST node)
{
    emit_line(ctx, "mov ax, [_UNDERLINE]");
}

void call_cleanup(context* ctx, symbol_link* funcsym, int count)
{
    int total_bytes = 0, i;
    type_kind prmtype;
    if (count > 0)
    {
        if (funcsym)
        {
            for (i = 0; i < funcsym->param_count; i++)
            {
                total_bytes += 2;
                prmtype = funcsym->params[i].type;
                if (prmtype == TYPE_RATIONAL || prmtype == TYPE_FLOAT)
                    total_bytes += 2;
            }
        }
        else
            total_bytes = count * 2;
        emit_line(ctx, "add sp, %d", total_bytes);
    }
}

void gen_func_call(context* ctx, AST node)
{
    symbol_link* funcsym;
    char* fname = node->children[0]->name;
    AST arg_list = node->children[1];
    int count = arg_list ? arg_list->children_count : 0;
    int total_bytes = 0, i;
    type_kind argtype;
    for (int i = count - 1; i >= 0; i--)
    {
        gen_expr(ctx, arg_list->children[i]);
        argtype = arg_list->children[i]->type;
        if (argtype == TYPE_RATIONAL || argtype == TYPE_FLOAT)
            emit_line(ctx, "push bx");
        emit_line(ctx, "push ax");
    }

    emit_line(ctx, "call %s", fname);

    call_cleanup(ctx, lookup(ctx, fname), count);
}

void gen_unary(context* ctx, AST node)
{
    node_kind kind = node->kind;
    gen_expr(ctx, node->children[0]);

    if (kind == NODE_LOG_NOT)
    {
        emit_line(ctx, "cmp ax, 0");
        emit_line(ctx, "mov ax, 0");
        emit_line(ctx, "sete al");
    }
    else if (kind == NODE_BIT_NOT)
    {
        emit_line(ctx, "not ax");
    }
    else if (kind == NODE_SUB)
    {
        emit_line(ctx, "neg ax");
    }
}

void gen_add(context* ctx, int is_float)
{
    if (is_float)
        emit_line(ctx, "call f_add");
    else
        emit_line(ctx, "add ax, bx");
}
void gen_sub(context* ctx, int is_float)
{
    if (is_float)
        emit_line(ctx, "call f_sub");
    else
        emit_line(ctx, "sub ax, bx");
}
void gen_mul(context* ctx, int is_float)
{
    if (is_float)
        emit_line(ctx, "call f_mul");
    else
        emit_line(ctx, "imul bx");
}
void gen_div(context* ctx, int is_float)
{
    if (is_float)
        emit_line(ctx, "call f_div");
    else
    {
        emit_line(ctx, "cwd");
        emit_line(ctx, "idiv bx");
    }
}
void gen_mod(context* ctx, int is_float)
{
    gen_div(ctx, 0);
    emit_line(ctx, "mov ax, dx");
}
void gen_quo(context* ctx, int is_float)
{
    gen_div(ctx, 0);
}

void gen_equal(context* ctx, int is_float)
{
    int lbl = new_label(ctx);
    if (is_float)
        emit_line(ctx, "call f_cmp");
    else
        emit_line(ctx, "cmp ax, bx");

    emit_line(ctx, "jne _equal_false_%d", lbl);
    emit_line(ctx, "mov ax, 1");
    emit_line(ctx, "jmp _equal_end_%d", lbl);
    emit_string(ctx, "_equal_false_%d:\n", lbl);
    emit_line(ctx, "mov ax, 0");
    emit_string(ctx, "_equal_end_%d:\n", lbl);
}
void gen_different(context* ctx, int is_float)
{
    int lbl = new_label(ctx);
    if (is_float)
        emit_line(ctx, "call f_cmp");
    else
        emit_line(ctx, "cmp ax, bx");

    emit_line(ctx, "je _diff_false_%d", lbl);
    emit_line(ctx, "mov ax, 1");
    emit_line(ctx, "jmp _diff_end_%d", lbl);
    emit_string(ctx, "_diff_false_%d:\n", lbl);
    emit_line(ctx, "mov ax, 0");
    emit_string(ctx, "_diff_end_%d:\n", lbl);
}
void gen_greater(context* ctx, int is_float)
{
    int lbl = new_label(ctx);
    if (is_float)
        emit_line(ctx, "call f_cmp");
    else
        emit_line(ctx, "cmp ax, bx");

    emit_line(ctx, "jng _grtr_false_%d", lbl);
    emit_line(ctx, "mov ax, 1");
    emit_line(ctx, "jmp _grtr_end_%d", lbl);
    emit_string(ctx, "_grtr_false_%d:\n", lbl);
    emit_line(ctx, "mov ax, 0");
    emit_string(ctx, "_grtr_end_%d:\n", lbl);
}
void gen_grt_eq(context* ctx, int is_float)
{
    int lbl = new_label(ctx);
    if (is_float)
        emit_line(ctx, "call f_cmp");
    else
        emit_line(ctx, "cmp ax, bx");

    emit_line(ctx, "jge  _grt_true_%d", lbl);
    emit_line(ctx, "mov ax, 0");
    emit_line(ctx, "jmp _grt_done_%d", lbl);
    emit_string(ctx, "_grt_true_%d :\n", lbl);
    emit_line(ctx, "mov ax, 1");
    emit_string(ctx, "_grt_done_%d :\n ", lbl);
}
void gen_less(context* ctx, int is_float)
{
    int lbl = new_label(ctx);
    if (is_float)
        emit_line(ctx, "call f_cmp");
    else
        emit_line(ctx, "cmp ax, bx");

    emit_line(ctx, "jnl _less_false_%d", lbl);
    emit_line(ctx, "mov ax, 1");
    emit_line(ctx, "jmp _less_end_%d", lbl);
    emit_string(ctx, "_less_false_%d:\n", lbl);
    emit_line(ctx, "mov ax, 0");
    emit_string(ctx, "_less_end_%d:\n", lbl);
}
void gen_lss_eq(context* ctx, int is_float)
{
    int lbl = new_label(ctx);
    if (is_float)
        emit_line(ctx, "call f_cmp");
    else
        emit_line(ctx, "cmp ax, bx");

    emit_line(ctx, "jle _grtr_true_%d", lbl);
    emit_line(ctx, "mov ax, 0");
    emit_line(ctx, "jmp _grtr_end_%d", lbl);
    emit_string(ctx, "_grtr_true_%d:\n", lbl);
    emit_line(ctx, "mov ax, 1");
    emit_string(ctx, "_grtr_end_%d:\n", lbl);
}

void gen_log_or(context* ctx, int is_float)
{
    int lbl = new_label(ctx);
    emit_line(ctx, "cmp ax, 0");
    emit_line(ctx, "jne _lor_true_%d", lbl);
    emit_line(ctx, "cmp bx, 0");
    emit_line(ctx, "jne _lor_true_%d", lbl);
    emit_line(ctx, "mov ax, 0");
    emit_line(ctx, "jmp _lor_done_%d", lbl);
    emit_string(ctx, "_lor_true_%d:\n", lbl);
    emit_line(ctx, "mov ax, 1");
    emit_string(ctx, "_lor_done_%d:\n", lbl);
}
void gen_log_and(context* ctx, int is_float)
{
    int lbl = new_label(ctx);
    emit_line(ctx, "cmp ax, 0");
    emit_line(ctx, "je _land_false_%d", lbl);
    emit_line(ctx, "cmp bx, 0");
    emit_line(ctx, "je _land_false_%d", lbl);
    emit_line(ctx, "mov ax, 1");
    emit_line(ctx, "jmp _land_done_%d", lbl);
    emit_string(ctx, "_land_false_%d:\n", lbl);
    emit_line(ctx, "mov ax, 0");
    emit_string(ctx, "_land_done_%d:\n", lbl);
}
void gen_log_not(context* ctx, int is_float)
{
    int lbl = new_label(ctx);
    emit_line(ctx, "cmp ax, 0");
    emit_line(ctx, "je _not_true_%d");
    emit_line(ctx, "mov ax, 0");
    emit_line(ctx, "jmp _not_end_%d");
    emit_string(ctx, "_not_true_%d:\n", lbl);
    emit_line(ctx, "mov ax, 1");
    emit_string(ctx, "_not_end_%d:\n", lbl);
}

void gen_bit_or(context* ctx, int is_float)
{
    emit_line(ctx, "or ax, bx");
}
void gen_bit_and(context* ctx, int is_float)
{
    emit_line(ctx, "and ax, bx");
}
void gen_bit_not(context* ctx, int is_float)
{
    emit_line(ctx, "not ax");
}
void gen_shl(context* ctx, int is_float)
{
    emit_line(ctx, "mov cl, bl");
    emit_line(ctx, "sal ax, cl");
}
void gen_shr(context* ctx, int is_float)
{
    emit_line(ctx, "mov cl, bl");
    emit_line(ctx, "sar ax, cl");
}
void gen_xor(context* ctx, int is_float)
{
    emit_line(ctx, "xor ax, bx");
}


typedef void (*emit_noarg_func)(context*, int);
static emit_noarg_func BINARY_OP[21] = {

    gen_add, //NODE_ADD
    gen_sub, //NODE_SUB
    gen_mul, //NODE_MUL
    gen_div, //NODE_DIV
    gen_mod, //NODE_MOD
    gen_quo, //NODE_QUO
    gen_log_or, //NODE_LOG_OR
    gen_bit_or, //NODE_BIT_OR
    gen_log_and, //NODE_LOG_AND
    gen_bit_and, //NODE_BIT_AND
    gen_log_not, //NODE_LOG_NOT
    gen_bit_not, //NODE_BIT_NOT
    gen_shr, //NODE_BIT_RIGHT
    gen_shl, //NODE_BIT_LEFT
    gen_equal, //NODE_LOG_EQUAL
    gen_different, //NODE_LOG_DIFFERENT
    gen_greater, //NODE_GREAT
    gen_grt_eq,//NODE_GREAT_EQUAL
    gen_less, //NODE_LESS
    gen_lss_eq, //NODE_LESS_EQUAL
    gen_xor //NODE_XOR

};

void conv_rat_to_float(context* ctx)
{
    emit_line(ctx, "call _conv_flt");
}

void conv_float_to_rat(context* ctx)
{
    emit_line(ctx, "call _conv_rat");
}

void gen_float_arith(context* ctx, AST node)
{
    if (node->children[0]->type == TYPE_RATIONAL)
    {
        conv_rat_to_float(ctx);
    }
    emit_line(ctx, "push ax");
    emit_line(ctx, "push bx");
    gen_expr(ctx, node->children[1]);
    emit_line(ctx, "mov cx, ax");
    if (IS_FLT_OR_RAT[node->children[1]->type])
        emit_line(ctx, "mov dx, bx");
    emit_line(ctx, "pop bx");
    emit_line(ctx, "pop ax");

    BINARY_OP[node->kind - NODE_ADD](ctx, 1);
}

void gen_binary(context* ctx, AST node)
{
    if (IS_FLT_OR_RAT[node->children[0]->type] || IS_FLT_OR_RAT[node->children[1]->type])
    {
        gen_float_arith(ctx, node);
    }
    else
    {
        emit_line(ctx, "push ax");
        gen_expr(ctx, node->children[1]);
        emit_line(ctx, "mov bx, ax");
        emit_line(ctx, "pop ax");

        BINARY_OP[node->kind - NODE_ADD](ctx, 0);
    }
}

void gen_arithmetic(context* ctx, AST node)
{
    gen_expr(ctx, node->children[0]);

    if (node->children[1])
    {
        gen_binary(ctx, node);
    }
    else
    {
        gen_unary(ctx, node);
    }

    emit_line(ctx, "mov [_UNDERLINE], ax");
}

void gen_type_cast(context* ctx, AST node)
{
    gen_expr(ctx, node->children[1]);
    type_kind to = node->children[0]->type, 
            from = node->children[1]->type;

    if (from != to)
    {
        if (to == TYPE_FLOAT && from == TYPE_RATIONAL)
            conv_rat_to_float(ctx);
        else if (to == TYPE_FLOAT)
            emit_line(ctx, "mov bx, 0");
        else if (to == TYPE_RATIONAL && from == TYPE_FLOAT)
            conv_float_to_rat(ctx);
        else if (to == TYPE_RATIONAL && from != TYPE_FLOAT)
            emit_line(ctx, "mov bx, 1");

        else if (to == TYPE_CHAR)
            emit_line(ctx, "cbw");
        else if (to == TYPE_BOOL)
        {
            emit_line(ctx, "cmp ax, 0");
            emit_line(ctx, "mov ax, 0");
            emit_line(ctx, "setne al");
        }
    }
}

void gen_parameter(context* ctx, AST node) {}

void gen_char(context* ctx, AST node)
{
    emit_line(ctx, "mov ax, %s", node->name);
}

void gen_string(context* ctx, AST node)
{
    int idx = add_string(node->name);
    emit_line(ctx, "lea ax, [_str_%d]", idx);
}

void gen_last_value(context* ctx, AST node)
{
    emit_line(ctx, "mov ax, [_UNDERLINE]");
}


void gen_start(context* ctx, AST node)
{
    emit_string(ctx, ".MODEL SMALL\n");
    emit_string(ctx, ".STACK 100h\n");
    emit_string(ctx, ".DATA\n");
    emit_string(ctx, "; --- global variables ---\n");
    emit_string(ctx, "true db 'true$'\n");
    emit_string(ctx, "false db 'false$'\n");
    emit_string(ctx, "_UNDERLINE dw 0\n");
    emit_string(ctx, "_UNDERLINE_hi dw 0\n");

    emit_string(ctx, "; --- code section ---\n");
    emit_string(ctx, ".CODE\n");
    emit_string(ctx, "MAIN PROC\n");
    emit_line(ctx, "mov ax, DGROUP");
    emit_line(ctx, "mov ds, ax");
    emit_line(ctx, "mov es, ax");
    emit_line(ctx, "mov bp, sp");
    emit_line(ctx, "sub sp, %d", node->local_size);

    func_buff = fopen("output/funcbuff.txt", "w");
    gen_node(ctx, node->children[0]);
    fclose(func_buff);

    emit_line(ctx, "add sp, %d", node->local_size);
    emit_line(ctx, "mov dl, 0Ah");
    emit_line(ctx, "call print_char");
    emit_line(ctx, "mov ax, 4C00h");
    emit_line(ctx, "int 21h");
    emit_string(ctx, "MAIN ENDP\n\n");

    emit_func_buffer(ctx);

    emit_procedures(ctx);

    if (strings_count > 0 || str_retbuff_count > 0) {
        emit_string(ctx, "\n.DATA\n");
        int i;
        for (i = 0; i < strings_count; i++)
            emit_string_literal(ctx, i, strings[i]);
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

    for (i = NODE_ADD; i <= NODE_XOR; i++)
        GENERATOR[i] = gen_arithmetic;

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