
#ifndef BOOK_H
#define BOOK_H

#include "common.h"
#include "node.h"
#include "vm.h"
#include "operation.h"

#define DEF_MAX_VAR  (1ull << 26)
#define DEF_MAX_REDX (1ull << 26)
#define DEF_MAX_OPER (1ull << 26)
#define DEF_MAX_ARGS 256

typedef struct {
    u64 vars;

    u64  redx_len;
    Pair redx_buf[DEF_MAX_REDX];

    u64 oper_len;
    u64 oper_ops[DEF_MAX_OPER];
    u32 oper_ins[DEF_MAX_OPER];

    Node  args[DEF_MAX_ARGS];
    Node  out;
} Def;

#define BOOK_MAX_AUX (1ull << 30)
#define BOOK_MAX_DEF (1ull << 16)

typedef struct {
    Node aux_buf[BOOK_MAX_AUX];
    u64  aux_curr; 

    Def defs[BOOK_MAX_DEF];
} Book;

Book* create_book();
void destroy_book(Book* book);

struct NetVM;
struct ThreadMem;

Node instance_def(NetVM* vm, ThreadMem* mem, Book* book, Def* def);

#endif
