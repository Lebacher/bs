#include "sem.h"

#include "queue.h"
#include "tcb.h"
#include "threads.h"

Semaphore_t* semaphore_create(int initial_value) {
    // WRITE YOUR CODE HERE
    //---------------------------------------------------------------------------------------------
    if (initial_value < 0) {
        abort();
    }

    Semaphore_t* sem = (Semaphore_t*)malloc(sizeof(Semaphore_t));

    if (sem == NULL) {
        perror("Error creating semaphore");
        exit(EXIT_FAILURE);
    }

    sem->value = initial_value;
    sem->queue = queue_new();

    return sem;
    //---------------------------------------------------------------------------------------------
    //return NULL;
}

void semaphore_destroy(Semaphore_t* sem) {
    // WRITE YOUR CODE HERE
    //---------------------------------------------------------------------------------------------
    if (sem == NULL) {
        return;
    }

    queue_destroy(sem->queue);
    free(sem);
    //---------------------------------------------------------------------------------------------
}

void sem_wait(Semaphore_t* sem) {
    // WRITE YOUR CODE HERE
    //---------------------------------------------------------------------------------------------
    block_sigprof();

    while (sem->value <= 0) {
        queue_enqueue(sem->queue, get_running_thread());
        unblock_sigprof();
        threads_yield(1);
        block_sigprof();
    }

    sem->value--;
    unblock_sigprof();
    //---------------------------------------------------------------------------------------------
}

void sem_post(Semaphore_t* sem) {
    // WRITE YOUR CODE HERE
    //---------------------------------------------------------------------------------------------
    block_sigprof();

    if (sem->value < 0) {
        if (sem->queue != NULL) {
            TCB* thread_in = queue_dequeue(sem->queue);
            QUEUE* cursor = get_feedback_queue();

            for (int i = 0; i < thread_in->feedback_depth; i++) {
                cursor = cursor->next;
            }

            queue_enqueue(cursor, thread_in);
        }
        sem->value++;
    }
    unblock_sigprof();
    //---------------------------------------------------------------------------------------------
}