/*
 * threads.h
 * Interface to a barebones user level thread library.
 */

#ifndef THREADS_H
#define THREADS_H

#include "queue.h"
#include "tcb.h"

#define FEEDBACK_DEPTH 4
#define SLICE_LENGTH_SECONDS 0
#define SLICE_LENGTH_MICROSECONDS 100
#define ANTI_AGING_CYCLES 10

/* Yields the currently running thread and hands the rest of the
   time slice to the next thread in line. */
void threads_yield(int dont_reschedule);

/* Create a new thread. func is the function that will be run once the
   thread starts execution and arg is the argument for that
   function. On success, returns an id equal or greater to 0. On
   failure, errno is set and -1 is returned. */
int threads_create(void* (*start_routine)(void*), void* arg);

/* Stop execution of the thread calling this function. */
void threads_exit(void* result);

/* Wait for the thread with matching id to finish execution, that is,
   for it to call threads_exit. On success, the threads result is
   written into result and id is returned. If no completed thread with
   matching id exists, 0 is returned. On error, -1 is returned and
   errno is set. */
int threads_join(int id, void** result);

/**
 * Public access to internal synchronisation to allow Semaphores to work
 */
void block_sigprof(void);
void unblock_sigprof(void);

/**
 * Returns the currently running thread.
 *
 * Only call this when sigprof is blocked.
 */
TCB* get_running_thread(void);

/**
 * Returns the entire feedback queue (without running thread).
 *
 * Only call this when sigprof is blocked.
 */
QUEUE* get_feedback_queue(void);

#endif
