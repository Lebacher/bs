#include "tcb.h"

#include <stdlib.h>

TCB* tcb_new(void) {
    static int next_id = 1;

    TCB* next;

    if ((next = (TCB*)calloc(1, sizeof(TCB))) == NULL) {
        return NULL;
    }

    next->id             = next_id++;
    next->feedback_depth = 0;
    next->used_slices    = 0;
    return next;
}

void tcb_destroy(TCB* block) {
    if (block->has_dynamic_stack) {
        free(block->context.uc_stack.ss_sp);
    }

    free(block);
}
