#include <string.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include "../../memory/system_alloc.h"

#define TEST_TIMES 1000

void test_system_alloc(void)
{
    size_t size = 0, actual_size = 0, alignment = 32;
    int i = 0;
    char *space = NULL;

    srand((unsigned)time(NULL));

    for (i = 0; i < TEST_TIMES; ++i) {
        size = rand() % 32767 + 1;
        space = (char *)system_alloc(size, &actual_size, alignment);
        if(space == NULL) {
            printf("ERR size=%u  actual_size=%u\n", (unsigned int)size,
                (unsigned int)actual_size);
            continue;
        }
        printf("OK size=%u  actual_size=%u\n", (unsigned int)size,
            (unsigned int)actual_size);
        memset(space, 'a', size);
        space[size-1] = '\0';
        //printf("SET size=%u  actual_size=%u\n", (unsigned int)size, (unsigned int)actual_size);
        //printf("SET %s\n", space);
        system_release(space, size);
    }

}

int main()
{
    //int i;
    test_system_alloc();
    //scanf("%d\n", &i);
    exit_system_allocator();
    return 0;
}
