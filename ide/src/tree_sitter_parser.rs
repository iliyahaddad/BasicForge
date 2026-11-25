// Tree-sitter integration for Basic language
use tree_sitter::{Language, Parser, Tree, Node, TreeCursor};
use std::path::Path;

pub struct BasicParser {
    parser: Parser,
    language: Option<Language>,
}

impl BasicParser {
    pub fn new() -> Result<Self, anyhow::Error> {
        // Try to load the Basic language, fall back to None if not available
        // The actual tree-sitter-basic language will be loaded once the grammar is compiled
        let mut parser = Parser::new();
        Ok(Self { parser, language: None })
    }

    pub fn parse(&mut self, source: &str) -> Option<Tree> {
        if self.language.is_some() {
            self.parser.parse(source, None)
        } else {
            None
        }
    }

    pub fn parse_incremental(&mut self, source: &str, old_tree: Option<&Tree>) -> Option<Tree> {
        if self.language.is_some() {
            self.parser.parse(source, old_tree)
        } else {
            None
        }
    }

    pub fn set_language(&mut self, language: Language) -> Result<(), anyhow::Error> {
        self.parser.set_language(&language)?;
        self.language = Some(language);
        Ok(())
    }
}

pub struct BasicTree<'a> {
    tree: &'a Tree,
    source: &'a str,
}

impl<'a> BasicTree<'a> {
    pub fn new(tree: &'a Tree, source: &'a str) -> Self {
        Self { tree, source }
    }

    pub fn root_node(&self) -> Node<'a> {
        self.tree.root_node()
    }

    pub fn get_text(&self, node: Node) -> &str {
        &self.source[node.byte_range()]
    }

    pub fn walk(&self) -> TreeCursor<'a> {
        self.tree.walk()
    }

    pub fn find_node_at_position(&self, line: usize, column: usize) -> Option<Node<'a>> {
        let point = tree_sitter::Point { row: line, column };
        self.root_node().descendant_for_point_range(point, point)
    }

    pub fn get_symbols(&self) -> Vec<SymbolInfo> {
        let mut symbols = Vec::new();
        self.collect_symbols(self.root_node(), &mut symbols);
        symbols
    }

    fn collect_symbols(&self, node: Node, symbols: &mut Vec<SymbolInfo>) {
        let kind = node.kind();
        match kind {
            "module_statement" => {
                if let Some(name_node) = node.child_by_field_name("name") {
                    symbols.push(SymbolInfo {
                        name: self.get_text(name_node).to_string(),
                        kind: SymbolKind::Module,
                        type_name: "Module".to_string(),
                        line: node.start_position().row as u32 + 1,
                        column: node.start_position().column as u32 + 1,
                        file: String::new(),
                    });
                }
            }
            "class_statement" => {
                if let Some(name_node) = node.child_by_field_name("name") {
                    symbols.push(SymbolInfo {
                        name: self.get_text(name_node).to_string(),
                        kind: SymbolKind::Class,
                        type_name: "Class".to_string(),
                        line: node.start_position().row as u32 + 1,
                        column: node.start_position().column as u32 + 1,
                        file: String::new(),
                    });
                }
            }
            "function_statement" => {
                if let Some(name_node) = node.child_by_field_name("name") {
                    symbols.push(SymbolInfo {
                        name: self.get_text(name_node).to_string(),
                        kind: SymbolKind::Function,
                        type_name: "Function".to_string(),
                        line: node.start_position().row as u32 + 1,
                        column: node.start_position().column as u32 + 1,
                        file: String::new(),
                    });
                }
            }
            "sub_statement" => {
                if let Some(name_node) = node.child_by_field_name("name") {
                    symbols.push(SymbolInfo {
                        name: self.get_text(name_node).to_string(),
                        kind: SymbolKind::Procedure,
                        type_name: "Sub".to_string(),
                        line: node.start_position().row as u32 + 1,
                        column: node.start_position().column as u32 + 1,
                        file: String::new(),
                    });
                }
            }
            "property_statement" => {
                if let Some(name_node) = node.child_by_field_name("name") {
                    symbols.push(SymbolInfo {
                        name: self.get_text(name_node).to_string(),
                        kind: SymbolKind::Property,
                        type_name: "Property".to_string(),
                        line: node.start_position().row as u32 + 1,
                        column: node.start_position().column as u32 + 1,
                        file: String::new(),
                    });
                }
            }
            "dim_statement" => {
                for child in node.children(&mut node.walk()) {
                    if child.kind() == "variable_declarator" {
                        if let Some(name_node) = child.child_by_field_name("name") {
                            symbols.push(SymbolInfo {
                                name: self.get_text(name_node).to_string(),
                                kind: SymbolKind::Variable,
                                type_name: "Variable".to_string(),
                                line: child.start_position().row as u32 + 1,
                                column: child.start_position().column as u32 + 1,
                                file: String::new(),
                            });
                        }
                    }
                }
            }
            "enum_statement" => {
                if let Some(name_node) = node.child_by_field_name("name") {
                    symbols.push(SymbolInfo {
                        name: self.get_text(name_node).to_string(),
                        kind: SymbolKind::Enum,
                        type_name: "Enum".to_string(),
                        line: node.start_position().row as u32 + 1,
                        column: node.start_position().column as u32 + 1,
                        file: String::new(),
                    });
                }
            }
            "interface_statement" => {
                if let Some(name_node) = node.child_by_field_name("name") {
                    symbols.push(SymbolInfo {
                        name: self.get_text(name_node).to_string(),
                        kind: SymbolKind::Interface,
                        type_name: "Interface".to_string(),
                        line: node.start_position().row as u32 + 1,
                        column: node.start_position().column as u32 + 1,
                        file: String::new(),
                    });
                }
            }
            _ => {}
        }

        // Recurse into children
        let mut cursor = node.walk();
        for child in node.children(&mut cursor) {
            self.collect_symbols(child, symbols);
        }
    }

    pub fn get_diagnostics(&self) -> Vec<Diagnostic> {
        let mut diagnostics = Vec::new();
        self.collect_diagnostics(self.root_node(), &mut diagnostics);
        diagnostics
    }

    fn collect_diagnostics(&self, node: Node, diagnostics: &mut Vec<Diagnostic>) {
        if node.is_error() || node.is_missing() {
            diagnostics.push(Diagnostic {
                range: Range {
                    start: Position {
                        line: node.start_position().row as u32,
                        character: node.start_position().column as u32,
                    },
                    end: Position {
                        line: node.end_position().row as u32,
                        character: node.end_position().column as u32,
                    },
                },
                severity: Some(1), // Error
                code: None,
                source: Some("tree-sitter".to_string()),
                message: format!("Syntax error at {}", node.kind()),
            });
        }

        let mut cursor = node.walk();
        for child in node.children(&mut cursor) {
            self.collect_diagnostics(child, diagnostics);
        }
    }
}

#[derive(Debug, Clone)]
pub struct SymbolInfo {
    pub name: String,
    pub kind: SymbolKind,
    pub type_name: String,
    pub line: u32,
    pub column: u32,
    pub file: String,
}

#[derive(Debug, Clone, PartialEq)]
pub enum SymbolKind {
    Module,
    Class,
    Function,
    Procedure,
    Property,
    Variable,
    Parameter,
    Enum,
    EnumMember,
    Interface,
    String,
    Integer,
    Double,
    Boolean,
    Long,
    Single,
    Unknown,
}

#[derive(Debug, Clone)]
pub struct Diagnostic {
    pub range: Range,
    pub severity: Option<u32>,
    pub code: Option<serde_json::Value>,
    pub source: Option<String>,
    pub message: String,
}

#[derive(Debug, Clone)]
pub struct Range {
    pub start: Position,
    pub end: Position,
}

#[derive(Debug, Clone)]
pub struct Position {
    pub line: u32,
    pub character: u32,
}

impl Default for BasicParser {
    fn default() -> Self {
        Self::new().expect("Failed to create BasicParser")
    }
}