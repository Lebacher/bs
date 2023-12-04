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
static unsigned int cycles = 0;

static bool init_queues(void);
static bool init_first_context(void);
static bool init_profiling_timer(void);

static void handle_sigprof(int, siginfo_t*, void*);
static void handle_thread_start();

static bool malloc_stack(TCB*);

TCB* get_running_thread() { return running; }
QUEUE* get_feedback_queue() { return feedback; }

int threads_create(void* (*start_routine)(void*), void* arg) {
    block_sigprof();

    // Init if necessary
	// 如果需要则初始化

    static bool initialized;

    if (!initialized) {
        if (!init_queues()) {
            abort();
        }

        if (!init_first_context()) {
            abort();
        }

        if (!init_profiling_timer()) {
            abort();
        }

        initialized = true;
    }

    // Create a thread control block for the newly created thread.
	// 为新创建的线程创建线程控制块。

    TCB* newTCB;

    if ((newTCB = tcb_new()) == NULL) {
        return -1;
    }

    if (getcontext(&newTCB->context) == -1) {
        tcb_destroy(newTCB);
        return -1;
    }

    if (!malloc_stack(newTCB)) {
        tcb_destroy(newTCB);
        return -1;
    }

    makecontext(&newTCB->context, handle_thread_start, 1, newTCB->id);

    newTCB->start_routine = start_routine;
    newTCB->argument      = arg;

    // Enqueue the newly created stack on the top feedback *level*
	// 将新创建的堆栈放入顶部反馈 *level* 的队列中

    if (queue_enqueue(feedback, newTCB) != 0) {
        tcb_destroy(newTCB);
        return -1;
    }

    unblock_sigprof();
    return newTCB->id;
}

void threads_exit(void* result) {
    if (running == NULL) {
        exit(EXIT_SUCCESS);
    }

    block_sigprof();
    running->return_value = result;

    if (queue_enqueue(completed, running) != 0) {
        abort();
    }

    // TODO: Pick the next process from feedback without enqueing the last
    // running one to mark it completed
	// TODO：从反馈中选择下一个进程，而不排队最后一个运行的进程以将其标记为已完成
    //---------------------------------------------------------------------------------------------
    QUEUE* cursor = get_feedback_queue();
    for (int i = 0; i < FEEDBACK_DEPTH; ++i) {
        if (cursor->head != NULL)
        {
            break;
        }
        cursor = cursor->next;
    }

    if (cursor != NULL) {
        /*if (queue_enqueue(completed, cursor->head->thread) != 0) {
            abort();
        }*/

        if ((running = queue_dequeue(cursor)) == NULL)  {
            abort();
        }
    }
    //---------------------------------------------------------------------------------------------

    if (running == NULL) {
        exit(EXIT_SUCCESS);
    }

    setcontext(&running->context); // also unblocks SIGPROF 还可以解锁 SIGPROF
}

int threads_join(int id, void** result) {
    if (id < 0) {
        errno = EINVAL;
        return -1;
    }

    block_sigprof();
    TCB* block = queue_remove_id(completed, id);
    unblock_sigprof();

    if (block == NULL) {
        return 0;
    } else {
        *result = block->return_value;
        tcb_destroy(block);
        return id;
    }
}

void threads_yield(int dont_reschedule) {
    if (dont_reschedule != 0) {
        // TODO: Change scheduler state such that in the next call of
        // handle_sigprof this TCB won't be re-enqueued (used for implemented
        // Semaphores)
		// TODO：更改调度程序状态，以便在下一次调用 handle_sigprof 时，
		// 该 TCB 不会重新排队（用于实现的信号量）
        //---------------------------------------------------------------------------------------------
        TCB* temp = get_running_thread();

        if (queue_enqueue(feedback, running) != 0) {
            abort();
        }

        if ((running = queue_dequeue(feedback)) == NULL) {
            abort();
        }

        while (feedback->head->thread != temp) {
            if (queue_enqueue(feedback, queue_dequeue(feedback)) != 0) {
                abort();
            }
        }

        //---------------------------------------------------------------------------------------------
    }
    raise(SIGPROF);
}

static bool init_queues(void) {
    if ((feedback = queue_new()) == NULL) {
        return false;
    }

    QUEUE** depth = &feedback->next;

    for (int i = 1; i < FEEDBACK_DEPTH; i++) {
        *depth = queue_new();

        if (*depth == NULL) {
            return false;
        }

        depth = &((*depth)->next);
    }

    if ((completed = queue_new()) == NULL) {
        queue_destroy(feedback);
        return false;
    }

    return true;
}

static bool init_first_context(void) {
    TCB* block;

    if ((block = tcb_new()) == NULL) {
        return false;
    }

    if (getcontext(&block->context) == -1) {
        tcb_destroy(block);
        return false;
    }

    running = block;
    return true;
}

static bool init_profiling_timer(void) {
    // Install signal handler 安装信号处理程序

    sigset_t all;
    sigfillset(&all);

    const struct sigaction alarm = {.sa_sigaction = handle_sigprof,
                                    .sa_mask      = all,
                                    .sa_flags     = SA_SIGINFO | SA_RESTART};

    struct sigaction old;

    if (sigaction(SIGPROF, &alarm, &old) == -1) {
        perror("sigaction");
        abort();
    }

    const struct itimerval timer = {
        // Defines interrupt time (== timeslice length)
		// 定义中断时间（==时间片长度）
        {SLICE_LENGTH_SECONDS, SLICE_LENGTH_MICROSECONDS},
        // Arm the timer as soon as possible
		// 尽快启动定时器
        {0, 1}};

    // Enable timer
	// 启用定时器

    if (setitimer(ITIMER_PROF, &timer, NULL) == -1) {
        if (sigaction(SIGPROF, &old, NULL) == -1) {
            perror("sigaction");
            abort();
        }

        return false;
    }

    return true;
}

static void handle_sigprof(int signum, siginfo_t* nfo, void* context) {
    int old_errno = errno;

    // TODO: Check for any waiting proccess in all queues
	// TODO: 检查所有队列中是否有等待进程
	//---------------------------------------------------------------------------------------------

    QUEUE* cursor = get_feedback_queue();
    size_t counter = 0;

    for (int i = 0; i < FEEDBACK_DEPTH; ++i) {
        counter += cursor->queue_size;
        cursor = cursor->next;
    }

    if (counter == 0 && running == NULL)
    {
        _exit(0);
    }
    //---------------------------------------------------------------------------------------------

    // Backup the current context
	// 备份当前上下文

    ucontext_t* stored  = &running->context;
    ucontext_t* updated = (ucontext_t*)context;

    stored->uc_flags    = updated->uc_flags;
    stored->uc_link     = updated->uc_link;
    stored->uc_mcontext = updated->uc_mcontext;
    stored->uc_sigmask  = updated->uc_sigmask;

    // TODO: Bump last running process into next lowest queue
	// TODO: 将最后一个正在运行的进程放入下一个最低队列
    // TODO: If last process yielded because of Semaphore-wait: do not requeue!
	// TODO：如果最后一个进程由于信号等待而放弃：不要重新排队！
    // But otherwise do!
	// 但否则就这样做！
    //---------------------------------------------------------------------------------------------
    cursor = feedback;
    counter = 0;

    for (int i = 0; i < FEEDBACK_DEPTH; ++i) {
        if (&(cursor->head) == NULL) {
        }
        else{
            counter = i;
        }
        cursor = cursor->next;
    }

    cursor = feedback;
    if (counter == 0){}
    else if (counter == (FEEDBACK_DEPTH - 1)){
        for (int i = 0; i < FEEDBACK_DEPTH - 1; ++i) {
            cursor = cursor->next;
        }
    }
    else{
        for (int i = 0; i < counter + 1; ++i) {
            cursor = cursor->next;
        }
    }

    if (queue_enqueue(cursor, running) != 0) {
        abort();
    }
    //---------------------------------------------------------------------------------------------
    running = NULL;

    // TODO: Pick next process from *best* feedback queue
	// TODO：从*最佳*反馈队列中选择下一个进程
    //---------------------------------------------------------------------------------------------
    if ((running = queue_dequeue(feedback)) == NULL) {
        abort();
    }
    //---------------------------------------------------------------------------------------------

    if (running == NULL) {
        fprintf(stderr, "Threads: All threads are waiting or dead, Abort");
        abort();
    }

    running->used_slices++;

    // TODO: Cycle-based Anti-Aging
	// TODO：基于周期的抗衰老
    //---------------------------------------------------------------------------------------------
    cycles++;
    cursor = feedback->next;
    if (cycles == ANTI_AGING_CYCLES){
        for (int i = 0; i < FEEDBACK_DEPTH - 1; ++i) {
            while (*(cursor->head) != NULL){
                if (queue_enqueue(feedback, cursor->head->thread) != 0) {
                    abort();
                }

                if (queue_dequeue(cursor) == NULL) {
                    abort();
                }
            }
            cursor = cursor->next;
        }
        cycles = 0;
    }
    //---------------------------------------------------------------------------------------------
    // Manually leave the signal handler
	// 手动离开信号处理程序
    errno = old_errno;
    if (setcontext(&running->context) == -1) {
        abort();
    }
}

static void handle_thread_start(void) {
    block_sigprof();
    TCB* that = running;
    unblock_sigprof();

    that->used_slices = 1;
    void* result      = that->start_routine(that->argument);
    threads_exit(result);
}

static bool malloc_stack(TCB* thread) {
    // Get the stack size
	// 获取堆栈大小

    struct rlimit limit;

    if (getrlimit(RLIMIT_STACK, &limit) == -1) {
        return false;
    }

    // Allocate memory
	// 分配内存

    void* stack;

    if ((stack = malloc(limit.rlim_cur)) == NULL) {
        return false;
    }

    // Update the thread control bock
	// 更新线程控制块

    thread->context.uc_stack.ss_flags = 0;
    thread->context.uc_stack.ss_size  = limit.rlim_cur;
    thread->context.uc_stack.ss_sp    = stack;
    thread->has_dynamic_stack         = true;

    return true;
}

void block_sigprof(void) {
    sigset_t sigprof;
    sigemptyset(&sigprof);
    sigaddset(&sigprof, SIGPROF);
    unsigned long int tmp = sigprof.__val[16];

    if (sigprocmask(SIG_BLOCK, &sigprof, NULL) == -1) {
        perror("sigprocmask");
        abort();
    }
    tmp++;
}

void unblock_sigprof(void) {
    sigset_t sigprof;
    sigemptyset(&sigprof);
    sigaddset(&sigprof, SIGPROF);
    unsigned long int tmp = sigprof.__val[16];

    if (sigprocmask(SIG_UNBLOCK, &sigprof, NULL) == -1) {
        perror("sigprocmask");
        abort();
    }
    tmp++;
}