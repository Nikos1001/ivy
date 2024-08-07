
use ivy_vm::{book::{Book, Def, Operation}, node::Node, run_vm};

unsafe fn print_f64s(n_ins: u64, ins: *const Node) -> Node {
    // TODO: output might get broken up if used in a multithreaded way
    print!("f64 debug: ");
    for i in 0..n_ins {
        let node = ins.wrapping_add(i as usize).as_ref().unwrap().copy();
        print!("{} ", node.as_f64().unwrap()); 
    }
    println!();
    Node::era()
}

fn pow2_sum(def: &mut Def, depth: u64) -> Node {
    if depth == 0 {
        1.0.into()
    } else {
        let a = pow2_sum(def, depth - 1);
        let b = pow2_sum(def, depth - 1);
        def.add_operation(Operation::Add, vec![a, b])
    }
}

fn main() {

    let mut book = Book::new();
    let main = book.add_def();

    let mut main = book.get_def(main);

    let sum = pow2_sum(&mut main, 24);
    let call = main.add_operation(
        Operation::Native(print_f64s),
        vec![sum] 
    );
    main.set_out(call);

    run_vm(book);

}
