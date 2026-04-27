#include "lexer.h"
#include "error.h"
#pragma warning (disable: 4996)

typedef enum STATES {
	ST_START,
	ST_INTEGER,
	ST_STRINGSTART,
	ST_CHARSTART,
	ST_CHARVALUE,
	ST_BACKSLASH,
	ST_FLOATINGNUMBER,
	ST_RATIONALNUMBER,
	ST_DOT,
	ST_DIV,
	ST_PLUS,
	ST_MULT,
	ST_MOD,
	ST_OR,
	ST_TILDE,
	ST_AND,
	ST_LEFT,
	ST_RIGHT,
	ST_NOT,
	ST_EQUALS,
	ST_MINUS,
	ST_IDENTIFIER,
	ST_B,
	ST_BO,
	ST_BOO,
	ST_BOOL,
	ST_BR,
	ST_BRE,
	ST_BREA,
	ST_BREAK,
	ST_C,
	ST_CH,
	ST_CHE,
	ST_CHEC,
	ST_CHECK,
	ST_CHA,
	ST_CHAR,
	ST_D,
	ST_DE,
	ST_DEC,
	ST_DECL,
	ST_DECLA,
	ST_DECLAR,
	ST_DECLARE,
	ST_E,
	ST_EL,
	ST_ELS,
	ST_ELSE,
	ST_EX,
	ST_EXC,
	ST_EXCE,
	ST_EXCEP,
	ST_EXCEPT,
	ST_EXCEPTI,
	ST_EXCEPTIO,
	ST_EXCEPTION,
	ST_F,
	ST_FA,
	ST_FAL,
	ST_FALS,
	ST_FALSE,
	ST_FL,
	ST_FLO,
	ST_FLOA,
	ST_FLOAT,
	ST_I,
	ST_IF,
	ST_IN,
	ST_INT,
	ST_L,
	ST_LO,
	ST_LOO,
	ST_LOOP,
	ST_N,
	ST_NA,
	ST_NAT,
	ST_NATU,
	ST_NATUR,
	ST_NATURA,
	ST_NATURAL,
	ST_P,
	ST_PA,
	ST_PAS,
	ST_PASS,
	ST_PO,
	ST_POI,
	ST_POIN,
	ST_POINT,
	ST_POINTE,
	ST_POINTER,
	ST_PR,
	ST_PRI,
	ST_PRIN,
	ST_PRINT,
	ST_R,
	ST_RA,
	ST_RAT,
	ST_RATI,
	ST_RATIO,
	ST_RATION,
	ST_RATIONA,
	ST_RATIONAL,
	ST_RE,
	ST_RET,
	ST_RETU,
	ST_RETUR,
	ST_RETURN,
	ST_S,
	ST_SC,
	ST_SCA,
	ST_SCAN,
	ST_ST,
	ST_STR,
	ST_STRI,
	ST_STRIN,
	ST_STRING,
	ST_T,
	ST_TR,
	ST_TRU,
	ST_TRUE,
	ST_U,
	ST_US,
	ST_USE,
	ST_V,
	ST_VO,
	ST_VOI,
	ST_VOID,

	ST_COMMENT1, // ignores input until controlflow or whitespace	... #comment ...
	ST_COMMENT2, // ignores input until newline						... ##comment \n
	ST_COMMENT3, // ignores input until comment end					... ### comment ###
	ST_COMMENT4, // 4 and 5 are for the comment3 delimiter 
	ST_COMMENT5,
	ST_ERROR
} state;

#define NUM_OF_STATES (ST_ERROR+1)
#define NUM_OF_INPUTS ((INPUT)CH_PRINTABLE+1)
#define NUM_OF_CHARS 128

static int CHAR_CLASS[NUM_OF_CHARS];
static int GOTO[NUM_OF_STATES][NUM_OF_INPUTS];
static void (*ACTION[NUM_OF_STATES][NUM_OF_INPUTS])(lexer*);


void illegal_character(lexer* lxr)
{
	// character does not fit its position according to the grammar
	err_append(lxr->err_list, error("ILLEGAL CHARACTER", "unexpected character: " + lxr->input[lxr->index], lxr->line, lxr->col));
}
void unknown_character(lexer* lxr)
{
	//input character is not an acceptable character of the grammar
	err_append(lxr->err_list, error("UNKNOWN CHARACTER", "unrecognized character: " + lxr->input[lxr->index], lxr->line, lxr->col));
}
void expected_error(lexer* lxr)
{
	// missing char / string literal delimiter: ' or " 
	char exp;
	if (lxr->data[lxr->count].type == STR_LITERAL)
		exp = '\"';
	else
		exp = '\'';
	err_append(lxr->err_list, error("EXPECTED CHARACTER", "expected a: " + exp, lxr->line, lxr->col));
}

void start_token(lexer* lxr)
{
	//Starts new token in lxr->data with lxr->input[index] 

	token* tkn;
	int i;
	if (lxr->count == lxr->size)
	{
		lxr->data = (token*)realloc(lxr->data, lxr->size * 2 * sizeof(token));
		if (lxr->data == NULL)
		{
			memory_error();
		}
		else
		{
			lxr->size *= 2;

			if (!lxr->data) memory_error();
			for (i = lxr->count; i < lxr->size; i++)
			{
				strcpy(lxr->data[i].lexeme, "\0");
			}
		}
	}

	if (lxr->data != NULL)
	{
		tkn = &(lxr->data[lxr->count]);
		tkn->type = ID;
		strcpy(tkn->lexeme, (char[2]) { lxr->input[(lxr->index)++], '\0' });
		tkn->line = lxr->line;
		tkn->col = lxr->col;
	}
}
void start_integer(lexer* lxr)
{
	//Starts new token for integer type
	start_token(lxr);
	lxr->data[lxr->count].type = INT_LITERAL;
	lxr->data[lxr->count].line = lxr->line;
	lxr->data[lxr->count].col = lxr->col;
}
void add_char(lexer* lxr)
{
	//adds char in lxr->input[index] to current token
	token* tkn = &(lxr->data[lxr->count]);
	strcat(tkn->lexeme, (char[2]) { lxr->input[lxr->index++], '\0' });
}

void conv_integer(lexer* lxr)
{
	//convert existing token to integer type ie: "-123" from operator to int
	add_char(lxr);
	lxr->data[lxr->count].type = INT_LITERAL;
}
void end_integer(lexer* lxr)
{
	//finishes handling current token and parsing lexeme into numeral (double) value
	token* temp = &(lxr->data[lxr->count]);
	temp->value = strtod(temp->lexeme, NULL);
	lxr->count++;
}
void conv_float(lexer* lxr)
{
	//converts current token type to float
	lxr->count -= 2;
	lxr->index--;
	add_char(lxr);
	add_char(lxr);
	lxr->data[lxr->count].type = FLOAT_LITERAL;
}
void end_float(lexer* lxr)
{
	//finishes handling float token
	token* tkn = &(lxr->data[lxr->count]);
	tkn->value = strtod(tkn->lexeme, NULL);
	lxr->count++;
}
void conv_rational(lexer* lxr) //TODO 
{
	//converts existing token into rational type assuming
	token* tkn;
	lxr->count -= 1;
	tkn = &(lxr->data[lxr->count]);
	strcat(tkn->lexeme, (char[2]) { '/', '\0' });
	add_char(lxr);
	tkn->type = RAT_LITERAL;
}
void end_rational(lexer* lxr)
{
	//finishes handling of rational type token - calculates the derivitive
	double dvnd, dvsr; //dividend, divisor
	char* end_ptr;

	dvnd = strtod(lxr->data[lxr->count].lexeme, &end_ptr);
	dvsr = strtod(end_ptr + 1, NULL);

	lxr->data[lxr->count].value = dvnd / dvsr;
	lxr->count++;
}

void conv_bool(lexer* lxr)
{
	//converts existing token into bool type;
	add_char(lxr);
	lxr->data[lxr->count].type = BOOL_LITERAL;
	lxr->data[lxr->count].value = strcmp(lxr->data[lxr->count].lexeme, "false");
}
void start_char(lexer* lxr)
{
	//starts new token of char type
	start_token(lxr);
	lxr->data[lxr->count].type = CHR_LITERAL;
	lxr->data[lxr->count].line = lxr->line;
	lxr->data[lxr->count].col = lxr->col;
}
void end_char(lexer* lxr)
{
	//called when second ' detected - end char token
	add_char(lxr);
	lxr->count++;
}
void start_string(lexer* lxr)
{
	//starts new token of string type
	start_token(lxr);
	lxr->data[lxr->count].type = STR_LITERAL;
	lxr->data[lxr->count].line = lxr->line;
	lxr->data[lxr->count].col = lxr->col;
}
void end_string(lexer* lxr)
{
	//called when second " detected, end string
	add_char(lxr);
	lxr->count++;
}
void conv_ident(lexer* lxr)
{
	//changes token type to identifier
	add_char(lxr);
	lxr->data[lxr->count].type = ID;
}

void ignore(lexer* lxr)
{
	//ignores current character
	lxr->index++;
}
void end_token(lexer* lxr)
{
	//ends token with checking for ending method
	symbol tkn_type;
	static void (*method)(lexer*);
	static void(*end_method[(symbol)TERMINALS_COUNT])(lexer*);
	for (tkn_type = ID; tkn_type < TERMINALS_COUNT; tkn_type++)
	{
		end_method[tkn_type] = ignore;
	}
	end_method[INT_LITERAL] = end_integer;
	end_method[RAT_LITERAL] = end_rational;
	end_method[FLOAT_LITERAL] = end_float;
	end_method[CHR_LITERAL] = expected_error;
	end_method[STR_LITERAL] = expected_error;

	tkn_type = lxr->data[lxr->count].type;
	method = end_method[tkn_type];
	if (method == ignore)
		lxr->count++;
	else {
		method(lxr);
	}
}

void start_controlflow(lexer* lxr)
{
	//adds controlflow token assuming last token was handled fully.
	static symbol cf_types[CH_SEMICOLON - CH_COMMA + 1] = { COMMA, OP_SQRBRACKET, CL_SQRBRACKET, OP_CRLBRACKET,
			CL_CRLBRACKET, OP_RNDBRACKET, CL_RNDBRACKET, SEMICOLON };

	start_token(lxr);
	lxr->data[lxr->count].type = cf_types[CHAR_CLASS[lxr->input[lxr->index - 1]] - CH_COMMA];
	lxr->data[lxr->count].line = lxr->line;
	lxr->data[lxr->count].col = lxr->col;
	lxr->count++;
}
void add_controlflow(lexer* lxr)
{
	//adds controlflow token with checking if last token was handeled fully.
	if (lxr->data[lxr->count].lexeme != NULL)
		end_token(lxr);
	start_controlflow(lxr);
}
void conv_break(lexer* lxr)
{
	// changes current token to controlflow type
	add_char(lxr);
	lxr->data[lxr->count].type = BREAK;
}
void conv_return(lexer* lxr)
{
	// changes current token to controlflow type
	add_char(lxr);
	lxr->data[lxr->count].type = RETURN;
}
void conv_pass(lexer* lxr)
{
	// changes current token to controlflow type
	add_char(lxr);
	lxr->data[lxr->count].type = PASS;
}
void conv_loop(lexer* lxr)
{
	// changes current token to controlflow type
	add_char(lxr);
	lxr->data[lxr->count].type = LOOP;
}
void conv_scan(lexer* lxr) 
{
	//changes current token to 'input' keyword
	add_char(lxr);
	lxr->data[lxr->count].type = SCAN;
}
void conv_print(lexer* lxr) 
{
	//changes current token to 'print' keyword
	add_char(lxr);
	lxr->data[lxr->count].type = PRINT;
}

void start_operator(lexer* lxr)
{
	//starts operator token and assigns operator types
	static symbol operators[14] =
	{
		PLUS,
		MULT,
		MOD,
		OR,
		UNKNOWN_OPERATOR, //tilde ~
		AND,
		LEFT,
		RIGHT,
		NOT,
		EQUALS,
		COLON,
		MINUS,
		DIVIDE,
		UNKNOWN_OPERATOR //dot .
	};
	start_token(lxr);
	INPUT ch = CHAR_CLASS[lxr->input[lxr->index-1]];
	lxr->data[lxr->count].type = operators[ch - CH_PLUS];
	lxr->data[lxr->count].line = lxr->line;
	lxr->data[lxr->count].col = lxr->col;
	lxr->count++;
}
void end_token_start_operator(lexer* lxr)
{
	//ends current token and starts new operator
	end_token(lxr);
	start_operator(lxr);
}

void conv_operator(lexer* lxr, symbol t)
{
	//converts current token to operator type
	token* tkn = &(lxr->data[lxr->count - 1]);
	strcat(tkn->lexeme, (char[2]) { lxr->input[lxr->index++], '\0' });
	tkn->type = t;
}
void conv_andand(lexer* lxr)
{
	//converts token type to operator: &&
	conv_operator(lxr, ANDAND);
}
void conv_oror(lexer* lxr)
{
	//converts token type to operator: ||
	conv_operator(lxr, OROR);
}
void conv_tildeor(lexer* lxr)
{
	//converts token type to operator: ~|
	conv_operator(lxr, TILDE_OR);
}
void conv_dblequals(lexer* lxr)
{
	//converts token type to operator: ==
	conv_operator(lxr, DBL_EQUALS);
}
void conv_dblright(lexer* lxr)
{
	//converts token type to operator: >>
	conv_operator(lxr, DBL_RIGHT);
}
void conv_rightequals(lexer* lxr)
{
	//converts token type to operator: >=
	conv_operator(lxr, RIGHT_EQUALS);
}
void conv_dblleft(lexer* lxr)
{
	//converts token type to operator: <<
	conv_operator(lxr, DBL_LEFT);
}
void conv_leftequals(lexer* lxr)
{
	//converts token type to operator: <=
	conv_operator(lxr, LEFT_EQUALS);
}
void conv_notequals(lexer* lxr)
{
	//converts token type to operator: !=
	conv_operator(lxr, NOT_EQUALS);
}
void conv_dblplus(lexer* lxr)
{
	//converts token type to operator: ++
	conv_operator(lxr, DBL_PLUS);
}
void conv_dblminus(lexer* lxr)
{
	//converts token type to operator: --
	conv_operator(lxr, DBL_MINUS);
}
void conv_dblmult(lexer* lxr)
{
	//converts token type to operator: **
	conv_operator(lxr, DBL_MULT);
}
void conv_dbldivide(lexer* lxr)
{
	//converts token type to operator: //
	conv_operator(lxr, DBL_DIVIDE);
}
void conv_dblmod(lexer* lxr)
{
	//converts token type to operator: %%
	conv_operator(lxr, DBL_MOD);
}
void conv_notnot(lexer* lxr)
{
	//converts token type to operator: !!
	conv_operator(lxr, NOTNOT);
}
void conv_arrow(lexer* lxr)
{
	//converts token type to operator: ->
	conv_operator(lxr, ARROW);
}

void conv_vartype(lexer* lxr)
{
	//convert to variable name type token ie int, float, string
	add_char(lxr);
	lxr->data[lxr->count].type = TYPE;
}
void conv_check(lexer* lxr)
{
	//converts to error handling type token ie exception
	add_char(lxr);
	lxr->data[lxr->count].type = CHECK;
}
void conv_exception(lexer* lxr)
{
	//converts token to keyword 'exception'
	add_char(lxr);
	lxr->data[lxr->count].type = EXCEPTION;
}
void conv_declare(lexer* lxr)
{
	//converts to declaration token
	add_char(lxr);
	lxr->data[lxr->count].type = DECLARE;
}
void conv_use(lexer* lxr)
{
	//converts token to keyword 'use'
	add_char(lxr);
	lxr->data[lxr->count].type = USE;
}
void conv_if(lexer* lxr)
{
	//converts to if token 
	add_char(lxr);
	lxr->data[lxr->count].type = IF;
}
void conv_else(lexer* lxr)
{
	//converts to else token
	add_char(lxr);
	lxr->data[lxr->count].type = ELSE;
}


#define SET(from, input, to, action) do { GOTO[from][input] = to;  ACTION[from][input] = action; } while(0)

void table_zero()
{
	// calibrates CHAR_CLASS, GOTO and ACTION tables to their default values
	int st, ch;
	
	for (ch = 0; ch <= 127; ch++)
		CHAR_CLASS[ch] = CH_PRINTABLE;

	for (st = ST_START; st <= ST_ERROR; st++)
	{
		for (ch = CH_UNKNOWN; ch <= CH_PRINTABLE; ch++)
		{
			SET(st, ch, ST_ERROR, illegal_character);
		}
	}
}
void init_char_class_table()
{
	// assigns the grammar-defined char 'class' to each ascii character 

	int ch;

	//invisible characters
	for (ch = 0; ch < 32; ch++)
		CHAR_CLASS[ch] = CH_UNKNOWN;
	CHAR_CLASS[127] = CH_UNKNOWN;

	//whitespace characters
	for (ch = 9; ch <= 13; ch++)
		CHAR_CLASS[ch] = CH_WHITESPACE;
	CHAR_CLASS[32] = CH_WHITESPACE;
	CHAR_CLASS['\n'] = CH_NEWLINE;

	//general letters
	for (ch = 'A'; ch <= 'Z'; ch++)
		CHAR_CLASS[ch] = CH_LETTER;
	for (ch = 'a'; ch <= 'z'; ch++)
		CHAR_CLASS[ch] = CH_LETTER;

	//letters for keywords
	CHAR_CLASS['a'] = A;
	CHAR_CLASS['b'] = B;
	CHAR_CLASS['c'] = C;
	CHAR_CLASS['d'] = D;
	CHAR_CLASS['e'] = E;
	CHAR_CLASS['f'] = F;
	CHAR_CLASS['g'] = G;
	CHAR_CLASS['h'] = H;
	CHAR_CLASS['i'] = I;
	CHAR_CLASS['k'] = K;
	CHAR_CLASS['l'] = L;
	CHAR_CLASS['n'] = N;
	CHAR_CLASS['o'] = O;
	CHAR_CLASS['p'] = P;
	CHAR_CLASS['r'] = R;
	CHAR_CLASS['s'] = S;
	CHAR_CLASS['t'] = T;
	CHAR_CLASS['u'] = U;
	CHAR_CLASS['v'] = V;
	CHAR_CLASS['x'] = X;

	//digits
	for (ch = '0'; ch <= '9'; ch++)
		CHAR_CLASS[ch] = CH_DIGIT;

	//special operator
	CHAR_CLASS['#'] = CH_HASHTAG;

	//general operators
	CHAR_CLASS['!'] = CH_NOT;
	CHAR_CLASS['%'] = CH_MOD;
	CHAR_CLASS['&'] = CH_AND;
	CHAR_CLASS[':'] = CH_COLON;
	CHAR_CLASS['*'] = CH_MULT;
	CHAR_CLASS['+'] = CH_PLUS;
	CHAR_CLASS['|'] = CH_OR;
	CHAR_CLASS['<'] = CH_LEFT;
	CHAR_CLASS['>'] = CH_RIGHT;
	CHAR_CLASS['='] = CH_EQUALS;
	
	//operators that can affect token types
	CHAR_CLASS['-'] = CH_MINUS;
	CHAR_CLASS['.'] = CH_DOT;
	CHAR_CLASS['/'] = CH_DIVIDE;

	//control flow punctuation
	CHAR_CLASS[','] = CH_COMMA;
	CHAR_CLASS['['] = CH_OP_SQRBRACKET;
	CHAR_CLASS[']'] = CH_CL_SQRBRACKET;
	CHAR_CLASS['{'] = CH_OP_CRLBRACKET;
	CHAR_CLASS['}'] = CH_CL_CRLBRACKET;
	CHAR_CLASS[';'] = CH_SEMICOLON;
	CHAR_CLASS['('] = CH_OP_RNDBRACKET;
	CHAR_CLASS[')'] = CH_CL_RNDBRACKET;

	//underline
	CHAR_CLASS['_'] = CH_UNDERLINE;

	//quotation marks
	CHAR_CLASS['\''] = CH_SQUOTE;
	CHAR_CLASS['\"'] = CH_DQUOTE;

	//backslash
	CHAR_CLASS['\\'] = CH_BACKSLASH;
}
void init_basic_cases()
{
	//initiates the tables for general state cases
	int st, ch;

	for (st = ST_INTEGER; st <= ST_ERROR; st++)
	{
		SET(st, CH_WHITESPACE, ST_START, end_token);
		SET(st, CH_NEWLINE, ST_START, end_token);
		for (ch = CH_COMMA; ch <= CH_SEMICOLON; ch++)
			SET(st, ch, ST_START, add_controlflow);
		for (ch = CH_HASHTAG; ch <= CH_DOT; ch++)
			ACTION[st][ch] = end_token_start_operator;

		SET(st, CH_UNKNOWN, ST_ERROR, unknown_character);
	}
	for (st = ST_IDENTIFIER; st <= ST_VOID; st++)
	{
		for (ch = CH_DIGIT; ch <= CH_PRINTABLE; ch++)
		{
			SET(st, ch, ST_IDENTIFIER, conv_ident);
		}
	}
}
void init_default_states()
{
	//initiates starting state and char and string states
	int ch;

	SET(ST_START, CH_WHITESPACE, ST_START, ignore);
	SET(ST_START, CH_NEWLINE, ST_START, ignore);
	SET(ST_START, CH_DIGIT, ST_INTEGER, start_integer);
	SET(ST_START, CH_DOT, ST_DOT, start_operator);
	SET(ST_START, CH_DIVIDE, ST_START, start_operator);
	SET(ST_START, CH_MINUS, ST_MINUS, start_operator);
	SET(ST_START, CH_SQUOTE, ST_CHARSTART, start_char);
	SET(ST_START, CH_DQUOTE, ST_STRINGSTART, start_string);

	for (ch = CH_COMMA; ch <= CH_SEMICOLON; ch++)
		SET(ST_START, ch, ST_START, start_controlflow);

	for (ch = CH_PLUS; ch <= CH_COLON; ch++)
	{
		SET(ST_START, ch, ST_START, start_operator);
	}

	for (ch = CH_LETTER; ch <= CH_PRINTABLE; ch++)
	{
		SET(ST_START, ch, ST_IDENTIFIER, start_token);
		SET(ST_CHARSTART, ch, ST_CHARVALUE, add_char);
		SET(ST_CHARVALUE, ch, ST_ERROR, illegal_character);
		SET(ST_BACKSLASH, ch, ST_CHARVALUE, add_char);
		SET(ST_STRINGSTART, ch, ST_STRINGSTART, add_char);
	}
	SET(ST_CHARSTART, CH_DIGIT, ST_CHARVALUE, add_char);
	SET(ST_STRINGSTART, CH_DIGIT, ST_STRINGSTART, add_char);
	SET(ST_STRINGSTART, CH_DQUOTE, ST_START, end_string);
	SET(ST_CHARVALUE, CH_SQUOTE, ST_START, end_char);

	for (ch = CH_UNKNOWN; ch <= CH_PRINTABLE; ch++)
		SET(ST_ERROR, ch, ST_ERROR, ignore);
	for (ch = CH_WHITESPACE; ch <= CH_DQUOTE; ch++)
		SET(ST_ERROR, ch, ST_START, end_token);
}

void init_operator_states()
{
	//initiates operator states - for multi character operators
	state st;
	int ch;
	for (st = ST_START; st < ST_COMMENT1; st++)
	{
		GOTO[st][CH_DOT] = ST_DOT;// is not a part of a multi character operator
		GOTO[st][CH_DIVIDE] = ST_DIV;
		GOTO[st][CH_PLUS] = ST_PLUS;
		GOTO[st][CH_MULT] = ST_MULT;
		GOTO[st][CH_MOD] = ST_MOD;
		GOTO[st][CH_OR] = ST_OR;
		GOTO[st][CH_TILDE] = ST_TILDE;// is not a standalone operator
		GOTO[st][CH_AND] = ST_AND;
		GOTO[st][CH_LEFT] = ST_LEFT;
		GOTO[st][CH_RIGHT] = ST_RIGHT;
		GOTO[st][CH_NOT] = ST_NOT;
		GOTO[st][CH_EQUALS] = ST_EQUALS;
		GOTO[st][CH_MINUS] = ST_MINUS;
		GOTO[st][CH_COLON] = ST_START;// is not a part of a multi character operator
		GOTO[st][CH_HASHTAG] = ST_COMMENT1;
	}
	for (st = ST_DOT; st <= ST_EQUALS; st++)
	{
		for (ch = CH_UNKNOWN; ch <= CH_PRINTABLE; ch++)
		{
			SET(st, ch, GOTO[ST_START][ch], ACTION[ST_START][ch]);
		}
	}
	
	SET(ST_PLUS, CH_PLUS, ST_START, conv_dblplus);
	SET(ST_MINUS, CH_MINUS, ST_START, conv_dblminus);
	SET(ST_MINUS, CH_RIGHT, ST_START, conv_arrow);
	SET(ST_MULT, CH_MULT, ST_START, conv_dblmult);
	SET(ST_DIV, CH_DIVIDE, ST_START, conv_dbldivide);
	SET(ST_MOD, CH_MOD, ST_START, conv_dblmod);
	SET(ST_AND, CH_AND, ST_START, conv_andand);
	SET(ST_OR, CH_OR, ST_START, conv_oror);
	SET(ST_TILDE, CH_OR, ST_START, conv_tildeor);
	SET(ST_NOT, CH_NOT, ST_START, conv_notnot);
	SET(ST_NOT, CH_EQUALS, ST_START, conv_notequals);
	SET(ST_RIGHT, CH_RIGHT, ST_START, conv_dblright);
	SET(ST_RIGHT, CH_EQUALS, ST_START, conv_rightequals);
	SET(ST_LEFT, CH_LEFT, ST_START, conv_dblleft);
	SET(ST_LEFT, CH_EQUALS, ST_START, conv_leftequals);
	SET(ST_EQUALS, CH_EQUALS, ST_START, conv_dblequals);
}
void init_numeric_states()
{
	//initiates the numeric states, handling integer rational and floating literals
	int ch;

	SET(ST_INTEGER, CH_DIGIT, ST_INTEGER, add_char);
	SET(ST_INTEGER, CH_WHITESPACE, ST_START, end_integer);
	
	for (ch = CH_HASHTAG; ch <= MINUS; ch++)
	{
		SET(ST_INTEGER, ch, ST_START, end_token_start_operator);
	}
	SET(ST_INTEGER, CH_DIVIDE, ST_DIV, end_token_start_operator);
	SET(ST_INTEGER, CH_DOT, ST_DOT, end_token_start_operator);

	SET(ST_DOT, CH_DIGIT, ST_FLOATINGNUMBER, conv_float);
	SET(ST_DIV, CH_DIGIT, ST_RATIONALNUMBER, conv_rational);

	SET(ST_FLOATINGNUMBER, CH_DIGIT, ST_FLOATINGNUMBER, add_char);
	SET(ST_FLOATINGNUMBER, CH_WHITESPACE, ST_START, end_float);
	SET(ST_RATIONALNUMBER, CH_DIGIT, ST_RATIONALNUMBER, add_char);
	SET(ST_RATIONALNUMBER, CH_WHITESPACE, ST_START, end_rational);
}

typedef struct {
	char* letters;
	void (*assignment)(lexer*);
}keyword;
void init_keywords(int start_num, keyword kws[], int kws_num)
{
	/*
		GETS: first state of keyword section, array of keyword structs, and its size
		DOES: sets a path for keyword acceptance, and the actions
		RETS: void
	*/

	int i /*, letter*/; 
	state state_num = start_num, current, last;
	char* word;
	char letter;

	for (i = 0; i < kws_num; i++)
	{
		current = ST_START;
		word = kws[i].letters; 
		letter = *word;
		
		if (GOTO[current][CHAR_CLASS[letter]] == ST_IDENTIFIER) {
			SET(current, CHAR_CLASS[*word], state_num, start_token);
			last = current;
			current = state_num;
			state_num++;
		}
		else
		{
			last = current;
			current = GOTO[ST_START][CHAR_CLASS[letter]];
		}
		word++;
		while (word[0] != '\0')
		{
			letter = *word;
			if (GOTO[current][CHAR_CLASS[letter]] == ST_IDENTIFIER) {
				SET(current, CHAR_CLASS[letter], state_num, add_char);
				last = current;
				current = state_num;
				state_num++;
			}
			else
			{
				last = current;
				current = GOTO[current][CHAR_CLASS[letter]];
			}
			word++;
		}
		ACTION[last][CHAR_CLASS[letter]] = kws[i].assignment;
	}
}
void init_keywords_states()
{
	// list of all keywords, passed to keyword state generator
	keyword keywords[] = {
		{ "bool", conv_vartype } ,
		{ "break", conv_break },
		{ "check", conv_check },
		{ "char", conv_vartype },
		{ "declare", conv_declare },
		{ "else", conv_else},
		{ "exception", conv_exception },
		{ "false", conv_bool },
		{ "float", conv_vartype },
		{ "if", conv_if },
		{ "int", conv_vartype },
		{ "loop", conv_loop },
		{ "natural", conv_vartype },
		{ "pass", conv_pass },
		{ "pointer", conv_vartype },
		{ "print", conv_print},
		{ "rational", conv_vartype },
		{ "return", conv_return },
		{ "scan", conv_scan },
		{ "string", conv_vartype },
		{ "true", conv_bool },
		{ "use", conv_use },
		{ "void", conv_vartype }
	};
	init_keywords(ST_B, keywords, 23);
}
void init_comments()
{
	//implement comment logic- 3 types of comment
	//comment 1: ... #comment ...	= comment terminated by whitespace or controlflow ( func(a #param1, b #param2))
	//comment 2: ... ##comment		= comment terminated by newline only like //
	//comment 3: ... ###comment###... = terminated by comment terminator. like /**/
	state st;
	INPUT ch;
	
	for (ch = CH_UNKNOWN; ch <= CH_PRINTABLE; ch++)
	{
		for (st = ST_COMMENT1; st <= ST_COMMENT3; st++)
		{
			SET(st, ch, st, ignore);
		}
	}
	SET(ST_COMMENT1, CH_WHITESPACE, ST_START, ignore);
	SET(ST_COMMENT1, CH_NEWLINE, ST_START, ignore);
	SET(ST_COMMENT1, CH_HASHTAG, ST_COMMENT2, ignore);

	SET(ST_COMMENT2, CH_NEWLINE, ST_START, ignore);
	SET(ST_COMMENT2, CH_HASHTAG, ST_COMMENT3, ignore);

	SET(ST_COMMENT3, CH_HASHTAG, ST_COMMENT4, ignore);

	for (ch = CH_UNKNOWN; ch <= CH_PRINTABLE; ch++)
	{
		SET(ST_COMMENT4, ch, ST_COMMENT4, ignore);
		SET(ST_COMMENT5, ch, ST_COMMENT5, ignore);
	}
	SET(ST_COMMENT4, CH_HASHTAG, ST_COMMENT5, ignore);
	SET(ST_COMMENT5, CH_HASHTAG, ST_START, ignore);
}

void init_tables() 
{
	//initiates all tables via respective functions
	table_zero();
	init_char_class_table();
	init_basic_cases();
	init_default_states();
	init_operator_states();
	init_numeric_states();
	init_keywords_states();
} 



#define input (lxr->input)
#define index (lxr->index)
void tokenize(lexer *lxr)
{
	/*
		GETS: pointer to lexer structure
		DOES: iterates over lxr->input and builds array of tokens
		RETS: array of token via lxr->data
	*/
	INPUT ch_class;
	state state = ST_START;
	void (*action)(lexer*);

	index = 0;
	lxr->data = malloc(2 * sizeof(token));
	lxr->size = 2;
	lxr->count = 0;
	lxr->line = lxr->col = 1;

	lxr->err_list = err_list();

	init_tables();


	while (input[index] != '\0' && input[index] <= 127)
	{
		ch_class = CHAR_CLASS[input[index]];

		action = ACTION[state][ch_class];
		state = GOTO[state][ch_class];

		action(lxr);
		if (input[index] == '\n')
		{
			lxr->col = 1;
			lxr->line++;
		}
		else
			lxr->col++;
	}
	if (state != 0)
		end_token(lxr);
	if (lxr->size == lxr->count)
	{
		lxr->data = (token*)realloc(lxr->data, (lxr->count+1) * sizeof(token));
		if (lxr->data == NULL) memory_error();
	}
	lxr->data[lxr->count++] = (token){ "$", 0, END_TOKEN, lxr->line, lxr->col };
}
