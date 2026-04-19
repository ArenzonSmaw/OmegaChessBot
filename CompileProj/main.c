#include <stdio.h>
#include "common.h"
#include "lexer.h"
#include "parser.h"


int main(/*int argc, char* argv[]*/) {

	//FILE* inp_file = fopen(argv[1], "r"); 
    FILE* inp_file = fopen("textexamples/inputexample1.txt", "r");
	lexer lxr;
    char* lexeme;
    long size;
    long terminator_index;
	
    fseek(inp_file, 0, SEEK_END);
    size = ftell(inp_file);
    rewind(inp_file);

    lxr.input = (char*)malloc(sizeof(char) * (size + 1));
    terminator_index = fread(lxr.input, sizeof(char), size, inp_file);
    
    lxr.input[size] = '\0';

    
    tokenize(&lxr);


        

	return 0;
}