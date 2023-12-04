/*
 * queue.h
 * Defines a queue to mange TCB elements.
 定义一个队列来管理 TCB 元素。
 */

#ifndef QUEUE_H
#define QUEUE_H

#include "tcb.h"

#include <unistd.h>

struct queue {
    struct node* head;
    struct queue* next;
    struct queue* previous;
    size_t size;
};
typedef struct queue QUEUE;

/* Create a new initialized QUEUE on the heap. Returns a pointer to
   the new block or NULL on error. */
   /* 在堆上创建一个新的初始化队列。 返回一个指向
    新块或错误时为 NULL。 */
QUEUE* queue_new(void);

/* Destroy queue, freeing all associated memory with it. It also frees
   all memory of the elements inside the queue. */
   /* 销毁队列，释放与其相关的所有内存。 它还释放了
    队列内元素的所有内存。 */
void queue_destroy(QUEUE* queue);

/* Return the number of items in queue. */
/* 返回队列中的项目数。 */
size_t queue_size(const QUEUE* queue);

/* Add elem to the end of queue. Returns 0 on succes and non-zero on
   failure.*/
   /* 将元素添加到队列末尾。 成功时返回 0，失败时返回非零。*/
int queue_enqueue(QUEUE* queue, TCB* elem);

/* Remove the first item from the queue and return it. The caller will
   have to free the reuturned element. Returns NULL if the queue is
   empty. */
   /* 从队列中删除第一项并返回它。 呼叫者将
    必须释放返回的元素。 如果队列为空，则返回 NULL
    空的。 */
TCB* queue_dequeue(QUEUE* queue);

/* Remove element with matching id from the queue. Returns the removed
   element or NULL if no such element exists. */
   /* 从队列中删除具有匹配 id 的元素。 返回删除的
    如果不存在这样的元素，则返回 NULL。 */
TCB* queue_remove_id(QUEUE* queue, int id);

#endif
