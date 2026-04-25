#ifndef AST_H
#define AST_H

#include "lexer.h"
#include "grammar.h"

typedef enum
{
	NODE_START,
	NODE_PROGRAM,
	NODE_FUNC_DECLARE,
	NODE_VAR_DECLARE,
	NODE_ASSIGNMENT,

	NODE_IF,
	NODE_WHILE,
	NODE_RETURN,
	NODE_BREAK,
	NODE_PASS,

	NODE_IDENT,
	NODE_LITERAL,
	NODE_CHAR,
	NODE_STRING,
	NODE_UNDERLINE,

	NODE_ADD,
	NODE_SUB,
	NODE_MUL,
	NODE_DIV,
	NODE_MOD,
	NODE_QUO,
	NODE_INC_UNARY,
	NODE_INC_BINARY,
	NODE_DEC_UNARY,
	NODE_DEC_BINARY,
	NODE_MAG_UNARY,
	NODE_MAG_BINARY,
	NODE_DIM_UNARY,
	NODE_DIM_BINARY,
	NODE_LOG_OR,
	NODE_BIT_OR,
	NODE_LOG_AND,
	NODE_BIT_AND,
	NODE_LOG_NOT,
	NODE_BIT_NOT,
	NODE_BIT_RIGHT,
	NODE_BIT_LEFT,
	NODE_LOG_EQUAL,
	NODE_LOG_DIFFERENT,
	NODE_GREAT,
	NODE_GREAT_EQUAL,
	NODE_LESS,
	NODE_LESS_EQUAL,
	NODE_XOR,
	NODE_RANGE,

	NODE_FUNC_CALL,
	NODE_BLOCK,

	KIND_COUNT

}node_kind;
typedef enum
{
	TYPE_INT,
	TYPE_FLOAT,
	TYPE_NATURAL,
	TYPE_RATIONAL,
	TYPE_CHAR,
	TYPE_STRING,
	TYPE_VOID,
	TYPE_POINTER,
	TYPE_ERROR,

}type_kind;

typedef enum {
	NAME = 1,
	NUM = 2,
	NONE = 3
}data_type;

data_type KIND_TO_DATA[KIND_COUNT] = { 0 };
void fill_kind_to_data()
{
	int i;
	for (i = 0; i < KIND_COUNT; i++)
	{
		//initialize with default 'none' value
		KIND_TO_DATA[i] = NONE;
	}
	KIND_TO_DATA[NODE_LITERAL] = KIND_TO_DATA[NODE_UNDERLINE] = NUM;
	KIND_TO_DATA[NODE_IDENT] = NAME;
}


typedef struct ast_node
{
	node_kind kind;
	type_kind type;
	union {
		char* name;
		double value;
	} data;
	int line, col;

	struct ast_node** children;
	int children_count, children_size;
} syntax_node, * AST;



AST init_ast(token tkn, node_kind kind, int children);
AST create_leaf(token, node_kind kind);
void alloc_children(AST ast, int children);
int insert_son(AST ast, AST son, int index);
int add_son(AST ast, AST son);
void alloc_children(AST ast, int children);

#endif