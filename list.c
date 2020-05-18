#include "list.h"

#define LIST_HEAD list->node
void ListInit(LIST* list)
{
    LIST_HEAD.next = &LIST_HEAD;
    LIST_HEAD.prev = &LIST_HEAD;
    list->count = 0;
}

void ListInsert(LIST *list, NODE *node, NODE *next)
{
    node->next = next;
    next->prev->next = node;
    node->prev = next->prev;
    next->prev = node;
    list->count++;
}

void ListAddTail(LIST *list, NODE *node)
{
    ListInsert(list, node, &LIST_HEAD); 
}

void ListDelete(LIST *list, NODE *node)
{
    if (list->count > 0 && node->prev != NULL && node->next != NULL)
    {
        node->prev->next = node->next;
        node->next->prev = node->prev;
        list->count--;
    }
}

void ListDestroy(LIST *list)
{
    NODE *node;
    for (node = LIST_HEAD.next; list->count > 0; node = LIST_HEAD.next)
    {
        ListDelete(list, node);
        free(node);
    }
    
}
