
#include "vm.h"

void run() {

    NetVM* vm = malloc(sizeof(NetVM));
    vm_init(vm);
    atomic_store_explicit(&vm->pair_buf[0].n0, NODE_VAR(0), memory_order_relaxed);
    atomic_store_explicit(&vm->pair_buf[0].n1, NODE_ERA, memory_order_relaxed);
    atomic_store_explicit(&vm->pair_buf[1].n0, NODE_ERA, memory_order_relaxed);
    atomic_store_explicit(&vm->pair_buf[1].n1, NODE_VAR(0), memory_order_relaxed);

    atomic_store_explicit(&vm->vars_buf[0], NODE_VAR(0), memory_order_relaxed);

    atomic_store_explicit(&vm->redx_buf[0].n0, NODE_CON(0), memory_order_relaxed);
    atomic_store_explicit(&vm->redx_buf[0].n1, NODE_DUP(1), memory_order_relaxed);

    vm->threads[0].pair_curr = 2;
    vm->threads[0].pair_cap = 2;
    vm->threads[0].vars_curr = 1;
    vm->threads[0].vars_cap = 1;
    vm->threads[0].redx_put = 1;

    vm_run(vm);
    // thread_run(vm, &vm->threads[0]);

    printf("INTERACTIONS: %llu\n", atomic_load_explicit(&vm->interactions, memory_order_relaxed));
    
}
