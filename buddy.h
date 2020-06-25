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

#define CREATE_BUDDY_NODE(node, data) do{                                                \
                                        node = (BUDDY_INFO *)malloc (sizeof(BUDDY_INFO));\
                                        node->start = data;                              \
                                    }while(0)

#define BUDDY_TYPE_VALID(type)     ({                                                                   \
                                        int bValid = (type >= BUDDY_TYPE_1M && type < BUDDY_TYPE_MAX);  \
                                        (bValid);                                                       \
                                    })

#define BUDDY_CAN_MERGE(base, front_start, behind_start, type)     ({                                                                  \
                                        int bCanMerge = (front_start + (BUDDY_BASE_SIZE<<(type)) == behind_start)                      \
                                                        && ((unsigned long)(front_start - base) % (BUDDY_BASE_SIZE<<(type+1)) == 0);   \
                                        (bCanMerge);                                                                                   \
                                    })

int BuddyInit(BUDDY_TYPE buddy_type, int num, void* (*alloc)(size_t size));
int BuddyAlloc(BUDDY_TYPE buddy_type, void **viraddr);
void BuddyRecycle(BUDDY_TYPE buddy_type, void *viraddr);
int BuddySmartAlloc(BUDDY_TYPE buddy_type, void **viraddr, void* (*move)(void *dest, const void* src, size_t n), BUDDY_INFO used_area[BUDDY_TYPE_MAX]);
void BuddyDestroy(void (*release)(void* ptr));
void BuddyPrt(void);

#endif