
#include "vm.h"
#include "book.h"

void run(Book* book) {

    NetVM* vm = malloc(sizeof(NetVM));
    vm_init(vm);

    u64 out_var_idx = alloc_var(vm, &vm->threads[0]);
    Node out = instance_def(vm, &vm->threads[0], book, &book->defs[0]);
    push_redx(vm, &vm->threads[0], NODE_VAR(out_var_idx), out);

    vm_run(vm);

    printf("INTERACTIONS: %llu\n", atomic_load_explicit(&vm->interactions, memory_order_relaxed));
    printf("%g\n", vm->var_buf[out_var_idx]);
    
}
