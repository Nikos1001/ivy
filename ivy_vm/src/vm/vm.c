
#include "vm.h"

void vm_init(NetVM* vm) {
    atomic_store_explicit(&vm->interactions, 0, memory_order_relaxed);

    for(u32 tid = 0; tid < N_THREADS; tid++) {
        vm->threads[tid] = (ThreadMem){
            .tid = tid,

            .pair_base = &vm->pair_buf[tid * PAIR_BLOCK_SIZE],
            .pair_curr = 0,
            .pair_cap = 0,

            .vars_base = &vm->vars_buf[tid * VARS_BLOCK_SIZE],
            .vars_curr = 0,
            .vars_cap = 0,

            .redx_base = &vm->redx_buf[tid * REDX_BLOCK_SIZE],
            .redx_put = 0,

            .prdx_put = 0
        };
    }
}

typedef struct {
    NetVM* vm;
    ThreadMem* mem;
} ThreadInfo;

void thread_func(void* param) {
    ThreadInfo* info = (ThreadInfo*)param;
    thread_run(info->vm, info->mem);
}

void vm_run(NetVM* vm) {
    ThreadInfo thread_info[N_THREADS];
    for(u32 tid = 0; tid < N_THREADS; tid++) {
        thread_info[tid].vm = vm;
        thread_info[tid].mem = &vm->threads[tid];
        pthread_create(&vm->threads[tid].thread, NULL, thread_func, &thread_info[tid]);
    }

    for(u32 tid = 0; tid < N_THREADS; tid++) {
        pthread_join(vm->threads[tid].thread, NULL);
        fflush(stdout);
    }

}

// Converts a node to its index in various tables.
// Converts SYM/F64 nodes to 8
// Converts NIL to 9
// Otherwise creates a 3 bit binary number from (node & NODE_F0), (node & NODE_F1), (node & NODE_F2)
// In other words:
//      VAR => 0
//      CAL => 1
//      CON => 2
//      DUP => 3
//      ERA => 4
//      OPI => 5
//      OPO => 6
//      SWI => 7
//      SYM => 8
//      NIL => 9
static inline u8 get_node_table_index(Node node) {
    // TODO: maybe there's some clever bitmath that could make this branchless? Profile.
    if(node == NODE_NIL) {
        return 9;
    } else if((node & QNAN) == QNAN) {
        return (node & NODE_F2) >> 61 | (node & (NODE_F1 | NODE_F0)) >> 48;
    } else {
        return 8;
    }
}

// ====== RESOURCE MANIPULATION ========

// TODO: this might potentially error
// TODO: replace the spin-alloc system with some kind of atomic free-chain to prevent memory explosion
static inline u32 new_pair(NetVM* vm, ThreadMem* mem, Node n0, Node n1) {
    while(true) {

        if(mem->pair_curr == mem->pair_cap) {
            atomic_store_explicit(&mem->pair_base[mem->pair_curr].n0, n0, memory_order_relaxed);
            atomic_store_explicit(&mem->pair_base[mem->pair_curr].n1, n1, memory_order_relaxed);
            mem->pair_cap++;
            mem->pair_curr++;
            return mem->tid * PAIR_BLOCK_SIZE + (mem->pair_curr - 1);
        }

        // We check n0 because it is the one that is set to NODE_NIL first in take_pair. Should prevent data races.
        if(atomic_load_explicit(&mem->pair_base[mem->pair_curr].n0, memory_order_relaxed) == NODE_NIL) {
            atomic_store_explicit(&mem->pair_base[mem->pair_curr].n0, n0, memory_order_relaxed);
            atomic_store_explicit(&mem->pair_base[mem->pair_curr].n1, n1, memory_order_relaxed);
            mem->pair_curr++;
            return mem->tid * PAIR_BLOCK_SIZE + (mem->pair_curr - 1);
        }

        mem->pair_curr++;
        // Works because PAIR_BLOCK_SIZE is guaranteed to be a power of 2
        mem->pair_curr &= PAIR_BLOCK_SIZE - 1;

    }
}

static inline Pair take_pair(NetVM* vm, u64 idx) {
    Node n0 = atomic_exchange_explicit(&vm->pair_buf[idx].n0, NODE_NIL, memory_order_relaxed);
    Node n1 = atomic_exchange_explicit(&vm->pair_buf[idx].n1, NODE_NIL, memory_order_relaxed);
    return MAKE_PAIR(n0, n1);
}

// TODO: this might potentially error
// TODO: replace the spin-alloc system with some kind of atomic free-chain to prevent memory explosion
static inline u32 alloc_var(NetVM* vm, ThreadMem* mem) {
    while(true) {

        if(mem->vars_curr == mem->vars_cap) {
            u32 var_idx = mem->vars_curr + mem->tid * VARS_BLOCK_SIZE;
            atomic_store_explicit(&vm->vars_buf[var_idx], NODE_VAR(var_idx), memory_order_relaxed);
            mem->vars_curr++;
            mem->vars_cap++;
            return var_idx;
        }

        if(atomic_load_explicit(&mem->vars_base[mem->vars_curr], memory_order_relaxed) == NODE_NIL) {
            u32 var_idx = mem->vars_curr + mem->tid * VARS_BLOCK_SIZE;
            atomic_store_explicit(&vm->vars_buf[var_idx], NODE_VAR(var_idx), memory_order_relaxed);
            mem->vars_curr++;
            return var_idx;
        }

        mem->vars_cap++;
        // Works because VARS_BLOCK_SIZE is guaranteed to be a power of 2
        mem->vars_cap &= VARS_BLOCK_SIZE - 1;

    }
}

static inline bool is_priority_pair(Node n0, Node n1) {
    u8 n0_idx = get_node_table_index(n0);
    u8 n1_idx = get_node_table_index(n1);
    const bool is_priority[10][10] = {
        //           VAR    CAL    CON    DUP    ERA    OPI    OPO    SWI    SYM    NIL 
        /* VAR */ { true , true , true , true , true , true , true , true , true , true  },
        /* CAL */ { true , false, false, false, false, false, false, false, false, true  },
        /* CON */ { true , false, true , false, true , false, false, false, false, true  },
        /* DUP */ { true , false, false, true , true , false, false, false, false, true  },
        /* ERA */ { true , false, true , true , true , false, false, false, false, true  },
        /* OPI */ { true , false, false, false, false, false, false, false, false, true  },
        /* OPO */ { true , false, false, false, false, false, false, false, false, true  },
        /* SWI */ { true , false, false, false, false, false, false, false, false, true  },
        /* SYM */ { true , false, false, false, false, false, false, false, false, true  },
        /* NIL */ { true , true , true , true , true , true , true , true , true , true  },
    };
    return is_priority[n0_idx][n1_idx];
}

static inline void push_redx(NetVM* vm, ThreadMem* mem, Node n0, Node n1) {
    if(is_priority_pair(n0, n1)) {
        mem->prdx[mem->prdx_put] = MAKE_PAIR(n0, n1);
        mem->prdx_put++;
    } else {
        atomic_store_pair(&mem->redx_base[mem->redx_put], MAKE_PAIR(n0, n1));
        mem->redx_put++; 
    }
}

static inline Pair pop_redx(NetVM* vm, ThreadMem* mem) {
    if(mem->prdx_put > 0) {
        mem->prdx_put--;
        return mem->prdx[mem->prdx_put];
    } else if(mem->redx_put > 0) {
        mem->redx_put--;
        return atomic_load_pair(&mem->redx_base[mem->redx_put]); 
    } else {
        return MAKE_PAIR(NODE_NIL, NODE_NIL);
    }
}

// ====== VM ERRORS =========

void out_of_redx_space(NetVM* vm, ThreadMem* mem) {
    fprintf(stderr, "VM FATAL ERROR: REDX SPACE EXHAUSTED\n");
    fprintf(stderr, "THREAD ID: %d\n", mem->tid);
    exit(-1);
}

// ====== DEBUG =============

void dump_thread_state(ThreadMem* mem) {
    
    // Dump pairs
    printf("====== PAIRS ======\n");
    for(u32 i = 0; i < 10; i++) {
        printf("%03d | ", i);
        dump_node(atomic_load_explicit(&mem->pair_base[i].n0, memory_order_relaxed));
        printf("\n    | ");
        dump_node(atomic_load_explicit(&mem->pair_base[i].n1, memory_order_relaxed));
        printf("\n");
    }
    printf("====== VARS  ======\n");
    for(u32 i = 0; i < 10; i++) {
        printf("%03d | ", i);
        dump_node(mem->vars_base[i]);
        printf("\n");
    }
    printf("====== REDX ======\n");
    for(i32 i = mem->redx_put - 1; i >= 0; i--) {
        Pair redx = atomic_load_pair(&mem->redx_base[i]); 
        printf("[ ");
        dump_node(redx.n0);
        printf(" ]\t[ ");
        dump_node(redx.n1);
        printf(" ]\n");
    }
}

// ====== EXECUTION =========

void thread_run(NetVM* vm, ThreadMem* mem) {

    // The VM uses computed goto for its rule dispatch
    // The rule table fits in very nicely with the label lookup
    // Not sure if this makes a big difference for performance, but I couldn't resist
    const void* dispatch_table[10][10] = {
        //             VAR        CAL        CON        DUP        ERA        OPI        OPO        SWI        SYM        NIL 
        /* VAR */ { &&do_link, &&do_link, &&do_link, &&do_link, &&do_link, &&do_link, &&do_link, &&do_link, &&do_link, &&do_halt },
        /* CAL */ { &&do_link, &&do_void, &&do_void, &&do_void, &&do_void, &&do_void, &&do_void, &&do_void, &&do_void, &&do_halt },
        /* CON */ { &&do_link, &&do_void, &&do_anni, &&do_comm, &&do_eras, &&do_void, &&do_void, &&do_void, &&do_void, &&do_halt },
        /* DUP */ { &&do_link, &&do_void, &&do_comm, &&do_anni, &&do_eras, &&do_void, &&do_void, &&do_void, &&do_void, &&do_halt },
        /* ERA */ { &&do_link, &&do_void, &&do_eras, &&do_eras, &&do_void, &&do_void, &&do_void, &&do_void, &&do_void, &&do_halt },
        /* OPI */ { &&do_link, &&do_void, &&do_void, &&do_void, &&do_void, &&do_void, &&do_void, &&do_void, &&do_void, &&do_halt },
        /* OPO */ { &&do_link, &&do_void, &&do_void, &&do_void, &&do_void, &&do_void, &&do_void, &&do_void, &&do_void, &&do_halt },
        /* SWI */ { &&do_link, &&do_void, &&do_void, &&do_void, &&do_void, &&do_void, &&do_void, &&do_void, &&do_void, &&do_halt },
        /* SYM */ { &&do_link, &&do_void, &&do_void, &&do_void, &&do_void, &&do_void, &&do_void, &&do_void, &&do_void, &&do_halt },
        /* NIL */ { &&do_halt, &&do_halt, &&do_halt, &&do_halt, &&do_halt, &&do_halt, &&do_halt, &&do_halt, &&do_halt, &&do_halt },
    };

    Pair redex;
    u8 n0_idx;
    u8 n1_idx;

    #define DISPATCH() \
        if(mem->redx_put == 0 && mem->prdx_put == 0) \
            goto end; \
        redex = pop_redx(vm, mem); \
        n0_idx = get_node_table_index(redex.n0); \
        n1_idx = get_node_table_index(redex.n1); \
        atomic_fetch_add_explicit(&vm->interactions, 1, memory_order_relaxed); \
        goto *dispatch_table[n0_idx][n1_idx]; 

    #define ENSURE_REDX_SPACE(cnt) \
        if(mem->redx_put > REDX_BLOCK_SIZE - cnt || mem->prdx_put > THREAD_PRDX_SIZE - cnt) { \
            out_of_redx_space(vm, mem); \
        }

    DISPATCH();

    u64 pair0_idx; 
    u64 pair1_idx;
    Pair pair0;
    Pair pair1;

    do_void:
        DISPATCH();
    do_link:
        ENSURE_REDX_SPACE(1);
        Node var = redex.n0;
        Node val = redex.n1;

        if(!NODE_IS_VAR(var)) {
            swap_nodes(&var, &val);
        }

        u32 var_idx = NODE_GET_VAR_IDX(var);
        Node var_node = NODE_VAR(var_idx);

        if(!atomic_compare_exchange_strong_explicit(&vm->vars_buf[var_idx], &var_node, val, memory_order_relaxed, memory_order_relaxed)) {
            Node other_val = atomic_exchange_explicit(&vm->vars_buf[var_idx], NODE_NIL, memory_order_relaxed);
            push_redx(vm, mem, val, other_val);
        }

        DISPATCH();
    do_anni:
        ENSURE_REDX_SPACE(2);

        pair0_idx = redex.n0 & U48_MASK;
        pair1_idx = redex.n1 & U48_MASK;
        pair0 = take_pair(vm, pair0_idx);
        pair1 = take_pair(vm, pair1_idx);

        push_redx(vm, mem, pair0.n0, pair1.n0);
        push_redx(vm, mem, pair0.n1, pair1.n1);

        DISPATCH();
    do_comm:
        ENSURE_REDX_SPACE(4);

        pair0_idx = redex.n0 & U48_MASK;
        pair1_idx = redex.n1 & U48_MASK;

        u64 pair0_mask = redex.n0 & NODE_TAG_MASK;
        u64 pair1_mask = redex.n1 & NODE_TAG_MASK;

        pair0 = take_pair(vm, pair0_idx);
        pair1 = take_pair(vm, pair1_idx);

        u32 v0 = alloc_var(vm, mem);
        u32 v1 = alloc_var(vm, mem);
        u32 v2 = alloc_var(vm, mem);
        u32 v3 = alloc_var(vm, mem);

        u32 dup0_pair = new_pair(vm, mem, NODE_VAR(v1), NODE_VAR(v0));
        u32 dup1_pair = new_pair(vm, mem, NODE_VAR(v3), NODE_VAR(v2));

        u32 con0_pair = new_pair(vm, mem, NODE_VAR(v1), NODE_VAR(v3));
        u32 con1_pair = new_pair(vm, mem, NODE_VAR(v0), NODE_VAR(v2));

        push_redx(vm, mem, pair1_mask | dup0_pair, pair0.n0);
        push_redx(vm, mem, pair1_mask | dup1_pair, pair0.n1);

        push_redx(vm, mem, pair0_mask | con0_pair, pair1.n0);
        push_redx(vm, mem, pair0_mask | con1_pair, pair1.n1);

        DISPATCH();
    do_eras:
        ENSURE_REDX_SPACE(2);

        Node pair = NODE_IS_ERA(redex.n0) ? redex.n1 : redex.n0;
        u64 pair_idx = pair & U48_MASK;

        Pair to_erase = take_pair(vm, pair_idx);

        // Optimization TODO: these are guaranteed to be high-priority, no need to check.
        push_redx(vm, mem, NODE_ERA, to_erase.n0);
        push_redx(vm, mem, NODE_ERA, to_erase.n1);
        DISPATCH();
    do_halt:
        return;
    end:

    printf("THREAD %d COMPLETED REDUCTIONS\n", mem->tid);
    fflush(stdout);

}
