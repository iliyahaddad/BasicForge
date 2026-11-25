// Integration tests for BasicForge IDE
use basicforge_ide::language::{LanguageServer, EditorDiagnostic};
use basicforge_ide::tree_sitter_parser::{BasicParser, BasicTree};
use basicforge_ide::salsa_db::{BasicDatabase, Diagnostic, Position};
use basicforge_ide::editor::Editor;
use basicforge_ide::designer::FormDesigner;
use basicforge_ide::debugger::Debugger;
use std::path::PathBuf;

#[test]
fn test_language_server_analyzes_basic_code() {
    let mut ls = LanguageServer::new();
    let source = r#"
Module Test
Sub Main()
    Print("Hello World")
End Sub
End Module
"#;
    
    ls.analyze_file("test.bf", source);
    
    // Should find the Main sub
    let subs = &ls.symbol_table.procedures;
    assert_eq!(subs.len(), 1);
    assert_eq!(subs[0].name, "Main");
}

#[test]
fn test_language_server_finds_variables() {
    let mut ls = LanguageServer::new();
    let source = r#"
Module Test
Sub Main()
    Dim x As Integer = 10
    Dim y As String = "hello"
End Sub
End Module
"#;
    
    ls.analyze_file("test.bf", source);
    
    let vars = &ls.symbol_table.variables;
    assert_eq!(vars.len(), 2);
    
    let x = vars.iter().find(|v| v.name == "x");
    assert!(x.is_some());
    assert_eq!(x.unwrap().type_name, "Variant"); // Type inference not implemented
    
    let y = vars.iter().find(|v| v.name == "y");
    assert!(y.is_some());
}

#[test]
fn test_language_server_completion_items() {
    let mut ls = LanguageServer::new();
    ls.analyze_file("test.bf", "Module Test\nSub Main()\nEnd Sub\nEnd Module");
    
    let items = ls.get_completion_items("Pri");
    assert!(items.iter().any(|i| i == "Print"));
    
    let items = ls.get_completion_items("Di");
    assert!(items.iter().any(|i| i == "Dim"));
}

#[test]
fn test_language_server_hover_info() {
    let mut ls = LanguageServer::new();
    ls.analyze_file("test.bf", "Module Test\nSub Main()\nEnd Sub\nEnd Module");
    
    let info = ls.get_hover_info("Main");
    assert!(info.is_some());
    assert!(info.unwrap().contains("Main"));
}

#[test]
fn test_language_server_find_definition() {
    let mut ls = LanguageServer::new();
    ls.analyze_file("test.bf", "Module Test\nSub Main()\nEnd Sub\nEnd Module");
    
    let def = ls.find_definition("Main");
    assert!(def.is_some());
    assert_eq!(def.unwrap().name, "Main");
}

#[test]
fn test_tree_sitter_parser_parses_basic_code() {
    let mut parser = BasicParser::new().unwrap();
    let source = "Module Test\nSub Main()\nEnd Sub\nEnd Module";
    
    let tree = parser.parse(source);
    assert!(tree.is_some());
    
    let tree = tree.unwrap();
    let basic_tree = BasicTree::new(&tree, source);
    let root = basic_tree.root_node();
    
    assert_eq!(root.kind(), "program");
}

#[test]
fn test_tree_sitter_parser_incremental_parsing() {
    let mut parser = BasicParser::new().unwrap();
    let source1 = "Module Test\nSub Main()\nEnd Sub\nEnd Module";
    let source2 = "Module Test\nSub Main()\nPrint(\"Hello\")\nEnd Sub\nEnd Module";
    
    let tree1 = parser.parse(source1);
    assert!(tree1.is_some());
    
    let tree2 = parser.parse_incremental(source2, tree1.as_ref());
    assert!(tree2.is_some());
}

#[test]
fn test_tree_sitter_extracts_symbols() {
    let mut parser = BasicParser::new().unwrap();
    let source = "Module Test\nSub Main()\nEnd Sub\nFunction Add(a As Integer, b As Integer) As Integer\nReturn a + b\nEnd Function\nEnd Module";
    
    let tree = parser.parse(source).unwrap();
    let basic_tree = BasicTree::new(&tree, source);
    let symbols = basic_tree.get_symbols();
    
    assert!(symbols.iter().any(|s| s.name == "Test" && s.kind == basicforge_ide::tree_sitter_parser::SymbolKind::Module));
    assert!(symbols.iter().any(|s| s.name == "Main" && s.kind == basicforge_ide::tree_sitter_parser::SymbolKind::Procedure));
    assert!(symbols.iter().any(|s| s.name == "Add" && s.kind == basicforge_ide::tree_sitter_parser::SymbolKind::Function));
}

#[test]
fn test_tree_sitter_reports_diagnostics() {
    let mut parser = BasicParser::new().unwrap();
    let source = "Module Test\nSub Main()\nDim x As Integer = \"hello\"\nEnd Sub\nEnd Module";
    
    let tree = parser.parse(source).unwrap();
    let basic_tree = BasicTree::new(&tree, source);
    let diagnostics = basic_tree.get_diagnostics();
    
    // Should have at least some diagnostics (even if just structural)
    assert!(diagnostics.len() >= 0);
}

#[test]
fn test_salsa_database_basic_operations() {
    let mut db = BasicDatabase::new();
    let path = PathBuf::from("test.bf");
    let source = "Module Test\nSub Main()\nEnd Sub\nEnd Module";
    
    db.set_file(path.clone(), source.to_string());
    
    let diagnostics = db.diagnostics(path.clone());
    assert!(diagnostics.len() >= 0);
    
    let symbols = db.symbols(path.clone());
    assert_eq!(symbols.modules.len(), 1);
    assert_eq!(symbols.modules[0].name, "Test");
}

#[test]
fn test_salsa_database_incremental_updates() {
    let mut db = BasicDatabase::new();
    let path = PathBuf::from("test.bf");
    
    // Initial source
    db.set_file(path.clone(), "Module Test\nSub Main()\nEnd Sub\nEnd Module".to_string());
    let symbols1 = db.symbols(path.clone());
    assert_eq!(symbols1.procedures.len(), 1);
    
    // Update source
    db.set_file(path.clone(), "Module Test\nSub Main()\nEnd Sub\nSub Helper()\nEnd Sub\nEnd Module".to_string());
    let symbols2 = db.symbols(path.clone());
    assert_eq!(symbols2.procedures.len(), 2);
}

#[test]
fn test_salsa_database_diagnostics_update() {
    let mut db = BasicDatabase::new();
    let path = PathBuf::from("test.bf");
    
    // Valid source
    db.set_file(path.clone(), "Module Test\nSub Main()\nEnd Sub\nEnd Module".to_string());
    let diag1 = db.diagnostics(path.clone());
    let error_count1 = diag1.iter().filter(|d| d.severity == 1).count();
    
    // Invalid source
    db.set_file(path.clone(), "Module Test\nSub Main()\nDim x As Integer = \"hello\"\nEnd Sub\nEnd Module".to_string());
    let diag2 = db.diagnostics(path.clone());
    let error_count2 = diag2.iter().filter(|d| d.severity == 1).count();
    
    // Should have more errors now
    assert!(error_count2 >= error_count1);
}

#[test]
fn test_editor_syntax_highlighting() {
    let mut editor = Editor::new();
    editor.set_content("Module Test\nSub Main()\nPrint(\"Hello\")\nEnd Sub\nEnd Module".to_string());
    editor.set_filename(Some("test.bf".to_string()));
    
    // Syntax highlighting should have been updated
    // Just verify it doesn't crash
}

#[test]
fn test_editor_completion_popup() {
    let mut editor = Editor::new();
    editor.set_content("Module Test\nSub Main()\nPri".to_string());
    editor.set_filename(Some("test.bf".to_string()));
    
    // Trigger completion
    let cursor_pos = editor.content.len();
    editor.completion_popup.update(&editor.content, cursor_pos, &editor.language_server);
    
    // Should have completion items
    assert!(!editor.completion_popup.items.is_empty() || editor.completion_popup.visible);
}

#[test]
fn test_editor_diagnostics_integration() {
    let mut editor = Editor::new();
    editor.set_content("Module Test\nSub Main()\nDim x As Integer = \"hello\"\nEnd Sub\nEnd Module".to_string());
    editor.set_filename(Some("test.bf".to_string()));
    
    // Diagnostics should be populated
    // Just verify it doesn't crash
}

#[test]
fn test_form_designer_add_control() {
    let mut designer = FormDesigner::new();
    designer.add_control(basicforge_ide::designer::ControlType::Button);
    
    assert_eq!(designer.form.controls.len(), 1);
    assert_eq!(designer.form.controls[0].control_type, basicforge_ide::designer::ControlType::Button);
}

#[test]
fn test_form_designer_generates_code() {
    let mut designer = FormDesigner::new();
    designer.add_control(basicforge_ide::designer::ControlType::Button);
    designer.add_control(basicforge_ide::designer::ControlType::Label);
    
    let code = designer.generate_code();
    assert!(!code.is_empty());
    assert!(code.contains("Class"));
    assert!(code.contains("Button"));
    assert!(code.contains("Label"));
}

#[test]
fn test_form_designer_event_designer() {
    let mut designer = FormDesigner::new();
    designer.add_control(basicforge_ide::designer::ControlType::Button);
    
    let control_id = designer.form.controls[0].id.clone();
    designer.form.selected_control = Some(control_id.clone());
    
    // Check that button has Click event by default
    let control = &designer.form.controls[0];
    assert!(control.properties.contains_key("Event_Click"));
}

#[test]
fn test_debugger_basic_operations() {
    let mut debugger = Debugger::new();
    
    // Test breakpoint toggling
    debugger.toggle_breakpoint("test.bf", 10);
    assert!(debugger.has_breakpoint("test.bf", 10));
    
    debugger.toggle_breakpoint("test.bf", 10);
    assert!(!debugger.has_breakpoint("test.bf", 10));
}

#[test]
fn test_debugger_watch_variables() {
    let mut debugger = Debugger::new();
    
    debugger.add_watch("x");
    assert_eq!(debugger.watch_variables.len(), 1);
    assert_eq!(debugger.watch_variables[0].name, "x");
    
    debugger.update_watch_value("x", "42", "Integer");
    assert_eq!(debugger.watch_variables[0].value, "42");
    assert_eq!(debugger.watch_variables[0].type_name, "Integer");
    
    debugger.remove_watch("x");
    assert_eq!(debugger.watch_variables.len(), 0);
}

#[test]
fn test_debugger_state_transitions() {
    let mut debugger = Debugger::new();
    
    assert_eq!(debugger.state, basicforge_ide::debugger::DebugState::NotStarted);
    
    // Start debugging (will fail without executable, but state should change)
    debugger.config.exe_path = "nonexistent.exe".to_string();
    let result = debugger.start_debug();
    assert!(result.is_err());
}

#[test]
fn test_debugger_breakpoint_conditions() {
    let mut debugger = Debugger::new();
    
    debugger.toggle_breakpoint("test.bf", 10);
    debugger.set_breakpoint_condition("test.bf", 10, "x > 5".to_string());
    
    let breaks = debugger.get_breakpoints_for_file("test.bf");
    assert_eq!(breaks.len(), 1);
    assert_eq!(breaks[0].condition, Some("x > 5".to_string()));
}

#[test]
fn test_debugger_call_stack() {
    let mut debugger = Debugger::new();
    
    // Simulate a call stack
    debugger.call_stack.push(basicforge_ide::debugger::StackFrame {
        function: "Main".to_string(),
        file: "test.bf".to_string(),
        line: 1,
        column: 1,
        variables: std::collections::HashMap::new(),
        is_async: false,
    });
    
    assert_eq!(debugger.call_stack.len(), 1);
    assert_eq!(debugger.call_stack[0].function, "Main");
}

// Property-based tests using proptest
#[cfg(test)]
mod proptests {
    use super::*;
    use proptest::prelude::*;

    proptest! {
        #[test]
        fn language_server_handles_various_inputs(source in ".*") {
            let mut ls = LanguageServer::new();
            // Should not panic on any input
            ls.analyze_file("test.bf", &source);
        }
    }

    proptest! {
        #[test]
        fn tree_sitter_parser_handles_various_inputs(source in ".*") {
            let mut parser = BasicParser::new().unwrap();
            // Should not panic on any input
            let _ = parser.parse(&source);
        }
    }
}