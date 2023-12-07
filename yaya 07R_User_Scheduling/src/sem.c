#include "sem.h"

#include "queue.h"
#include "tcb.h"
#include "threads.h"

Semaphore_t* semaphore_create(int initial_value) {
    
    if (initial_value < 0) {
        return NULL;
    }

    Semaphore_t* semaphore_new;
    if ((semaphore_new = (Semaphore_t*)calloc(1, sizeof(Semaphore_t))) == NULL)
    {
        return NULL;
    }

    semaphore_new->value = initial_value;
    return semaphore_new;
}

void semaphore_destroy(Semaphore_t* sem) {
    free(sem);

}

void sem_wait(Semaphore_t* sem) {
    block_sigprof();
    TCB* running_now = get_running_thread();
    if (sem->value <= 0) {
        int result_enqueue = queue_enqueue(sem->queue, running_now);
        unblock_sigprof();
        threads_yield(1);
    }
    else{
        unblock_sigprof();
        sem->value--;
    }
    

}

void sem_post(Semaphore_t* sem) {
    block_sigprof();
    if (sem->value < 0){
        if (sem->queue != NULL && queue_size(sem->queue) > 0) {
            TCB* running_next = queue_dequeue(sem->queue);
            QUEUE* iterator = get_feedback_queue();
            for (int i = 0; i < running_next->feedback_depth; ++i) {
                iterator = iterator->next;
            }
    
            sem->value++;
        }
    }
    unblock_sigprof();
}
