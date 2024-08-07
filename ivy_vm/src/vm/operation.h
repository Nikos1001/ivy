
#ifndef OPERATION_H
#define OPERATION_H

#include "node.h"

#define OP_ADD 0

typedef struct {
    u64 op;

    Aux  ins; 
    Node out;

    a64  n_unlinked;

    bool killed;
} Operation;

struct NetVM;
struct ThreadMem;

void perform_operation(struct NetVM* vm, struct ThreadMem* mem, u64 op_idx);

#endif
