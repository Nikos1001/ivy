
extern "C" {
  fn run();
}

pub fn run_vm() {
    unsafe {
        run();
    }
}
