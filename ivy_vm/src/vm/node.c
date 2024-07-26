
#include "node.h"

void dump_node(Node node) {
    if(NODE_IS_NIL(node)) {
        printf("NIL");
    } else if(NODE_IS_VAR(node)) {
        printf("VAR %llu", NODE_GET_VAR_IDX(node));
    } else if(NODE_IS_CAL(node)) {
        printf("CAL %llu", NODE_GET_CAL_IDX(node));
    } else if(NODE_IS_CON(node)) {
        printf("CON %llu", NODE_GET_CON_PAIR(node));
    } else if(NODE_IS_DUP(node)) {
        printf("DUP %llu", NODE_GET_DUP_PAIR(node));
    } else if(NODE_IS_ERA(node)) {
        printf("ERA");
    } else {
        printf("TODO: node.c, dump_node.\n");
    }
}
