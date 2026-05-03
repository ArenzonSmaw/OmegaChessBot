#include "grammar.h"




item_set* rule(item s, item* arr)
{
	item_set* gr = (item_set*)malloc(sizeof(item_set));
	gr->lhs = s;
	gr->rhs = arr;
}