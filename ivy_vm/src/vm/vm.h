
#ifndef VM_H
#define VM_H

#include "common.h"
#include "node.h"

#define DEBUG_MODE

#define THREAD_PRDX_SIZE (1ul << 16)

typedef struct ThreadMem {
    u32 tid;
    pthread_t thread;

    u64 aux_curr;
    u64 aux_last;
    u64 aux_free[256];

    u64 var_curr; 
    u64 var_last;
    u64 var_free;

    APair* redx_base;
    u32    redx_put;

    // Priority redex bag.
    // If reductions that decrease the size of the net are not executed first,
    // there's a good chance the net will blow up in size. 
    // As an example, consider the turnstile in the case the CON-DUP redex is always
    // reduced first.
    Pair prdx[THREAD_PRDX_SIZE];
    u32  prdx_put;
} ThreadMem;

#define VM_MAX_AUX_POW2 30
#define VM_MAX_VAR_POW2 30
#define VM_MAX_REDX_POW2 30
#define VM_MAX_AUX   (1ul << VM_MAX_AUX_POW2)
#define VM_MAX_VAR   (1ul << VM_MAX_VAR_POW2)
#define VM_MAX_REDX   (1ul << VM_MAX_REDX_POW2)

#define N_THREADS_POW2 3
#define N_THREADS (1 << N_THREADS_POW2)

#define AUX_BLOCK_SIZE_POW2 (VM_MAX_AUX_POW2 - N_THREADS_POW2)
#define AUX_BLOCK_SIZE (1ul << AUX_BLOCK_SIZE_POW2) 

#define VAR_BLOCK_SIZE_POW2 (VM_MAX_VAR_POW2 - N_THREADS_POW2)
#define VAR_BLOCK_SIZE (1ul << VAR_BLOCK_SIZE_POW2)

#define REDX_BLOCK_SIZE_POW2 (VM_MAX_REDX_POW2 - N_THREADS_POW2)
#define REDX_BLOCK_SIZE (1ul << REDX_BLOCK_SIZE_POW2)

typedef struct NetVM {
    Node  aux_buf[VM_MAX_AUX];
    ANode var_buf[VM_MAX_VAR];
    APair redx_buf[VM_MAX_REDX];

    ThreadMem threads[N_THREADS];

    a64 interactions;
} NetVM;

void vm_init(NetVM* vm);
void vm_run(NetVM* vm);

void thread_run(NetVM* vm, ThreadMem* mem);

Aux alloc_aux(NetVM* vm, ThreadMem* mem, u64 size);
Node* get_aux(NetVM* vm, Aux aux);
void free_aux(NetVM* vm, ThreadMem* mem, Aux aux);

u64 alloc_var(NetVM* vm, ThreadMem* mem);

void push_redx(NetVM* vm, ThreadMem* mem, Node n0, Node n1);

#endif
