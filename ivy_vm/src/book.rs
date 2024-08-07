
use std::ffi::c_void;
use crate::node::{Aux, Node};

pub struct Book {
    pub(crate) book: *mut c_void,
    curr_def: u64
}

pub struct Def<'a> {
    book: &'a mut Book,
    def: *mut c_void
}

pub enum Operation {
    Add,
    Native(unsafe fn(u64, *const Node) -> Node)
}

impl Operation {

    fn to_op_code(&self) -> u64 {
        match self {
            Operation::Add => 0,
            Operation::Native(func) => {
                let fn_addr = *func as *const unsafe fn(u64, *const Node) -> Node as u64;
                (1u64 << 63) | fn_addr
            }
        }
    }

}

pub struct DefID(u64);

extern "C" {
    fn create_book() -> *mut c_void;
    fn destroy_book(book: *mut c_void);

    fn add_aux(book: *mut c_void, size: u32, nodes: *const u64) -> u64;
    fn get_def(book: *mut c_void, id: u64) -> *mut c_void;

    fn def_set_out(def: *mut c_void, node: u64);
    fn def_add_var(def: *mut c_void) -> u64;
    fn def_add_redex(def: *mut c_void, a: u64, b: u64);
    fn def_add_oper(def: *mut c_void, op: u64, ins: u32) -> u64;

}

impl Book {

    pub fn new() -> Self {
        let book = unsafe { create_book() };
        Self {
            book,
            curr_def: 0
        }
    }

    pub fn add_def(&mut self) -> DefID {
        self.curr_def += 1; 
        DefID(self.curr_def - 1)
    }

    pub fn get_def<'a>(&'a mut self, id: DefID) -> Def<'a> {
        let def = unsafe { get_def(self.book, id.0) }; 
        Def {
            book: self,
            def
        } 
    }

    pub fn add_aux(&mut self, nodes: &[Node]) -> Aux {
        assert!(nodes.len() <= 256, "aux can have at most 256 nodes.");
        let size = nodes.len() as u32;
        unsafe { Aux(add_aux(self.book, size, nodes.as_ptr() as *const u64)) }
    }

    pub fn con(&mut self, nodes: &[Node]) -> Node {
        Node::con(self.add_aux(nodes))
    }

    pub fn dup(&mut self, nodes: &[Node]) -> Node {
        Node::dup(self.add_aux(nodes))
    }

}

impl Drop for Book {
    
    fn drop(&mut self) {
        unsafe {
            destroy_book(self.book);
        }
    }

}

impl<'a> Def<'a> {

    pub fn set_out(&mut self, node: Node) {
        unsafe {
            def_set_out(self.def, node.0);
        }
    }

    pub fn add_redex(&mut self, a: Node, b: Node) {
        unsafe {
            def_add_redex(self.def, a.0, b.0);
        }
    } 

    pub fn add_var(&mut self) -> (Node, Node) {
        let var_idx = unsafe { def_add_var(self.def) };
        (Node::var(var_idx), Node::var(var_idx))
    }

    pub fn con(&mut self, nodes: &[Node]) -> Node {
        self.book.con(nodes)
    }

    pub fn dup(&mut self, nodes: &[Node]) -> Node {
        self.book.dup(nodes)
    }

    pub fn add_operation(&mut self, op: Operation, ins: Vec<Node>) -> Node {
        assert!(ins.len() <= 256, "operation can have at most 256 inputs.");
        let op_idx = unsafe { def_add_oper(self.def, op.to_op_code(), ins.len() as u32) }; 
        for (idx, node) in ins.into_iter().enumerate() {
            self.add_redex(Node::opi(op_idx, idx as u64), node);
        }
        Node::opo(op_idx)
    }

}
