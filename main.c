#include "buddy.h"
#include "list.h"
#include <malloc.h>

int main(int argc, char **argv)
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
    return 0;
}
