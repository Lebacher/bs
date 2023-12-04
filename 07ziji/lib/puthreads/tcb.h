/*
 * tcb.h
 * Defines the TCB (thread control block) data structure and functions
 * to work with it.
 定义TCB（线程控制块）数据结构和与其配合使用的函数。
 */

#ifndef TCB_H
#define TCB_H

#include <stdbool.h>
#include <stdint.h>
#include <ucontext.h>

typedef struct {
    int id;
    ucontext_t context;
    bool has_dynamic_stack;
    int feedback_depth;
    int used_slices;
    void* (*start_routine)(void*);
    void* argument;
    void* return_value;
} TCB;

/* Create a new zeroed TCB on the heap. Returns a pointer to the new
   block or NULL on error. */
   /* 在堆上创建一个新的归零 TCB。 返回指向新的指针
    错误时块或 NULL。 */
TCB* tcb_new(void);

/* Destroy block, freeing all associated memory with it */
/* 销毁块，释放与其相关的所有内存 */
void tcb_destroy(TCB* block);

#endif
