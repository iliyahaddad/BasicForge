use tree_sitter::{Language};

extern "C" {
    fn tree_sitter_basicforge() -> *const Language;
}

pub fn language() -> Language {
    unsafe { Language::from_raw(tree_sitter_basicforge()) }
}