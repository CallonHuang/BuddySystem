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
static void *mem_start = NULL;

int BuddyInit(BUDDY_TYPE buddy_type, int num, void* (*alloc)(size_t size))
{
    int i = 0;
    BUDDY_INFO *new_node = NULL;
    if (!BUDDY_TYPE_VALID(buddy_type))
        return -1;    
    mem_start = alloc((BUDDY_BASE_SIZE << buddy_type) * num);
    for (i = 0; i < BUDDY_TYPE_MAX; i++)
        ListInit(&free_area[i].list);
    for (i = 0; i < num; i++) {
        CREATE_BUDDY_NODE(new_node, (mem_start + i * (BUDDY_BASE_SIZE << buddy_type)));
        ListAddTail(&free_area[buddy_type].list, (NODE *)new_node);
    }
    return 0;
}

static void SplitNode(BUDDY_TYPE target_type, BUDDY_TYPE find_type)
{
    BUDDY_INFO *target_node = NULL, *curr_node = NULL, *new_node = NULL;
    void *first_node_start = NULL, *second_node_start = NULL;
    int i = find_type;
    
    if (!BUDDY_TYPE_VALID(target_type) || !BUDDY_TYPE_VALID(find_type))
        return;
    do {
        target_node = (BUDDY_INFO *)free_area[i].list.node.next;

        /*before higher order decomposition, save the start address*/
        first_node_start = target_node->start;
        second_node_start = (first_node_start+(BUDDY_BASE_SIZE<<(i-1)));
        LIST_FREE_NODE(&free_area[i].list, target_node);

        /*must ensure in order to insert*/
        CREATE_BUDDY_NODE(new_node, first_node_start);
        curr_node = (BUDDY_INFO *)free_area[i-1].list.node.next;
        if (free_area[i-1].list.count > 0) {
            LIST_FOR_EACH(BUDDY_INFO, curr_node, free_area[i-1].list) {
                if (curr_node->start > target_node->start)
                    break;
            }
        }
        ListInsert(&free_area[i-1].list, (NODE *)new_node, (NODE *)curr_node);
        CREATE_BUDDY_NODE(new_node, second_node_start);
        ListInsert(&free_area[i-1].list, (NODE *)new_node, (NODE *)curr_node);
        i -= 1;
    } while (i != target_type);
}

static int MergeNode(BUDDY_TYPE target_type, 
    BUDDY_INFO *front, 
    BUDDY_INFO *behind)
{
    if (!BUDDY_TYPE_VALID(target_type))
        return -1;
    if (!BUDDY_CAN_MERGE(mem_start, front->start, behind->start, target_type)) {
        printf("Merge failed: target_type[%d] front->start[%p] behind->start[%p]\n", 
            target_type, front->start, behind->start);      
        return -1;
    }
    BUDDY_INFO *new_node = NULL;
    BUDDY_INFO *curr_node = NULL;

    /*must ensure in order to insert*/
    CREATE_BUDDY_NODE(new_node, front->start);
    LIST_FOR_EACH(BUDDY_INFO, curr_node, free_area[target_type+1].list) {
        if (curr_node->start > new_node->start)
            break;
    }
    ListInsert(&free_area[target_type+1].list, (NODE *)new_node, (NODE *)curr_node);

    /*release no used resources*/
    LIST_FREE_NODE(&free_area[target_type].list, front);
    LIST_FREE_NODE(&free_area[target_type].list, behind);
    return 0;
}

int BuddyAlloc(BUDDY_TYPE buddy_type, void **viraddr)
{
    int i = 0;
    BUDDY_INFO *target_node = NULL;
    if (!BUDDY_TYPE_VALID(buddy_type))
        return -1;
    if (free_area[buddy_type].list.count == 0) {

        /*find the higher order*/
        for (i = buddy_type; i < BUDDY_TYPE_MAX; i++) {
            if (free_area[i].list.count != 0)
                break;
        }
        if (i == BUDDY_TYPE_MAX)
            return -1;
        SplitNode(buddy_type, i);
    }

    if (free_area[buddy_type].list.count != 0) {
        target_node = (BUDDY_INFO *)free_area[buddy_type].list.node.next;
        *viraddr = target_node->start;
        LIST_FREE_NODE(&free_area[buddy_type].list, target_node);
        return 0;
    }
    return -1;
}

void BuddyRecycle(BUDDY_TYPE buddy_type, void *viraddr)
{
    int i = 0, bMerged = 0;
    BUDDY_INFO *curr_node = NULL;
    BUDDY_INFO *new_node;
    if (!BUDDY_TYPE_VALID(buddy_type))
        return;
    /*must ensure in order to insert*/
    CREATE_BUDDY_NODE(new_node, viraddr);
    LIST_FOR_EACH(BUDDY_INFO, curr_node, free_area[buddy_type].list) {
        if (curr_node->start > viraddr)
            break;
    }
    ListInsert(&free_area[buddy_type].list, (NODE *)new_node, (NODE *)curr_node);

    /*traversal all the list to merge*/
    for (i = buddy_type; i < BUDDY_TYPE_MAX-1; ) {	
        bMerged = 0;
        if (free_area[i].list.count > 1) {
            LIST_FOR_EACH(BUDDY_INFO, curr_node, free_area[i].list) {
                if ((curr_node->list.node.next != &free_area[i].list.node) 
                    && (MergeNode(i, curr_node, (BUDDY_INFO*)curr_node->list.node.next) == 0)) {
                    bMerged = 1;
                    break;
                }
            }
        }
        if (!bMerged) {
            i++;
        }
    }
}

static void MovingToMerge
(
    BUDDY_TYPE buddy_type, 
    BUDDY_INFO *curr_free_node, 
    BUDDY_INFO used_area[BUDDY_TYPE_MAX], 
    BUDDY_INFO *curr_used_node,
    void* (*move)(void *dest, const void* src, size_t n)
)
{
    if (!BUDDY_TYPE_VALID(buddy_type))
        return;
    BUDDY_INFO *next_free = (BUDDY_INFO*)curr_free_node->list.node.next;

    /*make sure the data not lost*/
    move(next_free->start, curr_used_node->start, (BUDDY_BASE_SIZE<<buddy_type));

    /*swap used node & free node*/              
    ListDelete(&free_area[buddy_type].list, (NODE *)next_free);
    ListInsert(&used_area[buddy_type].list, (NODE *)next_free, (NODE *)curr_used_node->list.node.next);

    ListDelete(&used_area[buddy_type].list, (NODE *)curr_used_node);
    ListInsert(&free_area[buddy_type].list, (NODE *)curr_used_node, (NODE *)curr_free_node->list.node.next);
    
    /*merge in order*/
    if (curr_free_node->start > curr_used_node->start)
        MergeNode(buddy_type, curr_used_node, curr_free_node);
    else
        MergeNode(buddy_type, curr_free_node, curr_used_node);
}

static void BuddyIntegrate
(
    BUDDY_INFO used_area[BUDDY_TYPE_MAX], 
    void* (*move)(void *dest, const void* src, size_t n)
)
{
    BUDDY_INFO behind_buddy;
    int i = 0;
    BUDDY_INFO *curr_used_node = NULL;
    for (i = 0; i < BUDDY_TYPE_MAX; ) {

        /*merge util one or zero*/
        if (free_area[i].list.count > 1) {
            BUDDY_INFO *curr_free_node = (BUDDY_INFO*)free_area[i].list.node.next;

            /*behind node is a buddy?*/
            behind_buddy.start = curr_free_node->start + (BUDDY_BASE_SIZE << i);

            /*front is buddy if behind cannot merge*/
            if (!BUDDY_CAN_MERGE(mem_start, curr_free_node->start, behind_buddy.start, i))
                behind_buddy.start = curr_free_node->start - (BUDDY_BASE_SIZE << i);

            /*find the buddy*/
            LIST_FOR_EACH(BUDDY_INFO, curr_used_node, used_area[i].list) {
                if (curr_used_node->start == behind_buddy.start) {
                    MovingToMerge(i, curr_free_node, used_area, curr_used_node, move);
                    break;
                }
            }
        } else {
            i++;
        }
    }
}

int BuddySmartAlloc(
    BUDDY_TYPE buddy_type, 
    void **viraddr, 
    void* (*move)(void *dest, const void* src, size_t n), 
    BUDDY_INFO used_area[BUDDY_TYPE_MAX]
)
{
    int remain_size = 0, i = 0;
    BUDDY_INFO* curr_node = NULL;
    if (!BUDDY_TYPE_VALID(buddy_type))
        return -1;
    
    /*try alloc*/
    if (BuddyAlloc(buddy_type, viraddr) != -1)
        return 0;

    /*count remain_size: enough or not?*/
    for (i = 0; i < BUDDY_TYPE_MAX; i++) {
        LIST_FOR_EACH(BUDDY_INFO, curr_node, free_area[i].list) {
            remain_size += (1<<i);
        }
    }
    if (remain_size < (1<<buddy_type))
        return -1;

    /*merge disconnected node*/
    BuddyIntegrate(used_area, move);

    return BuddyAlloc(buddy_type, viraddr);
}

void BuddyDestroy(void (*release)(void* ptr))
{
    int i = 0;
    for (i = 0; i < BUDDY_TYPE_MAX; i++)
        ListDestroy(&free_area[i].list);
    release(mem_start);
    mem_start = NULL;
}

void BuddyPrt()
{
    int i = 0;
    BUDDY_INFO *curr_node = NULL;
    printf("Now Buddy Info As Bellow:\n");
    for (i = 0; i < BUDDY_TYPE_MAX; i++) {
        if (free_area[i].list.count != 0) {
            printf("|%d| ", i);
            LIST_FOR_EACH(BUDDY_INFO, curr_node, free_area[i].list) {
                printf("|start: %p|", curr_node->start);
            }
            printf("\n");
        }
    }
}
