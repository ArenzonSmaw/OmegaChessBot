#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"
#include "lexer.h"

#pragma warning(disable:4996)

#define NUM_OF_STATES 106
#define NUM_OF_INPUTS 32
#define NUM_OF_CHARS 128


token_list node() {
	return (token_list)malloc(sizeof(token_node));
}

void unknown_character(token_list *lst, char value);
void illegal_character(token_list *lst, char value);

void start_token(token_list *lst, char value);

void start_natural(token_list *lst, char value);
void start_rational(token_list* lst, char value) {}
void start_int(token_list* lst, char value) {}
void start_float(token_list* lst, char value) {}
void start_bool(token_list *lst, char value);
void start_char(token_list *lst, char value);
void start_string(token_list *lst, char value);
void start_ident(token_list *lst, char value);

void start_controlflow(token_list* lst, char value);
void add_controlflow(token_list *lst, char value);

void add_char(token_list *lst, char value);
void add_digit(token_list *lst, char value);
void stay(token_list* lst, char value);
void skip(token_list *lst, char value) {}

void start_operator(token_list *lst, char value);
void add_operator(token_list *lst, char value);
void end_operator(token_list *lst, char value) {}

void start_vartype(token_list* lst, char value);
void start_errorhandler(token_list* lst, char value);
void start_declare(token_list* lst, char value);
void start_conditional(token_list* lst, char value);

void add_token(token_list *lst, char value) {}
void add_current(token_list* lst, char value) {}


static int CHAR_CLASS[NUM_OF_CHARS] = { PRINTABLE };
static int GOTO[NUM_OF_STATES][NUM_OF_INPUTS] = { 105 };
static void (*ACTION[NUM_OF_STATES][NUM_OF_INPUTS])(token_list*, char) = { illegal_character };

void init_tables()
{
	int ch, st;

	//CHAR CLASS TBL
	// 
	//invisible characters
	for (ch = 0; ch < 32; ch++)
		CHAR_CLASS[ch] = UNKNOWN;
	CHAR_CLASS[127] = UNKNOWN;

	//tabbing characters
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

	//GOTO + ACTION TBL
	for (st = 0; st < 106; st++)
	{
		GOTO[st][WHITESPACE] = 0;
		ACTION[st][WHITESPACE] = skip;

		GOTO[st][CF] = 0;
		ACTION[st][CF] = add_controlflow;

		GOTO[st][OP] = 4;
		ACTION[st][OP] = add_operator;
		
	}
	//floating point
	/*GOTO[0][DOT] = 2; for future development
	ACTION[0][DOT] = start_float;*/

	//default state
	GOTO[0][WHITESPACE] = 0;
	ACTION[0][WHITESPACE] = skip;
	GOTO[0][DIGIT] = 1;
	ACTION[0][DIGIT] = start_natural;

	//(strings and characters)
	GOTO[0][SQUOTE] = 104;
	ACTION[0][SQUOTE] = start_char;
	for (ch = DIGIT; ch <= X; ch++) 
	{
		GOTO[104][ch] = 105;
		ACTION[104][ch] = illegal_character;
	}
	GOTO[104][PRINTABLE] = 105;
	ACTION[104][PRINTABLE] = illegal_character;
	GOTO[104][SQUOTE] = 0;
	ACTION[104][SQUOTE] = add_char;
	GOTO[105][SQUOTE] = 0;
	ACTION[105][SQUOTE] = add_char;
	GOTO[0][DQUOTE] = 103;
	ACTION[0][DQUOTE] = start_string;
	GOTO[103][DQUOTE] = 0;
	ACTION[103][DQUOTE] = add_char;
	for (ch = DIGIT; ch <= X; ch++)
	{
		GOTO[103][ch] = 103;
		ACTION[103][ch] = add_char;
	}
	GOTO[103][WHITESPACE] = 103;
	ACTION[103][WHITESPACE] = add_char;
	GOTO[103][PRINTABLE] = 103;
	ACTION[103][PRINTABLE] = add_char;

	//redirection to identifier
	for (ch = LETTER; ch <= X; ch++)
	{
		GOTO[0][ch] = 5;
		ACTION[0][ch] = start_ident;
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
	GOTO[1][DIGIT] = 1;
	ACTION[1][DIGIT] = add_digit;
	
	/*GOTO[1][DOT] = 2; for future development
	ACTION[1][DOT] = to_float;

	GOTO[1][DIVIDE] = 2;
	ACTION[1][DIVIDE] = to_rational;*/


	//operators state
	GOTO[4][WHITESPACE] = 0;
	ACTION[4][WHITESPACE] = skip;
	GOTO[4][OPERATOR] = 4;
	ACTION[4][OPERATOR] = add_operator;
	GOTO[4][DIGIT] = 1;
	ACTION[4][DIGIT] = start_natural;
	GOTO[4][SQUOTE] = 104;
	ACTION[4][SQUOTE] = start_char;
	GOTO[4][DQUOTE] = 103;
	ACTION[4][DQUOTE] = start_string;
	for (ch = LETTER; ch <= X; ch++)
	{
		GOTO[4][ch] = 5;
		ACTION[4][ch] = start_ident;
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
		GOTO[5][ch] = 5;
		ACTION[5][ch] = stay;
	}
	GOTO[5][PRINTABLE] = 5;
	ACTION[5][PRINTABLE] = stay;

	for (st = 6; st <= 101; st++)
	{ 
		for (ch = DIGIT; ch <= X; ch++)
		{
			GOTO[st][ch] = 5;
			ACTION[st][ch] = add_char;
		}
		GOTO[st][PRINTABLE] = 5;
		ACTION[st][PRINTABLE] = add_char;

		GOTO[st][WHITESPACE] = 0;
		ACTION[st][WHITESPACE] = skip;
	}

	// keywords
	GOTO[6][O] = 7; //b|ool
	GOTO[7][O] = 8; //bo|ol
	GOTO[8][L] = 9; //boo|l
	ACTION[8][L] = start_vartype;

	GOTO[6][R] = 10; //b|reak
	GOTO[10][E] = 11; //br|eak
	GOTO[11][A] = 12; //bre|ak
	GOTO[12][K] = 13; //brea|k
	ACTION[12][K] = start_controlflow;

	GOTO[14][H] = 15; //c|har , c|heck
	
	GOTO[15][A] = 19; //ch|ar
	GOTO[19][R] = 20; //cha|r
	ACTION[19][R] = start_vartype;

	GOTO[15][E] = 16; //ch|eck
	GOTO[16][C] = 17; //che|ck
	GOTO[17][K] = 18; //chec|k
	ACTION[17][K] = start_errorhandler;

	GOTO[21][E] = 22; //d|ecalre
	GOTO[22][C] = 23; //de|clare
	GOTO[23][L] = 24; //dec|lare
	GOTO[24][A] = 25; //decl|are
	GOTO[25][R] = 26; //decla|re
	GOTO[26][E] = 27; //declar|e
	ACTION[26][E] = start_declare;

	GOTO[28][L] = 29; //e|lse
	GOTO[28][X] = 32; //e|xception

	GOTO[29][S] = 30; //el|se
	GOTO[30][E] = 31; //els|e
	ACTION[30][E] = start_conditional;

	GOTO[32][C] = 33; //ex|ception
	GOTO[33][E] = 34; //exc|eption
	GOTO[34][P] = 35; //exce|ption
	GOTO[35][T] = 36; //excep|tion
	GOTO[36][I] = 37; //except|ion
	GOTO[37][O] = 38; //excepti|on
	GOTO[38][N] = 39; //exceptio|n
	ACTION[38][N] = start_errorhandler;

	GOTO[40][L] = 41; //f|loat
	GOTO[41][O] = 42; //fl|oat
	GOTO[42][A] = 43; //flo|at
	GOTO[43][T] = 44; //floa|t
	ACTION[43][T] = start_vartype;

	GOTO[45][F] = 46; //i|f
	ACTION[45][F] = start_conditional;

	GOTO[45][N] = 47; //i|nt
	GOTO[47][T] = 48; //in|t
	ACTION[47][T] = start_vartype;

	GOTO[49][O] = 50; //l|ong, l|oop
	
	GOTO[50][O] = 53; //lo|op
	GOTO[53][P] = 54; //loo|p
	ACTION[53][P] = start_controlflow;

	GOTO[50][N] = 51; //lo|ng
	GOTO[51][G] = 52; //lon|g
	ACTION[51][G] = start_vartype;

	GOTO[55][A] = 56; //n|atural
	GOTO[56][T] = 57; //na|tural
	GOTO[57][U] = 58; //nat|ural
	GOTO[58][R] = 59; //natu|ral
	GOTO[59][A] = 60; //natur|al
	GOTO[60][L] = 61; //natura|l
	ACTION[60][L] = start_vartype;

	GOTO[62][A] = 63; //p|ass
	GOTO[63][S] = 64; //pa|ss
	GOTO[64][S] = 65; //pas|s
	ACTION[64][S] = start_controlflow;

	GOTO[62][O] = 66; //p|ointer
	GOTO[66][I] = 67; //po|inter
	GOTO[67][N] = 68; //poi|nter
	GOTO[68][T] = 69; //poin|ter
	GOTO[69][E] = 70; //point|er
	GOTO[70][R] = 71; //pointe|r
	ACTION[70][R] = start_vartype;

	GOTO[72][A] = 73; //r|ational
	GOTO[73][T] = 74; //ra|tional
	GOTO[74][I] = 75; //rat|ional
	GOTO[75][O] = 76; //rati|onal
	GOTO[76][N] = 77; //ratio|nal
	GOTO[77][A] = 78; //ration|al
	GOTO[78][L] = 79; //rationa|l
	ACTION[78][L] = start_vartype;

	GOTO[72][E] = 80; //r|eturn
	GOTO[80][T] = 81; //re|turn
	GOTO[81][U] = 82; //ret|urn
	GOTO[82][R] = 83; //retu|rn
	GOTO[83][N] = 84; //retur|n
	ACTION[83][N] = start_controlflow;

	GOTO[85][H] = 86; //s|hort
	GOTO[86][O] = 87; //sh|ort
	GOTO[87][R] = 88; //sho|rt
	GOTO[88][T] = 89; //shor|t
	ACTION[88][T] = start_vartype;

	GOTO[85][T] = 90; //s|tring
	GOTO[90][R] = 91; //st|ring
	GOTO[91][I] = 92; //str|ing
	GOTO[92][N] = 93; //stri|ng
	GOTO[93][G] = 94; //strin|g
	ACTION[93][G] = start_vartype;

	GOTO[95][S] = 96; //u|se
	GOTO[96][E] = 97; //us|e
	ACTION[96][E] = start_declare;

	GOTO[98][O] = 99;  //v|oid
	GOTO[99][I] = 100; //vo|id
	GOTO[100][D] = 101;//voi|d
	ACTION[100][D] = start_vartype;

	//error handling - panic mode recovery
	for (ch = UNKNOWN; ch <= PRINTABLE; ch++)
	{
		GOTO[105][ch] = 105;
		ACTION[105][ch] = add_char;
	}
	for (ch = CONTROLFLOW; ch <= DQUOTE; ch++)
	{
		GOTO[105][ch] = 0;
		ACTION[105][ch] = add_token;
	}
}

void start_token(token_list *lst, char ch)
{
	token_list token = node();
	strcpy(token->info.str_val, "\0");
	if (*lst != NULL)
	{
		token->next = (*lst)->next;
		
		(*lst)->next = token;
		*lst = token;
	}
	else 
	{
		token->next = lst;
		*lst = token;
	}
}
void start_natural(token_list *lst, char value)
{
	token_list token = node();
	token->info.num_val = value - '0';
	token->type = NATURAL;
	if (*lst != NULL)
	{
		token->next = (*lst)->next;
		(*lst)->next = token;
		*lst = token;
	}
	else
	{
		token->next = *lst;
		*lst = token;
	}
}
//void start_int(token_list* lst, char value)
//{
//	start_natural(lst, value);
//	(*lst)->type = INT;
//}
void add_digit(token_list *lst, char value)
{
	(*lst)->info.num_val *= 10;
	(*lst)->info.num_val += value - '0';
}
void start_bool(token_list *lst, char value)
{
	add_char(lst, value);
	(*lst)->type = BOOL;
}
void start_char(token_list *lst, char value)
{
	start_token(lst, value);
	add_char(lst, value);
	(*lst)->type = CHAR;
}
void start_string(token_list *lst, char value)
{
	start_token(lst, value);
	add_char(lst, value);
	(*lst)->type = STRING;
}
void start_ident(token_list *lst, char value)
{
	start_token(lst, value);
	add_char(lst, value);
	(*lst)->type = IDENTIFIER;
}

void add_controlflow(token_list *lst, char value)
{
	start_token(lst, value);
	add_char(lst, value);
	(*lst)->type = CONTROLFLOW;
}
void start_controlflow(token_list* lst, char value)
{
	add_controlflow(lst, value);
}

void add_char(token_list *lst, char value)
{
	strcat((*lst)->info.str_val, (char[2]){value, '\0'});
}

void start_operator(token_list *lst, char value)
{
	start_token(lst, value);
	add_char(lst, value);
	(*lst)->type = OPERATOR;
}
void add_operator(token_list *lst, char value)
{
	start_operator(lst, value);
}

void start_vartype(token_list* lst, char value)
{
	add_char(lst, value);
	(*lst)->type = VARTYPE;
}
void start_errorhandler(token_list* lst, char value)
{
	add_char(lst, value);
	(*lst)->type = ERROR_HANDLER;
}
void start_declare(token_list* lst, char value)
{
	add_char(lst, value);
	(*lst)->type = DECLARE;
}
void start_conditional(token_list* lst, char value)
{
	add_char(lst, value);
	(*lst)->type = CONDITIONAL;
}

void stay(token_list* lst, char value)
{
	add_char(lst, value);
}

void unknown_character(token_list *lst, char value)
{
	printf("unknown character %c", value);
}
void illegal_character(token_list *lst, char value)
{
	printf("illegal character %c", value);
}

token_list tokenize(char text[])
{
	char ch = *text;
	INPUT ch_class;
	int state = 0;
	token_list list = node();
	token_list pos;
	void (*action)(token_list*, char);

	init_tables(CHAR_CLASS, GOTO, ACTION);

	list->next = NULL;
	list->type = HEADER;
	pos = list;

	while (ch != '\0')
	{
		ch_class = CHAR_CLASS[ch];

		action = ACTION[state][ch_class];
		state = GOTO[state][ch_class];

		action(&pos, ch);

		ch = *(++text);
	}

	return list;
}
