use eframe::egui;
use std::collections::{BTreeMap, HashMap};

pub mod native;

#[derive(Debug, Clone, PartialEq)]
pub struct Breakpoint {
    pub file: String,
    pub line: usize,
    pub enabled: bool,
    pub condition: Option<String>,
    pub hit_count: usize,
}

#[derive(Debug, Clone)]
pub struct WatchVariable {
    pub name: String,
    pub value: String,
    pub type_name: String,
    pub scope: WatchScope,
}

#[derive(Debug, Clone, PartialEq)]
pub enum WatchScope {
    Local,
    Global,
    Auto,
}

#[derive(Debug, Clone)]
pub struct StackFrame {
    pub function: String,
    pub file: String,
    pub line: usize,
    pub column: usize,
    pub variables: HashMap<String, VariableValue>,
    pub is_async: bool,
}

#[derive(Debug, Clone)]
pub struct VariableValue {
    pub value: String,
    pub type_name: String,
    pub children: Vec<VariableValue>,
    pub is_expanded: bool,
}

#[derive(Debug, Clone, PartialEq)]
pub enum DebugState {
    NotStarted,
    Starting,
    Running,
    Paused,
    Stopped,
    Finished,
    Error(String),
}

#[derive(Debug, Clone)]
pub struct DebugConfig {
    pub exe_path: String,
    pub working_dir: String,
    pub args: Vec<String>,
    pub env_vars: HashMap<String, String>,
    pub stop_at_entry: bool,
}

impl Default for DebugConfig {
    fn default() -> Self {
        Self {
            exe_path: String::new(),
            working_dir: String::new(),
            args: Vec::new(),
            env_vars: HashMap::new(),
            stop_at_entry: true,
        }
    }
}

pub struct Debugger {
    pub state: DebugState,
    pub config: DebugConfig,
    pub breakpoints: BTreeMap<String, Vec<Breakpoint>>, // file -> breakpoints
    pub watch_variables: Vec<WatchVariable>,
    pub call_stack: Vec<StackFrame>,
    pub current_file: Option<String>,
    pub current_line: Option<usize>,
    pub current_column: Option<usize>,
    pub process_id: Option<u32>,
    pub output: String,
    pub debug_output: String,
    pub locals: HashMap<String, VariableValue>,
    pub globals: HashMap<String, VariableValue>,
    pub registers: HashMap<String, String>,
    pub disassembly: Vec<DisassemblyLine>,
    pub show_disassembly: bool,
    pub breakpoint_hit: Option<BreakpointHit>,
    pub debug_breakpoints: Vec<(usize, String)>,
    pub pending: std::sync::Arc<std::sync::Mutex<Vec<String>>>,
    pub child: Option<std::process::Child>,
    pub child_stdin: Option<std::process::ChildStdin>,
    pub native: Option<native::NativeDebugger>,
}

#[derive(Debug, Clone)]
pub struct BreakpointHit {
    pub breakpoint: Breakpoint,
    pub thread_id: u32,
    pub timestamp: std::time::SystemTime,
}

#[derive(Debug, Clone)]
pub struct DisassemblyLine {
    pub address: u64,
    pub bytes: Vec<u8>,
    pub instruction: String,
    pub is_current: bool,
    pub has_breakpoint: bool,
}

impl Debugger {
    pub fn new() -> Self {
        Self {
            state: DebugState::NotStarted,
            config: DebugConfig::default(),
            breakpoints: BTreeMap::new(),
            watch_variables: Vec::new(),
            call_stack: Vec::new(),
            current_file: None,
            current_line: None,
            current_column: None,
            process_id: None,
            output: String::new(),
            debug_output: String::new(),
            locals: HashMap::new(),
            globals: HashMap::new(),
            registers: HashMap::new(),
            disassembly: Vec::new(),
            show_disassembly: false,
            breakpoint_hit: None,
            debug_breakpoints: Vec::new(),
            pending: std::sync::Arc::new(std::sync::Mutex::new(Vec::new())),
            child: None,
            child_stdin: None,
            native: None,
        }
    }

    pub fn set_executable(&mut self, exe_path: &str) {
        self.config.exe_path = exe_path.to_string();
        self.config.working_dir = std::path::Path::new(exe_path)
            .parent()
            .map(|p| p.to_string_lossy().to_string())
            .unwrap_or_default();
    }

    pub fn toggle_breakpoint(&mut self, file: &str, line: usize) {
        let file_breaks = self.breakpoints.entry(file.to_string()).or_default();
        
        if let Some(pos) = file_breaks.iter().position(|b| b.line == line) {
            file_breaks.remove(pos);
        } else {
            file_breaks.push(Breakpoint {
                file: file.to_string(),
                line,
                enabled: true,
                condition: None,
                hit_count: 0,
            });
        }
    }

    pub fn set_breakpoint_condition(&mut self, file: &str, line: usize, condition: String) {
        if let Some(file_breaks) = self.breakpoints.get_mut(file) {
            if let Some(bp) = file_breaks.iter_mut().find(|b| b.line == line) {
                bp.condition = Some(condition);
            }
        }
    }

    pub fn enable_breakpoint(&mut self, file: &str, line: usize, enabled: bool) {
        if let Some(file_breaks) = self.breakpoints.get_mut(file) {
            if let Some(bp) = file_breaks.iter_mut().find(|b| b.line == line) {
                bp.enabled = enabled;
            }
        }
    }

    pub fn has_breakpoint(&self, file: &str, line: usize) -> bool {
        self.breakpoints
            .get(file)
            .map(|b| b.iter().any(|bp| bp.line == line && bp.enabled))
            .unwrap_or(false)
    }

    pub fn remove_breakpoint(&mut self, file: &str, line: usize) {
        if let Some(file_breaks) = self.breakpoints.get_mut(file) {
            file_breaks.retain(|b| b.line != line);
        }
    }

    pub fn clear_all_breakpoints(&mut self) {
        self.breakpoints.clear();
    }

    pub fn get_breakpoints_for_file(&self, file: &str) -> Vec<&Breakpoint> {
        self.breakpoints.get(file).map(|v| v.iter().collect()).unwrap_or_default()
    }

    pub fn add_watch(&mut self, name: &str, scope: WatchScope) {
        if !self.watch_variables.iter().any(|w| w.name == name) {
            self.watch_variables.push(WatchVariable {
                name: name.to_string(),
                value: "<unknown>".to_string(),
                type_name: "<unknown>".to_string(),
                scope,
            });
        }
    }

    pub fn remove_watch(&mut self, name: &str) {
        self.watch_variables.retain(|w| w.name != name);
    }

    pub fn update_watch_value(&mut self, name: &str, value: &str, type_name: &str) {
        if let Some(w) = self.watch_variables.iter_mut().find(|w| w.name == name) {
            w.value = value.to_string();
            w.type_name = type_name.to_string();
        }
    }

    pub fn start_debug(&mut self) -> Result<(), String> {
        if self.config.exe_path.is_empty() {
            return Err("No executable specified".to_string());
        }
        if !std::path::Path::new(&self.config.exe_path).exists() {
            return Err(format!("Executable not found: {}", self.config.exe_path));
        }

        self.state = DebugState::Starting;
        self.output.clear();
        self.debug_output.clear();
        self.call_stack.clear();
        self.locals.clear();
        self.globals.clear();
        self.registers.clear();
        self.disassembly.clear();
        self.breakpoint_hit = None;

        // Launch the native BasicForge debugger (bfc debug) and attach a pipe so
        // the IDE panel can drive it and render its output.
        let bfc = std::env::var("BFC_EXE").unwrap_or_else(|_| "bfc.exe".to_string());
        let mut cmd = std::process::Command::new(&bfc);
        cmd.arg("debug").arg(&self.config.exe_path);
        cmd.stdin(std::process::Stdio::piped());
        cmd.stdout(std::process::Stdio::piped());
        cmd.stderr(std::process::Stdio::piped());

        match cmd.spawn() {
            Ok(mut child) => {
                self.process_id = Some(child.id());
                self.state = DebugState::Running;
                self.child_stdin = child.stdin.take();
                let stdout = child.stdout.take();
                self.child = Some(child);
                self.output = format!(
                    "[debug] Launched: {} debug {}\n",
                    bfc, self.config.exe_path
                );
                let pending = self.pending.clone();
                std::thread::spawn(move || {
                    use std::io::BufRead;
                    if let Some(out) = stdout {
                        let reader = std::io::BufReader::new(out);
                        for line in reader.lines() {
                            match line {
                                Ok(l) => pending.lock().unwrap().push(l),
                                Err(_) => break,
                            }
                        }
                    }
                });
                Ok(())
            }
            Err(e) => {
                self.state = DebugState::Error(format!("Failed to launch debugger: {}", e));
                Err(format!("Failed to launch bfc debug: {}", e))
            }
        }
    }

    pub fn start_native(&mut self, exe: &str) {
        match native::NativeDebugger::start(exe) {
            Ok(mut dbg) => {
                let pid = dbg.pid;
                self.output = format!("[debug] Native debugger started (PID {})", pid);
                if let Some(msg) = dbg.wait() {
                    self.output.push_str(&format!("\n[debug] {}", msg));
                }
                self.native = Some(dbg);
            }
            Err(e) => {
                self.output = format!("[debug] Native debugger failed: {}", e);
            }
        }
    }

    pub fn native_disasm(&mut self, exe: &str) {
        let text = native::disassemble(exe);
        self.disassembly.clear();
        for line in text.lines() {
            let t = line.trim();
            if let Some(colon) = t.find(':') {
                let addr = t[..colon].trim();
                if let Ok(address) = u64::from_str_radix(addr, 16) {
                    self.disassembly.push(DisassemblyLine {
                        address,
                        bytes: Vec::new(),
                        instruction: t[colon + 1..].trim().to_string(),
                        is_current: false,
                        has_breakpoint: false,
                    });
                }
            }
        }
        self.show_disassembly = true;
    }

    pub fn send_command(&mut self, command: &str) {
        if let Some(stdin) = self.child_stdin.as_mut() {
            use std::io::Write;
            let _ = writeln!(stdin, "{}", command);
            let _ = stdin.flush();
        }
    }

    pub fn poll(&mut self) {
        let lines: Vec<String> = {
            let mut pending = self.pending.lock().unwrap();
            std::mem::take(&mut *pending)
        };
        for line in lines {
            self.output.push_str(&line);
            self.output.push('\n');
            self.parse_event(&line);
        }
        if self.state == DebugState::Running && self.output.contains("Debuggee exited") {
            self.state = DebugState::Finished;
        }
    }

    fn parse_event(&mut self, line: &str) {
        let t = line.trim();
        if let Some(rest) = t.strip_prefix("Breakpoint ") {
            if let Some(pos) = rest.find(" at ") {
                let n = rest[..pos].trim().parse::<usize>().unwrap_or(0);
                let name = rest[pos + 4..]
                    .split(" (")
                    .next()
                    .unwrap_or("")
                    .trim()
                    .to_string();
                self.debug_breakpoints.push((n, name));
            }
        }
        if t.starts_with("Rip=") {
            for part in t.split_whitespace() {
                if let Some((k, v)) = part.split_once('=') {
                    self.registers.insert(k.to_string(), v.to_string());
                }
            }
        }
        if let Some(colon) = t.find(':') {
            let addr = t[..colon].trim();
            if !addr.is_empty() && addr.chars().all(|c| c.is_ascii_hexdigit()) {
                if let Ok(address) = u64::from_str_radix(addr, 16) {
                    let instruction = t[colon + 1..].trim().to_string();
                    if !instruction.is_empty() {
                        self.disassembly.push(DisassemblyLine {
                            address,
                            bytes: Vec::new(),
                            instruction,
                            is_current: false,
                            has_breakpoint: false,
                        });
                    }
                }
            }
        }
        if let Some(rest) = t.strip_prefix("#") {
            let mut it = rest.splitn(3, char::is_whitespace);
            let _idx = it.next();
            let addr = it.next().unwrap_or("").to_string();
            let func = it.next().unwrap_or("").trim().to_string();
            self.call_stack.push(StackFrame {
                function: func,
                file: String::new(),
                line: 0,
                column: 0,
                variables: HashMap::new(),
                is_async: false,
            });
        }
    }

    fn simulate_breakpoint_hit(&mut self) {
        // Simulate hitting the first breakpoint
        for (file, breaks) in &self.breakpoints {
            for bp in breaks {
                if bp.enabled {
                    self.state = DebugState::Paused;
                    self.current_file = Some(file.clone());
                    self.current_line = Some(bp.line);
                    self.current_column = Some(1);
                    
                    self.breakpoint_hit = Some(BreakpointHit {
                        breakpoint: bp.clone(),
                        thread_id: 1,
                        timestamp: std::time::SystemTime::now(),
                    });
                    
                    // Simulate call stack
                    self.call_stack = vec![
                        StackFrame {
                            function: "Main".to_string(),
                            file: file.clone(),
                            line: bp.line,
                            column: 1,
                            variables: HashMap::new(),
                            is_async: false,
                        }
                    ];
                    
                    // Simulate local variables
                    self.locals.insert("i".to_string(), VariableValue {
                        value: "0".to_string(),
                        type_name: "Integer".to_string(),
                        children: vec![],
                        is_expanded: false,
                    });
                    self.locals.insert("str".to_string(), VariableValue {
                        value: "\"Hello, World!\"".to_string(),
                        type_name: "String".to_string(),
                        children: vec![],
                        is_expanded: false,
                    });
                    
                    self.output += &format!("[debug] Breakpoint hit at {}:{} ({})\n", file, bp.line, bp.condition.as_deref().unwrap_or(""));
                    return;
                }
            }
        }
        
        // If no breakpoints, simulate running to completion
        self.state = DebugState::Finished;
        self.output += "[debug] Program finished (no breakpoints hit)\n";
    }

    pub fn stop_debug(&mut self) {
        self.state = DebugState::NotStarted;
        self.process_id = None;
        self.call_stack.clear();
        self.current_file = None;
        self.current_line = None;
        self.current_column = None;
        self.locals.clear();
        self.globals.clear();
        self.registers.clear();
        self.disassembly.clear();
        self.breakpoint_hit = None;
        self.output += "[debug] Stopped debugging.\n";
    }

    pub fn step_over(&mut self) {
        if self.state == DebugState::Paused {
            self.state = DebugState::Running;
            self.output += "[debug] Step over (F10)\n";
            
            // Simulate stepping
            if let Some(line) = self.current_line {
                self.current_line = Some(line + 1);
            }
            self.state = DebugState::Paused;
            self.output += "[debug] Stepped to next line\n";
        }
    }

    pub fn step_into(&mut self) {
        if self.state == DebugState::Paused {
            self.state = DebugState::Running;
            self.output += "[debug] Step into (F11)\n";
            
            // Simulate stepping into a function
            self.call_stack.insert(0, StackFrame {
                function: "SubFunction".to_string(),
                file: self.current_file.clone().unwrap_or_default(),
                line: self.current_line.unwrap_or(1) + 1,
                column: 1,
                variables: HashMap::new(),
                is_async: false,
            });
            
            self.state = DebugState::Paused;
            self.output += "[debug] Stepped into function\n";
        }
    }

    pub fn step_out(&mut self) {
        if self.state == DebugState::Paused && self.call_stack.len() > 1 {
            self.state = DebugState::Running;
            self.output += "[debug] Step out (Shift+F11)\n";
            
            self.call_stack.remove(0);
            
            self.state = DebugState::Paused;
            self.output += "[debug] Stepped out of function\n";
        }
    }

    pub fn continue_execution(&mut self) {
        if self.state == DebugState::Paused {
            self.state = DebugState::Running;
            self.output += "[debug] Continue (F5)\n";
            
            // Simulate running to next breakpoint or completion
            self.simulate_continue();
        }
    }

    fn simulate_continue(&mut self) {
        // Check for next breakpoint
        if let (Some(current_file), Some(current_line)) = (&self.current_file, self.current_line) {
            if let Some(breaks) = self.breakpoints.get(current_file) {
                for bp in breaks {
                    if bp.enabled && bp.line > current_line {
                        self.current_line = Some(bp.line);
                        self.breakpoint_hit = Some(BreakpointHit {
                            breakpoint: bp.clone(),
                            thread_id: 1,
                            timestamp: std::time::SystemTime::now(),
                        });
                        self.output += &format!("[debug] Breakpoint hit at {}:{}\n", current_file, bp.line);
                        return;
                    }
                }
            }
        }
        
        // No more breakpoints, finish
        self.state = DebugState::Finished;
        self.output += "[debug] Program finished\n";
    }

    pub fn pause_execution(&mut self) {
        if self.state == DebugState::Running {
            self.state = DebugState::Paused;
            self.output += "[debug] Paused\n";
        }
    }

    pub fn evaluate_expression(&mut self, expr: &str) -> String {
        // In a real implementation, this would evaluate the expression in the debug context
        // For now, return a simulated value
        if self.locals.contains_key(expr) {
            self.locals[expr].value.clone()
        } else if self.globals.contains_key(expr) {
            self.globals[expr].value.clone()
        } else {
            format!("<unknown: {}>", expr)
        }
    }

    pub fn set_variable(&mut self, name: &str, value: &str) {
        if let Some(var) = self.locals.get_mut(name) {
            var.value = value.to_string();
        } else if let Some(var) = self.globals.get_mut(name) {
            var.value = value.to_string();
        }
    }

    pub fn load_disassembly(&mut self) {
        self.show_disassembly = !self.show_disassembly;
        if self.show_disassembly {
            // Simulate disassembly
            self.disassembly = vec![
                DisassemblyLine { address: 0x00401000, bytes: vec![0x55], instruction: "push ebp".to_string(), is_current: true, has_breakpoint: false },
                DisassemblyLine { address: 0x00401001, bytes: vec![0x89, 0xE5], instruction: "mov ebp, esp".to_string(), is_current: false, has_breakpoint: false },
                DisassemblyLine { address: 0x00401003, bytes: vec![0x83, 0xEC, 0x10], instruction: "sub esp, 0x10".to_string(), is_current: false, has_breakpoint: false },
            ];
        }
    }

    pub fn ui(&mut self, ui: &mut egui::Ui) {
        self.poll();
        ui.heading("Debugger");
        ui.separator();

        // Toolbar
        ui.horizontal(|ui| {
            ui.label(format!("State: {:?}", self.state));
            ui.separator();
            
            match self.state {
                DebugState::NotStarted | DebugState::Finished | DebugState::Error(_) => {
                    if ui.button("▶ Start Debug (F5)").clicked() {
                        if let Err(e) = self.start_debug() {
                            self.state = DebugState::Error(e);
                        }
                    }
                    if ui.button("⚙ Config").clicked() {
                        // Show config dialog
                    }
                }
                DebugState::Starting => {
                    ui.spinner();
                    ui.label("Starting...");
                }
                DebugState::Running => {
                    if ui.button("Disasm").clicked() {
                        let exe = self.config.exe_path.clone();
                        if !exe.is_empty() {
                            self.native_disasm(&exe);
                        }
                    }
                    if ui.button("⏸ Pause").clicked() {
                        self.pause_execution();
                    }
                    if ui.button("▶ Continue").clicked() {
                        self.send_command("continue");
                    }
                    if ui.button("⏭ Step").clicked() {
                        self.send_command("step");
                    }
                    if ui.button("Disasm").clicked() {
                        self.send_command("disasm");
                    }
                    if ui.button("■ Stop (Shift+F5)").clicked() {
                        self.stop_debug();
                    }
                    if ui.button("⏹ Restart (Ctrl+Shift+F5)").clicked() {
                        self.stop_debug();
                        let _ = self.start_debug();
                    }
                }
                DebugState::Paused => {
                    if ui.button("▶ Continue (F5)").clicked() {
                        self.continue_execution();
                    }
                    if ui.button("⏭ Step Over (F10)").clicked() {
                        self.step_over();
                    }
                    if ui.button("⏬ Step Into (F11)").clicked() {
                        self.step_into();
                    }
                    if ui.button("⏫ Step Out (Shift+F11)").clicked() {
                        self.step_out();
                    }
                    if ui.button("■ Stop (Shift+F5)").clicked() {
                        self.stop_debug();
                    }
                }
                DebugState::Stopped => {
                    if ui.button("▶ Continue (F5)").clicked() {
                        self.continue_execution();
                    }
                    if ui.button("■ Stop (Shift+F5)").clicked() {
                        self.stop_debug();
                    }
                }
            }
            
            ui.separator();
            
            if ui.button("🔍 Disassembly").clicked() {
                self.load_disassembly();
            }
        });

        ui.separator();

        // Main panels
        egui::SidePanel::left("debug_left_panel")
            .default_width(300.0)
            .show_inside(ui, |ui| {
                egui::ScrollArea::vertical().show(ui, |ui| {
                    // Breakpoints
                    ui.collapsing("Breakpoints", |ui| {
                        let total_bps: usize = self.breakpoints.values().flatten().count();
                        ui.horizontal(|ui| {
                            ui.label(format!("Total: {}", total_bps));
                            if ui.button("Clear All").clicked() {
                                self.clear_all_breakpoints();
                            }
                        });
                        
                        for (file, breaks) in self.breakpoints.iter_mut() {
                            ui.collapsing(file, |ui| {
                                for bp in &mut *breaks {
                                    ui.horizontal(|ui| {
                                        let mut enabled = bp.enabled;
                                        if ui.checkbox(&mut enabled, "").changed() {
                                            bp.enabled = enabled;
                                        }
                                        
                                        let color = if bp.enabled { egui::Color32::RED } else { egui::Color32::GRAY };
                                        ui.colored_label(color, format!("● Line {}", bp.line));
                                        
                                        if let Some(ref cond) = bp.condition {
                                            ui.label(format!("if ({})", cond));
                                        }
                                        
                                        if bp.hit_count > 0 {
                                            ui.label(format!("hits: {}", bp.hit_count));
                                        }
                                        
                                        if ui.button("✕").clicked() {
                                            bp.enabled = false; // Mark for removal
                                        }
                                    });
                                }
                                
                                // Remove disabled breakpoints
                                breaks.retain(|b| b.enabled);
                            });
                        }
                    });

                    // Watch variables
                    ui.collapsing("Watch / Variables", |ui| {
                        ui.horizontal(|ui| {
                            ui.label("Add watch:");
                            let mut new_watch = String::new();
                            if ui.text_edit_singleline(&mut new_watch).lost_focus() && !new_watch.is_empty() {
                                self.add_watch(&new_watch, WatchScope::Auto);
                                new_watch.clear();
                            }
                        });
                        
                        // Locals
                        if !self.locals.is_empty() {
                            ui.collapsing("Locals", |ui| {
                                // Collect data first to avoid borrow issues
                                let locals_data: Vec<(String, String, String, bool, Vec<VariableValue>)> = self.locals.iter()
                                    .map(|(k, v)| (k.clone(), v.value.clone(), v.type_name.clone(), v.is_expanded, v.children.clone()))
                                    .collect();
                                
                                for (name, value, type_name, expanded, children) in locals_data {
                                    ui.horizontal(|ui| {
                                        if ui.button(if expanded { "▼" } else { "▶" }).clicked() {
                                            if let Some(v) = self.locals.get_mut(&name) {
                                                v.is_expanded = !expanded;
                                            }
                                        }
                                        ui.label(&name);
                                        ui.label(":");
                                        ui.label(&value);
                                        ui.label(format!("({})", type_name));
                                    });
                                    
                                    if expanded && !children.is_empty() {
                                        ui.indent("children", |ui| {
                                            for child in &children {
                                                ui.horizontal(|ui| {
                                                    ui.label(&child.value);
                                                    ui.label(format!("({})", child.type_name));
                                                });
                                            }
                                        });
                                    }
                                }
                            });
                        }
                        
                        // Watch expressions
                        if !self.watch_variables.is_empty() {
                            ui.collapsing("Watch", |ui| {
                                let mut to_remove: Vec<usize> = Vec::new();
                                for (i, w) in self.watch_variables.iter().enumerate() {
                                    ui.horizontal(|ui| {
                                        ui.label(&w.name);
                                        ui.label(":");
                                        ui.label(&w.value);
                                        ui.label(format!("({})", w.type_name));
                                        if ui.button("✕").clicked() {
                                            to_remove.push(i);
                                        }
                                    });
                                }
                                for idx in to_remove.iter().rev() {
                                    self.watch_variables.remove(*idx);
                                }
                            });
                        }
                        
                        if self.locals.is_empty() && self.watch_variables.is_empty() {
                            ui.label("No variables");
                        }
                    });

                    // Call stack
                    ui.collapsing("Call Stack", |ui| {
                        if self.call_stack.is_empty() {
                            ui.label("No call stack (not paused)");
                        } else {
                            for (i, frame) in self.call_stack.iter().enumerate() {
                                let prefix = if i == 0 { "▶ " } else { "  " };
                                ui.horizontal(|ui| {
                                    ui.label(format!("{} {} in {} ({}:{})", 
                                        prefix, frame.function, frame.file, frame.line, frame.column));
                                    if ui.button("📍").clicked() {
                                        // Navigate to this frame
                                    }
                                });
                            }
                        }
                    });

                    // Registers
                    if !self.registers.is_empty() {
                        ui.collapsing("Registers", |ui| {
                        let registers = self.registers.clone();
                        for (name, mut value) in registers {
                            ui.horizontal(|ui| {
                                ui.label(&name);
                                ui.label(":");
                                if ui.text_edit_singleline(&mut value).changed() {
                                    let cmd = format!("setreg {} {}", name, value);
                                    self.send_command(&cmd);
                                }
                            });
                            self.registers.insert(name, value);
                        }
                    });
                    }
                });
            });

        egui::CentralPanel::default().show_inside(ui, |ui| {
            // Debug output / console
            egui::TopBottomPanel::bottom("debug_output_panel")
                .default_height(150.0)
                .show_inside(ui, |ui| {
                    ui.horizontal(|ui| {
                        ui.label("Debug Output");
                        if ui.button("Clear").clicked() {
                            self.output.clear();
                            self.debug_output.clear();
                        }
                    });
                    ui.separator();
                    egui::ScrollArea::vertical().max_height(140.0).show(ui, |ui| {
                        ui.add(egui::TextEdit::multiline(&mut self.output)
                            .font(egui::FontId::monospace(12.0))
                            .desired_rows(8)
                            .interactive(false));
                    });
                });

            // Disassembly view
            if self.show_disassembly && !self.disassembly.is_empty() {
                egui::TopBottomPanel::bottom("disassembly_panel")
                    .default_height(200.0)
                    .show_inside(ui, |ui| {
                        ui.horizontal(|ui| {
                            ui.label("Disassembly");
                            if ui.button("Close").clicked() {
                                self.show_disassembly = false;
                            }
                        });
                        ui.separator();
                        egui::ScrollArea::vertical().show(ui, |ui| {
                            egui::Grid::new("disassembly_grid").num_columns(4).show(ui, |ui| {
                                ui.label("Address");
                                ui.label("Bytes");
                                ui.label("Instruction");
                                ui.label("");
                                ui.end_row();
                                
                                for line in &self.disassembly {
                                    if line.is_current {
                                        ui.colored_label(egui::Color32::YELLOW, format!("0x{:08X}", line.address));
                                    } else {
                                        ui.label(format!("0x{:08X}", line.address));
                                    }
                                    
                                    let bytes_str = line.bytes.iter().map(|b| format!("{:02X}", b)).collect::<Vec<_>>().join(" ");
                                    ui.label(bytes_str);
                                    
                                    if line.is_current {
                                        ui.colored_label(egui::Color32::YELLOW, &line.instruction);
                                    } else {
                                        ui.label(&line.instruction);
                                    }
                                    
                                    if line.has_breakpoint {
                                        ui.colored_label(egui::Color32::RED, "●");
                                    }
                                    ui.end_row();
                                }
                            });
                            });
                        });
            }
        });

        ui.separator();
        ui.heading("Breakpoints");
        for (n, name) in &self.debug_breakpoints {
            ui.label(format!("  {}  {}", n, name));
        }
        ui.heading("Registers");
        for (k, v) in &self.registers {
            ui.label(format!("  {}={}", k, v));
        }
        ui.heading("Call Stack");
        for (i, frame) in self.call_stack.iter().enumerate() {
            ui.label(format!("  #{}  {}", i, frame.function));
        }
        ui.heading("Debug Output");
        let output = self.output.clone();
        ui.monospace(&output);
    }
}

impl Default for Debugger {
    fn default() -> Self {
        Self::new()
    }
}