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
void print_error(error_message *msg, FILE* out)
{
	fprintf(out,  "%s: %s, At: Line:%d, Col:%d.\n", msg->error_title, msg->error_message, msg->line, msg->col);
}

error_list err_list(FILE *err_out)
{
	error_list lst = (error_list)malloc(sizeof(error_node));
	lst->msg = (error_link)malloc(sizeof(error_message));
	if (lst->msg) {
		strcpy(lst->msg->error_message, "\0");
		strcpy(lst->msg->error_title, "\0");
		lst->msg->line = 0;
		lst->msg->col = 0;
		lst->msg->next = lst->msg;
		lst->msg->prev = lst->msg;
		lst->out = err_out;
	}
	else
		memory_error();
	return lst;
}

int err_is_empty(error_list lst)
{
	return !strcmp(lst->msg->next->error_title, "\0");
}

void err_append(error_list lst, error_message* msg)
{
	error_link temp = (error_link)malloc(sizeof(error_message));
	if (temp == NULL) memory_error();
	temp = msg;

	temp->next = lst->msg;
	temp->prev = lst->msg->prev;
	
	lst->msg->prev = temp;
	temp->prev->next = temp;
}


void print_errors(error_list lst)
{
	error_link temp = lst->msg->next;
	while (temp != lst->msg)
	{
		print_error(temp, lst->out);
		temp = temp->next;
	}
}

void free_err_list(error_list* lst)
{
	error_link temp = (*lst)->msg->next;
	error_link prev;
	fclose((*lst)->out);

	while (temp != (*lst)->msg)
	{
		prev = temp;
		temp = temp->next;
		free(prev);
	}
	free(temp);
	free(*lst);
	*lst = NULL;
}

void memory_error()
{
	printf("ran out of allocatable memory! exiting.");
	exit(1);
}