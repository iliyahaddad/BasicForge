// build.rs - Build script to compile tree-sitter grammar and generate build info
use std::process::Command;
use std::path::Path;

fn main() {
    // Tell cargo to re-run if grammar.js changes
    println!("cargo:rerun-if-changed=tree-sitter-basic/grammar.js");
    println!("cargo:rerun-if-changed=tree-sitter-basic/src/parser.c");
    println!("cargo:rerun-if-changed=tree-sitter-basic/src/scanner.c");
    println!("cargo:rerun-if-changed=VERSION.toml");

    let grammar_dir = Path::new("tree-sitter-basic");
    
    // Check if tree-sitter CLI is available
    let ts_status = Command::new("tree-sitter")
        .arg("--version")
        .status();

    if ts_status.is_ok() {
        // Generate the parser
        let status = Command::new("tree-sitter")
            .arg("generate")
            .current_dir(grammar_dir)
            .status()
            .expect("Failed to run tree-sitter generate");

        if !status.success() {
            panic!("tree-sitter generate failed");
        }
    } else {
        // tree-sitter CLI not available, check if parser.c already exists
        let parser_c = grammar_dir.join("src/parser.c");
        let scanner_c = grammar_dir.join("src/scanner.c");
        
        if !parser_c.exists() || !scanner_c.exists() {
            eprintln!("WARNING: tree-sitter CLI not found and parser.c/scanner.c not present.");
            eprintln!("Run 'tree-sitter generate' in tree-sitter-basic directory manually.");
        }
    }

    // Compile the C files
    let parser_c = grammar_dir.join("src/parser.c");
    let scanner_c = grammar_dir.join("src/scanner.c");
    
    if parser_c.exists() && scanner_c.exists() {
        cc::Build::new()
            .file(&parser_c)
            .file(&scanner_c)
            .include(grammar_dir.join("src"))
            .compile("tree-sitter-basic");
    } else {
        eprintln!("WARNING: parser.c/scanner.c not found, tree-sitter basic parser not compiled");
    }

    // Generate build info using vergen
    if let Err(e) = vergen::vergen(vergen::Config::default()) {
        eprintln!("WARNING: vergen failed: {}", e);
    }
}