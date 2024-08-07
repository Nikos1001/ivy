
#[must_use]
pub struct Aux(pub(crate) u64);

#[must_use]
pub struct Node(pub(crate) u64);

extern "C" {

    fn make_var(var: u64) -> u64;
    fn make_con(aux: u64) -> u64;
    fn make_dup(aux: u64) -> u64;
    fn make_era() -> u64;
    fn make_opi(op: u64, idx: u64) -> u64;
    fn make_opo(op: u64) -> u64;

}

impl Node {

    pub(crate) fn var(var: u64) -> Self {
        Self(unsafe { make_var(var) })
    }

    pub fn con(aux: Aux) -> Self {
        Self(unsafe { make_con(aux.0) })
    }

    pub fn dup(aux: Aux) -> Self {
        Self(unsafe { make_dup(aux.0) })
    }

    pub fn era() -> Self {
        Self(unsafe { make_era() })
    }

    pub(crate) fn opi(op: u64, idx: u64) -> Self {
        Self(unsafe { make_opi(op, idx) })
    }

    pub(crate) fn opo(op: u64) -> Self {
        Self(unsafe { make_opo(op) })
    }

}

impl From<f64> for Node {

    fn from(value: f64) -> Self {
        Self(value.to_bits())
    }

}