use std::env;
use std::path::PathBuf;

fn main() {
    let linker_script = PathBuf::from(env::var("CARGO_MANIFEST_DIR").unwrap()).join("linker.ld");
    println!("cargo:rustc-link-arg=-T{}", linker_script.display());
}
