
use std::os::raw::c_void;

pub mod book;
pub mod node;

extern "C" {
    fn run(book: *mut c_void);
}

pub fn run_vm(book: book::Book) {
    unsafe {
        run(book.book);
    }
}
