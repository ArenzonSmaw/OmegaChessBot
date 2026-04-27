#ifndef ERROR_H
#define ERROR_H

#include <stdlib.h>
#include <stdio.h>

#define MESSAGE_MAX_LENGTH 65
#define TITLE_MAX_LENGTH 21

typedef struct 
{
	char error_message[MESSAGE_MAX_LENGTH];
	char error_title[TITLE_MAX_LENGTH];
	int line, col;
}error_message;

typedef struct error_node
{
	error_message* msg;
	struct error_node* next;
	struct error_node* prev;
} error_link, *error_list;

error_message* error(char err_title[TITLE_MAX_LENGTH], char err_text[MESSAGE_MAX_LENGTH], int line, int col);

error_list err_list();
void err_append(error_list*, error_message*);
int err_is_empty(error_list);

void print_errors(error_list);

void memory_error();

#endif