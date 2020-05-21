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
    if (!BUDDY_TYPE_VALID(buddy_type))
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
        BUDDY_INFO* elem;
        CREATE_BUDDY_NODE(elem, (start+i*(BUDDY_BASE_SIZE<<buddy_type)));
        ListAddTail(&free_area[buddy_type].list, (NODE *)elem);
    }
    return 0;
}

static void SplitNode(BUDDY_TYPE target_type, BUDDY_TYPE find_type)
{
    BUDDY_INFO* target_node, *curr_node;
    void *first_node_start, *second_node_start;
    
    if (find_type == target_type)
    {
        return;
    }
    if (!BUDDY_TYPE_VALID(target_type) || !BUDDY_TYPE_VALID(find_type))
    {
        return;
    }
    target_node = (BUDDY_INFO *)free_area[find_type].list.node.next;
    first_node_start = target_node->start;
    second_node_start = (first_node_start+(BUDDY_BASE_SIZE<<(find_type-1)));
    LIST_FREE_NODE(&free_area[find_type].list, target_node);
    CREATE_BUDDY_NODE(target_node, first_node_start);
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
    CREATE_BUDDY_NODE(target_node, second_node_start);
    ListInsert(&free_area[find_type-1].list, (NODE *)target_node, (NODE *)curr_node);
    SplitNode(target_type, find_type-1);
}

static int MergeNode(BUDDY_TYPE target_type, 
    BUDDY_INFO *front, 
    BUDDY_INFO *behind)
{
    if (!BUDDY_TYPE_VALID(target_type))
    {
        return -1;
    } 
    if (!BUDDY_CAN_MERGE(start, front->start, behind->start, target_type))
    {
        printf("Merge failed: front->start[%p] behind->start[%p]\n", front->start, behind->start);      
        return -1;
    }
    BUDDY_INFO* target_node;
    BUDDY_INFO* cur_node = NULL;
    CREATE_BUDDY_NODE(target_node, front->start);
    LIST_FOR_EACH(BUDDY_INFO, cur_node, free_area[target_type+1].list)
    {
        if (cur_node->start > target_node->start)
        {
            break;
        }
    }
    ListInsert(&free_area[target_type+1].list, (NODE *)target_node, (NODE *)cur_node);

    LIST_FREE_NODE(&free_area[target_type].list, front);
    LIST_FREE_NODE(&free_area[target_type].list, behind);
    return 0;
}

int BuddyAlloc(BUDDY_TYPE buddy_type, void **viraddr)
{
    int i = 0;
    BUDDY_INFO* target_node;
    if (!BUDDY_TYPE_VALID(buddy_type))
    {
        return -1;
    } 
    if (free_area[buddy_type].list.count != 0)
    {
        target_node = (BUDDY_INFO *)free_area[buddy_type].list.node.next;
		*viraddr = target_node->start;
        LIST_FREE_NODE(&free_area[buddy_type].list, target_node);
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
    BUDDY_INFO* target_node;
    if (!BUDDY_TYPE_VALID(buddy_type))
    {
        return;
    }

    CREATE_BUDDY_NODE(target_node, viraddr);
    LIST_FOR_EACH(BUDDY_INFO, cur_node, free_area[buddy_type].list)
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
            LIST_FOR_EACH(BUDDY_INFO, cur_node, free_area[i].list)
            {
                if (cur_node->list.node.next != &free_area[i].list.node)
                {
                    if (MergeNode(i, cur_node, (BUDDY_INFO*)cur_node->list.node.next) == 0)
                    {
                        bMerged = 1;
                        break;
                    }
                }
            }
        }
        if (!bMerged)
            i++;
        else
            bMerged = 0;
    }
}

static void MovingToMerge
(
    BUDDY_TYPE buddy_type, 
    BUDDY_INFO *curr_free_node, 
    BUDDY_INFO used_area[BUDDY_TYPE_MAX], 
    BUDDY_INFO *curr_used_node,
    void *(*move)(void *dest, const void* src, size_t n)
)
{
    if (!BUDDY_TYPE_VALID(buddy_type))
    {
        return;
    }
    BUDDY_INFO *next_free = (BUDDY_INFO*)curr_free_node->list.node.next;
    move(next_free->start, curr_used_node->start, (BUDDY_BASE_SIZE<<buddy_type));
                        
    ListDelete(&free_area[buddy_type].list, (NODE *)next_free);
    ListInsert(&used_area[buddy_type].list, (NODE *)next_free, (NODE *)curr_used_node->list.node.next);

    ListDelete(&used_area[buddy_type].list, (NODE *)curr_used_node);
    ListInsert(&free_area[buddy_type].list, (NODE *)curr_used_node, (NODE *)curr_free_node->list.node.next);
    if (curr_free_node->start > curr_used_node->start)
    {
        MergeNode(buddy_type, curr_used_node, curr_free_node);
    }
    else
    {
        MergeNode(buddy_type, curr_free_node, curr_used_node);
    }
    
}

static void BuddyIntegrate
(
    BUDDY_INFO used_area[BUDDY_TYPE_MAX], 
    void *(*move)(void *dest, const void* src, size_t n)
)
{
    BUDDY_INFO behind;
    int i = 0;
    BUDDY_INFO* target_node;
    for (i = 0; i < BUDDY_TYPE_MAX; )
    {
        if (free_area[i].list.count > 1)
        {
            BUDDY_INFO *front = (BUDDY_INFO*)free_area[i].list.node.next;
            behind.start = front->start + (BUDDY_BASE_SIZE<<i);
            if (!BUDDY_CAN_MERGE(start, front->start, behind.start, i))
            {
                behind.start = front->start - (BUDDY_BASE_SIZE<<i);
            }
            LIST_FOR_EACH(BUDDY_INFO, target_node, used_area[i].list)
            {
                if (target_node->start == behind.start)
                {
                    MovingToMerge(i, front, used_area, target_node, move);
                    break;
                }
            }
        }
        else
        {
            i++;
        }
    }
}

int BuddySmartAlloc(
    BUDDY_TYPE buddy_type, 
    void **viraddr, 
    void *(*move)(void *dest, const void* src, size_t n), 
    BUDDY_INFO used_area[BUDDY_TYPE_MAX]
)
{
    int remain_size = 0, i = 0;
    if (!BUDDY_TYPE_VALID(buddy_type))
    {
        return -1;
    }
    if (BuddyAlloc(buddy_type, viraddr) != -1)
    {
        return 0;
    }
    for (i = 0; i < BUDDY_TYPE_MAX; i++)
    {
        BUDDY_INFO* target_node;
        LIST_FOR_EACH(BUDDY_INFO, target_node, free_area[i].list)
        {
            remain_size += (1<<i);
        }
    }
    if (remain_size < (1<<buddy_type))
    {
        printf("total is not enough\n");
        return -1;
    }
    BuddyIntegrate(used_area, move);

    return BuddyAlloc(buddy_type, viraddr);
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
            LIST_FOR_EACH(BUDDY_INFO, target_node, free_area[i].list)
            {
                printf("|start: %p|", target_node->start);
            }
            printf("\n");
        }
    }
}
