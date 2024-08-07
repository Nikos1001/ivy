
use ivy_vm::{book::{Book, Operation}, run_vm};

fn main() {

    let mut book = Book::new();
    let main = book.add_def();

    let mut main = book.get_def(main);

    let sum = main.add_operation(
        Operation::Add,
        vec![2.0.into(), 3.0.into()]
    );
    let sum2 = main.add_operation(
        Operation::Add,
        vec![5.0.into(), sum]
    );
    main.set_out(sum2);

    run_vm(book);

}
