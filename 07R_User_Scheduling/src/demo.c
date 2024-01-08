#include "demo.h"
typedef struct Result {
    int result;
    int used_slices;
} Result_t;

typedef struct Arguments {
    Semaphore_t* sem1;
    Semaphore_t* sem2;
    Semaphore_t* sem3;
} Arguments_t;

static void* thread_h(void* arg) {
    Result_t* r       = calloc(1, sizeof(Result_t));
    Arguments_t* args = arg;

    sem_wait(args->sem1);
    char* h1 = "Hamilton";
    for (int i = 0; h1[i] != '\0'; i++) {
        threads_yield(0);
        printf("\033[0;33m%c\033[0m", h1[i]);
    }
    printf("\n");
    sem_post(args->sem2);

    sem_wait(args->sem1);
    char* h2 = "Burr";
    for (int i = 0; h2[i] != '\0'; i++) {
        threads_yield(0);
        printf("\033[0;33m%c\033[0m", h2[i]);
    }
    printf("\n");
    sem_post(args->sem2);

    r->result      = 1;
    r->used_slices = get_running_thread()->used_slices;
    return r;
}

static void* thread_l(void* arg) {
    Result_t* r       = calloc(1, sizeof(Result_t));
    Arguments_t* args = arg;

    sem_wait(args->sem2);
    char* l1 = "Timon";
    for (int i = 0; l1[i] != '\0'; i++) {
        threads_yield(0);
        printf("\033[0;32m%c\033[0m", l1[i]);
    }
    printf("\n");
    sem_post(args->sem3);

    sem_wait(args->sem2);
    char* l2 = "Pumba";
    for (int i = 0; l2[i] != '\0'; i++) {
        threads_yield(0);
        printf("\033[0;32m%c\033[0m", l2[i]);
    }
    printf("\n");
    sem_post(args->sem3);

    r->result      = 2;
    r->used_slices = get_running_thread()->used_slices;
    return r;
}

static void* thread_w(void* arg) {
    Result_t* r       = calloc(1, sizeof(Result_t));
    Arguments_t* args = arg;

    sem_wait(args->sem3);
    char* w1 = "Tony";
    for (int i = 0; w1[i] != '\0'; i++) {
        threads_yield(0);
        printf("\033[0;35m%c\033[0m", w1[i]);
    }
    printf("\n");
    sem_post(args->sem1);

    sem_wait(args->sem3);
    char* w2 = "Maria";
    for (int i = 0; w2[i] != '\0'; i++) {
        threads_yield(0);
        printf("\033[0;35m%c\033[0m", w2[i]);
    }
    printf("\n");
    sem_post(args->sem1);

    r->result      = 3;
    r->used_slices = get_running_thread()->used_slices;
    return r;
}

int demo(void) {
    TCB *threads[3];

    Arguments_t* args = calloc(1, sizeof(Arguments_t));

    args->sem1 = semaphore_create(1);
    args->sem2 = semaphore_create(0);
    args->sem3 = semaphore_create(0);

    threads[0] = threads_create(thread_h, args);
    threads[1] = threads_create(thread_l, args);
    threads[2] = threads_create(thread_w, args);

    for (int i = 0; i < 3; ++i) {
        if (threads[i] == NULL) {
            perror("Creating thread failed");
            semaphore_destroy(args->sem1);
            semaphore_destroy(args->sem2);
            semaphore_destroy(args->sem3);
            free(args);
            return 1;
        }
    }

    char* names[3] = {"hamilton", "lion king", "westside story"};

    for (int i = 0; i < 3; ++i) {
        TCB *id = threads[i];

        while (1) {
            void* res;
            if (threads_join(id, &res) > 0) {
                Result_t* r = (Result_t*)res;
                printf(
                    "\033[0;34m[thread \"%s\" joined, used %d slices]\033[0m\n",
                    names[i], r->used_slices);
                free(r);
                break;
            }
        }
    }

    semaphore_destroy(args->sem1);
    semaphore_destroy(args->sem2);
    semaphore_destroy(args->sem3);
    free(args);

    return 0;
}
