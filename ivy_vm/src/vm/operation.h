
#ifndef OPERATION_H
#define OPERATION_H

#include "node.h"

#define OP_ADD 0

typedef struct {
    u64 op;

    Aux  ins; 
    Node out;

    a64  n_unlinked;
} Operation;

#endif
