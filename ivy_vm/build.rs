
fn main() {
    println!("cargo:rerun-if-changed=src/vm/*.c");
    println!("cargo:rerun-if-changed=src/vm/*.h");

    let _ = cc::Build::new()
        .file("src/vm/vm.c") 
        .file("src/vm/node.c") 
        .file("src/vm/run.c") 
        .try_compile("vm");

}
