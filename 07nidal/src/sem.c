#include "sem.h"

#include "queue.h"
#include "tcb.h"
#include "threads.h"

Semaphore_t *semaphore_create(int initial_value)
{
    // WRITE YOUR CODE HERE ..done
    Semaphore_t *sem = malloc(sizeof(Semaphore_t));
    sem->value = initial_value;
    sem->queue = queue_new();
    if (sem->queue != NULL)
    {
        return sem;
    }
    free(sem);
    return NULL;
}

void semaphore_destroy(Semaphore_t *sem)
{
    // WRITE YOUR CODE HERE ..done
    queue_destroy(sem->queue);
    free(sem);
}

void sem_wait(Semaphore_t *sem)
{
    // WRITE YOUR CODE HERE ..done
    block_sigprof();
    if (sem->value <= 0)
    {
        if (queue_enqueue(sem->queue, get_running_thread()) != 0)
        {
            exit(1);
        }
        threads_yield(1);
    }
    sem->value = sem->value - 1;
    unblock_sigprof();
}

void sem_post(Semaphore_t *sem)
{
    // WRITE YOUR CODE HERE ..done
    int counter = 0;
    block_sigprof();
    if (sem->value < 0)
    {
        TCB *reschedule;
        QUEUE *move = get_feedback_queue();
        if ((reschedule = queue_dequeue(sem->queue)) != NULL)
        {
            while (counter < FEEDBACK_DEPTH)
            {
                if (counter == reschedule->feedback_depth)
                {
                    if (queue_enqueue(move, reschedule) != 0)
                    {
                        exit(1);
                    }
                }
                move = move->next;
                counter++;
            }
        }
        else 
        {
            exit(1);
        }
    }
    sem->value = sem->value + 1;
    unblock_sigprof();
}