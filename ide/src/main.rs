mod app;
mod build_info;
mod debugger;
mod designer;
mod editor;
mod language;
mod lsp;
mod project;
mod salsa_db;
mod tree_sitter_parser;
mod ui;

use std::env;

fn main() {
    let args: Vec<String> = env::args().collect();
    
    if args.iter().any(|a| a == "--version" || a == "-v") {
        println!("{}", build_info::full_version_string());
        return;
    }
    
    if args.iter().any(|a| a == "--lsp") {
        println!("Starting BasicForge LSP server...");
        println!("LSP server mode not yet fully implemented");
        return;
    }
    
    let options = eframe::NativeOptions {
        viewport: egui::ViewportBuilder::default()
            .with_inner_size(egui::vec2(1200.0, 800.0))
            .with_min_inner_size(egui::vec2(800.0, 600.0)),
        ..Default::default()
    };
    let _ = eframe::run_native(
        "BasicForge IDE",
        options,
        Box::new(|cc| Ok(Box::new(app::BasicForgeApp::new(cc)))),
    );
}
