#include <stdio.h>
#include "common.h"
#include "lexer.h"
#include "parser.h"


int main(/*int argc, char* argv[]*/) {

	//FILE* inp_file = fopen(argv[1], "r"); 
    FILE* inp_file = fopen("textexamples/inputexample1.txt", "r");
	lexer lxr;
	
    fseek(inp_file, 0, SEEK_END);
    long size = ftell(inp_file);
    rewind(inp_file);

    lxr.input = (char*)malloc(sizeof(char) * (size + 1));

    fread(lxr.input, sizeof(char), size, inp_file);
    lxr.input[size] = '\0';

    
    tokenize(&lxr);

    for (int i = 0; i < lxr.count; i++)
        printf("%s  ", lxr.data[i].lexeme);
        

	return 0;
}