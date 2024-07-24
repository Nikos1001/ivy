
use crate::desc::NetDesc;

pub struct Book {
    pub descs: Vec<(NetDesc, usize)>
}

impl Book {

    pub fn new() -> Self {
        Self {
            descs: Vec::new()
        }
    }

    // Allocation and setting is split to allow for recursion
    pub fn alloc_desc(&mut self) -> usize {
        self.descs.push((NetDesc::new(), 0));
        self.descs.len() - 1
    }

    pub fn set_desc(&mut self, idx: usize, desc: NetDesc, desc_principal_out: usize) {
        self.descs[idx] = (desc, desc_principal_out);
    }

    pub fn get(&self, idx: usize) -> (&NetDesc, usize) {
        (&self.descs[idx].0, self.descs[idx].1)
    }

}
