use crate::debugger::Debugger;
use crate::designer::FormDesigner;
use crate::editor::Editor;
use crate::language::EditorDiagnostic;
use crate::project::Project;
use crate::ui::OutputWindow;
use eframe::egui;
use std::path::Path;

pub struct BasicForgeApp {
    pub editor: Editor,
    pub project: Project,
    pub output: OutputWindow,
    pub form_designer: FormDesigner,
    pub debugger: Debugger,
    pub lsp: Option<crate::lsp::LspClient>,
    show_diagnostics: bool,
    build_in_progress: bool,
    current_file: Option<String>,
    diagnostics: Vec<EditorDiagnostic>,
    active_tab: Tab,
}

#[derive(Debug, Clone, Copy, PartialEq)]
enum Tab {
    Editor,
    FormDesigner,
    Output,
    Debug,
}

impl BasicForgeApp {
    pub fn new(_cc: &eframe::CreationContext<'_>) -> Self {
        Self {
            editor: Editor::new(),
            project: Project::new(),
            output: OutputWindow::new(),
            form_designer: FormDesigner::new(),
            debugger: Debugger::new(),
            lsp: None,
            show_diagnostics: true,
            build_in_progress: false,
            current_file: None,
            diagnostics: Vec::new(),
            active_tab: Tab::Editor,
        }
    }

    pub fn build_project(&mut self) {
        self.build_in_progress = true;
        self.output.log("Building project...");

        let project_path = self.project.path.clone();
        let output = std::process::Command::new("bfc.exe")
            .arg("build")
            .arg(&project_path)
            .output();

        match output {
            Ok(result) => {
                if result.status.success() {
                    self.output.log("Build succeeded");
                    let stdout = String::from_utf8_lossy(&result.stdout);
                    for line in stdout.lines() {
                        self.output.log(line);
                    }
                } else {
                    self.output.log("Build failed");
                    let stderr = String::from_utf8_lossy(&result.stderr);
                    for line in stderr.lines() {
                        self.output.log(line);
                        self.parse_compiler_error(line);
                    }
                }
            }
            Err(e) => {
                self.output.log(&format!("Build error: {}", e));
            }
        }

        self.build_in_progress = false;
    }

    pub fn check_project(&mut self) {
        self.output.log("Checking project...");
        let project_path = self.project.path.clone();
        let output = std::process::Command::new("bfc.exe")
            .arg("check")
            .arg(&project_path)
            .output();

        match output {
            Ok(result) => {
                let stderr = String::from_utf8_lossy(&result.stderr);
                for line in stderr.lines() {
                    if !line.is_empty() {
                        self.output.log(line);
                        self.parse_compiler_error(line);
                    }
                }
                if result.status.success() {
                    self.output.log("No errors found");
                }
            }
            Err(e) => {
                self.output.log(&format!("Check error: {}", e));
            }
        }
    }

    pub fn run_project(&mut self) {
        self.output.log("Running project...");
        let exe_path = format!("{}.exe", self.project.name);
        match std::process::Command::new(&exe_path).output() {
            Ok(result) => {
                let stdout = String::from_utf8_lossy(&result.stdout);
                self.output.log(&format!("Exit code: {}", result.status.code().unwrap_or(-1)));
                for line in stdout.lines() {
                    self.output.log(line);
                }
            }
            Err(e) => {
                self.output.log(&format!("Run error: {}", e));
            }
        }
    }

    pub fn debug_project(&mut self) {
        self.output.log("Debugging project...");
        let exe_path = format!("{}.exe", self.project.name);
        self.debugger.set_executable(&exe_path);
        if let Err(e) = self.debugger.start_debug() {
            self.output.log(&format!("Debug error: {}", e));
        } else {
            self.active_tab = Tab::Debug;
            self.output.log(&format!("Debugger started for: {}", exe_path));
        }
    }

    pub fn new_project(&mut self) {
        self.project = Project::new();
        self.editor.set_content(String::new());
        self.editor.set_filename(None);
        self.current_file = None;
        self.output.log("New project created");
    }

    pub fn open_project(&mut self, path: &str) {
        self.project.load(path);
        self.output.log(&format!("Project opened: {}", path));
    }

    pub fn save_project(&mut self) {
        if self.project.path.is_empty() {
            self.output.log("No project path set");
            return;
        }
        if let Err(e) = self.project.save() {
            self.output.log(&format!("Save project error: {}", e));
        } else {
            self.output.log("Project saved");
        }
    }

    pub fn open_file(&mut self, path: &str) {
        match std::fs::read_to_string(path) {
            Ok(content) => {
                let content_clone = content.clone();
                self.editor.set_content(content);
                self.editor.set_filename(Some(path.to_string()));
                self.current_file = Some(path.to_string());
                self.output.log(&format!("Opened: {}", path));

                self.editor.language_server.analyze_file(path, &content_clone);
                self.editor.set_diagnostics(self.editor.language_server.get_diagnostics().clone());
            }
            Err(e) => {
                self.output.log(&format!("Open error: {}", e));
            }
        }
    }

    pub fn save_file(&mut self) {
        let filename_opt = self.editor.filename.clone();
        if let Some(filename) = filename_opt {
            if let Err(e) = std::fs::write(&filename, &self.editor.content) {
                self.output.log(&format!("Save error: {}", e));
            } else {
                self.output.log(&format!("Saved: {}", filename));

                let content_clone = self.editor.content.clone();
                self.editor.language_server.analyze_file(&filename, &content_clone);
                self.editor.set_diagnostics(self.editor.language_server.get_diagnostics().clone());
            }
        } else {
            self.output.log("No file to save");
        }
    }

    pub fn new_file(&mut self) {
        self.editor.set_content(String::new());
        self.editor.set_filename(None);
        self.current_file = None;
        self.output.log("New file created");
    }

    pub fn parse_compiler_error(&mut self, line: &str) {
        let line = line.trim();
        if let Some(colon_idx) = line.find(':') {
            let file_part = &line[..colon_idx];
            let rest = &line[colon_idx + 1..];

            let paren_start = rest.find('(').unwrap_or(0);
            if paren_start > 0 {
                let paren_end = rest[paren_start..].find(')').unwrap_or(0);
                if paren_end > 0 {
                    let loc_part = &rest[paren_start + 1..paren_start + paren_end];
                    let parts: Vec<&str> = loc_part.split(',').collect();
                    if parts.len() >= 2 {
                        let line_num: u32 = parts[0].trim().parse().unwrap_or(0);
                        let col_num: u32 = parts[1].trim().parse().unwrap_or(0);

                        let after_paren = &rest[paren_start + paren_end + 1..];
                        let msg_start = after_paren.find(':').unwrap_or(0);
                        let message = if msg_start > 0 {
                            after_paren[msg_start + 1..].trim().to_string()
                        } else {
                            after_paren.trim().to_string()
                        };

                        let diagnostic = EditorDiagnostic {
                            file: file_part.to_string(),
                            line: line_num.saturating_sub(1),
                            column: col_num.saturating_sub(1),
                            end_line: line_num.saturating_sub(1),
                            end_column: col_num.saturating_sub(1) + 1,
                            severity: 1,
                            message,
                            source: "bfc".to_string(),
                        };

                        self.diagnostics.push(diagnostic);
                        self.editor.set_diagnostics(self.diagnostics.clone());
                    }
                }
            }
        }
    }

    pub fn goto_diagnostic(&mut self, index: usize) {
        if let Some(diag) = self.diagnostics.get(index).cloned() {
            let diag_file = diag.file.clone();
            self.open_file(&diag_file);
            self.output.log(&format!("Jumped to {}:{}:{}", diag_file, diag.line + 1, diag.column + 1));
        }
    }
}

impl eframe::App for BasicForgeApp {
    fn update(&mut self, ctx: &egui::Context, _frame: &mut eframe::Frame) {
        if let Some(client) = &self.lsp {
            for msg in client.poll() {
                self.output.log(&format!("[LSP] {}", msg));
            }
        }
        // Top menu bar
        egui::TopBottomPanel::top("menu_bar").show(ctx, |ui| {
            egui::menu::bar(ui, |ui| {
                ui.menu_button("File", |ui| {
                    if ui.button("New Project").clicked() {
                        self.new_project();
                        ui.close_menu();
                    }
                    if ui.button("Open Project").clicked() {
                        self.output.log("File dialog not yet implemented - use Project Explorer");
                        ui.close_menu();
                    }
                    if ui.button("Save Project").clicked() {
                        self.save_project();
                        ui.close_menu();
                    }
                    if ui.separator().clicked() {}
                    if ui.button("New File").clicked() {
                        self.new_file();
                        ui.close_menu();
                    }
                    if ui.button("Open File").clicked() {
                        self.output.log("File dialog not yet implemented - use Project Explorer");
                        ui.close_menu();
                    }
                    if ui.button("Save File").clicked() {
                        self.save_file();
                        ui.close_menu();
                    }
                    if ui.separator().clicked() {}
                    if ui.button("New Form (.bfform)").clicked() {
                        self.form_designer.new_form("Form1");
                        self.active_tab = Tab::FormDesigner;
                        ui.close_menu();
                    }
                    if ui.button("Open Form (.bfform)").clicked() {
                        self.output.log("File dialog not yet implemented - use Project Explorer");
                        ui.close_menu();
                    }
                    if ui.button("Save Form (.bfform)").clicked() {
                        if let Err(e) = self.form_designer.save_form() {
                            self.output.log(&format!("Save form error: {}", e));
                        } else {
                            self.output.log("Form saved");
                        }
                        ui.close_menu();
                    }
                    if ui.separator().clicked() {}
                    if ui.button("Exit").clicked() {
                        ctx.send_viewport_cmd(egui::ViewportCommand::Close);
                    }
                });
                ui.menu_button("Build", |ui| {
                    if ui.button("Build").clicked() {
                        self.build_project();
                        ui.close_menu();
                    }
                    if ui.button("Check").clicked() {
                        self.check_project();
                        ui.close_menu();
                    }
                    if ui.button("Run").clicked() {
                        self.run_project();
                        ui.close_menu();
                    }
                    if ui.separator().clicked() {}
                    if ui.button("Debug (F5)").clicked() {
                        self.debug_project();
                        ui.close_menu();
                    }
                });
                ui.menu_button("View", |ui| {
                    ui.checkbox(&mut self.show_diagnostics, "Show Diagnostics");
                    ui.close_menu();
                });
                ui.menu_button("Tools", |ui| {
                    if ui.button("Start LSP").clicked() {
                        match crate::lsp::LspClient::start("bfc.exe") {
                            Ok(mut client) => {
                                client.send(
                                    r#"{"jsonrpc":"2.0","id":1,"method":"initialize","params":{}}"#,
                                );
                                client.notify("initialized", "{}");
                                self.output.log("LSP client started (bfc --lsp)");
                                self.lsp = Some(client);
                            }
                            Err(e) => self.output.log(&format!("LSP failed: {}", e)),
                        }
                        ui.close_menu();
                    }
                });
                ui.menu_button("Debug", |ui| {
                    if ui.button("Debug (Native)").clicked() {
                        let exe = self.debugger.config.exe_path.clone();
                        if exe.is_empty() {
                            self.output.log("Set an executable in Debugger settings first");
                        } else {
                            self.debugger.start_native(&exe);
                        }
                        ui.close_menu();
                    }
                });
            });
        });

        // Left panel: Project Explorer
        egui::SidePanel::left("project_panel").show(ctx, |ui| {
            ui.heading("Project Explorer");
            ui.separator();
            self.project.ui(ui);
        });

        // Central panel: Editor / Form Designer tabs
        egui::CentralPanel::default().show(ctx, |ui| {
            ui.horizontal(|ui| {
                ui.selectable_value(&mut self.active_tab, Tab::Editor, "Editor");
                ui.selectable_value(&mut self.active_tab, Tab::FormDesigner, "Form Designer");
                ui.selectable_value(&mut self.active_tab, Tab::Output, "Output");
                ui.selectable_value(&mut self.active_tab, Tab::Debug, "Debug");
            });
            ui.separator();
            
            match self.active_tab {
                Tab::Editor => {
                    ui.heading("Editor");
                    self.editor.ui(ui, ctx, &mut self.output);
                    
                    // ... existing editor code ...
                    if !self.editor.diagnostics().is_empty() {
                        ui.separator();
                        ui.label(format!("⚠ {} diagnostics in current file", self.editor.diagnostics().len()));
                        let diagnostics: Vec<_> = self.editor.diagnostics().to_vec();
                        for (i, diag) in diagnostics.iter().enumerate() {
                            if ui.button(format!("{}:{} - {}", diag.line + 1, diag.column + 1, diag.message)).clicked() {
                                self.goto_diagnostic(i);
                            }
                        }
                    }
    
                    // Auto-completion using local language server
                    if self.editor.content != self.editor.previous_content {
                        let cursor_pos = self.editor.content.len();
                        if cursor_pos > 0 {
                            let last_char = self.editor.content.chars().last().unwrap_or(' ');
                            if last_char.is_ascii_alphabetic() || last_char == '_' || last_char == '.' {
                                let completions = self.editor.language_server.get_completion_items(&self.editor.content[..cursor_pos.min(self.editor.content.len())]);
                                if !completions.is_empty() {
                                    let popup = self.editor.completion_popup_mut();
                                    popup.items = completions;
                                    popup.visible = true;
                                    popup.selected = 0;
                                }
                            }
                        }
                    }
    
                    // Go-to-definition on Ctrl+Click (simplified - just click on word)
                    if ctx.input(|i| i.pointer.button_clicked(egui::PointerButton::Primary)) {
                        if let Some(pos) = ctx.pointer_hover_pos() {
                            // Convert screen position to text position (approximate)
                            let char_pos = (pos.x / 9.0) as usize;
                            let line_pos = (pos.y / 18.0) as usize;
                            if let Some((_start, _end, word)) = self.editor.get_word_at_position(char_pos, line_pos) {
                                // Clone the symbol to avoid borrow conflict
                                let symbol_opt = self.editor.language_server.find_definition(&word);
                                if let Some(symbol) = symbol_opt {
                                    self.output.log(&format!("Going to definition: {} at {}:{}", symbol.name, symbol.file, symbol.line));
                                    if std::path::Path::new(&symbol.file).exists() {
                                        self.open_file(&symbol.file);
                                    }
                                } else {
                                    self.output.log(&format!("No definition found for: {}", word));
                                }
                            }
                        }
                    }
                }
                Tab::FormDesigner => {
                    ui.heading("Form Designer");
                    self.form_designer.ui(ui, &self.project, &mut self.editor, &mut self.output);
                }
                Tab::Output => {
                    ui.heading("Output");
                    self.output.ui(ui);
                }
                Tab::Debug => {
                    ui.heading("Debugger");
                    self.debugger.ui(ui);
                }
            }
        });
    }
}