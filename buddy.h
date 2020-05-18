#ifndef _BUDDY_H_
#define _BUDDY_H_

#include "list.h"
#include <malloc.h>


#define BUDDY_BASE_SIZE (1*1024*1024)

typedef struct
{
    LIST list;
    /* data */
    void *start;
} BUDDY_INFO;

typedef enum 
{
    BUDDY_TYPE_1M = 0,
    BUDDY_TYPE_2M,
    BUDDY_TYPE_4M,
    BUDDY_TYPE_8M,
    BUDDY_TYPE_16M,
    BUDDY_TYPE_32M,
    BUDDY_TYPE_MAX
} BUDDY_TYPE;

int BuddyInit(BUDDY_TYPE buddy_type, int num, void* (*alloc)(size_t size));
int BuddyAlloc(BUDDY_TYPE buddy_type, void **viraddr);
void BuddyRecycle(BUDDY_TYPE buddy_type, void *viraddr);
void BuddyDestroy(void (*release)(void* ptr));
void BuddyPrt(void);

#endif