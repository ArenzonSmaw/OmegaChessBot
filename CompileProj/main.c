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

    free_err_list((*smt)->error);
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
}


int main(/*int argc, char* argv[]*/) {

	//FILE* inp_file = fopen(argv[1], "r"); //INPUT
 //   FILE* out_file = fopen(argv[2], "w"); //OUTPUT
    char* inp_file = "textexamples/inputexample1.txt";
    char* out_file = "output/outexample1.txt";
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
        call_generate(ctx, prsr, out_file);
    }


	return 0;
}