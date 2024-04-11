use std::{env, fs};

fn main() {
    let Ok(ld_script_path) = env::var("LD_SCRIPT_PATH") else {
        std::process::exit(0);
    };

    let files = fs::read_dir(ld_script_path).unwrap();
    files
        .filter_map(Result::ok)
        .filter(|entry| entry.path().extension().map_or(false, |ext| ext == "ld"))
        .for_each(|f| println!("cargo:rerun-if-changed:={}", f.path().display()));

    println!("cargo:rerun-if-changed=src/boot.s");
    println!("cargo:rerun-if-changed=build.rs");
}
