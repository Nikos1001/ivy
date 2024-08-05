
#include "book.h"
#include "vm.h"

Book* create_book() {
    Book* book = malloc(sizeof(Book));    
    book->aux_curr = 0;
    return book;
}

void destroy_book(Book* book) {
    free(book);
}

static inline Node update_node(NetVM* vm, ThreadMem* mem, Node node, Node* aux_buf, u64* vars);

static inline Aux update_aux(NetVM* vm, ThreadMem* mem, Aux aux, Node* aux_buf, u64* vars) {
    u64 aux_size = AUX_SIZE(aux);
    u64 aux_begin = AUX_BEGIN(aux);
    Node* aux_nodes = &aux_buf[aux_begin]; 

    Aux new_aux = alloc_aux(vm, mem, aux_size);
    Node* new_aux_nodes = get_aux(vm, new_aux);
    for(u32 i = 0; i < aux_size; i++) {
        new_aux_nodes[i] = update_node(vm, mem, aux_nodes[i], aux_buf, vars);
    }

    return new_aux;
}

static inline Node update_node(NetVM* vm, ThreadMem* mem, Node node, Node* aux_buf, u64* vars) {
    if(NODE_IS_VAR(node)) {
        return NODE_VAR(vars[NODE_GET_VAR_IDX(node)]);
    } else if(NODE_IS_CON(node)) {
        return NODE_CON(update_aux(vm, mem, NODE_GET_CON_AUX(node), aux_buf, vars));
    } else if(NODE_IS_DUP(node)) {
        return NODE_DUP(update_aux(vm, mem, NODE_GET_CON_AUX(node), aux_buf, vars));
    } else if(NODE_IS_ERA(node)) {
        return NODE_ERA;
    } else if(NODE_IS_OPI(node)) {
        
    } else if(NODE_IS_OPO(node)) {

    } else if(NODE_IS_SWI(node)) {

    } else if(NODE_IS_SYM(node)) {
        return node;
    }
}

// Create an instance of a definition, creating fresh auxes, variables, etc. Returns the output node of the definition
Node instance_def(NetVM* vm, ThreadMem* mem, Book* book, Def* def) {
    u64 vars[DEF_MAX_VAR];

    for(u32 i = 0; i < def->vars; i++) {
        vars[i] = alloc_var(vm, mem);
    }

    for(u32 i = 0; i < def->redx_len; i++) {
        push_redx(
            vm,
            mem,
            update_node(vm, mem, def->redx_buf[i].n0, book->aux_buf, vars),
            update_node(vm, mem, def->redx_buf[i].n1, book->aux_buf, vars)
        );
    }

    return update_node(vm, mem, def->out, book->aux_buf, vars);
}

// ======== BOOK MANIPULATION ========

Aux add_aux(Book* book, u32 size, Node* nodes) {
    memcpy(&book->aux_buf[book->aux_curr], nodes, sizeof(Node) * size);
    book->aux_curr += size;
    return MAKE_AUX(size, book->aux_curr - size);
}

Def* get_def(Book* book, u64 id) {
    return &book->defs[id];
}

// ======== DEF MANIPULATION ========

void def_set_out(Def* def, Node out) {
    def->out = out;
}

u64 def_add_var(Def* def) {
    def->vars++;
    return def->vars - 1;
}

void def_add_redex(Def* def, Node a, Node b) {
    def->redx_buf[def->redx_len] = MAKE_PAIR(a, b);
    def->redx_len++;
}
