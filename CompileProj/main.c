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
    lxr.input[terminator_index] = '\0';

    
    tokenize(&lxr);

    for (int i = 0; i < lxr.count; i++) {
        lexeme = lxr.data[i].lexeme;
        printf("%s  ", lexeme);
    }

    printf("\n\n");
    printf("%s", lxr.input);
    printf("end!");
        

	return 0;
}