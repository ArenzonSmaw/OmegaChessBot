#ifndef ERROR_H
#define ERROR_H

#include <stdlib.h>
#include <stdio.h>

typedef struct 
{
	char error_message[32];
	char error_title[21];
	int line, col;
}error_message;

typedef struct error_node
{
	error_message* msg;
	struct error_node* next;
	struct error_node* prev;
} error_link, *error_list;

error_message* error(char err_title[21], char err_text[32], int line, int col);

error_list err_list();
void err_append(error_list*, error_message*);
int err_is_empty(error_list);

void print_errors(error_list);

void memory_error();

#endif