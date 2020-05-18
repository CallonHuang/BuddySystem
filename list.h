#ifndef _LIST_H_
#define _LIST_H_
#include <stdlib.h>

typedef struct node
{
	struct node * next;
	struct node * prev;
}NODE;

typedef struct
{
    NODE node;
    int count;
}LIST;

void ListInit(LIST* list);
void ListInsert(LIST *list, NODE *node, NODE *next);
void ListAddTail(LIST *list, NODE *node);
void ListDelete(LIST *list, NODE *node);
void ListDestroy(LIST *list);

#endif
