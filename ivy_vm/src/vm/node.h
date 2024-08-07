
#ifndef NODE_H
#define NODE_H

#include "common.h"

typedef u64 Node; 

static inline void swap_nodes(Node* a, Node* b) {
    Node temp = *a;
    *a = *b;
    *b = temp;
}

typedef struct {
    Node n0;
    Node n1;
} Pair;

#define MAKE_PAIR(n0, n1) ((Pair){(n0), (n1)})

typedef u64 Aux;
#define MAKE_AUX(size, begin) ((((u64)size - 1) << 40) | (begin))
#define AUX_SIZE(aux) ((((aux) >> 40) & 0xFF) + 1)
#define AUX_BEGIN(aux) ((aux) & ((1ul << 40) - 1))

#define QNAN     ((u64)0x7ffc000000000000)
#define U62_MASK ((u64)0x3FFFFFFFFFFFFFFF)
#define U48_MASK ((u64)0x0000FFFFFFFFFFFF)

#define NODE_F0 (((u64)1 << 48))
#define NODE_F1 (((u64)1 << 49))
#define NODE_F2 (((u64)1 << 63))

#define NODE_TAG_MASK (QNAN | NODE_F0 | NODE_F1 | NODE_F2)

#define NODE_VAR_TAG        QNAN 
#define NODE_VAR(idx)       (NODE_VAR_TAG | (idx))
#define NODE_IS_VAR(node)   (((node) & NODE_TAG_MASK) == NODE_VAR_TAG)

#define NODE_CAL_TAG        (QNAN | NODE_F0) 
#define NODE_CAL(idx)       (NODE_CAL_TAG | (idx))
#define NODE_IS_CAL(node)   (((node) & NODE_TAG_MASK) == NODE_CAL_TAG)

#define NODE_CON_TAG        (QNAN | NODE_F1)
#define NODE_CON(aux)      (NODE_CON_TAG | (aux))
#define NODE_IS_CON(node)   (((node) & NODE_TAG_MASK) == NODE_CON_TAG)

#define NODE_DUP_TAG        (QNAN | NODE_F0 | NODE_F1)
#define NODE_DUP(aux)       (NODE_DUP_TAG | (aux))
#define NODE_IS_DUP(node)   (((node) & NODE_TAG_MASK) == NODE_DUP_TAG)

#define NODE_ERA_TAG        (QNAN | NODE_F2)
#define NODE_ERA            NODE_ERA_TAG 
#define NODE_IS_ERA(node)   (node == NODE_ERA) 

#define NODE_OPI_TAG        (QNAN | NODE_F0 | NODE_F2)
#define NODE_OPI(op, idx)   (NODE_OPI_TAG | (op) | ((idx) << 40))
#define NODE_IS_OPI(node)   (((node) & NODE_TAG_MASK) == NODE_OPI_TAG)

#define NODE_OPO_TAG        (QNAN | NODE_F1 | NODE_F2)
#define NODE_OPO(op)        (NODE_OPO_TAG | (op))
#define NODE_IS_OPO(node)   (((node) & NODE_TAG_MASK) == NODE_OPO_TAG)

#define NODE_SWI_TAG        (QNAN | NODE_F0 | NODE_F1 | NODE_F2)
#define NODE_SWI(pair)      (NODE_SWI_TAG | (pair))
#define NODE_IS_SWI(node)   (((node) & NODE_TAG_MASK) == NODE_SWI_TAG)

#define NODE_NIL ((u64)0xFFFFFFFFFFFFFFFF)
#define NODE_IS_NIL(node) (node == NODE_NIL)

#define NODE_GET_VAR_IDX(node)  ((node) & U48_MASK)
#define NODE_GET_CAL_IDX(node)  ((node) & U48_MASK)
#define NODE_GET_CON_AUX(node)  ((node) & U48_MASK)
#define NODE_GET_DUP_AUX(node)  ((node) & U48_MASK)
#define NODE_GET_OPI_OP(node)   ((node) & 0xFFFFFFFFFF)
#define NODE_GET_OPI_IDX(node)  ((((node) & U48_MASK) >> 40) & 0xFF)
#define NODE_GET_OPO_OP(node)   ((node) & U48_MASK)

static inline u64 bitcast_f64_to_u64(f64 x) {
    union {
        f64 f;
        u64 u;
    } temp;
    temp.f = x;
    return temp.u;
}

static inline f64 bitcast_u64_to_f64(u64 x) {
    union {
        f64 f;
        u64 u;
    } temp;
    temp.u = x;
    return temp.f;
}

#define NODE_SYM(sym)       ((sym) & U62_MASK)
#define NODE_F64(num)       bitcast_f64_to_u64(num)
#define NODE_IS_SYM(node)   (((node) & QNAN) != QNAN)
#define NODE_GET_SYM(node)  (node) 

typedef a64 ANode;
typedef struct {
    ANode n0;
    ANode n1;
} APair;

static inline Pair atomic_load_pair(APair* pair) {
    Node n0 = atomic_load_explicit(&pair->n0, memory_order_relaxed);
    Node n1 = atomic_load_explicit(&pair->n1, memory_order_relaxed);
    return MAKE_PAIR(n0, n1);
}

static inline void atomic_store_pair(APair* pair, Pair value) {
    atomic_store_explicit(&pair->n0, value.n0, memory_order_relaxed);
    atomic_store_explicit(&pair->n1, value.n1, memory_order_relaxed);
}

void dump_node(Node node);

#endif
