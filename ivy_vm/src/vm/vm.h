
#ifndef VM_H
#define VM_H

#include "common.h"
#include "node.h"

typedef struct {
    u32 tid;
    pthread_t thread;

    APair* pair_base;
    u32    pair_curr;
    u32    pair_cap;

    ANode* vars_base; 
    u32    vars_curr;
    u32    vars_cap;

    APair* redx_base;
    u32    redx_put;
} ThreadMem;

#define VM_MAX_PAIR_POW2 30
#define VM_MAX_VARS_POW2 30
#define VM_MAX_REDX_POW2 30
#define VM_MAX_PAIR   (1 << VM_MAX_PAIR_POW2)
#define VM_MAX_VARS   (1 << VM_MAX_VARS_POW2)
#define VM_MAX_REDX   (1 << VM_MAX_REDX_POW2)

#define N_THREADS_POW2 3
#define N_THREADS (1 << N_THREADS_POW2)

#define PAIR_BLOCK_SIZE_POW2 (VM_MAX_PAIR_POW2 - N_THREADS_POW2)
#define PAIR_BLOCK_SIZE (1 << PAIR_BLOCK_SIZE_POW2) 

#define VARS_BLOCK_SIZE_POW2 (VM_MAX_VARS_POW2 - N_THREADS_POW2)
#define VARS_BLOCK_SIZE (1 << VARS_BLOCK_SIZE_POW2)

#define REDX_BLOCK_SIZE_POW2 (VM_MAX_REDX_POW2 - N_THREADS_POW2)
#define REDX_BLOCK_SIZE (1 << REDX_BLOCK_SIZE_POW2)

typedef struct {
    APair pair_buf[VM_MAX_PAIR];
    ANode vars_buf[VM_MAX_VARS];
    APair redx_buf[VM_MAX_REDX];

    ThreadMem threads[N_THREADS];

    a64 interactions;
} NetVM;

void vm_init(NetVM* vm);
void vm_run(NetVM* vm);

void thread_run(NetVM* vm, ThreadMem* mem);

#endif
