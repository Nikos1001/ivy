
#include "vm.h"
#include "book.h"
#include <time.h>

void run(Book* book) {

    NetVM* vm = malloc(sizeof(NetVM));
    vm_init(vm);

    u64 out_var_idx = alloc_var(vm, &vm->threads[0]);
    Node out = instance_def(vm, &vm->threads[0], book, &book->defs[0]);
    push_redx(vm, &vm->threads[0], NODE_VAR(out_var_idx), out);

    time_t start = clock();
    vm_run(vm);
    time_t end = clock();
    f64 time_taken = (f64)(end - start) / (f64)CLOCKS_PER_SEC;

    u64 interactions = atomic_load_explicit(&vm->interactions, memory_order_relaxed);
    printf("INTERACTIONS: %llu\n", interactions);
    printf("TIME TAKEN: %g\n", time_taken);
    printf("MIPS: %g\n", (f64)interactions / time_taken / 1000000.0);
}
