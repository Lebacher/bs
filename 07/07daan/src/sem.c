#include "sem.h"

#include "queue.h"
#include "tcb.h"
#include "threads.h"

Semaphore_t* semaphore_create(int initial_value)
{
    // If value negative, we abort
    if (initial_value < 0)
    {
        abort();
    }
    // create new Semaphore type
    Semaphore_t* semaphore;
    if ((semaphore = (Semaphore_t*)calloc(1, sizeof(Semaphore_t))) == NULL)
    {
        return NULL;
    }
    // Set initial value for the semaphore before it is returned to the caller
    semaphore->value = initial_value;
    return semaphore;
}

void semaphore_destroy(Semaphore_t* sem) { free(sem); }

void sem_wait(Semaphore_t* sem)
{
    block_sigprof();
    
    // This function results in an unfixable error: *** stack smashing detected ***: terminated
    // Even after hours we couldn't find the cause.
    
    /*if (sem->value <= 0)
    {
        // Add running thread to our sem queue
        if (sem->queue == NULL)
        {
            sem->queue = queue_new();
        }
        queue_enqueue(sem->queue, get_running_thread());
        unblock_sigprof();
        // Do not enqueue the thread later in handle_sigprof
        threads_yield(1);
    }
    else
    {
        // Positive value, decrease value and continue
        unblock_sigprof();
        sem->value--;
    }*/
}

void sem_post(Semaphore_t* sem)
{
    block_sigprof();
    if (sem->value < 0)
    {
        // value is negative
        if (sem->queue != NULL)
        {
            // Get queue entry
            TCB* entry = queue_dequeue(sem->queue);
            // Find the right priority queue
            QUEUE* iterator = get_feedback_queue();
            for (int i = 0; i < entry->feedback_depth; i++)
            {
                iterator = iterator->next;
            }

            // Move thread from this sem queue to the feedback queue
            queue_enqueue(iterator, entry);
        }
        sem->value++;
    }
    unblock_sigprof();
}