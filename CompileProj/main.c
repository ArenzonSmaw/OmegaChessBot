#include <stdio.h>
#include "compiler.h"

int call_tokenize(lexer** lxr, char* inp, char* err)
{
    /*
        GETS: lexer pointer, input string and error file name
        DOES: initiates and tokenizes input string
        RETS: 1 if tokenization successful, 0 if there were any errors
    */
    int successful = 1;
    *lxr = lexer_init(inp, err);

    tokenize(*lxr);
    if (!err_is_empty((*lxr)->err_list))
    {
        //display error messages
        print_errors((*lxr)->err_list);
        successful = 0;
    }

    free_err_list(&((*lxr)->err_list));
    return successful;
}

int call_parse(parser** prsr, lexer* lxr, char* err, char* parser_file)
{
    /*
        GETS: pointer to parser and lexer, name of the error file and parser table data file
        DEOS: initiates parser and calls for parsing function
        RETS: 1 if parsing is successful, 0 if any errors appear
    */
    int successful = 1;

    *prsr = init_parser(lxr, err);

    parse(*prsr, "parser/slr.txt");

    if (!err_is_empty((*prsr)->err_lst))
    {
        successful = 0;
        print_errors((*prsr)->err_lst);
    }

    free_err_list(&(*prsr)->err_lst);
    return successful;
}

int call_semanticize(semanticer** smt, parser* prsr, char* err)
{
    /*
        GETS: pointer to semanticer and parser structs, name of error file
        DOES: initiates semanticer and calls for semantic analysis 
        RETS: 1 if semantization uccessful, 0 if any errors occur
    */
    int successful = 1;
    *smt = init_semanticer(prsr->ast, err);

    semanticize(*smt);

    if (!err_is_empty((*smt)->error))
    {
        print_errors((*smt)->error);
        successful = 0;
    }

    free_err_list(&(*smt)->error);
    return successful;
}

int call_generate(context* ctx, semanticer* smt, char* out)
{
    /*
        GETS: pointers to context and semanticer structs, and output file name
        DOES: initiates context and runs code generation function
        RETS: produces file with assembly code in the output file named file
    */
    FILE* output = fopen(out, "w");

    ctx = init_context(output, smt->current_scope);

    code_generate(ctx, smt->ast);

    fclose(output);
}

void preorder(AST ast)
{
    if (ast)
    {
        printf("%d ", ast->kind);
        for (int i = 0; i < ast->children_count; i++)
            preorder(ast->children[i]);
    }
}
void postorder(AST ast)
{
    if (ast)
    {
        for (int i = 0; i < ast->children_count; i++)
            postorder(ast->children[i]);
        switch (ast->kind)
        {
        case NODE_START:
            printf("START ");
            break;
        case NODE_PROGRAM:
            printf("PRGRM ");
            break;
        case NODE_USE_DECLARE:
            printf("use-declare ");
            break;
        case NODE_FUNC_DECLARE:
            printf("func-declare ");
            break;
        case NODE_VAR_DECLARE:
            printf("declare ");
            break;
        case NODE_ASSIGNMENT:
            printf("= ");
            break;

        case NODE_IF:
            printf("if ");
            break;
        case NODE_LOOP:
            printf("loop ");
            break;
        case NODE_RETURN:
            printf("return ");
            break;
        case NODE_BREAK:
            printf("break ");
            break;
        case NODE_PASS:
            printf("pass ");
            break;

        case NODE_IDENT:
            printf("id ");
            break;
        case NODE_LITERAL:
            printf("lit ");
            break;
        case NODE_CHAR:
            printf("ch ");
            break;
        case NODE_STRING:
            printf("str ");
            break;
        case NODE_UNDERLINE:
            printf("_ ");
            break;
        case NODE_SCAN:
            printf("scan ");
            break;
        case NODE_PRINT:
            printf("print ");
            break;

        case NODE_ADD:
            printf("+ ");
            break;
        case NODE_SUB:
            printf("- ");
            break;
        case NODE_MUL:
            printf("* ");
            break;
        case NODE_DIV:
            printf("/ ");
            break;
        case NODE_MOD:
            printf("% ");
            break;
        case NODE_QUO:
            printf("// ");
            break;
        case NODE_LOG_OR:
            printf("|| ");
            break;
        case NODE_BIT_OR:
            printf("| ");
            break;
        case NODE_LOG_AND:
            printf("&& ");
            break;
        case NODE_BIT_AND:
            printf("& ");
            break;
        case NODE_LOG_NOT:
            printf("! ");
            break;
        case NODE_BIT_NOT:
            printf("!! ");
            break;
        case NODE_BIT_RIGHT:
            printf(">> ");
            break;
        case NODE_BIT_LEFT:
            printf("<< ");
            break;
        case NODE_LOG_EQUAL:
            printf("== ");
            break;
        case NODE_LOG_DIFFERENT:
            printf("!= ");
            break;
        case NODE_GREAT:
            printf("> ");
            break;
        case NODE_GREAT_EQUAL:
            printf(">= ");
            break;
        case NODE_LESS:
            printf("< ");
            break;
        case NODE_LESS_EQUAL:
            printf("<= ");
            break;
        case NODE_XOR:
            printf("~| ");
            break;

        case NODE_FUNC_CALL:
            printf("call ");
            break;
        case NODE_BLOCK:
            printf("block ");
            break;
        case NODE_STMT_LIST:
            printf("stmt-list ");
            break;
        case NODE_TYPE_CAST:
            printf("cast ");
            break;
        case NODE_PARAMETER:
            printf("param ");
            break;
        }
    }
}

int main(/*int argc, char* argv[]*/) {

	//FILE* inp_file = fopen(argv[1], "r"); //INPUT
 //   FILE* out_file = fopen(argv[2], "w"); //OUTPUT
    char* inp_file = "textexamples/inputexample2.txt";
    char* out_file = "output/target.asm";
	lexer* lxr = NULL;
    parser* prsr = NULL;
    semanticer* smt = NULL;
    context* ctx = NULL;
    
    int successful = 0;
	

    successful = call_tokenize(&lxr, inp_file, "output/lexer_error.txt");
    
    if (successful)
    {
        successful = call_parse(&prsr, lxr, "output/parser_error.txt", "parser/slr.txt");
    }
    if (successful)
    {
        successful = call_semanticize(&smt, prsr, "output/semantic_error.txt");
    }
    if (successful)
    {
        call_generate(ctx, smt, out_file);
    }


	return 0;
}