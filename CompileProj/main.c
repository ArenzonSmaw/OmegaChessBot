#include <stdio.h>
#include "compiler.h"


int main(/*int argc, char* argv[]*/) {

	//FILE* inp_file = fopen(argv[1], "r"); 
    FILE* inp_file = fopen("textexamples/littleexample.txt", "r");
	lexer lxr;
    parser prsr;
    char* lexeme;
    long size;
    long terminator_index;
	
    fseek(inp_file, 0, SEEK_END);
    size = ftell(inp_file);
    rewind(inp_file);

    lxr.input = (char*)malloc(sizeof(char) * (size + 1));
    terminator_index = fread(lxr.input, sizeof(char), size, inp_file);
    
    lxr.input[terminator_index] = '\0';

    tokenize(&lxr);
    if (!err_is_empty(lxr.err_list))
    {
        //display error messages
        print_errors(lxr.err_list);
    }
    else 
    {
        prsr.input = lxr.data;
        prsr.input_size = lxr.count;

        parse(&prsr, "parser/slr.txt");
    }

	return 0;
}