#ifndef AST_H
#define AST_H

#include "lexer.h"
#include "grammar.h"

typedef enum
{
	NODE_START,
	NODE_PROGRAM,
	NODE_USE_DECLARE,
	NODE_FUNC_DECLARE,
	NODE_VAR_DECLARE,
	NODE_ASSIGNMENT,

	NODE_IF,
	NODE_LOOP,
	NODE_RETURN,
	NODE_BREAK,
	NODE_PASS,

	NODE_IDENT,
	NODE_LITERAL,
	NODE_CHAR,
	NODE_STRING,
	NODE_UNDERLINE,
	NODE_SCAN,
	NODE_PRINT,

	NODE_ADD,
	NODE_SUB,
	NODE_MUL,
	NODE_DIV,
	NODE_MOD,
	NODE_QUO,
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

	NODE_FUNC_CALL,
	NODE_BLOCK,
	NODE_STMT_LIST,
	NODE_TYPE_CAST,
	NODE_PARAMETER,

	KIND_COUNT

}node_kind;
typedef enum
{
	TYPE_INT,
	TYPE_FLOAT,
	TYPE_NATURAL,
	TYPE_RATIONAL,
	TYPE_BOOL,
	TYPE_CHAR,
	TYPE_STRING,
	TYPE_VOID,
	TYPE_POINTER,
	TYPE_EXCEPTION,
	TYPE_ERROR,

}type_kind;

typedef enum {
	NAME = 1,
	NUM = 2,
	NONE = 3
}data_type;

data_type KIND_TO_DATA[KIND_COUNT];
//fills the kind_to_type array
void fill_kind_to_data();



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



AST init_ast(token* tkn, node_kind kind, int children);
/*
	GETS:		pointer to token tkn, semantic kind, and number of children to allocate
	RETURNS:	pointer to ast node, initialized with token value, kind and allocated children array
*/

AST create_leaf(token*, node_kind kind);
/*
	GETS:		pointer to token tkn and semantic kind
	RETURN:		pointer to ast node, with initialized token value and kind, and children array initialized to NULL
*/

void alloc_children(AST ast, int children);
/*
	GETS:		pointer to ast node and number of children to allocate
	RETURNS:	funtion allocates the children array. if ast->children is not NULL, it frees the array and allocates with desired size
*/
void realloc_children(AST ast, int children);
/*
	GETS:		pointer to ast node and number of children to reallocate to
	RETURNS:	reallocates the children array to desired size.
*/

int insert_son(AST ast, AST son, int index);
/*
	GETS:		pointer to father node, son node and desired index in the children array.
	RETURNS:	if ast->children[index] is available, assign son node and return 1. else, return 0
*/

int add_son(AST ast, AST son);
/*
	GETS:		pointer to father node and son node.
	RETURNS		finds available cell in children array. if not found, reallocates to array size +1 and assigns son node.
*/


#endif