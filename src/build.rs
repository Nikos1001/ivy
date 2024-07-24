
use crate::{desc::NetDesc, node::Node};

impl NetDesc {

    // Node Helpers
    pub fn make_era(&mut self) -> usize {
        self.add_node(Node::ERA)
    }

    pub fn make_con(&mut self, a: usize, b: usize) -> usize {
        self.add_node(Node::CON(a, b))
    }

    pub fn make_dup(&mut self, a: usize, b: usize) -> usize {
        self.add_node(Node::DUP(a, b))
    }

    pub fn make_ref(&mut self, desc_idx: usize) -> usize {
        self.add_node(Node::REF(desc_idx))
    }

    // Mux/Demux
    pub fn mux3(&mut self, a: usize, b: usize, c: usize) -> usize {
        let con = self.make_con(b, c);
        self.make_con(a, con) 
    }

    pub fn demux3(&mut self, inp: usize, a: usize, b: usize, c: usize) {
        let con = self.make_con(c, b);
        let demux = self.make_con(con, a);
        self.add_redex(demux, inp);
    }

    // Church Arithmetic
    pub fn church_0(&mut self) -> usize {
        let (v1, v2) = self.add_var();
        let e = self.make_era();
        self.mux3(e, v1, v2)
    }

    pub fn church_succ(&mut self, n: usize) -> usize {
        let (v1, v2) = self.add_var();
        let con = self.make_con(n, v1);
        let e = self.make_era();
        self.mux3(con, e, v2)
    } 

    pub fn church_pred(&mut self, n: usize, out: usize) {
        let (v1, v2) = self.add_var();
        let con = self.make_con(v1, v2);
        let zero = self.church_0();
        self.demux3(n, con, zero, out);
    }

    pub fn church_n(&mut self, n: u64) -> usize {
        let mut node = self.church_0();
        for _ in 0..n {
            node = self.church_succ(node);
        }
        node
    }

    pub fn church_ifz(&mut self, inp: usize, zero_branch: usize, nzero_branch: usize, output: usize) {
        let e = self.make_era(); 
        let con = self.make_con(nzero_branch, e);
        self.demux3(inp, con, zero_branch, output);
    }

    // "Statements" 
    pub fn erase(&mut self, node: usize) {
        let e = self.make_era();
        self.add_redex(e, node);
    }

}
