
#ifndef _SCHEDULER_SEMAPHORE
#define _SCHEDULER_SEMAPHORE
#include <stdatomic.h>
#include <stdlib.h>

#include "queue.h"

/**
 * Semaphore struct manages both the value of the semaphore and the queue of
 * TCBs waiting for it to unlock.
 Semaphore 结构体管理信号量的值和等待信号量解锁的 TCB 队列。
 * 
 * This implementation uses the popular trick [citation needed] to encode the
 * number of waiting threads as negative values.
 此实现使用流行的技巧[需要引用]将等待线程的数量编码为负值。
 *
 * Furthermore, everytime sem_post is called while other threads are waiting on
 * it the first thread in the queue is started again. This means that
 * initializing a semaphore using semaphore_create with negative initial_value
 * will crash.
 此外，每次在其他线程等待时调用 sem_post 时，队列中的第一个线程都会再次启动。 
 这意味着使用带有负initial_value 的semaphore_create 初始化信号量将会崩溃。
 */
typedef struct Semaphore {
    _Atomic(int) value; // Atomic may not be necessary as block_sigprof should
                        // ensure synchronous access.
    QUEUE* queue;
} Semaphore_t;

/**
 * Initializes and returns a new semaphore.
 初始化并返回一个新的信号量。
 */
Semaphore_t* semaphore_create(int initial_value);

/**
 * Destroys a semaphore and frees all memory managed by the semaphore.
 销毁信号量并释放信号量管理的所有内存。
 *
 * Does not free memory associated with threads in the semaphores queue, which
 * could or could not be referenced from other locations.
 不释放与信号量队列中的线程关联的内存，该内存可以或不能从其他位置引用。
 */
void semaphore_destroy(Semaphore_t* sem);

/**
 * Blocks if the semaphores value is less or equal to 0.
 如果信号量值小于或等于 0，则阻塞。
 *
 * Internally adds calling thread to internal queue and removes it from
 * threading libraries feedback queue if blocking.
 在内部将调用线程添加到内部队列，并在阻塞时将其从线程库反馈队列中删除。
 */
void sem_wait(Semaphore_t* sem);

/**
 * Increments the semaphores value by one.
 将信号量值加一。
 *
 * This is internally also responsible for continuing on waiting threads.
 这在内部还负责继续等待线程。
 */
void sem_post(Semaphore_t* sem);
#endif