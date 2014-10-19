#include <unistd.h>      // for sbrk
#include <sys/mmap.h>    // for mmap
#include <stdint.h>      // for uintptr_t, intptr_t
#include <stddef.h>      // for size_t, NULL, ptrdiff_t
#include <errno.h>       // for EAGAIN, errno
#include <malloc.h>
#include <string.h>

#include "system_alloc.h"

// Structure for discovering alignment
union memory_aligner {
    void *p;
    double d;
    size_t s;
}CACHELINE_ALIGNED;

system_allocator_t *system_allocator;
static int8_t system_alloc_inited = FALSE;

void *mmap_system_alloc(size_t size, size_t *actual_size, size_t alignment)
{
    void *result = NULL;
    size_t extra = 0, aligned_size = 0, adjust = 0;
    uintptr_t ptr;

    // Enforce page alignment
    if (alignment < PAGESIZE) alignment = PAGESIZE;
    aligned_size = ((size + alignment - 1) / alignment) * alignment;
    if (aligned_size < size) {
        return NULL;
    }
    size = aligned_size;

    // "actual_size" indicates that the bytes from the returned pointer
    // p up to and including (p + actual_size - 1) have been allocated.
    if (actual_size) {
        *actual_size = size;
    }

    // Ask for extra memory if alignment > PAGESIZE
    if (alignment > PAGESIZE) {
        extra = alignment - PAGESIZE;
    }

    result = mmap(NULL, size + extra,
                      PROT_READ|PROT_WRITE,
                      MAP_PRIVATE|MAP_ANONYMOUS,
                      -1, 0);
    if((void *)MAP_FAILED == result) {
        return NULL;
    }

    // Adjust the return memory so it is aligned
    ptr = (uintptr_t)result;
    if ((ptr & (alignment - 1)) != 0) {
        adjust = alignment - (ptr & (alignment - 1));
    }

    // Return the unused memory to the system
    if (adjust > 0) {
        munmap((void *)ptr, adjust);
    }
    if (adjust < extra) {
        munmap((void *)(ptr + adjust + size), extra - adjust);
    }

    ptr += adjust;
    return (void *)ptr;
}

void *sbrk_system_alloc(size_t size, size_t *actual_size, size_t alignment)
{
    void *result, *extra_result;
    uintptr_t ptr;
    size_t extra = 0;
    // sbrk will release memory if passed a negative number, so we do
    // a strict check here
    if(0 > (ptrdiff_t)(size + alignment)) return NULL;

    size = ((size + alignment - 1) / alignment) * alignment;

    if(actual_size) {
        *actual_size = size;
    }

    // Check that we we're not asking for so much more memory that we'd
    // wrap around the end of the virtual address space. (This seems
    // like something sbrk() should check for us, and indeed opensolaris
    // does, but glibc does not, Without this check, sbrk may succeed
    // when it ought to fail.)
    if((intptr_t)(sbrk(0)) + size < size) {
        return NULL;
    }

    result = sbrk(size);
    if(result == (void *)(-1)) {
        return NULL;
    }

    // Is it aligned?
    ptr = (uintptr_t)(result);
    if((ptr & (alignment - 1)) == 0) {
        return result;
    }

    // Try to get more memory for alignment
    extra = alignment - (ptr & (alignment - 1));
    extra_result = sbrk(extra);
    if((uintptr_t)extra_result == (ptr + size)) {
        // Contiguous with previous result
        return (void *)(ptr + extra);
    }

    // Give up and ask for "size + alignment - 1" bytes so
    // that we can find an aligned region within it.
    result = sbrk(size + alignment - 1);
    if(result == (void *)-1) {
        return NULL;
    }
    ptr = (uintptr_t)result;
    if((ptr & (alignment - 1)) != 0) {
        ptr += alignment - (ptr & (alignment - 1));
    }
    return (void *)ptr;
}

int8_t init_system_allocator(void)
{
    system_allocator = (system_allocator_t *)malloc(sizeof(system_allocator_t));
    if (system_allocator == NULL) {
        return INIT_ERR;
    }

    // init sbrk
    system_allocator->alloc_method[0] = sbrk_system_alloc;
    system_allocator->alloc_name[0] = (char *)malloc(5);
    if (system_allocator->alloc_name[0] == NULL) {
        goto err;
    }
    memcpy(system_allocator->alloc_name[0], "sbrk", 5);
    system_allocator->failed[0] = FALSE;

    // init mmap
    system_allocator->alloc_method[1] = mmap_system_alloc;
    system_allocator->alloc_name[1] = (char *)malloc(5);
    if (system_allocator->alloc_name[1] == NULL) {
        goto err;
    }
    memcpy(system_allocator->alloc_name[1], "mmap", 5);
    system_allocator->failed[1] = FALSE;

    return ALLOC_OK;

err:
    exit_system_allocator();
    return INIT_ERR;
}

void exit_system_allocator(void)
{
    int8_t i = 0;
    if(system_alloc_inited && system_allocator) {
        for(i = 0; i < MAX_ALLOC_METHOD; ++i) {
            if(system_allocator->alloc_name[i]) {
                free(system_allocator->alloc_name[i]);
            }
        }
        free(system_allocator);
    }
}

void *system_alloc(size_t size, size_t *actual_size, size_t alignment)
{
    void *result;
    size_t actual_size_storage;
    int8_t i;

    if (system_alloc_inited == FALSE) {
        if(ALLOC_OK == init_system_allocator()) {
            system_alloc_inited = TRUE;
        }
        else {
            return NULL;
        }
    }

    // Enforce minimum alignment
    if(alignment < sizeof(union memory_aligner))
        alignment = sizeof(union memory_aligner);

    if(actual_size == NULL) {
        actual_size = &actual_size_storage;
    }

    for(i = 0; i < MAX_ALLOC_METHOD; ++i) {
        if(!system_allocator->failed[i] &&
            system_allocator->alloc_method[i] != NULL) {
            result = system_allocator->alloc_method[i](size, actual_size, alignment);
            if(result != NULL) {
                return result;
            }
            system_allocator->failed[i] = TRUE;
        }
    }

    // After both failed, reset "failed_" to false so that a single failed
    // allocation won't make the allocator never work again.
    for(i = 0; i < MAX_ALLOC_METHOD; ++i) {
         system_allocator->failed[i] = FALSE;
    }
    return NULL;
}

void system_release(void* start, size_t length)
{
    const size_t pagemask = PAGESIZE - 1;
    size_t new_start = (size_t)start;
    size_t end = new_start + length;
    size_t new_end = end;
    int result;

    // Round up the starting address and round down the ending address
    // to be page aligned:
    new_start = (new_start + PAGESIZE - 1) & ~pagemask;
    new_end = new_end & ~pagemask;

    if(new_end > new_start) {
        do {
            result = madvise((char *)new_start, new_end - new_start, MADV_DONTNEED);
        }while(result == -1 && errno == EAGAIN);
    }
}

