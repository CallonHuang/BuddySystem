#include "buddy.h"

/*
list topology:
   ---------------------------
   | ----------------------- |
   | |                     | |
   v |                     v |
   HEAD<-->node<->node<...>node

free_area:
   |0|1M|-->list
   |1|2M|-->list
   |2|4M|-->list
   |3|8M|-->list
   |4|16M|->list
   |5|32M|->list
*/
static BUDDY_INFO free_area[BUDDY_TYPE_MAX] = {0};
static void *start = NULL;

int BuddyInit(BUDDY_TYPE buddy_type, int num, void* (*alloc)(size_t size))
{
    int i = 0;
    if (buddy_type == BUDDY_TYPE_MAX)
    {
        return -1;
    }    
    start = alloc((BUDDY_BASE_SIZE<<buddy_type)*num);
    for (i = 0; i < BUDDY_TYPE_MAX; i++)
    {
        ListInit(&free_area[i].list);
    }
    printf("all of start is %p\n", start);
    for (i = 0; i < num; i++)
    {
        BUDDY_INFO *elem = (BUDDY_INFO *)malloc (sizeof(BUDDY_INFO));
        if (NULL == elem)
        {
            return -1;
        }
        elem->start = (start+i*(BUDDY_BASE_SIZE<<buddy_type));
        ListAddTail(&free_area[buddy_type].list, (NODE *)elem);
    }
    return 0;
}

static void SplitNode(BUDDY_TYPE target_type, BUDDY_TYPE find_type)
{
    BUDDY_INFO* target_node, *curr_node;
    void *start1, *start2;
    if (find_type == target_type)
    {
        return;
    }
    target_node = (BUDDY_INFO *)free_area[find_type].list.node.next;
    start1 = target_node->start;
    start2 = (start1+(BUDDY_BASE_SIZE<<(find_type-1)));
    ListDelete(&free_area[find_type].list, (NODE *)target_node);
    free(target_node);
    target_node = (BUDDY_INFO *)malloc (sizeof(BUDDY_INFO));
    target_node->start = start1;
    curr_node = (BUDDY_INFO *)free_area[find_type-1].list.node.next;
    if (free_area[find_type-1].list.count > 0)
    {
        for (; curr_node != (BUDDY_INFO *)&free_area[find_type-1].list.node; 
            curr_node = (BUDDY_INFO *)curr_node->list.node.next)
        {
            if (curr_node->start > target_node->start)
            {
                break;
            }
        }
    }
    ListInsert(&free_area[find_type-1].list, (NODE *)target_node, (NODE *)curr_node);
    target_node = (BUDDY_INFO *)malloc (sizeof(BUDDY_INFO));
    target_node->start = start2;
    ListInsert(&free_area[find_type-1].list, (NODE *)target_node, (NODE *)curr_node);
    SplitNode(target_type, find_type-1);
}

static int MergeNode(BUDDY_TYPE target_type, 
    BUDDY_INFO *front, 
    BUDDY_INFO *behind)
{
    if ((front->start + (BUDDY_BASE_SIZE<<target_type) != behind->start)
        || ((unsigned long)(front->start - start) % (BUDDY_BASE_SIZE<<(target_type+1)) != 0))
    {
        printf("Merge failed: front->start[%p] behind->start[%p]\n", front->start, behind->start);      
        return -1;
    }
    BUDDY_INFO* target_node = (BUDDY_INFO *)malloc (sizeof(BUDDY_INFO));
    BUDDY_INFO* cur_node = NULL;
    target_node->start = front->start;
    for (cur_node = (BUDDY_INFO*)free_area[target_type+1].list.node.next; 
        cur_node != (BUDDY_INFO*)&free_area[target_type+1].list.node; 
        cur_node = (BUDDY_INFO*)cur_node->list.node.next)
    {
        if (cur_node->start > target_node->start)
        {
            break;
        }
    }
    ListInsert(&free_area[target_type+1].list, (NODE *)target_node, (NODE *)cur_node);
    ListDelete(&free_area[target_type].list, (NODE *)front);
    free(front);
    ListDelete(&free_area[target_type].list, (NODE *)behind);
    free(behind);
    return 0;
}

int BuddyAlloc(BUDDY_TYPE buddy_type, void **viraddr)
{
    int i = 0;
    BUDDY_INFO* target_node;
    if (free_area[buddy_type].list.count != 0)
    {
        target_node = (BUDDY_INFO *)free_area[buddy_type].list.node.next;
		*viraddr = target_node->start;
        ListDelete(&free_area[buddy_type].list, (NODE *)target_node);
        free(target_node);
    }
    else
    {
        for (i = buddy_type; i < BUDDY_TYPE_MAX; i++)
        {
            if (free_area[i].list.count != 0)
            {
                break;
            }
        }
        if (i != BUDDY_TYPE_MAX)
        {
            SplitNode(buddy_type, i);
            return BuddyAlloc(buddy_type, viraddr);
        }
        else
        {
            return -1;
        }
    }
    return 0;
}

void BuddyRecycle(BUDDY_TYPE buddy_type, void *viraddr)
{
    int i = 0, bMerged = 0;
    BUDDY_INFO* cur_node = NULL;
    BUDDY_INFO* target_node = (BUDDY_INFO *)malloc (sizeof(BUDDY_INFO));
    target_node->start = viraddr;
    for (cur_node = (BUDDY_INFO*)free_area[buddy_type].list.node.next; 
        cur_node != (BUDDY_INFO*)&free_area[buddy_type].list.node; 
        cur_node = (BUDDY_INFO*)cur_node->list.node.next)
    {
        if (cur_node->start > viraddr)
        {
            break;
        }
    }
    ListInsert(&free_area[buddy_type].list, (NODE *)target_node, (NODE *)cur_node);
    for (i = buddy_type; i < BUDDY_TYPE_MAX-1; )
    {	
        if (free_area[i].list.count > 1)
        {
            for (cur_node = (BUDDY_INFO*)free_area[i].list.node.next; 
                cur_node->list.node.next != &free_area[i].list.node; 
                cur_node = (BUDDY_INFO*)cur_node->list.node.next)
            {
                if (MergeNode(i, cur_node, (BUDDY_INFO*)cur_node->list.node.next) == 0)
                {
                    bMerged = 1;
                    break;
                }
            }
        }
        if (!bMerged)
            i++;
        else
            bMerged = 0;
    }
}

void BuddyDestroy(void (*release)(void* ptr))
{
    int i = 0;
    for (i = 0; i < BUDDY_TYPE_MAX; i++)
    {
        ListDestroy(&free_area[i].list);
    }
    release(start);
    start = NULL;
}

void BuddyPrt()
{
    int i = 0;
    printf("Now Buddy Info As Bellow:\n");
    for (i = 0; i < BUDDY_TYPE_MAX; i++)
    {
        if (free_area[i].list.count != 0)
        {
            printf("|%d| ", i);
            BUDDY_INFO* target_node;
            for (target_node = (BUDDY_INFO*)free_area[i].list.node.next; 
                target_node != (BUDDY_INFO*)&free_area[i].list.node; 
                target_node = (BUDDY_INFO*)target_node->list.node.next)
            {
                printf("|start: %p|", target_node->start);
            }
            printf("\n");
        }
    }
}
