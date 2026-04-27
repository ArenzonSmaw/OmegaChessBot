#include "error.h"
#include <string.h>
#pragma warning (disable:4996)

error_message* error(char title[TITLE_MAX_LENGTH], char message[MESSAGE_MAX_LENGTH], int line, int col)
{
	error_message* er = (error_message*)malloc(sizeof(error_message));
	if (er != NULL)
	{
		strcpy(er->error_title, title);
		strcpy(er->error_message, message);
		er->line = line;
		er->col = col;
	}
	else
	{
		memory_error();
	}
	return er;
}
void print_error(error_message *msg)
{
	printf("%s: %s, At: Line:%d, Col:%d.", msg->error_title, msg->error_message, msg->line, msg->col);
}

error_list err_list()
{
	error_list lst = (error_list)malloc(sizeof(error_link));
	if (lst) {
		lst->msg = NULL;
		lst->next = lst;
		lst->prev = lst;
	}
	else
		memory_error();
	return lst;
}

int err_is_empty(error_list lst)
{
	return lst->next->msg == NULL;
}

void err_append(error_list lst, error_message* msg)
{
	error_list temp = (error_list)malloc(sizeof(error_link));
	if (temp == NULL) memory_error();
	temp->msg = msg;

	temp->next = lst;
	temp->prev = lst->prev;
	
	lst->prev = temp;
	temp->prev->next = temp;
}


void print_errors(error_list lst)
{
	while (!err_is_empty(lst))
	{
		print_error(lst->msg);
	}
}

void memory_error()
{
	printf("memory!");
	exit(1);
}