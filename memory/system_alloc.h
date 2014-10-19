#ifndef __SYSTEM_ALLOC__H__
#define __SYSTEM_ALLOC__H__

#include <sys/types.h>

#ifndef PAGESIZE
#define PAGESIZE 8096
#endif

#define ALLOC_OK 0
#define INIT_ERR -1
#define ALLOC_ERR -2

#define TRUE   1
#define FALSE  0

#define MAX_ALLOC_METHOD 2
typedef void * (*alloc_func)(size_t size, size_t *actual_bytes, size_t alignment);

typedef struct {
    alloc_func alloc_method[MAX_ALLOC_METHOD];
    char *alloc_name[MAX_ALLOC_METHOD];
    int8_t failed[MAX_ALLOC_METHOD];
}system_allocator_t;

void *mmap_system_alloc(size_t size, size_t *actual_size, size_t alignment);
void *sbrk_system_alloc(size_t size, size_t *actual_size, size_t alignment);

void *system_alloc(size_t size, size_t *actual_size, size_t alignment);
void system_release(void* start, size_t length);

int8_t init_system_allocator(void);
void exit_system_allocator(void);

#endif  /* __SYSTEM_ALLOC__H__ */
