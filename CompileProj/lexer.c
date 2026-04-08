#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"
#include "lexer.h"

#pragma warning(disable:4996)

#define NUM_OF_STATES 109
#define NUM_OF_INPUTS 32
#define NUM_OF_CHARS 128


void start_token(lexer* lxr);
void end_token(lexer* lxr);

void conv_rational(lexer* lxr);
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
static int GOTO[NUM_OF_STATES][NUM_OF_INPUTS] = { 107 };
static void (*ACTION[NUM_OF_STATES][NUM_OF_INPUTS])(lexer*) = { illegal_character };

void set_table(int state, INPUT input_char, int goto_state, void (*action)(lexer*))
{
	GOTO[state][input_char] = goto_state;
	ACTION[state][input_char] = action;
}
static void init_tables() // NEEDS UPDATING
{
	int ch, st; //character, state

	//CHAR CLASS TBL
	// 
	//invisible characters
	for (ch = 0; ch < 32; ch++)
		CHAR_CLASS[ch] = UNKNOWN;
	CHAR_CLASS[127] = UNKNOWN;
	//whitespace characters
	for (ch = 9; ch <= 13; ch++)
		CHAR_CLASS[ch] = WHITESPACE;
	CHAR_CLASS[32] = WHITESPACE;

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

	//general operators
	CHAR_CLASS['!'] = CHAR_CLASS['%'] = CHAR_CLASS['&'] = CHAR_CLASS[':'] = CHAR_CLASS['*'] = CHAR_CLASS['+'] = CHAR_CLASS['|'] = CHAR_CLASS['\\'] = OP;
	for (ch = '<'; ch <= '>'; ch++)
		CHAR_CLASS[ch] = OP;
	//operators that can affect token types
	CHAR_CLASS['-'] = MINUS;
	CHAR_CLASS['.'] = DOT;
	CHAR_CLASS['/'] = DIVIDE;

	//control flow punctuation
	CHAR_CLASS[','] = CHAR_CLASS['['] = CHAR_CLASS[']'] = CHAR_CLASS['{'] = CHAR_CLASS['}'] = CHAR_CLASS[';'] = CF;
	for (ch = '\''; ch <= ')'; ch++)
		CHAR_CLASS[ch] = CF;

	//quotation marks
	CHAR_CLASS['\''] = SQUOTE;
	CHAR_CLASS['\"'] = DQUOTE;

	//backslash
	CHAR_CLASS['\\'] = BACKSLASH;

	//GOTO + ACTION TBL
	for (st = 0; st < 106; st++)
	{
		set_table(st, WHITESPACE, 0, end_token);

		set_table(st, CF, 0, add_controlflow);

		set_table(st, OP, 4, end_token_start_operator);
	}
	//floating point
	set_table(0, DOT, 2, start_operator);
	set_table(2, DIGIT, 1, conv_float);

	//default state]
	set_table(0, WHITESPACE, 0, ignore);
	set_table(0, DIGIT, 1, start_integer);

	//(strings and characters)
	set_table(0, SQUOTE, 104, start_char);
	for (ch = DIGIT; ch <= X; ch++) 
	{
		set_table(104, ch, 105, illegal_character);
	}
	set_table(104, PRINTABLE, 107, illegal_character); // ADD 107 STATE
	set_table(104, SQUOTE, 0, end_char);
	set_table(105, SQUOTE, 0, end_char);

	set_table(0, DQUOTE, 103, start_string);
	set_table(103, DQUOTE, 0, end_string);
	for (ch = DIGIT; ch <= X; ch++)
	{
		set_table(103, ch, 103, add_char);
	}
	set_table(103, WHITESPACE, 103, add_char);
	set_table(103, PRINTABLE, 103, add_char);

	//redirection to identifier
	for (ch = LETTER; ch <= X; ch++)
	{
		set_table(0, ch, 5, start_token);
	}

	//redirection to keyword states
	GOTO[0][B] = 6;
	GOTO[0][C] = 14;
	GOTO[0][D] = 21;
	GOTO[0][E] = 28;
	GOTO[0][F] = 40;
	GOTO[0][I] = 45;
	GOTO[0][L] = 49;
	GOTO[0][N] = 55;
	GOTO[0][P] = 62;
	GOTO[0][R] = 72;
	GOTO[0][S] = 85;
	GOTO[0][U] = 95;
	GOTO[0][V] = 98;

	//numeral state
	set_table(1, DIGIT, 1, add_char);
	
	set_table(1, DOT, 2, conv_float);

	set_table(1, DIVIDE, 3, end_token_start_operator);
	set_table(3, DIGIT, 108, conv_rational);//ADD 108 STATE!!!

	set_table(1, WHITESPACE, 0, end_integer);
	set_table(1, CF, 0, add_controlflow);



	//operators state
	set_table(4, WHITESPACE, 0, ignore);
	set_table(4, OP, 4, add_operator);
	set_table(4, DIGIT, 1, start_integer);
	set_table(4, SQUOTE, 104, start_char);
	set_table(4, DQUOTE, 103, start_string);

	for (ch = LETTER; ch <= X; ch++)
	{
		set_table(4, ch, 5, start_token);
	}
	GOTO[4][B] = 6;
	GOTO[4][C] = 14;
	GOTO[4][D] = 21;
	GOTO[4][E] = 28;
	GOTO[4][F] = 40;
	GOTO[4][I] = 45;
	GOTO[4][L] = 49;
	GOTO[4][N] = 55;
	GOTO[4][P] = 62;
	GOTO[4][R] = 72;
	GOTO[4][S] = 85;
	GOTO[4][U] = 95;
	GOTO[4][V] = 98;


	//identifiers
	for (ch = DIGIT; ch <= X; ch++)
	{
		set_table(5, ch, 5, add_char);
	}
	set_table(5, PRINTABLE, 5, add_char);

	for (st = 6; st <= 101; st++)
	{ 
		for (ch = DIGIT; ch <= X; ch++)
		{
			set_table(st, ch, 5, add_char);
		}
		set_table(st, PRINTABLE, 5, add_char);
		set_table(st, WHITESPACE, 0, end_token);
	}

	// keywords
	GOTO[6][O] = 7; //b|ool
	GOTO[7][O] = 8; //bo|ol
	GOTO[8][L] = 9; //boo|l
	ACTION[8][L] = conv_vartype; 
	ACTION[9][PRINTABLE] = conv_ident;

	GOTO[6][R] = 10; //b|reak
	GOTO[10][E] = 11; //br|eak
	GOTO[11][A] = 12; //bre|ak
	GOTO[12][K] = 13; //brea|k
	ACTION[12][K] = conv_controlflow;
	ACTION[13][PRINTABLE] = conv_ident;

	GOTO[14][H] = 15; //c|har , c|heck
	
	GOTO[15][A] = 19; //ch|ar
	GOTO[19][R] = 20; //cha|r
	ACTION[19][R] = conv_vartype;
	ACTION[20][PRINTABLE] = conv_ident;

	GOTO[15][E] = 16; //ch|eck
	GOTO[16][C] = 17; //che|ck
	GOTO[17][K] = 18; //chec|k
	ACTION[17][K] = conv_errorhandler;
	ACTION[18][PRINTABLE] = conv_ident;

	GOTO[21][E] = 22; //d|ecalre
	GOTO[22][C] = 23; //de|clare
	GOTO[23][L] = 24; //dec|lare
	GOTO[24][A] = 25; //decl|are
	GOTO[25][R] = 26; //decla|re
	GOTO[26][E] = 27; //declar|e
	ACTION[26][E] = conv_declare;
	ACTION[27][PRINTABLE] = conv_ident;

	GOTO[28][L] = 29; //e|lse
	GOTO[28][X] = 32; //e|xception

	GOTO[29][S] = 30; //el|se
	GOTO[30][E] = 31; //els|e
	ACTION[30][E] = conv_conditional;
	ACTION[31][PRINTABLE] = conv_ident;

	GOTO[32][C] = 33; //ex|ception
	GOTO[33][E] = 34; //exc|eption
	GOTO[34][P] = 35; //exce|ption
	GOTO[35][T] = 36; //excep|tion
	GOTO[36][I] = 37; //except|ion
	GOTO[37][O] = 38; //excepti|on
	GOTO[38][N] = 39; //exceptio|n
	ACTION[38][N] = conv_errorhandler;
	ACTION[39][PRINTABLE] = conv_ident;

	GOTO[40][L] = 41; //f|loat
	GOTO[41][O] = 42; //fl|oat
	GOTO[42][A] = 43; //flo|at
	GOTO[43][T] = 44; //floa|t
	ACTION[43][T] = conv_vartype;
	ACTION[44][PRINTABLE] = conv_ident;

	GOTO[45][F] = 46; //i|f
	ACTION[45][F] = conv_conditional;
	ACTION[46][PRINTABLE] = conv_ident;

	GOTO[45][N] = 47; //i|nt
	GOTO[47][T] = 48; //in|t
	ACTION[47][T] = conv_vartype;
	ACTION[48][PRINTABLE] = conv_ident;

	GOTO[49][O] = 50; //l|ong, l|oop
	
	GOTO[50][O] = 53; //lo|op
	GOTO[53][P] = 54; //loo|p
	ACTION[53][P] = conv_controlflow;
	ACTION[54][PRINTABLE] = conv_ident;

	GOTO[50][N] = 51; //lo|ng
	GOTO[51][G] = 52; //lon|g
	ACTION[51][G] = conv_vartype;
	ACTION[52][PRINTABLE] = conv_ident;

	GOTO[55][A] = 56; //n|atural
	GOTO[56][T] = 57; //na|tural
	GOTO[57][U] = 58; //nat|ural
	GOTO[58][R] = 59; //natu|ral
	GOTO[59][A] = 60; //natur|al
	GOTO[60][L] = 61; //natura|l
	ACTION[60][L] = conv_vartype;
	ACTION[61][PRINTABLE] = conv_ident;

	GOTO[62][A] = 63; //p|ass
	GOTO[63][S] = 64; //pa|ss
	GOTO[64][S] = 65; //pas|s
	ACTION[64][S] = conv_controlflow;
	ACTION[65][PRINTABLE] = conv_ident;

	GOTO[62][O] = 66; //p|ointer
	GOTO[66][I] = 67; //po|inter
	GOTO[67][N] = 68; //poi|nter
	GOTO[68][T] = 69; //poin|ter
	GOTO[69][E] = 70; //point|er
	GOTO[70][R] = 71; //pointe|r
	ACTION[70][R] = conv_vartype;
	ACTION[71][PRINTABLE] = conv_ident;

	GOTO[72][A] = 73; //r|ational
	GOTO[73][T] = 74; //ra|tional
	GOTO[74][I] = 75; //rat|ional
	GOTO[75][O] = 76; //rati|onal
	GOTO[76][N] = 77; //ratio|nal
	GOTO[77][A] = 78; //ration|al
	GOTO[78][L] = 79; //rationa|l
	ACTION[78][L] = conv_vartype;
	ACTION[79][PRINTABLE] = conv_ident;

	GOTO[72][E] = 80; //r|eturn
	GOTO[80][T] = 81; //re|turn
	GOTO[81][U] = 82; //ret|urn
	GOTO[82][R] = 83; //retu|rn
	GOTO[83][N] = 84; //retur|n
	ACTION[83][N] = conv_controlflow;
	ACTION[84][PRINTABLE] = conv_ident;

	GOTO[85][H] = 86; //s|hort
	GOTO[86][O] = 87; //sh|ort
	GOTO[87][R] = 88; //sho|rt
	GOTO[88][T] = 89; //shor|t
	ACTION[88][T] = conv_vartype;
	ACTION[89][PRINTABLE] = conv_ident;

	GOTO[85][T] = 90; //s|tring
	GOTO[90][R] = 91; //st|ring
	GOTO[91][I] = 92; //str|ing
	GOTO[92][N] = 93; //stri|ng
	GOTO[93][G] = 94; //strin|g
	ACTION[93][G] = conv_vartype;
	ACTION[94][PRINTABLE] = conv_ident;

	GOTO[95][S] = 96; //u|se
	GOTO[96][E] = 97; //us|e
	ACTION[96][E] = conv_declare;
	ACTION[97][PRINTABLE] = conv_ident;

	GOTO[98][O] = 99;  //v|oid
	GOTO[99][I] = 100; //vo|id
	GOTO[100][D] = 101;//voi|d
	ACTION[100][D] = conv_vartype;
	ACTION[101][PRINTABLE] = conv_ident;

	//error handling - panic mode recovery
	for (ch = UNKNOWN; ch <= PRINTABLE; ch++)
	{
		set_table(107, ch, 107, add_char);
	}
	for (ch = CF; ch <= DQUOTE; ch++)
	{
		set_table(107, ch, 0, end_token);
	}
} 

void start_token(lexer* lxr)
{
	//Starts new token in lxr->data with lxr->input[index] 
	
	token *tkn;
	int i;
	if (lxr->count == lxr->size)
	{
		realloc(lxr->data, lxr->size * 2);
		lxr->size *= 2;

		if (!lxr->data) memory_error();
		for (i = lxr->count; i < lxr->size; i++)
		{
			strcpy(lxr->data[i].lexeme, "\0");
		}
	}

	tkn = &(lxr->data[lxr->count]);
	tkn->type = IDENTIFIER;
	strcpy(tkn->lexeme, (char[2]){ lxr->input[(lxr->index)++], '\0' });
}
void start_integer(lexer *lxr)
{
	//Starts new token for integer type
	start_token(lxr);
	lxr->data[lxr->count].type = INT;
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
void end_integer(lexer* lxr)
{
	//finishes handling current token and parsing lexeme into integer value
	token* temp = &(lxr->data[lxr->count]);
	lxr->index++;
	temp->value = strtod(temp->lexeme, NULL);
	lxr->count++;
}
void conv_float(lexer* lxr)
{
	//converts current token type to float
	add_char(lxr);
	lxr->data[lxr->count].type = FLOAT;
}
void end_float(lexer* lxr)
{
	//finishes handling float token
	token* tkn = &(lxr->data[lxr->count]);
	tkn->value = strtod(tkn->lexeme, NULL);
}
void conv_rational(lexer* lxr) //TODO 
{
	//converts existing token into rational type assuming
	token* tkn;
	lxr->count -= 1;
	tkn = &(lxr->data[lxr->count]);
	strcat(tkn->lexeme, (char[2]) { '/', '\0' });
	add_char(lxr);
	tkn->type = RATIONAL;
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
	lxr->data[lxr->count].type = BOOL;
}
void start_char(lexer *lxr)
{
	//starts new token of char type
	start_token(lxr);
	lxr->data[lxr->count].type = CHAR;
}
void end_char(lexer* lxr)
{
	//called when second ' detected - end char token
	add_char(lxr);
	lxr->count++;
}
void start_string(lexer *lxr)
{
	//starts new token of string type
	start_token(lxr);
	lxr->data[lxr->count].type = STRING;
}
void end_string(lexer* lxr)
{
	//called when second " detected, end string
	add_char(lxr);
	lxr->count++;
}
void conv_ident(lexer *lxr)
{
	//changes token type to identifier
	add_char(lxr);
	lxr->data[lxr->count].type = IDENTIFIER;
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
	start_token(lxr);
	lxr->data[lxr->count].type = CONTROLFLOW;
	lxr->count++;
}
void conv_controlflow(lexer* lxr)
{
	// changes current token to controlflow type
	add_char(lxr);
	lxr->data[lxr->count].type = CONTROLFLOW;
}

void end_token_start_operator(lexer* lxr)
{
	//ends current token and starts new operator
	end_token(lxr);
	start_operator(lxr);
}
void start_operator(lexer *lxr)
{
	//starts operator token and checks for dual character operators ie ++ += /= 
	start_token(lxr);
	lxr->data[lxr->count].type = OPERATOR;
	if (CHAR_CLASS[lxr->input[lxr->index]] == OPERATOR)
	{
		lxr->index++;
		add_char(lxr);
	}
	else
	{
		lxr->count++;
	}
}
void add_operator(lexer *lxr)
{
	//starts new operator token assuming single character operator;
	start_operator(lxr);
	lxr->data[lxr->count].type = OPERATOR;
	lxr->count++;
}

void conv_vartype(lexer *lxr)
{
	//convert to variable name type token ie int, float, string
	add_char(lxr);
	lxr->data[lxr->count].type = VARTYPE;
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
	static void (*method)(lexer*);
	static void(*END_METHOD[(type)DECLARE + 1])(lexer*) =
	{	
		/*INT*/		end_integer, 
		/*CF*/		ignore, 
		/*CHAR*/	expected_error, 
		/*FLOAT*/	end_float, 
		/*OP*/		ignore, 
		/*STR*/		expected_error, 
		/*NAT*/		ignore, 
		/*RAT*/		ignore,
		/*BOOL*/	ignore, 
		/*COND*/	ignore, 
		/*IDENT*/	ignore, 
		/*VARTYPE*/	ignore, 
		/*ERR*/		ignore, 
		/*DECLARE*/	ignore
	};
	method = END_METHOD[lxr->data[lxr->count].type];
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
	int state = 0;
	void (*action)(lexer*);

	index = 0;
	lxr->data = malloc(2 * sizeof(token));
	lxr->size = 2;
	lxr->count = 0;

	init_tables(CHAR_CLASS, GOTO, ACTION);


	while (input[index] != '\0')
	{
		ch_class = CHAR_CLASS[input[index]];

		action = ACTION[state][ch_class];
		state = GOTO[state][ch_class];

		action(lxr);
	}
}
