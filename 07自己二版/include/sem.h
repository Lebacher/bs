
#ifndef _SCHEDULER_SEMAPHORE
#define _SCHEDULER_SEMAPHORE
#include <stdatomic.h>
#include <stdlib.h>

#include "queue.h"

/**
 * Semaphore struct manages both the value of the semaphore and the queue of
 * TCBs waiting for it to unlock.
 *
 * This implementation uses the popular trick [citation needed] to encode the
 * number of waiting threads as negative values.
 *
 * Furthermore, everytime sem_post is called while other threads are waiting on
 * it the first thread in the queue is started again. This means that
 * initializing a semaphore using semaphore_create with negative initial_value
 * will crash.
 */
typedef struct Semaphore {
    _Atomic(int) value; // Atomic may not be necessary as block_sigprof should
                        // ensure synchronous access.
    QUEUE* queue;
} Semaphore_t;

/**
 * Initializes and returns a new semaphore.
 */
Semaphore_t* semaphore_create(int initial_value);

/**
 * Destroys a semaphore and frees all memory managed by the semaphore.
 *
 * Does not free memory associated with threads in the semaphores queue, which
 * could or could not be referenced from other locations.
 */
void semaphore_destroy(Semaphore_t* sem);

/**
 * Blocks if the semaphores value is less or equal to 0.
 *
 * Internally adds calling thread to internal queue and removes it from
 * threading libraries feedback queue if blocking.
 */
void sem_wait(Semaphore_t* sem);

/**
 * Increments the semaphores value by one.
 *
 * This is internally also responsible for continuing on waiting threads.
 */
void sem_post(Semaphore_t* sem);
#endif