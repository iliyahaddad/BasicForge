use serde::{Deserialize, Serialize};
use serde_json::Value;
use std::collections::HashMap;

#[derive(Debug, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
struct LSPRequest {
    jsonrpc: String,
    id: u64,
    method: String,
    params: Value,
}

#[derive(Debug, Deserialize)]
#[serde(rename_all = "camelCase")]
struct LSPResponse {
    jsonrpc: String,
    id: u64,
    #[serde(skip_serializing_if = "Option::is_none")]
    result: Option<Value>,
    #[serde(skip_serializing_if = "Option::is_none")]
    error: Option<Value>,
}

#[derive(Debug, Deserialize)]
#[serde(rename_all = "camelCase")]
struct LSPNotification {
    jsonrpc: String,
    method: String,
    params: Value,
}

#[derive(Debug, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
struct Diagnostic {
    range: Range,
    severity: Option<u32>,
    code: Option<Value>,
    source: Option<String>,
    message: String,
}

#[derive(Debug, Serialize, Deserialize, Clone)]
#[serde(rename_all = "camelCase")]
struct Range {
    start: Position,
    end: Position,
}

#[derive(Debug, Serialize, Deserialize, Clone)]
#[serde(rename_all = "camelCase")]
struct Position {
    line: u32,
    character: u32,
}

#[derive(Debug, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
struct PublishDiagnosticsParams {
    uri: String,
    diagnostics: Vec<Diagnostic>,
}

#[derive(Debug, Serialize, Deserialize, Clone)]
#[serde(rename_all = "camelCase")]
pub struct CompletionItem {
    pub label: String,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub kind: Option<u32>,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub detail: Option<String>,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub documentation: Option<String>,
}

#[derive(Debug, Serialize, Deserialize, Clone)]
#[serde(rename_all = "camelCase")]
pub struct Location {
    pub uri: String,
    pub range: Range,
}

#[derive(Debug, Clone)]
pub struct EditorDiagnostic {
    pub file: String,
    pub line: u32,
    pub column: u32,
    pub end_line: u32,
    pub end_column: u32,
    pub severity: u32,
    pub message: String,
    pub source: String,
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

impl std::fmt::Display for SymbolKind {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        let s = match self {
            SymbolKind::Module => "Module",
            SymbolKind::Class => "Class",
            SymbolKind::Function => "Function",
            SymbolKind::Procedure => "Procedure",
            SymbolKind::Property => "Property",
            SymbolKind::Variable => "Variable",
            SymbolKind::Parameter => "Parameter",
            SymbolKind::Enum => "Enum",
            SymbolKind::EnumMember => "EnumMember",
            SymbolKind::Interface => "Interface",
            SymbolKind::String => "String",
            SymbolKind::Integer => "Integer",
            SymbolKind::Double => "Double",
            SymbolKind::Boolean => "Boolean",
            SymbolKind::Long => "Long",
            SymbolKind::Single => "Single",
            SymbolKind::Unknown => "Unknown",
        };
        write!(f, "{}", s)
    }
}

pub struct SymbolEntry {
    pub name: String,
    pub kind: SymbolKind,
    pub type_name: String,
    pub line: u32,
    pub column: u32,
    pub file: String,
    pub is_public: bool,
}

impl Clone for SymbolEntry {
    fn clone(&self) -> Self {
        SymbolEntry {
            name: self.name.clone(),
            kind: self.kind.clone(),
            type_name: self.type_name.clone(),
            line: self.line,
            column: self.column,
            file: self.file.clone(),
            is_public: self.is_public,
        }
    }
}

pub struct SymbolTable {
    pub modules: Vec<SymbolEntry>,
    pub classes: Vec<SymbolEntry>,
    pub functions: Vec<SymbolEntry>,
    pub procedures: Vec<SymbolEntry>,
    pub properties: Vec<SymbolEntry>,
    pub variables: Vec<SymbolEntry>,
    pub parameters: Vec<SymbolEntry>,
    pub enums: Vec<SymbolEntry>,
    pub enum_members: Vec<SymbolEntry>,
    pub types: Vec<SymbolEntry>,
    pub type_aliases: Vec<SymbolEntry>,
    pub events: Vec<SymbolEntry>,
    pub interfaces: Vec<SymbolEntry>,
}

impl Default for SymbolTable {
    fn default() -> Self {
        Self {
            modules: Vec::new(),
            classes: Vec::new(),
            functions: Vec::new(),
            procedures: Vec::new(),
            properties: Vec::new(),
            variables: Vec::new(),
            parameters: Vec::new(),
            enums: Vec::new(),
            enum_members: Vec::new(),
            types: Vec::new(),
            type_aliases: Vec::new(),
            events: Vec::new(),
            interfaces: Vec::new(),
        }
    }
}

pub struct SymbolTableFile {
    pub file_path: String,
    pub modules: Vec<SymbolEntry>,
    pub classes: Vec<SymbolEntry>,
    pub functions: Vec<SymbolEntry>,
    pub procedures: Vec<SymbolEntry>,
    pub properties: Vec<SymbolEntry>,
    pub variables: Vec<SymbolEntry>,
    pub parameters: Vec<SymbolEntry>,
    pub enums: Vec<SymbolEntry>,
    pub enum_members: Vec<SymbolEntry>,
    pub types: Vec<SymbolEntry>,
    pub type_aliases: Vec<SymbolEntry>,
    pub events: Vec<SymbolEntry>,
    pub interfaces: Vec<SymbolEntry>,
}

impl Default for SymbolTableFile {
    fn default() -> Self {
        Self {
            file_path: String::new(),
            modules: Vec::new(),
            classes: Vec::new(),
            functions: Vec::new(),
            procedures: Vec::new(),
            properties: Vec::new(),
            variables: Vec::new(),
            parameters: Vec::new(),
            enums: Vec::new(),
            enum_members: Vec::new(),
            types: Vec::new(),
            type_aliases: Vec::new(),
            events: Vec::new(),
            interfaces: Vec::new(),
        }
    }
}

pub struct LanguageServer {
    pub symbol_table: SymbolTable,
    pub file_symbols: HashMap<String, SymbolTableFile>,
    pub diagnostics: Vec<EditorDiagnostic>,
    pub keywords: Vec<String>,
    pub builtin_types: Vec<String>,
    pub builtin_functions: Vec<String>,
}

impl LanguageServer {
    pub fn new() -> Self {
        Self {
            symbol_table: SymbolTable::default(),
            file_symbols: HashMap::new(),
            diagnostics: Vec::new(),
            keywords: Self::get_keywords(),
            builtin_types: Self::get_builtin_types(),
            builtin_functions: Self::get_builtin_functions(),
        }
    }

    fn get_keywords() -> Vec<String> {
        vec![
            "And", "As", "Boolean", "ByVal", "ByRef", "Case", "Class", "Const", "Dim",
            "Do", "Else", "ElseIf", "End", "EndIf", "EndSub", "EndFunction", "EndClass",
            "EndModule", "EndSelect", "EndWhile", "Enum", "False", "For", "Each", "In",
            "Friend", "Function", "Get", "If", "Implements", "Inherits", "Interface",
            "Is", "Let", "Like", "Long", "Loop", "Me", "Mod", "New", "Next", "Not",
            "Nothing", "On", "Option", "Or", "Overloads", "Overridable", "Overrides",
            "ParamArray", "Preserve", "Private", "Property", "Public", "RaiseEvent",
            "ReadOnly", "RemoveHandler", "Resume", "Return", "Select", "Set", "Shadows",
            "Shared", "Single", "Static", "Step", "String", "Structure", "Sub",
            "SyncLock", "Then", "Throw", "To", "True", "Try", "TypeOf", "Using",
            "Variant", "When", "While", "Wend", "With", "WithEvents", "WriteOnly",
            "Xor", "Print", "Return",
        ]
        .into_iter()
        .map(|s| s.to_string())
        .collect()
    }

    fn get_builtin_types() -> Vec<String> {
        vec![
            "Integer", "Long", "Single", "Double", "Boolean", "String", "Void",
        ]
        .into_iter()
        .map(|s| s.to_string())
        .collect()
    }

    fn get_builtin_functions() -> Vec<String> {
        vec![
            "Print", "Len", "Trim", "Left", "Right", "Mid", "InStr", "Replace", "Format",
            "Split", "Join", "CInt", "CLng", "CDbl", "CStr", "CBool",
        ]
        .into_iter()
        .map(|s| s.to_string())
        .collect()
    }

    pub fn analyze_file(&mut self, file_path: &str, content: &str) {
        let mut file_symbols = SymbolTableFile {
            file_path: file_path.to_string(),
            modules: Vec::new(),
            classes: Vec::new(),
            functions: Vec::new(),
            procedures: Vec::new(),
            properties: Vec::new(),
            variables: Vec::new(),
            parameters: Vec::new(),
            enums: Vec::new(),
            enum_members: Vec::new(),
            types: Vec::new(),
            type_aliases: Vec::new(),
            events: Vec::new(),
            interfaces: Vec::new(),
        };

        let lines: Vec<&str> = content.lines().collect();
        let mut current_class: Option<String> = None;
        let mut current_module: Option<String> = None;
        let mut current_function: Option<String> = None;
        let mut current_procedure: Option<String> = None;

        for (line_idx, line) in lines.iter().enumerate() {
            let line_num = (line_idx + 1) as u32;
            let trimmed = line.trim();

            if trimmed.is_empty() || trimmed.starts_with('\'') {
                continue;
            }

            if trimmed.starts_with("Module ") || trimmed.starts_with("module ") {
                let name = trimmed.split_whitespace().nth(1).unwrap_or("").to_string();
                current_module = Some(name.clone());
                let symbol = SymbolEntry {
                    name: name.clone(),
                    kind: SymbolKind::Module,
                    type_name: "Module".to_string(),
                    line: line_num,
                    column: 1,
                    file: file_path.to_string(),
                    is_public: true,
                };
                self.symbol_table.modules.push(symbol.clone());
                file_symbols.modules.push(symbol);
                continue;
            }

            if trimmed.starts_with("Class ") || trimmed.starts_with("class ") {
                let name = trimmed.split_whitespace().nth(1).unwrap_or("").to_string();
                current_class = Some(name.clone());
                let symbol = SymbolEntry {
                    name: name.clone(),
                    kind: SymbolKind::Class,
                    type_name: "Class".to_string(),
                    line: line_num,
                    column: 1,
                    file: file_path.to_string(),
                    is_public: true,
                };
                self.symbol_table.classes.push(symbol.clone());
                file_symbols.classes.push(symbol.clone());
                continue;
            }

            if trimmed.starts_with("Sub ") || trimmed.starts_with("sub ") {
                let parts: Vec<&str> = trimmed.split_whitespace().collect();
                let name = parts.get(1).unwrap_or(&"").to_string();
                let symbol = SymbolEntry {
                    name: name.clone(),
                    kind: SymbolKind::Procedure,
                    type_name: "Sub".to_string(),
                    line: line_num,
                    column: 1,
                    file: file_path.to_string(),
                    is_public: true,
                };
                self.symbol_table.procedures.push(symbol.clone());
                file_symbols.procedures.push(symbol.clone());
                current_function = Some(name.clone());
                current_procedure = Some(name.clone());
                continue;
            }

            if trimmed.starts_with("Function ") || trimmed.starts_with("function ") {
                let parts: Vec<&str> = trimmed.split_whitespace().collect();
                let name = parts.get(1).unwrap_or(&"").to_string();
                let symbol = SymbolEntry {
                    name: name.clone(),
                    kind: SymbolKind::Function,
                    type_name: "Function".to_string(),
                    line: line_num,
                    column: 1,
                    file: file_path.to_string(),
                    is_public: true,
                };
                self.symbol_table.functions.push(symbol.clone());
                file_symbols.functions.push(symbol.clone());
                current_function = Some(name.clone());
                continue;
            }

            if trimmed.starts_with("Property ") || trimmed.starts_with("property ") {
                let name = trimmed.split_whitespace().nth(1).unwrap_or("").to_string();
                let symbol = SymbolEntry {
                    name: name.clone(),
                    kind: SymbolKind::Property,
                    type_name: "Property".to_string(),
                    line: line_num,
                    column: 1,
                    file: file_path.to_string(),
                    is_public: true,
                };
                file_symbols.properties.push(symbol.clone());
                self.symbol_table.properties.push(symbol.clone());
                continue;
            }

            if trimmed.starts_with("Dim ") || trimmed.starts_with("dim ") {
                let var_name = trimmed.split_whitespace().nth(1).unwrap_or("").to_string();
                let symbol = SymbolEntry {
                    name: var_name.clone(),
                    kind: SymbolKind::Variable,
                    type_name: "Variant".to_string(),
                    line: line_num,
                    column: 1,
                    file: file_path.to_string(),
                    is_public: false,
                };
                file_symbols.variables.push(symbol.clone());
                self.symbol_table.variables.push(symbol.clone());
                continue;
            }

            if trimmed.starts_with("Enum ") || trimmed.starts_with("enum ") {
                let name = trimmed.split_whitespace().nth(1).unwrap_or("").to_string();
                let symbol = SymbolEntry {
                    name: name.clone(),
                    kind: SymbolKind::Enum,
                    type_name: "Enum".to_string(),
                    line: line_num,
                    column: 1,
                    file: file_path.to_string(),
                    is_public: true,
                };
                file_symbols.enums.push(symbol.clone());
                self.symbol_table.enums.push(symbol.clone());
                continue;
            }

            if trimmed.starts_with("Interface ") || trimmed.starts_with("interface ") {
                let name = trimmed.split_whitespace().nth(1).unwrap_or("").to_string();
                let symbol = SymbolEntry {
                    name: name.clone(),
                    kind: SymbolKind::Interface,
                    type_name: "Interface".to_string(),
                    line: line_num,
                    column: 1,
                    file: file_path.to_string(),
                    is_public: true,
                };
                file_symbols.interfaces.push(symbol.clone());
                self.symbol_table.interfaces.push(symbol.clone());
                continue;
            }
        }

        self.file_symbols.insert(file_path.to_string(), file_symbols);
    }

    pub fn analyze_source(&mut self, source: &str, file_path: &str) {
        self.analyze_file(file_path, source);
    }

    pub fn get_completion_items(&self, text: &str) -> Vec<String> {
        let keywords = &self.keywords;
        let builtin_types = &self.builtin_types;
        let builtin_functions = &self.builtin_functions;

        let mut results: Vec<String> = keywords.iter().cloned().collect();
        results.extend(builtin_types.iter().cloned());
        results.extend(builtin_functions.iter().cloned());

        let input = text.trim();
        if input.is_empty() {
            return results;
        }

        results.into_iter()
            .filter(|item| item.to_lowercase().starts_with(&input.to_lowercase()))
            .collect()
    }

    pub fn get_diagnostics(&self) -> Vec<EditorDiagnostic> {
        self.diagnostics.clone()
    }

    pub fn get_hover_info(&self, symbol_name: &str) -> Option<String> {
        for symbol in &self.symbol_table.procedures {
            if symbol.name == symbol_name {
                return Some(format!("{}: {}", symbol.name, symbol.type_name));
            }
        }
        for symbol in &self.symbol_table.functions {
            if symbol.name == symbol_name {
                return Some(format!("{}: {} (line {}, col {})", symbol.name, symbol.type_name, symbol.line, symbol.column));
            }
        }
        None
    }

    pub fn find_definition(&self, symbol_name: &str) -> Option<SymbolInfo> {
        for symbol in &self.symbol_table.procedures {
            if symbol.name == symbol_name {
                return Some(SymbolInfo {
                    name: symbol.name.clone(),
                    kind: symbol.kind.clone(),
                    type_name: symbol.type_name.clone(),
                    line: symbol.line,
                    column: symbol.column,
                    file: symbol.file.clone(),
                });
            }
        }
        for symbol in &self.symbol_table.functions {
            if symbol.name == symbol_name {
                return Some(SymbolInfo {
                    name: symbol.name.clone(),
                    kind: symbol.kind.clone(),
                    type_name: symbol.type_name.clone(),
                    line: symbol.line,
                    column: symbol.column,
                    file: symbol.file.clone(),
                });
            }
        }
        for symbol in &self.symbol_table.classes {
            if symbol.name == symbol_name {
                return Some(SymbolInfo {
                    name: symbol.name.clone(),
                    kind: symbol.kind.clone(),
                    type_name: symbol.type_name.clone(),
                    line: symbol.line,
                    column: symbol.column,
                    file: symbol.file.clone(),
                });
            }
        }
        for symbol in &self.symbol_table.variables {
            if symbol.name == symbol_name {
                return Some(SymbolInfo {
                    name: symbol.name.clone(),
                    kind: symbol.kind.clone(),
                    type_name: symbol.type_name.clone(),
                    line: symbol.line,
                    column: symbol.column,
                    file: symbol.file.clone(),
                });
            }
        }
        None
    }
}

pub fn analyze_source_code(source: &str, file_path: &str) -> Vec<EditorDiagnostic> {
    let mut diagnostics = Vec::new();
    let lines: Vec<&str> = source.lines().collect();

    for (line_idx, line) in lines.iter().enumerate() {
        let trimmed = line.trim();
        if trimmed.is_empty() || trimmed.starts_with('\'') {
            continue;
        }

        if trimmed.starts_with("Dim ") || trimmed.starts_with("dim ") {
            let parts: Vec<&str> = trimmed.split_whitespace().collect();
            if parts.len() >= 2 {
                let var_name = parts.get(1).unwrap_or(&"");
                if var_name.chars().next().map_or(false, |c| c.is_ascii_digit()) {
                    diagnostics.push(EditorDiagnostic {
                        file: file_path.to_string(),
                        line: line_idx as u32,
                        column: 0,
                        end_line: line_idx as u32,
                        end_column: var_name.len() as u32,
                        severity: 2,
                        message: format!("Variable name '{}' cannot start with a digit", var_name),
                        source: "bfc".to_string(),
                    });
                }
            }
        }
    }

    diagnostics
}

pub fn syntax_check(source: &str) -> Vec<EditorDiagnostic> {
    let mut diagnostics = Vec::new();
    let mut brace_count: i32 = 0;
    let mut paren_count: i32 = 0;

    for (line_idx, line) in source.lines().enumerate() {
        for ch in line.chars() {
            match ch {
                '(' => paren_count += 1,
                ')' => paren_count = paren_count.saturating_sub(1),
                '{' => brace_count += 1,
                '}' => brace_count = brace_count.saturating_sub(1),
                _ => {}
            }
        }

        if paren_count < 0 || brace_count < 0 {
            diagnostics.push(EditorDiagnostic {
                file: "unknown".to_string(),
                line: line_idx as u32,
                column: 0,
                end_line: line_idx as u32,
                end_column: line.len() as u32,
                severity: 2,
                message: "Unmatched closing bracket".to_string(),
                source: "bfc".to_string(),
            });
        }
    }

    if paren_count > 0 || brace_count > 0 {
        diagnostics.push(EditorDiagnostic {
            file: "unknown".to_string(),
            line: source.lines().count() as u32 - 1,
            column: 0,
            end_line: source.lines().count() as u32,
            end_column: 0,
            severity: 2,
            message: "Unmatched opening bracket".to_string(),
            source: "bfc".to_string(),
        });
    }

    diagnostics
}

pub fn find_symbol_at_position(source: &str, line: usize, column: usize) -> Option<SymbolInfo> {
    let lines: Vec<&str> = source.lines().collect();
    if line >= lines.len() {
        return None;
    }

    let line_str = lines[line];
    if column >= line_str.len() {
        return None;
    }

    let char_pos = column;
    let char_at = line_str.chars().nth(char_pos);

    let mut word_start_idx = char_pos;
    for (i, c) in line_str[..char_pos].char_indices().rev() {
        if !c.is_ascii_alphanumeric() && c != '_' {
            word_start_idx = i + 1;
            break;
        }
        if i == 0 {
            word_start_idx = 0;
        }
    }

    let mut word_end_idx = char_pos;
    for (i, c) in line_str[char_pos..].char_indices() {
        if !c.is_ascii_alphanumeric() && c != '_' {
            word_end_idx = char_pos + i;
            break;
        }
        if i == line_str[char_pos..].len() - 1 {
            word_end_idx = line_str.len();
        }
    }

    if word_start_idx >= word_end_idx {
        return None;
    }

    let word = &line_str[word_start_idx..word_end_idx];
    let word_upper = word.to_uppercase();

    let keywords = [
        "SUB", "FUNCTION", "CLASS", "MODULE", "ENUM", "INTERFACE", "PROPERTY",
        "DIM", "AS", "INTEGER", "LONG", "SINGLE", "DOUBLE", "BOOLEAN", "STRING",
        "PUBLIC", "PRIVATE", "PROTECTED", "FRIEND", "SHARED", "STATIC", "CONST",
        "IF", "THEN", "ELSE", "FOR", "NEXT", "WHILE", "WEND", "DO", "LOOP",
        "SELECT", "CASE", "TRY", "CATCH", "FINALLY", "THROW", "RETURN",
        "CALL", "NEW", "ME", "AS",
    ];

    if keywords.contains(&word_upper.as_str()) {
        let kind = match word_upper.as_str() {
            "SUB" | "FUNCTION" => SymbolKind::Function,
            "CLASS" => SymbolKind::Class,
            "MODULE" => SymbolKind::Module,
            "ENUM" => SymbolKind::Enum,
            "INTERFACE" => SymbolKind::Interface,
            "PROPERTY" => SymbolKind::Property,
            _ => SymbolKind::Procedure,
        };

        return Some(SymbolInfo {
            name: word.to_string(),
            kind,
            type_name: word_upper.clone(),
            line: line as u32,
            column: word_start_idx as u32,
            file: "".to_string(),
        });
    }

    None
}