
#include "node.h"

void dump_node(Node node) {
    if(NODE_IS_NIL(node)) {
        printf("NIL");
    } else if(NODE_IS_VAR(node)) {
        printf("VAR %llu", NODE_GET_VAR_IDX(node));
    } else if(NODE_IS_CAL(node)) {
        printf("CAL %llu", NODE_GET_CAL_IDX(node));
    } else if(NODE_IS_CON(node)) {
        Aux aux = NODE_GET_CON_AUX(node);
        printf("CON %llu %llu", AUX_SIZE(aux), AUX_BEGIN(aux));
    } else if(NODE_IS_DUP(node)) {
        Aux aux = NODE_GET_DUP_AUX(node);
        printf("DUP %llu %llu", AUX_SIZE(aux), AUX_BEGIN(aux));
    } else if(NODE_IS_ERA(node)) {
        printf("ERA");
    } else if(NODE_IS_SYM(node)) {
        printf("SYM %llu", NODE_GET_SYM(node)); 
    } else {
        printf("TODO: node.c, dump_node.\n");
    }
}

// ======== HELPERS FOR RUST FFI ========

Node make_var(u64 var) {
    return NODE_VAR(var);
}

Node make_con(Aux aux) {
    return NODE_CON(aux);
}

Node make_dup(Aux aux) {
    return NODE_DUP(aux);
}

Node make_era() {
    return NODE_ERA;
}
