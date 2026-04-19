#include "lexer.h"
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
#define NUM_OF_INPUTS ((INPUT)PRINTABLE+1)
#define NUM_OF_CHARS 128

void start_token(lexer* lxr);
void end_token(lexer* lxr);

void conv_rational(lexer* lxr);
void end_rational(lexer* lxr);
void start_integer(lexer* lxr);
void conv_integer(lexer* lxr);
void end_integer(lexer* lxr);
void conv_float(lexer* lxr);
void end_float(lexer* lxr);
void conv_bool(lexer* lxr);
void start_char(lexer* lxr);
void end_char(lexer* lxr);
void start_string(lexer* lxr);
void end_string(lexer* lxr);
void conv_ident(lexer* lxr);

void start_controlflow(lexer* lxr);
void add_controlflow(lexer* lxr);
void conv_controlflow(lexer* lxr);

void add_char(lexer* lxr);
void ignore(lexer* lxr);

void end_token_start_operator(lexer*);
void start_operator(lexer* lxr);
void add_operator(lexer* lxr);

void conv_vartype(lexer* lxr);
void conv_errorhandler(lexer* lxr);
void conv_declare(lexer* lxr);
void conv_conditional(lexer* lxr);


static int CHAR_CLASS[NUM_OF_CHARS] = { PRINTABLE };
static int GOTO[NUM_OF_STATES][NUM_OF_INPUTS] = { ST_ERROR };
static void (*ACTION[NUM_OF_STATES][NUM_OF_INPUTS])(lexer*) = { illegal_character };

#define SET(from, input, to, action) do { GOTO[from][input] = to;  ACTION[from][input] = action; } while(0)

void table_zero()
{
	int st, ch;
	
	for (ch = 0; ch <= 127; ch++)
		CHAR_CLASS[ch] = PRINTABLE;

	for (st = ST_START; st <= ST_ERROR; st++)
	{
		for (ch = UNKNOWN; ch <= PRINTABLE; ch++)
		{
			SET(st, ch, ST_ERROR, illegal_character);
		}
	}
}
void init_char_class_table()
{
	int ch;

	//invisible characters
	for (ch = 0; ch < 32; ch++)
		CHAR_CLASS[ch] = UNKNOWN;
	CHAR_CLASS[127] = UNKNOWN;

	//whitespace characters
	for (ch = 9; ch <= 13; ch++)
		CHAR_CLASS[ch] = WHITESPACE;
	CHAR_CLASS[32] = WHITESPACE;
	CHAR_CLASS['\n'] = NEWLINE;

	//general letters
	for (ch = 'A'; ch <= 'Z'; ch++)
		CHAR_CLASS[ch] = LETTER;
	for (ch = 'a'; ch <= 'z'; ch++)
		CHAR_CLASS[ch] = LETTER;

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
		CHAR_CLASS[ch] = DIGIT;

	//special operator
	CHAR_CLASS['#'] = HASHTAG;

	//general operators
	CHAR_CLASS['!'] = NOT;
	CHAR_CLASS['%'] = MOD;
	CHAR_CLASS['&'] = AND;
	CHAR_CLASS[':'] = COLON;
	CHAR_CLASS['*'] = MULT;
	CHAR_CLASS['+'] = PLUS;
	CHAR_CLASS['|'] = OR;
	CHAR_CLASS['<'] = LEFT;
	CHAR_CLASS['>'] = RIGHT;
	CHAR_CLASS['='] = EQUALS;
	
	//operators that can affect token types
	CHAR_CLASS['-'] = MINUS;
	CHAR_CLASS['.'] = DOT;
	CHAR_CLASS['/'] = DIVIDE;

	//control flow punctuation
	CHAR_CLASS[','] = CF_COMMA;
	CHAR_CLASS['['] = CF_OP_SQRBRACKET;
	CHAR_CLASS[']'] = CF_CL_SQRBRACKET;
	CHAR_CLASS['{'] = CF_OP_CRLBRACKET;
	CHAR_CLASS['}'] = CF_CL_CRLBRACKET;
	CHAR_CLASS[';'] = CF_SEMICOLON;
	CHAR_CLASS['('] = CF_OP_RNDBRACKET;
	CHAR_CLASS[')'] = CF_CL_RNDBRACKET;

	//underline
	CHAR_CLASS['_'] = UNDERLINE;

	//quotation marks
	CHAR_CLASS['\''] = SQUOTE;
	CHAR_CLASS['\"'] = DQUOTE;

	//backslash
	CHAR_CLASS['\\'] = BACKSLASH;
}
void init_basic_cases()
{
	//initiates the tables for general state cases
	int st, ch;

	for (st = ST_INTEGER; st <= ST_ERROR; st++)
	{
		SET(st, WHITESPACE, ST_START, end_token);
		SET(st, NEWLINE, ST_START, end_token);
		SET(st, CF, ST_START, add_controlflow);
		for (ch = HASHTAG; ch <= DOT; ch++)
			ACTION[st][ch] = end_token_start_operator;

		SET(st, UNKNOWN, ST_ERROR, unknown_character);
	}
	for (st = ST_IDENTIFIER; st <= ST_VOID; st++)
	{
		for (ch = DIGIT; ch <= PRINTABLE; ch++)
		{
			SET(st, ch, ST_IDENTIFIER, conv_ident);
		}
	}
}
void init_default_states()
{
	//initiates starting state and char and string states
	int ch;

	SET(ST_START, WHITESPACE, ST_START, ignore);
	SET(ST_START, NEWLINE, ST_START, ignore);
	SET(ST_START, DIGIT, ST_INTEGER, start_integer);
	SET(ST_START, DOT, ST_DOT, start_operator);
	SET(ST_START, DIVIDE, ST_START, start_operator);
	SET(ST_START, MINUS, ST_MINUS, start_operator);
	SET(ST_START, SQUOTE, ST_CHARSTART, start_char);
	SET(ST_START, DQUOTE, ST_STRINGSTART, start_string);
	SET(ST_START, CF, ST_START, start_controlflow);

	for (ch = PLUS; ch <= COLON; ch++)
	{
		SET(ST_START, ch, ST_START, start_operator);
	}

	for (ch = LETTER; ch <= PRINTABLE; ch++)
	{
		SET(ST_START, ch, ST_IDENTIFIER, start_token);
		SET(ST_CHARSTART, ch, ST_CHARVALUE, add_char);
		SET(ST_CHARVALUE, ch, ST_ERROR, illegal_character);
		SET(ST_BACKSLASH, ch, ST_CHARVALUE, add_char);
		SET(ST_STRINGSTART, ch, ST_STRINGSTART, add_char);
	}
	SET(ST_CHARSTART, DIGIT, ST_CHARVALUE, add_char);
	SET(ST_STRINGSTART, DIGIT, ST_STRINGSTART, add_char);
	SET(ST_STRINGSTART, DQUOTE, ST_START, end_string);
	SET(ST_CHARVALUE, SQUOTE, ST_START, end_char);

	for (ch = UNKNOWN; ch <= PRINTABLE; ch++)
		SET(ST_ERROR, ch, ST_ERROR, ignore);
	for (ch = WHITESPACE; ch <= DQUOTE; ch++)
		SET(ST_ERROR, ch, ST_START, end_token);
}

#define PAIR_OPERATORS(st, inp) do{SET(st, inp, ST_START, add_operator);} while(0);
void init_operator_states()
{
	//initiates operator states - for multi character operators
	state st;
	int ch;
	for (st = ST_START; st < ST_COMMENT1; st++)
	{
		GOTO[st][DOT] = ST_DOT;//
		GOTO[st][DIVIDE] = ST_DIV;
		GOTO[st][PLUS] = ST_PLUS;
		GOTO[st][MULT] = ST_MULT;
		GOTO[st][MOD] = ST_MOD;
		GOTO[st][OR] = ST_OR;
		GOTO[st][TILDE] = ST_TILDE;//
		GOTO[st][AND] = ST_AND;
		GOTO[st][LEFT] = ST_LEFT;
		GOTO[st][RIGHT] = ST_RIGHT;
		GOTO[st][NOT] = ST_NOT;
		GOTO[st][EQUALS] = ST_EQUALS;
		GOTO[st][MINUS] = ST_MINUS;
		GOTO[st][COLON] = ST_START;//
		GOTO[st][HASHTAG] = ST_COMMENT1;
	}
	for (st = ST_DOT; st <= ST_EQUALS; st++)
	{
		for (ch = UNKNOWN; ch <= PRINTABLE; ch++)
		{
			SET(st, ch, GOTO[ST_START][ch], ACTION[ST_START][ch]);
		}
	}
	
	PAIR_OPERATORS(ST_PLUS, PLUS);
	PAIR_OPERATORS(ST_MINUS, MINUS);
	PAIR_OPERATORS(ST_MINUS, RIGHT);
	PAIR_OPERATORS(ST_MULT, MULT);
	PAIR_OPERATORS(ST_DIV, DIVIDE);
	PAIR_OPERATORS(ST_MOD, MOD);
	PAIR_OPERATORS(ST_AND, AND);
	PAIR_OPERATORS(ST_OR, OR);
	PAIR_OPERATORS(ST_TILDE, OR);
	PAIR_OPERATORS(ST_NOT, NOT);
	PAIR_OPERATORS(ST_NOT, EQUALS);
	PAIR_OPERATORS(ST_RIGHT, RIGHT);
	PAIR_OPERATORS(ST_RIGHT, EQUALS);
	PAIR_OPERATORS(ST_LEFT, LEFT);
	PAIR_OPERATORS(ST_LEFT, EQUALS);
	PAIR_OPERATORS(ST_EQUALS, EQUALS);
}
void init_numeric_states()
{
	int ch;

	SET(ST_INTEGER, DIGIT, ST_INTEGER, add_char);
	SET(ST_INTEGER, WHITESPACE, ST_START, end_integer);
	
	for (ch = HASHTAG; ch <= MINUS; ch++)
	{
		SET(ST_INTEGER, ch, ST_START, end_token_start_operator);
	}
	SET(ST_INTEGER, DIVIDE, ST_DIV, end_token_start_operator);
	SET(ST_INTEGER, DOT, ST_DOT, end_token_start_operator);

	SET(ST_DOT, DIGIT, ST_FLOATINGNUMBER, conv_float);
	SET(ST_DIV, DIGIT, ST_RATIONALNUMBER, conv_rational);

	SET(ST_FLOATINGNUMBER, DIGIT, ST_FLOATINGNUMBER, add_char);
	SET(ST_FLOATINGNUMBER, WHITESPACE, ST_START, end_float);
	SET(ST_RATIONALNUMBER, DIGIT, ST_RATIONALNUMBER, add_char);
	SET(ST_RATIONALNUMBER, WHITESPACE, ST_START, end_rational);
}

typedef struct {
	char* letters;
	void (*assignment)(lexer*);
}keyword;
void init_keywords(int start_num, keyword kws[], int kws_num)
{
	//keyword state generator

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
		{ "check", conv_errorhandler },
		{ "char", conv_vartype },
		{ "declare", conv_declare },
		{ "else", conv_conditional},
		{ "exception", conv_errorhandler },
		{ "false", conv_bool },
		{ "float", conv_vartype },
		{ "if", conv_conditional },
		{ "int", conv_vartype },
		{ "loop", conv_loop },
		{ "natural", conv_vartype },
		{ "pass", conv_pass },
		{ "pointer", conv_vartype },
		{ "rational", conv_vartype },
		{ "return", conv_return },
		{ "string", conv_vartype },
		{ "true", conv_bool },
		{ "use", conv_declare },
		{ "void", conv_vartype }
	};
	init_keywords(ST_B, keywords, 21);
}
void init_comments()
{
	//implement comment logic- 3 types of comment
	//comment 1: ... #comment ...	= comment terminated by whitespace or controlflow ( func(a #param1, b #param2))
	//comment 2: ... ##comment		= comment terminated by newline only like //
	//comment 3: ... ###comment###... = terminated by comment terminator. like /**/
	state st;
	INPUT ch;
	
	for (ch = UNKNOWN; ch <= PRINTABLE; ch++)
	{
		for (st = ST_COMMENT1; st <= ST_COMMENT3; st++)
		{
			SET(st, ch, st, ignore);
		}
	}
	SET(ST_COMMENT1, WHITESPACE, ST_START, ignore);
	SET(ST_COMMENT1, NEWLINE, ST_START, ignore);
	SET(ST_COMMENT1, HASHTAG, ST_COMMENT2, ignore);

	SET(ST_COMMENT2, NEWLINE, ST_START, ignore);
	SET(ST_COMMENT2, HASHTAG, ST_COMMENT3, ignore);

	SET(ST_COMMENT3, HASHTAG, ST_COMMENT4, ignore);

	for (ch = UNKNOWN; ch <= PRINTABLE; ch++)
	{
		SET(ST_COMMENT4, ch, ST_COMMENT4, ignore);
		SET(ST_COMMENT5, ch, ST_COMMENT5, ignore);
	}
	SET(ST_COMMENT4, HASHTAG, ST_COMMENT5, ignore);
	SET(ST_COMMENT5, HASHTAG, ST_START, ignore);
}

void init_tables() 
{
	//initiates all tables
	table_zero();
	init_char_class_table();
	init_basic_cases();
	init_default_states();
	init_operator_states();
	init_numeric_states();
	init_keywords_states();
} 

void start_token(lexer* lxr)
{
	//Starts new token in lxr->data with lxr->input[index] 
	
	token *tkn;
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
	}
}
void start_integer(lexer *lxr)
{
	//Starts new token for integer type
	start_token(lxr);
	lxr->data[lxr->count].type = NUM_LITERAL;
}
void add_char(lexer* lxr)
{
	//adds char in lxr->input[index] to current token
	token *tkn = &(lxr->data[lxr->count]);
	strcat(tkn->lexeme, (char[2]) { lxr->input[lxr->index++], '\0' });
}

void conv_integer(lexer* lxr)
{
	//convert existing token to integer type ie: "-123" from operator to int
	add_char(lxr);
	lxr->data[lxr->count].type = INT;
}
void end_numeral(lexer* lxr)
{
	//finishes handling current token and parsing lexeme into numeral (double) value
	token* temp = &(lxr->data[lxr->count]);
	temp->value = strtod(temp->lexeme, NULL);
	lxr->count++;
	temp->type = LITERAL;
}
void conv_float(lexer* lxr)
{
	//converts current token type to float
	lxr->count -= 2;
	lxr->index--;
	add_char(lxr);
	add_char(lxr);
	lxr->data[lxr->count].type = NUM_LITERAL;
}
//void end_float(lexer* lxr)
//{
//	//finishes handling float token
//	token* tkn = &(lxr->data[lxr->count]);
//	tkn->value = strtod(tkn->lexeme, NULL);
//	lxr->count++;
//}
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
	lxr->data[lxr->count].type = LITERAL;
	lxr->count++;
}

void conv_bool(lexer* lxr)
{
	//converts existing token into bool type;
	add_char(lxr);
	lxr->data[lxr->count].type = BOOL_LITERAL;
	lxr->data[lxr->count].value = strcmp(lxr->data[lxr->count].lexeme, "false");
}
void start_char(lexer *lxr)
{
	//starts new token of char type
	start_token(lxr);
	lxr->data[lxr->count].type = TXT_LITERAL;
}
void end_char(lexer* lxr)
{
	//called when second ' detected - end char token
	add_char(lxr);
	lxr->data[lxr->count].type = LITERAL;
	lxr->count++;
}
void start_string(lexer *lxr)
{
	//starts new token of string type
	start_token(lxr);
	lxr->data[lxr->count].type = TXT_LITERAL;
}
void end_string(lexer* lxr)
{
	//called when second " detected, end string
	add_char(lxr);
	lxr->data[lxr->count].type = LITERAL;
	lxr->count++;
}
void conv_ident(lexer *lxr)
{
	//changes token type to identifier
	add_char(lxr);
	lxr->data[lxr->count].type = ID;
}

void add_controlflow(lexer *lxr)
{
	//adds controlflow token with checking if last token was handeled fully.
	if (lxr->data[lxr->count].lexeme != NULL)
		end_token(lxr);
	start_controlflow(lxr);
}
void start_controlflow(lexer *lxr)
{
	//adds controlflow token assuming last token was handled fully.
	static type cf_types[CF_SEMICOLON - CF_COMMA + 1] = {COMMA, OP_SQRBRACKET, CL_SQRBRACKET, OP_CRLBRACKET,
			CL_CRLBRACKET, OP_RNDBRACKET, CL_RNDBRACKET, SEMICOLON };
	
	start_token(lxr);
	lxr->data[lxr->count].type = cf_types[CHAR_CLASS[lxr->input[lxr->index-1]]-CF_COMMA];
	lxr->count++;
}
void conv_break(lexer* lxr)
{
	// changes current token to controlflow type
	add_char(lxr);
	lxr->data[lxr->count].type = CF_BREAK;
}
void conv_return(lexer* lxr)
{
	// changes current token to controlflow type
	add_char(lxr);
	lxr->data[lxr->count].type = CF_RETURN;
}
void conv_pass(lexer* lxr)
{
	// changes current token to controlflow type
	add_char(lxr);
	lxr->data[lxr->count].type = CF_PASS;
}
void conv_loop(lexer* lxr)
{
	// changes current token to controlflow type
	add_char(lxr);
	lxr->data[lxr->count].type = CF_LOOP;
}

void end_token_start_operator(lexer* lxr)
{
	//ends current token and starts new operator
	end_token(lxr);
	start_operator(lxr);
}
void start_operator(lexer *lxr)
{
	//starts operator token 
	
	start_token(lxr);
	lxr->data[lxr->count].type = OPERATOR;
	lxr->count++;
}
void add_operator(lexer* lxr)
{
	token* tkn = &(lxr->data[lxr->count - 1]);
	strcat(tkn->lexeme, (char[2]) { lxr->input[lxr->index++], '\0' });
}

void conv_vartype(lexer *lxr)
{
	//convert to variable name type token ie int, float, string
	add_char(lxr);
	lxr->data[lxr->count].type = TYPE;
}
void conv_errorhandler(lexer *lxr)
{
	//converts to error handling type token ie exception
	add_char(lxr);
	lxr->data[lxr->count].type = ERROR_HANDLER;
}
void conv_declare(lexer *lxr)
{
	//converts to declaration token
	add_char(lxr);
	lxr->data[lxr->count].type = DECLARE;
}
void conv_conditional(lexer *lxr)
{
	//converts to conditional type token ie if else
	add_char(lxr);
	lxr->data[lxr->count].type = CONDITIONAL;
}

void ignore(lexer *lxr)
{
	//ignores current character
	lxr->index++;
}
void end_token(lexer* lxr)
{
	//ends token with checking for ending method
	type tkn_type;
	static void (*method)(lexer*);
	static void(*end_method[(type)DECLARE + 1])(lexer*);
	for (tkn_type = UNKNOWN; tkn_type <= DECLARE; tkn_type++)
	{
		end_method[tkn_type] = ignore;
	}
	end_method[INT] = end_integer;
	end_method[RATIONAL] = end_rational;
	end_method[FLOAT] = end_float;
	end_method[CHAR] = expected_error;
	end_method[STRING] = expected_error;

	tkn_type = lxr->data[lxr->count].type;
	method = end_method[lxr->data[lxr->count].type];
	if (method == ignore)
		lxr->count++;
	else {
		method(lxr);
	}
}


#define input (lxr->input)
#define index (lxr->index)
void tokenize(lexer *lxr)
{
	INPUT ch_class;
	state state = ST_START;
	void (*action)(lexer*);

	index = 0;
	lxr->data = malloc(2 * sizeof(token));
	lxr->size = 2;
	lxr->count = 0;

	init_tables();


	while (input[index] != '\0' && input[index] <= 127)
	{
		ch_class = CHAR_CLASS[input[index]];

		action = ACTION[state][ch_class];
		state = GOTO[state][ch_class];

		action(lxr);
	}
}
