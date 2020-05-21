#include "buddy.h"
#include "list.h"
#include <malloc.h>
#include <string.h>
//base function test
void TestProgram1()
{
    void *p[5];
    BuddyInit(BUDDY_TYPE_32M, 5, malloc);
    BuddyPrt();
    printf("================Begin Alloc=================\n");
    BuddyAlloc(BUDDY_TYPE_1M, &p[0]);
    BuddyPrt();
    printf("alloced: p[%p]\n", p[0]);
    BuddyAlloc(BUDDY_TYPE_4M, &p[1]);
    BuddyPrt();
    printf("alloced: p[%p]\n", p[1]);
    BuddyAlloc(BUDDY_TYPE_4M, &p[2]);
    BuddyPrt();
    printf("alloced: p[%p]\n", p[2]);
    BuddyAlloc(BUDDY_TYPE_2M, &p[3]);
    BuddyPrt();
    printf("alloced: p[%p]\n", p[3]);
    BuddyAlloc(BUDDY_TYPE_2M, &p[4]);
    BuddyPrt();
    printf("alloced: p[%p]\n", p[4]);
    /*
    Occupied memory:
    ----
    |//|
    ----
    
    So, now state show as below：
    ------------------------------------------...---
    |/|1|//|////|////|//| 2|     16         | 4*32 |
    ------------------------------------------...---
    */
    printf("================Begin Recycle=================\n");
    
    
    BuddyRecycle(BUDDY_TYPE_4M, p[1]);
    BuddyPrt();
    BuddyRecycle(BUDDY_TYPE_4M, p[2]);
    BuddyPrt();
    BuddyRecycle(BUDDY_TYPE_2M, p[4]);
    BuddyPrt();
    BuddyRecycle(BUDDY_TYPE_1M, p[0]);
    BuddyPrt();
    BuddyRecycle(BUDDY_TYPE_2M, p[3]);
    BuddyPrt();
    
    printf("==================Destroy===================\n");
    BuddyDestroy(free);
    BuddyPrt();
}

//smart alloc test
static BUDDY_INFO used_area[BUDDY_TYPE_MAX] = {0};
void UsedInfoPrint()
{
    int i = 0;
    printf("+++++++++++++Now Used Buddy Info As Bellow+++++++++++++\n");
    for (i = 0; i < BUDDY_TYPE_MAX; i++)
    {
        if (used_area[i].list.count != 0)
        {
            printf("|%d| ", i);
            BUDDY_INFO* target_node;
            LIST_FOR_EACH(BUDDY_INFO, target_node, used_area[i].list)
            {
                printf("|start: %p|", target_node->start);
            }
            printf("\n");
        }
    }
}

void AllocRecord(BUDDY_TYPE type)
{
    if (!BUDDY_TYPE_VALID(type))
    {
        return;
    }
    BUDDY_INFO* new_node;
    void *p = NULL;
    BuddyAlloc(type, &p);
    BuddyPrt();
    CREATE_BUDDY_NODE(new_node, p);
    ListAddTail(&used_area[type].list, (NODE *)new_node);
}

void RecycleUsed(BUDDY_TYPE type, int index)
{
    if (!BUDDY_TYPE_VALID(type))
    {
        return;
    }
    BUDDY_INFO *cur_node = (BUDDY_INFO *)used_area[type].list.node.next;
    int i = 0;
    for (i = 0; i < index; i++)
    {
        cur_node = (BUDDY_INFO *)cur_node->list.node.next;
    }
    BuddyRecycle(type, cur_node->start);
    BuddyPrt();
    LIST_FREE_NODE(&used_area[type].list, cur_node);
}

void TestProgram2()
{
    int i = 0;
    BUDDY_INFO * new_node = NULL, *cur_node = NULL;
    void *p;
    printf("++++++++++++++++++Test Smart Alloc+++++++++++++++++++\n");
    for (i = 0; i < BUDDY_TYPE_MAX; i++)
    {
        ListInit(&used_area[i].list);
    }
    BuddyInit(BUDDY_TYPE_16M, 1, malloc);
    BuddyPrt();
    printf("++++++++++++++++++Get Ready++++++++++++++++++\n");
    AllocRecord(BUDDY_TYPE_1M);
    AllocRecord(BUDDY_TYPE_1M);
    AllocRecord(BUDDY_TYPE_2M);
    AllocRecord(BUDDY_TYPE_1M);
    AllocRecord(BUDDY_TYPE_1M);
    AllocRecord(BUDDY_TYPE_2M);
    AllocRecord(BUDDY_TYPE_1M);
    AllocRecord(BUDDY_TYPE_1M);
    AllocRecord(BUDDY_TYPE_2M);
    AllocRecord(BUDDY_TYPE_1M);
    AllocRecord(BUDDY_TYPE_1M);
    AllocRecord(BUDDY_TYPE_2M);
/*
    Occupied memory:
    ----
    |//|
    ----
    
    So, now state show as below：
    -----------------------------
    |/|/|//|/|/|//|/|/|//|/|/|//|
    -----------------------------
*/
    RecycleUsed(BUDDY_TYPE_1M, 1);
    RecycleUsed(BUDDY_TYPE_1M, 2);
    RecycleUsed(BUDDY_TYPE_1M, 3);
    RecycleUsed(BUDDY_TYPE_1M, 4);
/* 
    So, now state show as below：
    -----------------------------
    |/|1|//|/|1|//|/|1|//|/|1|//|
    -----------------------------
*/   
    UsedInfoPrint();

    printf("+++++++++++++Start Buddy Smart Alloc+++++++++++++\n");
    BuddySmartAlloc(BUDDY_TYPE_4M, &p, memcpy, used_area);
    BuddyPrt();
    CREATE_BUDDY_NODE(new_node, p);
    LIST_FOR_EACH(BUDDY_INFO, cur_node, used_area[BUDDY_TYPE_4M].list)
    {
        if (cur_node->start > new_node->start)
        {
            break;
        }
    }
    ListInsert(&used_area[BUDDY_TYPE_4M].list, (NODE *)new_node, (NODE *)cur_node);
    UsedInfoPrint();

    for (i = 0; i < BUDDY_TYPE_MAX; i++)
    {
        ListDestroy(&used_area[i].list);
    }
    BuddyDestroy(free);
    BuddyPrt();
}


int main(int argc, char **argv)
{
    TestProgram1();
    TestProgram2();
    return 0;
}
