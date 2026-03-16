#include "common.h"
#include "lexer.h"
#include "io.h"

int dict(string str) {
	if (str == "int") return 1;
	if (str == "char") return 2;
}

void printTokenType(int n) {
	switch (n) {
	case 1:
		printLn("int");
		break;

	case 2:
		printLn("char");
	}
}

/*void Parse() {
	
	int token;

	while (token != "end") {
		token = getNextToken();
	}


}*/