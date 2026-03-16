#include <stdio.h>
#include "common.h"



void unknown_character();
void illegal_character();

void start_natural();
void start_rational();
void start_int();
void start_float();
void start_bool();
void start_char();
void start_string();
void start_ident();

void start_controlflow();
void add_controlflow();

void stay();

void start_operator();
void add_operator();
void end_operator();

void add_token();
void add_current();


typedef enum INPUT {
	UNKNOWN,
	WHITESPACE,
	CONTROLFLOW,
	SQUOTE, DQUOTE,
	DIGIT,
	LETTER,
	A, B, C, D, E, F, G, H, I, K, L, N, O, P, R, S, T, U, V, X,
	OPERATOR,
	DIVIDE, MINUS, DOT,
	PRINTABLE
} INPUT;

typedef enum TYPES {
	INT,
	CHAR,
	FLOAT,
	STRING,
	NATURAL,
	RATIONAL,
	BOOL,
	IDENTIFIER
} TYPES;

void init_tables()
{
	int ch, st;
	int CHAR_CLASS[128] = { PRINTABLE };

	int GOTO[106][30] = { 105 };
	void* ACTION[105][30];

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
	CHAR_CLASS['!'] = CHAR_CLASS['%'] = CHAR_CLASS['&'] = CHAR_CLASS[':'] = CHAR_CLASS['*'] = CHAR_CLASS['+'] = CHAR_CLASS['|'] = CHAR_CLASS['\\'] = OPERATOR;
	for (ch = '<'; ch <= '>'; ch++)
		CHAR_CLASS[ch] = OPERATOR;
	//operators that can affect tokens
	CHAR_CLASS['-'] = MINUS;
	CHAR_CLASS['.'] = DOT;
	CHAR_CLASS['/'] = DIVIDE;

	//control flow punctuation
	CHAR_CLASS['"'] = CHAR_CLASS[','] = CHAR_CLASS['['] = CHAR_CLASS[']'] = CHAR_CLASS['{'] = CHAR_CLASS['}'] = CONTROLFLOW;
	for (ch = '\''; ch <= ')'; ch++)
		CHAR_CLASS[ch] = CONTROLFLOW;

	//GOTO TBL
	for (st = 0; st < 106; st++)
	{
		GOTO[st][WHITESPACE] = 0;
		GOTO[st][CONTROLFLOW] = 0;

		for (ch = OPERATOR; ch < DOT; ch++)
			GOTO[st][ch] = 4;
	}

	//default state
	GOTO[0][WHITESPACE] = 0;
	GOTO[0][DIGIT] = 1;
	GOTO[0][DOT] = 2;
	GOTO[0][MINUS] = 4;
	GOTO[0][DIVIDE] = 4;
	//(strings and characters)
	GOTO[0][SQUOTE] = 104;
	for (ch = DIGIT; ch <= X; ch++)
		GOTO[104][ch] = 105;
	GOTO[104][PRINTABLE] = 105;
	GOTO[104][SQUOTE] = 0;
	GOTO[105][SQUOTE] = 0;
	GOTO[0][DQUOTE] = 103;	
	GOTO[103][DQUOTE] = 0;
	for (ch = DIGIT; ch <= X; ch++)
		GOTO[103][ch] = 103;
	GOTO[103][PRINTABLE] = 103;

	//redirection to identifier
	for (ch = LETTER; ch <= X; ch++)
		GOTO[0][ch] = 5;

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
	GOTO[1][DOT] = 2;
	GOTO[1][MINUS] = 4;
	GOTO[1][DIVIDE] = 3;

	//'dot' token
	GOTO[2][DIGIT] = 1;
	GOTO[2][OPERATOR] = 4;

	//'division' token
	GOTO[3][DIGIT] = 1;
	GOTO[3][WHITESPACE] = 105;

	//operators state
	GOTO[4][WHITESPACE] = 4;
	for (ch = OPERATOR; ch <= DOT; ch++)
		GOTO[4][ch] = 4;
	GOTO[4][DIGIT] = 1;
	GOTO[4][SQUOTE] = 104;
	GOTO[4][DQUOTE] = 103;

	for (ch = LETTER; ch <= X; ch++)
		GOTO[4][ch] = 5;

	//identifiers
	for (ch = DIGIT; ch <= X; ch++)
		GOTO[5][ch] = 5;
	GOTO[5][PRINTABLE] = 5;

	for (st = 6; st <= 101; st++)
	{ 
		for (ch = DIGIT; ch <= X; ch++)
			GOTO[st][ch] = 5;
		GOTO[st][PRINTABLE] = 5;
	}

	// keywords
	GOTO[6][O] = 7; //b|ool
	GOTO[7][O] = 8; //bo|ol
	GOTO[8][L] = 9; //boo|l

	GOTO[6][R] = 10; //b|reak
	GOTO[10][E] = 11; //br|eak
	GOTO[11][A] = 12; //bre|ak
	GOTO[12][K] = 13; //brea|k

	GOTO[14][H] = 15; //c|har , c|heck

	GOTO[19][R] = 20; //cha|r
	GOTO[15][A] = 19; //ch|ar

	GOTO[15][E] = 16; //ch|eck
	GOTO[16][C] = 17; //che|ck
	GOTO[17][K] = 18; //chec|k

	GOTO[21][E] = 22; //d|ecalre
	GOTO[22][C] = 23; //de|clare
	GOTO[23][L] = 24; //dec|lare
	GOTO[24][A] = 25; //decl|are
	GOTO[25][R] = 26; //decla|re
	GOTO[26][E] = 27; //declar|e

	GOTO[28][L] = 29; //e|lse
	GOTO[28][X] = 32; //e|xception

	GOTO[29][S] = 30; //el|se
	GOTO[30][E] = 31; //els|e

	GOTO[32][C] = 33; //ex|ception
	GOTO[33][E] = 34; //exc|eption
	GOTO[34][P] = 35; //exce|ption
	GOTO[35][T] = 36; //excep|tion
	GOTO[36][I] = 37; //except|ion
	GOTO[37][O] = 38; //excepti|on
	GOTO[38][N] = 39; //exceptio|n

	GOTO[40][L] = 41; //f|loat
	GOTO[41][O] = 42; //fl|oat
	GOTO[42][A] = 43; //flo|at
	GOTO[43][T] = 44; //floa|t

	GOTO[45][F] = 46; //i|f

	GOTO[45][N] = 47; //i|nt
	GOTO[47][T] = 48; //in|t

	GOTO[49][O] = 50; //l|ong, l|oop
	
	GOTO[50][O] = 53; //lo|op
	GOTO[53][P] = 54; //loo|p

	GOTO[51][G] = 52; //lon|g
	GOTO[50][N] = 51; //lo|ng
	

	GOTO[55][A] = 56; //n|atural
	GOTO[56][T] = 57; //na|tural
	GOTO[57][U] = 58; //nat|ural
	GOTO[58][R] = 59; //natu|ral
	GOTO[59][A] = 60; //natur|al
	GOTO[60][L] = 61; //natura|l

	GOTO[62][A] = 63; //p|ass
	GOTO[63][S] = 64; //pa|ss
	GOTO[64][S] = 65; //pas|s

	GOTO[62][O] = 66; //p|ointer
	GOTO[66][I] = 67; //po|inter
	GOTO[67][N] = 68; //poi|nter
	GOTO[68][T] = 69; //poin|ter
	GOTO[69][E] = 70; //point|er
	GOTO[70][R] = 71; //pointe|r

	GOTO[72][A] = 73; //r|ational
	GOTO[73][T] = 74; //ra|tional
	GOTO[74][I] = 75; //rat|ional
	GOTO[75][O] = 76; //rati|onal
	GOTO[76][N] = 77; //ratio|nal
	GOTO[77][A] = 78; //ration|al
	GOTO[78][L] = 79; //rationa|l

	GOTO[72][E] = 80; //r|eturn
	GOTO[80][T] = 81; //re|turn
	GOTO[81][U] = 82; //ret|urn
	GOTO[82][R] = 83; //retu|rn
	GOTO[83][N] = 84; //retur|n

	GOTO[85][H] = 86; //s|hort
	GOTO[86][O] = 87; //sh|ort
	GOTO[87][R] = 88; //sho|rt
	GOTO[88][T] = 89; //shor|t

	GOTO[85][T] = 90; //s|tring
	GOTO[90][R] = 91; //st|ring
	GOTO[91][I] = 92; //str|ing
	GOTO[92][N] = 93; //stri|ng
	GOTO[93][G] = 94; //strin|g

	GOTO[95][S] = 96; //u|se
	GOTO[96][E] = 97; //us|e

	GOTO[98][O] = 99;  //v|oid
	GOTO[99][I] = 100; //vo|id
	GOTO[100][D] = 101;//voi|d

	//error handling - panic mode recovery
	for (ch = UNKNOWN; ch <= PRINTABLE; ch++)
		GOTO[105][ch] = 105;
	for (ch = CONTROLFLOW; ch <= DQUOTE; ch++)
		GOTO[105][ch] = 0;

	//ACTION TBL

}
