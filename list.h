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

#define LIST_FOR_EACH(TYPE, cur, list_struct)  \
for (cur = (TYPE *)list_struct.node.next; cur != (TYPE *)&list_struct.node; cur = (TYPE *)cur->list.node.next)

#define LIST_FREE_NODE(list, node) do{                                  \
                                        ListDelete(list, (NODE *)node);\
                                        free(node);                     \
                                    }while(0)

void ListInit(LIST* list);
void ListInsert(LIST *list, NODE *node, NODE *next);
void ListAddTail(LIST *list, NODE *node);
void ListDelete(LIST *list, NODE *node);
void ListDestroy(LIST *list);

#endif
