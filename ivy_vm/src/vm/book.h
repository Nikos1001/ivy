
#ifndef BOOK_H
#define BOOK_H

#include "common.h"
#include "node.h"
#include "vm.h"
#include "operation.h"

#define DEF_MAX_VAR  65536
#define DEF_MAX_REDX 1024
#define DEF_MAX_ARGS 256
#define DEF_MAX_OPER 4096 

typedef struct {
    u32  vars;

    u32  redx_len;
    Pair redx_buf[DEF_MAX_REDX];

    u32 oper_len;
    u64 oper_ops[DEF_MAX_OPER];
    u32 oper_ins[DEF_MAX_OPER];

    Node  args[DEF_MAX_ARGS];
    Node  out;
} Def;

#define BOOK_MAX_AUX (1 << 30)
#define BOOK_MAX_DEF 65536

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
