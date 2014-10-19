#ifndef __SPLOCK_H__
#define __SPLOCK_H__


#include "config.h"

#ifdef GCC_ATOMIC
#include <stdint.h>
#include <sched.h>

typedef uint8_t splock_t;

#define splock_init(lock) ({                                                   \
    *(lock) = 0;                                                               \
})

#define splock_lock(lock) do{                                                  \
    while(!__sync_bool_compare_and_swap(lock, 0, 1)){                          \
        sched_yield();                                                         \
    }                                                                          \
}while(0)

//与phread_mutex_trylock一致，加锁成功返回0
#define splock_try_lock(lock) ({                                               \
    (__sync_bool_compare_and_swap(lock, 0, 1) ? 0 : 1);                        \
})

#define splock_unlock(lock) do{                                                \
    *(lock) = 0;                                                               \
}while(0)

#else

#include <pthread.h> 

typedef pthread_mutex_t splock_t;
#define splock_init(lock) pthread_mutex_init((lock), NULL)
#define splock_lock(lock) pthread_mutex_lock(lock)
#define splock_try_lock(lock) pthread_mutex_trylock(lock)
#define splock_unlock(lock) pthread_mutex_unlock(lock)

#endif  /* GCC_ATOMIC */

#endif  /* __SPLOCK_H__ */