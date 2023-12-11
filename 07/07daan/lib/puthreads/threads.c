#include "threads.h"
#include "queue.h"
#include "tcb.h"

#include <errno.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <ucontext.h>

static QUEUE* completed;
static TCB* running;
static QUEUE* feedback;
// static unsigned int cycles = 0;

bool rescheudle = true;

static bool init_queues(void);
static bool init_first_context(void);
static bool init_profiling_timer(void);

static void handle_sigprof(int, siginfo_t*, void*);
static void handle_thread_start();

static bool malloc_stack(TCB*);

TCB* get_running_thread() { return running; }
QUEUE* get_feedback_queue() { return feedback; }

int threads_create(void* (*start_routine)(void*), void* arg)
{
    block_sigprof();

    // Init if necessary

    static bool initialized;

    if (!initialized)
    {
        if (!init_queues())
        {
            abort();
        }

        if (!init_first_context())
        {
            abort();
        }

        if (!init_profiling_timer())
        {
            abort();
        }

        initialized = true;
    }

    // Create a thread control block for the newly created thread.

    TCB* newTCB;

    if ((newTCB = tcb_new()) == NULL)
    {
        return -1;
    }

    if (getcontext(&newTCB->context) == -1)
    {
        tcb_destroy(newTCB);
        return -1;
    }

    if (!malloc_stack(newTCB))
    {
        tcb_destroy(newTCB);
        return -1;
    }

    makecontext(&newTCB->context, handle_thread_start, 1, newTCB->id);

    newTCB->start_routine = start_routine;
    newTCB->argument      = arg;

    // Enqueue the newly created stack on the top feedback *level*

    if (queue_enqueue(feedback, newTCB) != 0)
    {
        tcb_destroy(newTCB);
        return -1;
    }

    unblock_sigprof();
    return newTCB->id;
}

void threads_exit(void* result)
{
    if (running == NULL)
    {
        exit(EXIT_SUCCESS);
    }
    block_sigprof();
    running->return_value = result;

    if (queue_enqueue(completed, running) != 0)
    {
        abort();
    }

    // Find highest available queue (threads available) to run next thread
    QUEUE** iterator = &feedback;
    do
    {
        if ((*iterator)->head != NULL)
        {
            break;
        }
        iterator = &((*iterator)->next);
    } while ((*iterator) != NULL);
    running = NULL;
    // Run the thread
    if ((*iterator)->head == NULL ||
        ((running = queue_dequeue(*iterator)) == NULL))
    {
        abort();
    }

    // running one to mark it completed

    if (running == NULL)
    {
        exit(EXIT_SUCCESS);
    }

    setcontext(&running->context); // also unblocks SIGPROF
}

int threads_join(int id, void** result)
{
    if (id < 0)
    {
        errno = EINVAL;
        return -1;
    }

    block_sigprof();
    TCB* block = queue_remove_id(completed, id);
    unblock_sigprof();

    if (block == NULL)
    {
        return 0;
    }
    else
    {
        *result = block->return_value;
        tcb_destroy(block);
        return id;
    }
}

void threads_yield(int dont_reschedule)
{
    if (dont_reschedule != 0)
    {
        // Set variable so that handle_sigprof does not rescheudle
        rescheudle = false;
    }
    raise(SIGPROF);
}

static bool init_queues(void)
{
    if ((feedback = queue_new()) == NULL)
    {
        return false;
    }

    QUEUE** depth = &feedback->next;

    for (int i = 1; i < FEEDBACK_DEPTH; i++)
    {
        *depth = queue_new();

        if (*depth == NULL)
        {
            return false;
        }

        depth = &((*depth)->next);
    }

    if ((completed = queue_new()) == NULL)
    {
        queue_destroy(feedback);
        return false;
    }

    return true;
}

static bool init_first_context(void)
{
    TCB* block;

    if ((block = tcb_new()) == NULL)
    {
        return false;
    }

    if (getcontext(&block->context) == -1)
    {
        tcb_destroy(block);
        return false;
    }

    running = block;
    return true;
}

static bool init_profiling_timer(void)
{
    // Install signal handler

    sigset_t all;
    sigfillset(&all);

    const struct sigaction alarm = {.sa_sigaction = handle_sigprof,
                                    .sa_mask      = all,
                                    .sa_flags     = SA_SIGINFO | SA_RESTART};

    struct sigaction old;

    if (sigaction(SIGPROF, &alarm, &old) == -1)
    {
        perror("sigaction");
        abort();
    }

    const struct itimerval timer = {
        // Defines interrupt time (== timeslice length)
        {SLICE_LENGTH_SECONDS, SLICE_LENGTH_MICROSECONDS},
        // Arm the timer as soon as possible
        {0, 1}};

    // Enable timer

    if (setitimer(ITIMER_PROF, &timer, NULL) == -1)
    {
        if (sigaction(SIGPROF, &old, NULL) == -1)
        {
            perror("sigaction");
            abort();
        }

        return false;
    }

    return true;
}

static void handle_sigprof(int signum, siginfo_t* nfo, void* context)
{
    int old_errno = errno;

    QUEUE** iterator       = &feedback;
    size_t counter_threads = 0;

    // iterate through all elements (priority queues) of the feedback queue and
    // summarize their number of elements
    while (*iterator != NULL)
    {

        counter_threads += queue_size(*iterator);

        iterator = &((*iterator)->next);
    }

    // if no thread is ready for execution, exit the program
    if (counter_threads == 0 && running == NULL)
    {
        _exit(EXIT_SUCCESS);
    }

    // Backup the current context

    ucontext_t* stored  = &running->context;
    ucontext_t* updated = (ucontext_t*)context;

    stored->uc_flags    = updated->uc_flags;
    stored->uc_link     = updated->uc_link;
    stored->uc_mcontext = updated->uc_mcontext;
    stored->uc_sigmask  = updated->uc_sigmask;

    // Find queue with lower priority or lowewst priority
    iterator          = &feedback;
    int current_queue = 0;
    while ((*iterator)->next != NULL)
    {
        iterator = &((*iterator)->next);
        current_queue++;
        if (current_queue == running->feedback_depth + 1)
        {
            break;
        }
    }

    // Do not requeue if semaphore value was set
    if (rescheudle)
    {

        // Find queue to requeue to. Either lower or lowest (if already low)
        if (running->feedback_depth != FEEDBACK_DEPTH - 1)
        {
            running->feedback_depth--;
        }

        // Round robin for the lowest priority queue
        // Is added to itself, iterator is the lowest priority queue
        if (queue_enqueue(*iterator, running) != 0)
        {
            abort();
        }
    }
    else
    {
        // Reset rescheudle for the future
        rescheudle = true;
    }

    running = NULL;

    // Loop to find highest priority queue available (that has elements)
    iterator = &feedback;
    do
    {
        if ((*iterator)->head != NULL)
        {
            break;
        }
        iterator = &((*iterator)->next);
    } while ((*iterator) != NULL);

    // Run next thread from this queue
    if ((*iterator)->head == NULL ||
        (running = queue_dequeue(*iterator)) == NULL)
    {
        abort();
    }

    if (running == NULL)
    {
        fprintf(stderr, "Threads: All threads are waiting or dead, Abort");
        abort();
    }

    running->used_slices++;

    // TODO: Cycle-based Anti-Aging

    // Manually leave the signal handler
    errno = old_errno;
    if (setcontext(&running->context) == -1)
    {
        abort();
    }
}

static void handle_thread_start(void)
{
    block_sigprof();
    TCB* that = running;
    unblock_sigprof();

    that->used_slices = 1;
    void* result      = that->start_routine(that->argument);
    threads_exit(result);
}

static bool malloc_stack(TCB* thread)
{
    // Get the stack size

    struct rlimit limit;

    if (getrlimit(RLIMIT_STACK, &limit) == -1)
    {
        return false;
    }

    // Allocate memory

    void* stack;

    if ((stack = malloc(limit.rlim_cur)) == NULL)
    {
        return false;
    }

    // Update the thread control bock

    thread->context.uc_stack.ss_flags = 0;
    thread->context.uc_stack.ss_size  = limit.rlim_cur;
    thread->context.uc_stack.ss_sp    = stack;
    thread->has_dynamic_stack         = true;

    return true;
}

void block_sigprof(void)
{
    sigset_t sigprof;
    sigemptyset(&sigprof);
    sigaddset(&sigprof, SIGPROF);
    unsigned long int tmp = sigprof.__val[16];

    if (sigprocmask(SIG_BLOCK, &sigprof, NULL) == -1)
    {
        perror("sigprocmask");
        abort();
    }
    tmp++;
}

void unblock_sigprof(void)
{
    sigset_t sigprof;
    sigemptyset(&sigprof);
    sigaddset(&sigprof, SIGPROF);
    unsigned long int tmp = sigprof.__val[16];

    if (sigprocmask(SIG_UNBLOCK, &sigprof, NULL) == -1)
    {
        perror("sigprocmask");
        abort();
    }
    tmp++;
}