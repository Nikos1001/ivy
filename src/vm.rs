
use crate::{book::Book, desc::NetDesc, node::Node};

pub struct NetVM<'a> {
    nodes: Vec<Node>,
    vars: Vec<usize>,

    free_nodes: Vec<usize>,
    free_vars: Vec<usize>,
    redex_stack: Vec<(usize, usize)>,

    book: &'a Book
}

impl<'a> NetVM<'a> {

    pub fn new(book: &'a Book) -> Self {
        Self {
            nodes: vec![Node::None],
            vars: Vec::new(),

            free_nodes: Vec::new(),
            free_vars: Vec::new(),
            redex_stack: Vec::new(),
            book 
        }
    }

    fn add_desc(&mut self, desc: &NetDesc) -> (Vec<usize>, Vec<usize>) {
        let var_idx_map: Vec<usize> = (0..desc.n_vars).map(|_| self.alloc_var()).collect(); 
        let node_idx_map: Vec<usize> = desc.nodes.iter().map(|node| self.alloc_node(match node {
            Node::VAR(var_idx) => Node::VAR(var_idx_map[*var_idx]),
            node => *node
        })).collect();
        for (a, b) in &desc.redexes {
            self.add_redex(node_idx_map[*a], node_idx_map[*b]);
        }
        (node_idx_map, var_idx_map)
    }

    pub fn load_main_desc(&mut self, desc: &NetDesc, out_node: usize) -> usize {
        let (node_idx_map, _) = self.add_desc(desc);
        let var = self.alloc_var();
        let var_node = self.alloc_node(Node::VAR(var));
        self.add_redex(var_node, node_idx_map[out_node]);
        var
    }

    pub fn dump(&self) {
        println!("==== NODES       ====");
        for i in 1..self.nodes.len() {
            println!("{:0>6} | {:?}", i, self.nodes[i]);
        }
        println!("==== VARS        ====");
        for i in 0..self.vars.len() {
            print!("{:0>6} | ", i);
            if self.vars[i] == usize::MAX {
                println!("UNLINKED");
            } else if self.vars[i] == 0 {
                println!("NOT IN USE");
            } else {
                println!("{:?}", self.vars[i]);
            }
        }
        println!("==== REDEX STACK ====");
        for (a, b) in self.redex_stack.iter().rev() {
            println!("({}, {})", *a, *b);
        }
    }
    
    pub fn is_done(&self) -> bool {
        self.redex_stack.is_empty()
    }

    pub fn step(&mut self) {
        let Some((a, b)) = self.redex_stack.pop() else { return; };

        match self.nodes[a] {
            Node::ERA => {
                match self.nodes[b] {
                    Node::None => panic!("should never interact with none node."),
                    Node::ERA => self.interact_erase(a, b),
                    Node::CON(_, _) => self.interact_ec(a, b),
                    Node::DUP(_, _) => self.interact_ed(a, b),
                    Node::VAR(_) => self.interact_var(a, b),
                    Node::REF(_) => self.interact_erase(a, b) 
                }
            },
            Node::CON(_, _) => {
                match self.nodes[b] {
                    Node::None => panic!("should never interact with none node."),
                    Node::ERA => self.interact_ec(b, a),
                    Node::CON(_, _) => self.interact_cc(a, b),
                    Node::DUP(_, _) => self.interact_cd(a, b),
                    Node::VAR(_) => self.interact_var(a, b),
                    Node::REF(_) => self.interact_ref(a, b),
                }
            },
            Node::DUP(_, _) => {
                match self.nodes[b] {
                    Node::None => panic!("should never interact with none node."),
                    Node::ERA => self.interact_ed(b, a),
                    Node::CON(_, _) => self.interact_cd(b, a),
                    Node::DUP(_, _) => self.interact_dd(a, b),
                    Node::VAR(_) => self.interact_var(a, b),
                    Node::REF(_) => self.interact_ref(a, b),
                } 
            },
            Node::VAR(_) => self.interact_var(b, a),
            Node::REF(_) => {
                match self.nodes[b] {
                    Node::None => panic!("should never interact with none node."),
                    Node::ERA => self.interact_erase(a, b),
                    Node::VAR(_) => self.interact_var(a, b),
                    _ => self.interact_ref(b, a)
                }
            },
            Node::None => panic!("should never interact with none node."),
        }
    }

    pub fn alloc_node(&mut self, node: Node) -> usize {
        if let Some(idx) = self.free_nodes.pop() {
            self.nodes[idx] = node;
            idx
        } else {
            self.nodes.push(node);
            self.nodes.len() - 1
        }
    }
    
    pub fn free_node(&mut self, node: usize) {
        if let Node::None = self.nodes[node] {
            panic!("node {} already freed.", node);
        }
        self.nodes[node] = Node::None;
        self.free_nodes.push(node);
    }

    pub fn alloc_var(&mut self) -> usize {
        if let Some(idx) = self.free_vars.pop() {
            self.vars[idx] = usize::MAX;
            idx
        } else {
            self.vars.push(usize::MAX);
            self.vars.len() - 1
        }
    }

    pub fn free_var(&mut self, var: usize) {
        self.vars[var] = 0;
        self.free_vars.push(var);
    }
    
    pub fn interact_erase(&mut self, a: usize, b: usize) {
        self.free_node(a);
        self.free_node(b);
    }

    pub fn add_redex(&mut self, a: usize, b: usize) {
        self.redex_stack.push((a, b));
    }

    pub fn interact_ec(&mut self, e: usize, c: usize) {
        let Node::CON(con_a, con_b) = self.nodes[c] else { panic!("c should be CON node") };
        self.free_node(c);
        let new_e = self.alloc_node(Node::ERA);
        self.redex_stack.push((e, con_a));
        self.redex_stack.push((new_e, con_b));
    }

    pub fn interact_ed(&mut self, e: usize, d: usize) {
        let Node::DUP(dup_a, dup_b) = self.nodes[d] else { panic!("d should be DUP node") };
        self.free_node(d);
        let new_e = self.alloc_node(Node::ERA);
        self.redex_stack.push((e, dup_a));
        self.redex_stack.push((new_e, dup_b));
    }

    pub fn interact_cc(&mut self, a: usize, b: usize) {
        let Node::CON(con_a0, con_a1) = self.nodes[a] else { panic!("a should be CON node") };
        let Node::CON(con_b0, con_b1) = self.nodes[b] else { panic!("a should be CON node") };
        self.free_node(a);
        self.free_node(b);
        self.redex_stack.push((con_a0, con_b1));
        self.redex_stack.push((con_a1, con_b0));
    }

    pub fn interact_cd(&mut self, c: usize, d: usize) {
        let Node::CON(con_0, con_1) = self.nodes[c] else { panic!("c should be a CON node") };
        let Node::DUP(dup_0, dup_1) = self.nodes[d] else { panic!("d should be a DUP node") };
        self.free_node(c);
        self.free_node(d);

        let v0 = self.alloc_var();
        let v0 = self.alloc_node(Node::VAR(v0));

        let v1 = self.alloc_var();
        let v1 = self.alloc_node(Node::VAR(v1));

        let v2 = self.alloc_var();
        let v2 = self.alloc_node(Node::VAR(v2));

        let v3 = self.alloc_var();
        let v3 = self.alloc_node(Node::VAR(v3));

        let d0 = self.alloc_node(Node::DUP(v1, v0));
        let d1 = self.alloc_node(Node::DUP(v3, v2));
        let c0 = self.alloc_node(Node::CON(v0, v2));
        let c1 = self.alloc_node(Node::CON(v1, v3));

        self.redex_stack.push((d0, con_0));
        self.redex_stack.push((d1, con_1));
        self.redex_stack.push((c0, dup_1));
        self.redex_stack.push((c1, dup_0));
    }
    
    pub fn interact_dd(&mut self, a: usize, b: usize) {
        let Node::DUP(dup_a0, dup_a1) = self.nodes[a] else { panic!("a should be DUP node") };
        let Node::DUP(dup_b0, dup_b1) = self.nodes[b] else { panic!("a should be DUP node") };
        self.free_node(a);
        self.free_node(b);
        self.redex_stack.push((dup_a0, dup_b0));
        self.redex_stack.push((dup_a1, dup_b1));
    }

    pub fn interact_var(&mut self, node: usize, var: usize) {
        let Node::VAR(var_idx) = self.nodes[var] else { panic!("var should be VAR node") };
        if self.vars[var_idx] == usize::MAX { // First interaction, variable linking
            self.vars[var_idx] = node;
            self.free_node(var);
        } else { // Second interaction, variable resolution
            self.redex_stack.push((node, self.vars[var_idx]));
            self.free_node(var);
            self.free_var(var_idx);
        }
    }

    pub fn interact_ref(&mut self, node: usize, ref_node: usize) {
        let Node::REF(desc_idx) = self.nodes[ref_node] else { panic!("ref_node should be REF node") };
        self.free_node(ref_node);
        let (desc, out_idx) = self.book.get(desc_idx);
        let (node_idx_map, _) = self.add_desc(desc);
        self.add_redex(node_idx_map[out_idx], node);
    }

}
