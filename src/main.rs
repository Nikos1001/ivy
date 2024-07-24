
use book::Book;
use desc::NetDesc;
use vm::NetVM;

mod vm;
mod build;
mod node;
mod desc;
mod book;

fn build_add(book: &mut Book) -> usize {
    let add_book_idx = book.alloc_desc(); 
    let mut add_desc = NetDesc::new();
    let (out_v1, out_v2) = add_desc.add_var();

    let (dup_a_if, dup_a_pred) = add_desc.add_var();
    let dup_a = add_desc.make_dup(dup_a_if, dup_a_pred);
    let (dup_b_if, dup_b_succ) = add_desc.add_var();
    let dup_b = add_desc.make_dup(dup_b_if, dup_b_succ);
    let (c_v1, c_v2) = add_desc.add_var();
    add_desc.demux3(out_v1, dup_a, dup_b, c_v1);

    let (new_c_v1, new_c_v2) = add_desc.add_var();
    add_desc.church_ifz(dup_a_if, dup_b_if, new_c_v1, c_v2);

    let (pred_out_v1, pred_out_v2) = add_desc.add_var();
    add_desc.church_pred(dup_a_pred, pred_out_v1);

    let succ = add_desc.church_succ(dup_b_succ);

    let mux = add_desc.mux3(pred_out_v2, succ, new_c_v2);
    let add_subcall = add_desc.make_ref(add_book_idx);
    add_desc.add_redex(mux, add_subcall);

    book.set_desc(add_book_idx, add_desc, out_v2);
    add_book_idx
}

fn main() {
    let mut book = Book::new();

    let add_desc_idx = build_add(&mut book);

    let mut main = NetDesc::new();
    let (outvar_v1, outvar_v2) = main.add_var();
    let a = main.church_n(2);
    let b = main.church_n(3);
    let mux = main.mux3(a, b, outvar_v1);
    let add_ref = main.make_ref(add_desc_idx);
    main.add_redex(mux, add_ref);

    let mut net = NetVM::new(&book);
    let outvar = net.load_main_desc(&main, outvar_v2);
    net.dump();

    println!();
    while !net.is_done() {
        net.step();
        net.dump();
        println!();
    }
    println!();
    println!("NET REDUCTION COMPLETE <3");
    println!("OUTPUT VARIABLE INDEX: {}", outvar);
}
