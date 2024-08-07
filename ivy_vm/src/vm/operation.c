
#include "operation.h"
#include "vm.h"

void perform_operation(NetVM* vm, ThreadMem* mem, u64 op_idx) {
    Node out;
    Operation* op = &vm->oper_buf[op_idx];
    u64 n_ins = AUX_SIZE(op->ins);
    Node* ins = get_aux(vm, op->ins);
    if(op->killed) {
        push_redx(vm, mem, NODE_ERA, op->out);
        for(u64 i = 0; i < n_ins; i++) {
            push_redx(vm, mem, NODE_ERA, ins[i]);
        }
    } else if(op->op == OP_ADD) {
        f64 sum = 0.0;
        for(u64 i = 0; i < n_ins; i++) {
            sum += bitcast_u64_to_f64(ins[i]); 
        }
        out = NODE_F64(sum);
    } else if(op->op & (1ull << 63)) { // Native function
        u64 func_ptr = op->op & (~(1ull << 63));
        Node (*func)(u64, Node*) = (void*)func_ptr;
        out = func(n_ins, ins);
    }

    push_redx(vm, mem, out, op->out);

    free_aux(vm, mem, op->ins);
    free_oper(vm, mem, op_idx);
}