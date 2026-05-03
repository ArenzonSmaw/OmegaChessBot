#ifndef ERROR_H
#define ERROR_H

#include <stdlib.h>
#include <stdio.h>

#define MESSAGE_MAX_LENGTH 129
#define TITLE_MAX_LENGTH 21

typedef struct error_message
{
	char error_message[MESSAGE_MAX_LENGTH];
	char error_title[TITLE_MAX_LENGTH];
	int line, col;
	struct error_message* next;
	struct error_message* prev;
}error_message, *error_link;

typedef struct error_node
{
	error_link msg;
	FILE* out;
} error_node, *error_list;

error_message* error(char err_title[TITLE_MAX_LENGTH], char err_text[MESSAGE_MAX_LENGTH], int line, int col);

error_list err_list(FILE *output_file);
void err_append(error_list*, error_message*);
int err_is_empty(error_list);
void free_err_list(error_list*);

void print_errors(error_list);

void memory_error();

#endif