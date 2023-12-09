/*
 * threads.h
 * Interface to a barebones user level thread library.
 准系统用户级线程库的接口。
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
   /* 产生当前正在运行的线程，并将剩余的时间片交给队列中的下一个线程。 */
void threads_yield(int dont_reschedule);

/* Create a new thread. func is the function that will be run once the
   thread starts execution and arg is the argument for that
   function. On success, returns an id equal or greater to 0. On
   failure, errno is set and -1 is returned. */
   /* 创建一个新线程。 func 是线程开始执行后将运行的函数，
   arg 是该函数的参数。 成功时，返回等于或大于 0 的 id。
   失败时，设置 errno 并返回 -1。 */
int threads_create(void* (*start_routine)(void*), void* arg);

/* Stop execution of the thread calling this function. */
/* 停止执行调用此函数的线程。 */
void threads_exit(void* result);

/* Wait for the thread with matching id to finish execution, that is,
   for it to call threads_exit. On success, the threads result is
   written into result and id is returned. If no completed thread with
   matching id exists, 0 is returned. On error, -1 is returned and
   errno is set. */
   /* 等待id匹配的线程完成执行，即调用threads_exit。 
   成功后，线程结果将写入 result 并返回 id。 
   如果不存在具有匹配 id 的已完成线程，则返回 0。 
   出错时，返回 -1 并设置 errno。 */
int threads_join(int id, void** result);

/**
 * Public access to internal synchronisation to allow Semaphores to work
 公共访问内部同步以允许信号量工作
 */
void block_sigprof(void);
void unblock_sigprof(void);

/**
 * Returns the currently running thread.
 返回当前正在运行的线程。
 *
 * Only call this when sigprof is blocked.
 仅当 sigprof 被阻止时才调用此函数。
 */
TCB* get_running_thread(void);

/**
 * Returns the entire feedback queue (without running thread).
 返回整个反馈队列（不运行线程）。
 *
 * Only call this when sigprof is blocked.
 仅当 sigprof 被阻止时才调用此函数。
 */
QUEUE* get_feedback_queue(void);

#endif
