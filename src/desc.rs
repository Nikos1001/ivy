
use crate::node::Node;

pub struct NetDesc {
    pub nodes: Vec<Node>,
    pub n_vars: usize,
    pub redexes: Vec<(usize, usize)>
}

impl NetDesc {

    pub fn new() -> Self {
        Self {
            nodes: Vec::new(),
            n_vars: 0,
            redexes: Vec::new()
        }
    }

    pub fn add_node(&mut self, node: Node) -> usize {
        self.nodes.push(node);
        self.nodes.len() - 1
    }

    pub fn add_var(&mut self) -> (usize, usize) {
        self.n_vars += 1;
        let v1 = self.add_node(Node::VAR(self.n_vars - 1));
        let v2 = self.add_node(Node::VAR(self.n_vars - 1));
        (v1, v2)
    }

    pub fn add_redex(&mut self, a: usize, b: usize) {
        self.redexes.push((a, b));
    }

}
