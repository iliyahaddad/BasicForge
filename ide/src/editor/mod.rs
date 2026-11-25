use crate::language::{EditorDiagnostic, LanguageServer};
use crate::salsa_db::BasicDatabase;
use crate::tree_sitter_parser::{BasicParser, BasicTree};
use eframe::egui;
use tree_sitter::Tree;
use std::path::PathBuf;

const KEYWORDS: &[&str] = &[
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
];

fn is_keyword(word: &str) -> bool {
    KEYWORDS.iter().any(|&k| k.eq_ignore_ascii_case(word))
}

pub struct CompletionPopup {
    pub visible: bool,
    pub items: Vec<String>,
    pub selected: usize,
    filter: String,
}

impl CompletionPopup {
    pub fn new() -> Self {
        Self {
            visible: false,
            items: Vec::new(),
            selected: 0,
            filter: String::new(),
        }
    }

    pub fn update(&mut self, text: &str, cursor_pos: usize, language_server: &LanguageServer) {
        let before_cursor = &text[..cursor_pos.min(text.len())];
        let word_start = before_cursor.rfind(|c: char| !c.is_ascii_alphanumeric() && c != '_')
            .map(|i| i + 1)
            .unwrap_or(0);
        let current_word = &before_cursor[word_start..];

        if current_word.is_empty() {
            self.visible = false;
            return;
        }

        self.items = language_server.get_completion_items(current_word);
        self.visible = !self.items.is_empty();
        self.selected = 0;
        self.filter = current_word.to_string();
    }

    pub fn selected_item(&self) -> Option<&str> {
        self.items.get(self.selected).map(|s| s.as_str())
    }

    pub fn ui(&mut self, ui: &mut eframe::egui::Ui) -> Option<String> {
        if !self.visible || self.items.is_empty() {
            return None;
        }

        let mut selected_item = None;
        egui::Window::new("Completion")
            .collapsible(false)
            .resizable(false)
            .title_bar(false)
            .show(ui.ctx(), |ui| {
                for (i, item) in self.items.iter().enumerate() {
                    if ui.selectable_label(i == self.selected, item).clicked() {
                        selected_item = Some(item.clone());
                        self.visible = false;
                    }
                }
            });
        selected_item
    }
}

pub struct Editor {
    pub content: String,
    pub filename: Option<String>,
    pub syntax_highlighted: Vec<(usize, usize, egui::Color32)>,
    pub diagnostics: Vec<crate::language::EditorDiagnostic>,
    pub completion_popup: CompletionPopup,
    pub previous_content: String,
    pub language_server: LanguageServer,
    pub hover_info: Option<(egui::Vec2, String)>,
    pub hover_pos: Option<egui::Pos2>,
    pub hover_timer: std::time::Instant,
    pub breakpoints: std::collections::HashMap<usize, bool>, // line number -> enabled
    pub parser: Option<BasicParser>,
    pub tree: Option<Tree>,
    pub salsa_db: Option<BasicDatabase>,
    pub search_query: String,
    pub replace_query: String,
    pub show_search: bool,
    pub search_results: Vec<(usize, usize)>, // (start, end) byte positions
    pub current_search_index: usize,
}

impl Editor {
    pub fn new() -> Self {
        let parser = BasicParser::new().ok();
        let salsa_db = Some(BasicDatabase::new());
        Self {
            content: String::new(),
            filename: None,
            syntax_highlighted: Vec::new(),
            diagnostics: Vec::new(),
            completion_popup: CompletionPopup::new(),
            previous_content: String::new(),
            language_server: LanguageServer::new(),
            hover_info: None,
            hover_pos: None,
            hover_timer: std::time::Instant::now(),
            breakpoints: std::collections::HashMap::new(),
            parser,
            tree: None,
            salsa_db,
            search_query: String::new(),
            replace_query: String::new(),
            show_search: false,
            search_results: Vec::new(),
            current_search_index: 0,
        }
    }

    pub fn search(&mut self) {
        self.search_results.clear();
        self.current_search_index = 0;
        if self.search_query.is_empty() {
            return;
        }
        let query_lower = self.search_query.to_lowercase();
        let content_lower = self.content.to_lowercase();
        let mut start = 0;
        while let Some(pos) = content_lower[start..].find(&query_lower) {
            let abs_pos = start + pos;
            self.search_results.push((abs_pos, abs_pos + self.search_query.len()));
            start = abs_pos + self.search_query.len();
        }
    }

    pub fn replace_all(&mut self) -> usize {
        if self.search_query.is_empty() {
            return 0;
        }
        let count = self.content.matches(&self.search_query).count();
        self.content = self.content.replace(&self.search_query, &self.replace_query);
        self.search_results.clear();
        self.previous_content.clear(); // Force re-highlighting
        count
    }

    pub fn next_search(&mut self) {
        if self.search_results.is_empty() {
            return;
        }
        self.current_search_index = (self.current_search_index + 1) % self.search_results.len();
    }

    pub fn prev_search(&mut self) {
        if self.search_results.is_empty() {
            return;
        }
        if self.current_search_index == 0 {
            self.current_search_index = self.search_results.len() - 1;
        } else {
            self.current_search_index -= 1;
        }
    }

    pub fn completion_popup_mut(&mut self) -> &mut CompletionPopup {
        &mut self.completion_popup
    }

    pub fn ui(&mut self, ui: &mut eframe::egui::Ui, _ctx: &eframe::egui::Context, output: &mut crate::ui::OutputWindow) {
        self.render_toolbar(ui, output);
        ui.separator();

        self.maybe_update_completion_and_diagnostics();
        self.update_syntax_highlighting();

        self.render_editor_area(ui);
        self.render_status_bar(ui);
    }

    fn render_toolbar(&mut self, ui: &mut egui::Ui, output: &mut crate::ui::OutputWindow) {
        ui.horizontal(|ui| {
            match &self.filename {
                Some(name) => ui.label(format!("File: {}", name)),
                None => ui.label("No file open"),
            };
            ui.separator();
            if ui.button("Open").clicked() {
                output.log("File dialog not yet implemented - use Project Explorer");
            }
            if ui.button("Save").clicked() {
                self.save_current_file();
                output.log("File saved");
            }
            if ui.button("New").clicked() {
                self.content.clear();
                self.filename = None;
                output.log("New file created");
            }
            if ui.button("Find").clicked() {
                self.show_search = !self.show_search;
            }
        });
        
        if self.show_search {
            ui.horizontal(|ui| {
                ui.label("Find:");
                let response = ui.text_edit_singleline(&mut self.search_query);
                if response.changed() || ui.button("Next").clicked() {
                    self.search();
                    self.next_search();
                }
                if ui.button("Prev").clicked() {
                    self.prev_search();
                }
                ui.label("Replace:");
                ui.text_edit_singleline(&mut self.replace_query);
                if ui.button("Replace All").clicked() {
                    let count = self.replace_all();
                    output.log(&format!("Replaced {} occurrences", count));
                }
                if ui.button("✕").clicked() {
                    self.show_search = false;
                    self.search_results.clear();
                }
                if !self.search_results.is_empty() {
                    ui.label(format!("{}/{}", self.current_search_index + 1, self.search_results.len()));
                }
            });
            ui.separator();
        }
    }

    fn maybe_update_completion_and_diagnostics(&mut self) {
        if self.content != self.previous_content {
            let cursor_pos = self.content.len();
            if cursor_pos > 0 {
                let last_char = self.content.chars().last().unwrap_or(' ');
                if last_char.is_ascii_alphabetic() || last_char == '_' || last_char == '.' {
                    self.completion_popup.update(&self.content, cursor_pos, &self.language_server);
                } else {
                    self.completion_popup.visible = false;
                }
            }

            // Parse with tree-sitter incrementally
            if let Some(parser) = &mut self.parser {
                self.tree = parser.parse_incremental(&self.content, self.tree.as_ref());
            }

            // Get diagnostics from tree-sitter
            if let Some(tree) = self.tree.clone() {
                let content = self.content.clone();
                let basic_tree = BasicTree::new(&tree, &content);
                let ts_diagnostics = basic_tree.get_diagnostics();
                // Convert tree-sitter diagnostics to editor diagnostics
                self.diagnostics = ts_diagnostics.into_iter().map(|d| crate::language::EditorDiagnostic {
                    file: self.filename.clone().unwrap_or_default(),
                    line: d.range.start.line,
                    column: d.range.start.character,
                    end_line: d.range.end.line,
                    end_column: d.range.end.character,
                    severity: d.severity.unwrap_or(2),
                    message: d.message,
                    source: d.source.unwrap_or_else(|| "tree-sitter".to_string()),
                }).collect();
            }

            // Get semantic diagnostics from Salsa
            if let Some(ref mut db) = self.salsa_db {
                if let Some(ref filename) = self.filename {
                    let path = PathBuf::from(filename);
                    db.set_file(path.clone(), self.content.clone());
                    let salsa_diagnostics = db.diagnostics(path);
                    for d in salsa_diagnostics.as_ref() {
                        self.diagnostics.push(crate::language::EditorDiagnostic {
                            file: filename.clone(),
                            line: d.range.start.line as u32,
                            column: d.range.start.column as u32,
                            end_line: d.range.end.line as u32,
                            end_column: d.range.end.column as u32,
                            severity: d.severity,
                            message: d.message.clone(),
                            source: d.source.clone(),
                        });
                    }
                }
            }

            self.previous_content = self.content.clone();

            if let Some(ref filename) = self.filename {
                self.language_server.analyze_file(filename, &self.content);
                // Merge language server diagnostics
                let ls_diagnostics = self.language_server.get_diagnostics().clone();
                self.diagnostics.extend(ls_diagnostics);
            }
        }
    }

    fn render_editor_area(&mut self, ui: &mut egui::Ui) {
        egui::ScrollArea::both()
            .id_salt("editor_scroll")
            .show(ui, |ui| {
                self.render_text_edit(ui);
                self.render_completion_popup(ui);
                self.render_hover_tooltip(ui);
            });
    }

    fn render_text_edit(&mut self, ui: &mut egui::Ui) -> egui::Response {
        // Apply search highlighting
        let mut display_text = self.content.clone();
        
        // Mark search results with special characters for highlighting
        if !self.search_results.is_empty() {
            let mut new_text = String::new();
            let mut last_pos = 0;
            for (i, (start, end)) in self.search_results.iter().enumerate() {
                if *start >= last_pos {
                    new_text.push_str(&self.content[last_pos..*start]);
                    // Highlight current match
                    let match_text = &self.content[*start..*end];
                    if i == self.current_search_index {
                        new_text.push_str(&format!("⟦{}⟧", match_text)); // Current match
                    } else {
                        new_text.push_str(&format!("⟨{}⟩", match_text)); // Other matches
                    }
                    last_pos = *end;
                }
            }
            if last_pos < self.content.len() {
                new_text.push_str(&self.content[last_pos..]);
            }
            display_text = new_text;
        }

        if !self.syntax_highlighted.is_empty() {
            let mut displayed_text = String::new();
            let mut last_end = 0;
            for (start, end, color) in &self.syntax_highlighted {
                if *start > last_end {
                    displayed_text.push_str(&self.content[last_end..*start]);
                }
                displayed_text.push_str(&format!(
                    "<span style=\"color:#{:02x}{:02x}{:02x}\">{}</span>",
                    color.r(), color.g(), color.b(),
                    &self.content[*start..*end]
                ));
                last_end = *end;
            }
            if last_end < self.content.len() {
                displayed_text.push_str(&self.content[last_end..]);
            }

            ui.add(egui::TextEdit::multiline(&mut self.content)
                .interactive(false))
        } else {
            // Use display_text for search highlighting
            ui.add(egui::TextEdit::multiline(&mut display_text)
                .desired_rows(40)
                .interactive(false))
        }
    }

    fn render_completion_popup(&mut self, ui: &mut egui::Ui) {
        if self.completion_popup.visible {
            if let Some(item) = self.completion_popup.ui(ui) {
                self.apply_completion(&item);
            }
        }
    }

    fn render_hover_tooltip(&mut self, ui: &mut egui::Ui) {
        if let Some(pos) = self.hover_pos {
            let char_pos = (pos.x / 9.0) as usize;
            let line_pos = (pos.y / 18.0) as usize;
            if let Some((_start, _end, word)) = self.get_word_at_position(char_pos, line_pos) {
                if let Some(info) = self.language_server.get_hover_info(&word) {
                    self.hover_info = Some((pos.to_vec2(), info));
                    self.hover_timer = std::time::Instant::now();
                }
            }
        }

        if let Some((hover_pos, hover_text)) = &self.hover_info {
            if self.hover_timer.elapsed() < std::time::Duration::from_secs(5) {
                egui::Area::new(egui::Id::new("hover_tooltip"))
                    .anchor(egui::Align2::LEFT_TOP, *hover_pos)
                    .show(ui.ctx(), |ui| {
                        ui.add(egui::Label::new(hover_text.as_str()).wrap());
                    });
            } else {
                self.hover_info = None;
            }
        }
    }

    fn render_status_bar(&self, ui: &mut egui::Ui) {
        ui.separator();
        ui.horizontal(|ui| {
            ui.label(format!("Lines: {}", self.content.lines().count()));
            ui.label(format!("Chars: {}", self.content.len()));
            if !self.diagnostics.is_empty() {
                ui.separator();
                ui.label(format!("⚠ {} diagnostics", self.diagnostics.len()));
            }
            if !self.completion_popup.visible && self.completion_popup.items.is_empty() && !self.content.is_empty() {
                ui.separator();
                ui.label("Ready");
            }
        });
    }

    pub fn save_current_file(&self) {
        if let Some(ref filename) = self.filename {
            if let Err(e) = std::fs::write(filename, &self.content) {
                eprintln!("Save error: {}", e);
            }
        }
    }

    pub fn open_file(&mut self, path: &str) {
        match std::fs::read_to_string(path) {
            Ok(content) => {
                self.content = content.clone();
                self.filename = Some(path.to_string());
                self.update_syntax_highlighting();
                self.language_server.analyze_file(path, &content);
                self.diagnostics = self.language_server.get_diagnostics().clone();
            }
            Err(e) => {
                eprintln!("Open error: {}", e);
            }
        }
    }

    pub fn set_content(&mut self, content: String) {
        self.content = content.clone();
        self.update_syntax_highlighting();

        if let Some(ref filename) = self.filename {
            self.language_server.analyze_file(filename, &content);
            self.diagnostics = self.language_server.get_diagnostics().clone();
        }
    }

    pub fn set_filename(&mut self, filename: Option<String>) {
        self.filename = filename;
    }

    pub fn content(&self) -> &str {
        &self.content
    }

    pub fn set_diagnostics(&mut self, diagnostics: Vec<crate::language::EditorDiagnostic>) {
        self.diagnostics = diagnostics;
    }

    pub fn diagnostics(&self) -> &[crate::language::EditorDiagnostic] {
        &self.diagnostics
    }

    pub fn get_word_at_position(&self, char_pos: usize, line_pos: usize) -> Option<(usize, usize, String)> {
        let lines: Vec<&str> = self.content.lines().collect();
        if line_pos >= lines.len() {
            return None;
        }

        let line = lines[line_pos];
        if char_pos >= line.len() {
            return None;
        }

        let chars: Vec<char> = line.chars().collect();
        let mut start = char_pos;
        while start > 0 && (chars[start - 1].is_ascii_alphanumeric() || chars[start - 1] == '_') {
            start -= 1;
        }
        let mut end = char_pos;
        while end < chars.len() && (chars[end].is_ascii_alphanumeric() || chars[end] == '_') {
            end += 1;
        }

        if start == end {
            return None;
        }

        let word = chars[start..end].iter().collect::<String>();
        Some((start, end, word))
    }

    fn update_syntax_highlighting(&mut self) {
        self.syntax_highlighted.clear();
        
        // Use tree-sitter for syntax highlighting if available
        if let Some(tree) = self.tree.clone() {
            let content = self.content.clone();
            let basic_tree = BasicTree::new(&tree, &content);
            self.highlight_with_tree_sitter(&basic_tree);
            return;
        }
        
        // Fallback to regex-based highlighting
        self.highlight_with_regex();
    }

    fn highlight_with_tree_sitter(&mut self, basic_tree: &BasicTree) {
        let root = basic_tree.root_node();
        self.highlight_node(root, basic_tree);
    }

    fn highlight_node(&mut self, node: tree_sitter::Node, basic_tree: &BasicTree) {
        let kind = node.kind();
        let start = node.start_byte();
        let end = node.end_byte();
        
        // Map tree-sitter node types to colors
        let color = match kind {
            "comment" => egui::Color32::from_rgb(128, 128, 128),
            "string_literal" | "char_literal" => egui::Color32::from_rgb(255, 0, 255),
            "number_literal" => egui::Color32::from_rgb(255, 128, 0),
            "boolean_literal" => egui::Color32::from_rgb(0, 128, 128),
            "identifier" => {
                let text = basic_tree.get_text(node);
                if is_keyword(text) {
                    egui::Color32::from_rgb(0, 0, 255)
                } else {
                    egui::Color32::from_rgb(0, 0, 0)
                }
            }
            _ => return, // Don't highlight other nodes directly
        };
        
        if start != end {
            self.syntax_highlighted.push((start, end, color));
        }
        
        // Recurse into children
        let mut cursor = node.walk();
        for child in node.children(&mut cursor) {
            self.highlight_node(child, basic_tree);
        }
    }

    fn highlight_with_regex(&mut self) {
        let mut pos = 0;
        let chars: Vec<char> = self.content.chars().collect();

        while pos < chars.len() {
            let c = chars[pos];

            if c.is_whitespace() {
                pos += 1;
                continue;
            }

            if c == '\'' {
                let start = pos;
                while pos < chars.len() && chars[pos] != '\n' {
                    pos += 1;
                }
                self.syntax_highlighted.push((start, pos, egui::Color32::from_rgb(128, 128, 128)));
                continue;
            }

            if c == '"' {
                let start = pos;
                pos += 1;
                while pos < chars.len() && chars[pos] != '"' {
                    if chars[pos] == '\\' && pos + 1 < chars.len() {
                        pos += 2;
                    } else {
                        pos += 1;
                    }
                }
                if pos < chars.len() {
                    pos += 1;
                }
                self.syntax_highlighted.push((start, pos, egui::Color32::from_rgb(255, 0, 255)));
                continue;
            }

            if c.is_ascii_digit() {
                let start = pos;
                while pos < chars.len() && (chars[pos].is_ascii_digit() || chars[pos] == '.') {
                    pos += 1;
                }
                self.syntax_highlighted.push((start, pos, egui::Color32::from_rgb(255, 128, 0)));
                continue;
            }

            if c.is_ascii_alphabetic() || c == '_' {
                let start = pos;
                while pos < chars.len() && (chars[pos].is_ascii_alphanumeric() || chars[pos] == '_') {
                    pos += 1;
                }
                let word = &self.content[start..pos];
                if is_keyword(word) {
                    self.syntax_highlighted.push((start, pos, egui::Color32::from_rgb(0, 0, 255)));
                } else if word == "True" || word == "False" {
                    self.syntax_highlighted.push((start, pos, egui::Color32::from_rgb(0, 128, 128)));
                }
                continue;
            }

            pos += 1;
        }
    }

    fn apply_completion(&mut self, item: &str) {
        let cursor_pos = self.content.len();
        let before_cursor = &self.content[..cursor_pos.min(self.content.len())];
        let word_start = before_cursor.rfind(|c: char| !c.is_ascii_alphanumeric() && c != '_')
            .map(|i| i + 1)
            .unwrap_or(0);
        let current_word = &before_cursor[word_start..];

        if !current_word.is_empty() {
            self.content.replace_range(word_start..cursor_pos, item);
        } else {
            self.content.push_str(item);
        }
    }
}

impl Default for Editor {
    fn default() -> Self {
        Self::new()
    }
}