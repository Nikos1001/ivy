
use ivy_vm::{book::Book, node::Node, run_vm};

fn main() {

    let mut book = Book::new();
    let main = book.add_def();

    let mut main = book.get_def(main);
    main.set_out(42.0.into());

    let (v0, v1) = main.add_var();
    let con = main.con(&[v0, Node::era()]);
    let dup = main.dup(&[v1, Node::era()]);
    main.add_redex(con, dup);


    run_vm(book);

}
