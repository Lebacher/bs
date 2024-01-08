#include "sem.h"

#include "queue.h"
#include "tcb.h"
#include "threads.h"

Semaphore_t* semaphore_create(int initial_value) {
    // WRITE YOUR CODE HERE
    //--------------------------------------------------------------------
    if (initial_value < 0) 
    {
        abort();
    }

    Semaphore_t* sem = (Semaphore_t*)malloc(sizeof(Semaphore_t));

    //Initialize value and queue of semaphore.
    if (sem == NULL) 
    {
        perror("Error creating semaphore");
        exit(EXIT_FAILURE);
    }

    sem->value = initial_value;
    sem->queue = queue_new();

    return sem;
    //--------------------------------------------------------------------
}

void semaphore_destroy(Semaphore_t* sem) {
    // WRITE YOUR CODE HERE
    //--------------------------------------------------------------------
    if (sem == NULL) 
    {
        return;
    }

    //Destroy the queue of semaphore.
    queue_destroy(sem->queue);

    //free the memory of semaphore.
    free(sem);
    //--------------------------------------------------------------------
}

void sem_wait(Semaphore_t* sem) {
    // WRITE YOUR CODE HERE
    //--------------------------------------------------------------------
    block_sigprof();
    if (sem->value <= 0)
    {
        if (queue_enqueue(sem->queue, get_running_thread()) != 0) 
        {
            abort();
        }
        threads_yield(1);
    }
    sem->value--;
    unblock_sigprof();
    //--------------------------------------------------------------------
}

void sem_post(Semaphore_t* sem) {
    // WRITE YOUR CODE HERE
    //--------------------------------------------------------------------
	int counter = 0;
    block_sigprof();
    if (sem->value < 0)
    {
        if (sem->queue != NULL) 
        {
            TCB* temp_thread;
            
            if ((temp_thread = queue_dequeue(sem->queue)) == NULL) 
            {
                abort();
            }
            
            QUEUE* cursor = get_feedback_queue();

            for (int i = 0; i < temp_thread->feedback_depth; i++) 
            {
                cursor = cursor->next;
            }

            //Put it into the queue.
            if (queue_enqueue(cursor, temp_thread) != 0) 
            {
                abort();
            }
        }
        sem->value++;
    }
    unblock_sigprof();
    //--------------------------------------------------------------------
}
