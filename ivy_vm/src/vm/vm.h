
#ifndef VM_H
#define VM_H

#include "common.h"
#include "node.h"
#include "operation.h"

#define DEBUG_MODE

#define THREAD_PRDX_SIZE (1ul << 16)

typedef struct ThreadMem {
    u32 tid;
    pthread_t thread;

    // The current position of the bump allocator in the VM's aux buffer
    u64 aux_curr;
    // The last node index in this thread's segment of the aux buffer
    u64 aux_last;
    // The first free aux blocks that belong to this thread.
    // aux_free[n] is the first free aux of size n
    u64 aux_free[256];

    u64 var_curr; 
    u64 var_last;
    u64 var_free;

    APair* redx_base;
    u32    redx_put;

    u64 oper_curr;
    u64 oper_last;
    u64 oper_free;

    // Priority redex bag.
    // If reductions that decrease the size of the net are not executed first,
    // there's a good chance the net will blow up in size. 
    // As an example, consider the turnstile in the case the CON-DUP redex is always
    // reduced first.
    Pair prdx[THREAD_PRDX_SIZE];
    u32  prdx_put;

    // Temporary buffers needed for instancing a definition
    u64* instance_vars;
    u64* instance_oper;
} ThreadMem;

#define VM_MAX_AUX_POW2 30
#define VM_MAX_VAR_POW2 30
#define VM_MAX_REDX_POW2 30
#define VM_MAX_OPER_POW2 30
#define VM_MAX_AUX   (1ul << VM_MAX_AUX_POW2)
#define VM_MAX_VAR   (1ul << VM_MAX_VAR_POW2)
#define VM_MAX_REDX   (1ul << VM_MAX_REDX_POW2)
#define VM_MAX_OPER   (1ul << VM_MAX_OPER_POW2)

#define N_THREADS_POW2 3
#define N_THREADS (1 << N_THREADS_POW2)

#define AUX_BLOCK_SIZE_POW2 (VM_MAX_AUX_POW2 - N_THREADS_POW2)
#define AUX_BLOCK_SIZE (1ul << AUX_BLOCK_SIZE_POW2) 

#define VAR_BLOCK_SIZE_POW2 (VM_MAX_VAR_POW2 - N_THREADS_POW2)
#define VAR_BLOCK_SIZE (1ul << VAR_BLOCK_SIZE_POW2)

#define REDX_BLOCK_SIZE_POW2 (VM_MAX_REDX_POW2 - N_THREADS_POW2)
#define REDX_BLOCK_SIZE (1ul << REDX_BLOCK_SIZE_POW2)

#define OPER_BLOCK_SIZE_POW2 (VM_MAX_OPER_POW2 - N_THREADS_POW2)
#define OPER_BLOCK_SIZE (1ul << OPER_BLOCK_SIZE_POW2)

typedef struct NetVM {
    Node  aux_buf[VM_MAX_AUX];
    ANode var_buf[VM_MAX_VAR];
    APair redx_buf[VM_MAX_REDX];
    Operation oper_buf[VM_MAX_OPER];

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

u64 alloc_oper(NetVM* vm, ThreadMem* mem, u64 op, u64 ins);
void free_oper(NetVM* vm, ThreadMem* mem, u64 oper);

#endif
