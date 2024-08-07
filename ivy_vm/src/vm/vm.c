
#include "vm.h"

#define INITIAL_PAIR_CAPACITY 128

void vm_init(NetVM* vm) {
    atomic_store_explicit(&vm->interactions, 0, memory_order_relaxed);

    for(u64 tid = 0; tid < N_THREADS; tid++) {
        vm->threads[tid] = (ThreadMem){
            .tid = tid,

            .aux_curr = tid * AUX_BLOCK_SIZE,
            .aux_last = (tid + 1) * AUX_BLOCK_SIZE,

            .var_curr = tid * VAR_BLOCK_SIZE,
            .var_last = (tid + 1) * VAR_BLOCK_SIZE, 
            .var_free = UINT64_MAX,

            .redx_base = &vm->redx_buf[tid * REDX_BLOCK_SIZE],
            .redx_put = 0,

            .oper_curr = tid * OPER_BLOCK_SIZE,
            .oper_last = (tid + 1) * OPER_BLOCK_SIZE,
            .oper_free = UINT64_MAX,

            .prdx_put = 0
        };

        for(u32 i = 0; i < 256; i++) {
            vm->threads[tid].aux_free[i] = UINT64_MAX;
        }
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

// ====== VM ERRORS =========

void vm_panic(NetVM* vm, ThreadMem* mem, const char* msg) {
    fprintf(stderr, "VM FATAL ERROR: %s\n", msg);
    fprintf(stderr, "THREAD ID: %d\n", mem->tid);
    exit(-1);
}

// ====== RESOURCE MANIPULATION ========

// Allocates an aux block of a given size
Aux alloc_aux(NetVM* vm, ThreadMem* mem, u64 size) {
    if(mem->aux_free[size - 1] != UINT64_MAX) {
        u64 block = mem->aux_free[size - 1];
        mem->aux_free[size - 1] = vm->aux_buf[block];
        return MAKE_AUX(size, block);
    }

    mem->aux_curr += size;
    if(mem->aux_curr > mem->aux_last) {
        vm_panic(vm, mem, "AUX SPACE EXHAUSTED");
    }

    return MAKE_AUX(size, mem->aux_curr - size);
}

Node* get_aux(NetVM* vm, Aux aux) {
    return &vm->aux_buf[AUX_BEGIN(aux)];
}

void free_aux(NetVM* vm, ThreadMem* mem, Aux aux) {
    u64 size = AUX_SIZE(aux);
    u64 begin = AUX_BEGIN(aux);
    vm->aux_buf[begin] = mem->aux_free[size - 1];
    mem->aux_free[size - 1] = begin; 
}

u64 alloc_var(NetVM* vm, ThreadMem* mem) {
    if(mem->var_free != UINT64_MAX) {
        u64 var = mem->var_free;
        mem->var_free = atomic_load_explicit(&vm->var_buf[var], memory_order_relaxed);
        atomic_store_explicit(&vm->var_buf[var], NODE_VAR(var), memory_order_relaxed);
        return var;
    } 
    if(mem->var_curr == mem->var_last) {
        vm_panic(vm, mem, "VAR SPACE EXHAUSTED");
    }
    mem->var_curr++;
    u64 var = mem->var_curr - 1;
    atomic_store_explicit(&vm->var_buf[var], NODE_VAR(var), memory_order_relaxed);
    return var;
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

void push_redx(NetVM* vm, ThreadMem* mem, Node n0, Node n1) {
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

static inline void init_oper(NetVM* vm, ThreadMem* mem, u64 oper_idx, u64 op, u64 ins) {
    Operation* oper = &vm->oper_buf[oper_idx];
    oper->op = op;
    oper->ins = alloc_aux(vm, mem, ins);
    atomic_store_explicit(&oper->n_unlinked, ins + 1, memory_order_relaxed);
}

u64 alloc_oper(NetVM* vm, ThreadMem* mem, u64 op, u64 ins) {
    if(mem->oper_free != UINT64_MAX) {
        u64 oper_idx = mem->oper_free;
        mem->oper_free = vm->oper_buf[oper_idx].op;
        init_oper(vm, mem, oper_idx, op, ins);
        return oper_idx;
    }
    if(mem->oper_curr == mem->oper_last) {
        vm_panic(vm, mem, "OPERATION SPACE EXHAUSTED");
    }
    mem->oper_curr++;
    u64 oper_idx = mem->oper_curr - 1;
    init_oper(vm, mem, oper_idx, op, ins);
    return oper_idx;
}

static inline void free_oper(NetVM* vm, ThreadMem* mem, u64 oper) {
    Operation* op = &vm->oper_buf[oper];
    op->op = mem->oper_free;
    mem->oper_free = oper;
}

// ====== DEBUG =============

void dump_thread_state(NetVM* vm, ThreadMem* mem) {
    printf("====== AUX  ======\n");
    for(u64 i = 0; i < 16; i++) {
        printf("%03d | ", i);
        dump_node(vm->aux_buf[i]);
        printf("\n"); 
    }
    printf("====== PRDX ======\n");
    for(i32 i = mem->prdx_put - 1; i >= 0; i--) {
        Pair redx = mem->prdx[i];
        printf("[ ");
        dump_node(redx.n0);
        printf(" ]\t[ ");
        dump_node(redx.n1);
        printf(" ]\n");
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
    printf("\n");
}

// ====== EXECUTION =========

static inline void perform_operation(NetVM* vm, ThreadMem* mem, u64 op_idx) {
    Node out;
    Operation* op = &vm->oper_buf[op_idx];
    if(op->op == OP_ADD) {
        f64 sum = 0.0;
        u64 ins = AUX_SIZE(op->ins);
        Node* vals = get_aux(vm, op->ins);
        for(u64 i = 0; i < ins; i++) {
            sum += bitcast_u64_to_f64(vals[i]); 
        }
        out = NODE_F64(sum);
    }

    push_redx(vm, mem, out, op->out);

    free_aux(vm, mem, op->ins);
    free_oper(vm, mem, op_idx);
}

void thread_run(NetVM* vm, ThreadMem* mem) {

    // The VM uses computed goto for its rule dispatch
    // The rule table fits in very nicely with the label lookup
    // Not sure if this makes a big difference for performance, but I couldn't resist
    const void* dispatch_table[10][10] = {
        //             VAR        CAL        CON        DUP        ERA        OPI        OPO        SWI        SYM        NIL 
        /* VAR */ { &&do_link, &&do_link, &&do_link, &&do_link, &&do_link, &&do_link, &&do_outl, &&do_link, &&do_link, &&do_halt },
        /* CAL */ { &&do_link, &&do_void, &&do_void, &&do_void, &&do_void, &&do_inpl, &&do_outl, &&do_void, &&do_void, &&do_halt },
        /* CON */ { &&do_link, &&do_void, &&do_anni, &&do_comm, &&do_eras, &&do_inpl, &&do_outl, &&do_void, &&do_void, &&do_halt },
        /* DUP */ { &&do_link, &&do_void, &&do_comm, &&do_anni, &&do_eras, &&do_inpl, &&do_outl, &&do_void, &&do_void, &&do_halt },
        /* ERA */ { &&do_link, &&do_void, &&do_eras, &&do_eras, &&do_void, &&do_kili, &&do_kilo, &&do_void, &&do_void, &&do_halt },
        /* OPI */ { &&do_link, &&do_inpl, &&do_inpl, &&do_inpl, &&do_kili, &&do_halt, &&do_outl, &&do_inpl, &&do_inpl, &&do_halt },
        /* OPO */ { &&do_outl, &&do_outl, &&do_outl, &&do_outl, &&do_kilo, &&do_outl, &&do_halt, &&do_outl, &&do_outl, &&do_halt },
        /* SWI */ { &&do_link, &&do_void, &&do_void, &&do_void, &&do_void, &&do_inpl, &&do_outl, &&do_void, &&do_void, &&do_halt },
        /* SYM */ { &&do_link, &&do_void, &&do_void, &&do_void, &&do_void, &&do_inpl, &&do_outl, &&do_void, &&do_void, &&do_halt },
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
            vm_panic(vm, mem, "REDEX SPACE EXHAUSTED"); \
        }

    DISPATCH();

    u64 aux_size;
    u64 aux0_size;
    u64 aux1_size;

    Aux aux;
    Aux aux0;
    Aux aux1;

    Node* aux_nodes;
    Node* aux0_nodes;
    Node* aux1_nodes;

    u64 op, idx;
    Operation* operation;
    Node* inputs;

    do_void:
        DISPATCH();
    do_link:
        ENSURE_REDX_SPACE(1);
        Node var = redex.n0;
        Node val = redex.n1;

        if(!NODE_IS_VAR(var)) {
            swap_nodes(&var, &val);
        }

        u64 var_idx = NODE_GET_VAR_IDX(var);
        Node var_node = NODE_VAR(var_idx);

        if(!atomic_compare_exchange_strong_explicit(&vm->var_buf[var_idx], &var_node, val, memory_order_relaxed, memory_order_relaxed)) {
            Node other_val = atomic_exchange_explicit(&vm->var_buf[var_idx], mem->var_free, memory_order_relaxed);
            mem->var_free = var_idx;
            push_redx(vm, mem, val, other_val);
        }

        DISPATCH();
    do_anni: 
        aux0 = redex.n0 & U48_MASK;
        aux1 = redex.n1 & U48_MASK;

        aux_size = AUX_SIZE(aux0);

        #ifdef DEBUG_MODE
        aux1_size = AUX_SIZE(aux1);
        if(aux_size != aux1_size) {
            vm_panic(vm, mem, "ANNIHILATION AUX SIZES ARE NOT THE SAME");
        }
        #endif

        ENSURE_REDX_SPACE(aux_size);

        aux0_nodes = get_aux(vm, aux0); 
        aux1_nodes = get_aux(vm, aux1); 

        for(u64 i = 0; i < aux_size; i++) {
            push_redx(vm, mem, aux0_nodes[i], aux1_nodes[i]);
        }

        free_aux(vm, mem, aux0);
        free_aux(vm, mem, aux1);

        DISPATCH();
    do_comm:
        aux0 = redex.n0 & U48_MASK;
        aux0_size = AUX_SIZE(aux0);

        aux1 = redex.n1 & U48_MASK;
        aux1_size = AUX_SIZE(aux1);

        ENSURE_REDX_SPACE(aux0_size + aux1_size);

        aux0_nodes = get_aux(vm, aux0);
        aux1_nodes = get_aux(vm, aux1);

        u64 n0_mask = redex.n0 & NODE_TAG_MASK;
        u64 n1_mask = redex.n1 & NODE_TAG_MASK;

        u64 vars[256][256];
        for(u64 i = 0; i < aux0_size; i++) {
            for(u64 j = 0; j < aux1_size; j++) {
                vars[i][j] = alloc_var(vm, mem);
            }
        }

        for(u64 i = 0; i < aux0_size; i++) {
            aux = alloc_aux(vm, mem, aux1_size);
            aux_nodes = get_aux(vm, aux);
            for(u64 j = 0; j < aux1_size; j++) {
                aux_nodes[j] = NODE_VAR(vars[i][j]);
            }
            Node n = n1_mask | aux;
            push_redx(vm, mem, n, aux0_nodes[i]);
        }

        for(u64 i = 0; i < aux1_size; i++) {
            aux = alloc_aux(vm, mem, aux0_size);
            aux_nodes = get_aux(vm, aux);
            for(u64 j = 0; j < aux0_size; j++) {
                aux_nodes[j] = NODE_VAR(vars[j][i]);
            }
            Node n = n0_mask | aux;
            push_redx(vm, mem, n, aux1_nodes[i]);
        }

        free_aux(vm, mem, aux0);
        free_aux(vm, mem, aux1);

        DISPATCH();
    do_eras:
        if(NODE_IS_ERA(redex.n0)) {
            swap_nodes(&redex.n0, &redex.n1);
        }

        aux = redex.n0 & U48_MASK;

        aux_size = AUX_SIZE(aux);
        aux_nodes = get_aux(vm, aux);

        ENSURE_REDX_SPACE(aux_size);

        for(u64 i = 0; i < aux_size; i++) {
            push_redx(vm, mem, aux_nodes[i], NODE_ERA);
        }

        free_aux(vm, mem, aux);

        DISPATCH();
    do_inpl:
        if(NODE_IS_OPI(redex.n1)) {
            swap_nodes(&redex.n0, &redex.n1);
        }
        op = NODE_GET_OPI_OP(redex.n0);
        idx = NODE_GET_OPI_IDX(redex.n0);
        operation = &vm->oper_buf[op];
        inputs = get_aux(vm, operation->ins);
        inputs[idx] = redex.n1;
        if(atomic_fetch_sub_explicit(&operation->n_unlinked, 1, memory_order_relaxed) == 1) {
            perform_operation(vm, mem, op);
        }
        DISPATCH();
    do_outl:
        if(NODE_IS_OPO(redex.n1)) {
            swap_nodes(&redex.n0, &redex.n1);
        }
        op = NODE_GET_OPO_OP(redex.n0);
        operation = &vm->oper_buf[op];
        inputs = get_aux(vm, operation->ins);
        operation->out = redex.n1;
        if(atomic_fetch_sub_explicit(&operation->n_unlinked, 1, memory_order_relaxed) == 1) {
            perform_operation(vm, mem, op);
        }
        DISPATCH();
    do_kili:
        // TODO: kill operation input
        DISPATCH();
    do_kilo:
        // TODO: kill operation input
        DISPATCH();
    do_halt:
        return;
    end:

    fflush(stdout);

}
