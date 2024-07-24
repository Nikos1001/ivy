
#[derive(Clone, Copy, Debug)]
pub enum Node {
    None,
    ERA,
    CON(usize, usize),
    DUP(usize, usize),
    VAR(usize),
    REF(usize)
}
