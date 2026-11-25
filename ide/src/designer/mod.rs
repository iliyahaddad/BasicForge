use crate::editor::Editor;
use crate::project::Project;
use crate::ui::OutputWindow;
use eframe::egui;
use serde::{Deserialize, Serialize};
use std::collections::HashMap;
use std::fs;

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct FormControl {
    pub id: String,
    pub control_type: ControlType,
    pub x: f32,
    pub y: f32,
    pub width: f32,
    pub height: f32,
    pub properties: HashMap<String, String>,
    pub events: HashMap<String, String>,
}

#[derive(Debug, Clone, Serialize, Deserialize, PartialEq, Copy)]
pub enum ControlType {
    Button,
    Label,
    TextBox,
    CheckBox,
    RadioButton,
    GroupBox,
    Panel,
    PictureBox,
    ListBox,
    ComboBox,
    TrackBar,
    ProgressBar,
    DateTimePicker,
    ListView,
    TreeView,
    RichTextBox,
    TabControl,
    ToolBar,
    StatusBar,
    UpDown,
}

impl Default for ControlType {
    fn default() -> Self {
        ControlType::Button
    }
}

impl ControlType {
    pub fn all() -> Vec<ControlType> {
        vec![
            ControlType::Button,
            ControlType::Label,
            ControlType::TextBox,
            ControlType::CheckBox,
            ControlType::RadioButton,
            ControlType::GroupBox,
            ControlType::Panel,
            ControlType::PictureBox,
            ControlType::ListBox,
            ControlType::ComboBox,
            ControlType::TrackBar,
            ControlType::ProgressBar,
            ControlType::DateTimePicker,
            ControlType::ListView,
            ControlType::TreeView,
            ControlType::RichTextBox,
            ControlType::TabControl,
            ControlType::ToolBar,
            ControlType::StatusBar,
            ControlType::UpDown,
        ]
    }
    
    pub fn display_name(&self) -> &'static str {
        match self {
            ControlType::Button => "Button",
            ControlType::Label => "Label",
            ControlType::TextBox => "TextBox",
            ControlType::CheckBox => "CheckBox",
            ControlType::RadioButton => "RadioButton",
            ControlType::GroupBox => "GroupBox",
            ControlType::Panel => "Panel",
            ControlType::PictureBox => "PictureBox",
            ControlType::ListBox => "ListBox",
            ControlType::ComboBox => "ComboBox",
            ControlType::TrackBar => "TrackBar",
            ControlType::ProgressBar => "ProgressBar",
            ControlType::DateTimePicker => "DateTimePicker",
            ControlType::ListView => "ListView",
            ControlType::TreeView => "TreeView",
            ControlType::RichTextBox => "RichTextBox",
            ControlType::TabControl => "TabControl",
            ControlType::ToolBar => "ToolBar",
            ControlType::StatusBar => "StatusBar",
            ControlType::UpDown => "UpDown",
        }
    }

    pub fn icon(&self) -> &'static str {
        match self {
            ControlType::Button => "🔘",
            ControlType::Label => "🏷️",
            ControlType::TextBox => "📝",
            ControlType::CheckBox => "☑️",
            ControlType::RadioButton => "🔘",
            ControlType::GroupBox => "📦",
            ControlType::Panel => "📋",
            ControlType::PictureBox => "🖼️",
            ControlType::ListBox => "📋",
            ControlType::ComboBox => "🔽",
            ControlType::TrackBar => "🎚️",
            ControlType::ProgressBar => "📊",
            ControlType::DateTimePicker => "📅",
            ControlType::ListView => "📋",
            ControlType::TreeView => "🌳",
            ControlType::RichTextBox => "📝",
            ControlType::TabControl => "🗂️",
            ControlType::ToolBar => "🧰",
            ControlType::StatusBar => "📊",
            ControlType::UpDown => "🔼",
        }
    }
    
    pub fn default_size(&self) -> (f32, f32) {
        match self {
            ControlType::Button => (80.0, 30.0),
            ControlType::Label => (100.0, 20.0),
            ControlType::TextBox => (150.0, 25.0),
            ControlType::CheckBox => (100.0, 20.0),
            ControlType::RadioButton => (100.0, 20.0),
            ControlType::GroupBox => (200.0, 150.0),
            ControlType::Panel => (200.0, 150.0),
            ControlType::PictureBox => (150.0, 150.0),
            ControlType::ListBox => (150.0, 100.0),
            ControlType::ComboBox => (150.0, 25.0),
            ControlType::TrackBar => (150.0, 45.0),
            ControlType::ProgressBar => (200.0, 20.0),
            ControlType::DateTimePicker => (160.0, 25.0),
            ControlType::ListView => (200.0, 150.0),
            ControlType::TreeView => (200.0, 150.0),
            ControlType::RichTextBox => (240.0, 160.0),
            ControlType::TabControl => (260.0, 180.0),
            ControlType::ToolBar => (240.0, 30.0),
            ControlType::StatusBar => (240.0, 22.0),
            ControlType::UpDown => (24.0, 24.0),
        }
    }
    
    pub fn available_events(&self) -> Vec<&'static str> {
        let mut events: Vec<&'static str> = match self {
            ControlType::Button => vec!["Click", "DoubleClick"],
            ControlType::Label => vec!["Click", "DoubleClick"],
            ControlType::TextBox => vec!["TextChanged", "KeyPress", "Enter"],
            ControlType::CheckBox => vec!["CheckedChanged", "Click"],
            ControlType::RadioButton => vec!["CheckedChanged", "Click"],
            ControlType::GroupBox => vec!["Enter", "Leave"],
            ControlType::Panel => vec!["Paint", "Click"],
            ControlType::PictureBox => vec!["Click", "DoubleClick", "Paint"],
            ControlType::ListBox => vec!["SelectedIndexChanged", "Click", "DoubleClick", "DrawItem"],
            ControlType::ComboBox => vec!["SelectedIndexChanged", "Click", "DropDown", "DropDownClosed", "TextChanged"],
            ControlType::TrackBar => vec!["Scroll", "Click"],
            ControlType::ProgressBar => vec![],
            ControlType::DateTimePicker => vec!["ValueChanged", "Click", "DropDown"],
            ControlType::ListView => vec!["SelectedIndexChanged", "DoubleClick", "Click", "ColumnClick"],
            ControlType::TreeView => vec!["AfterSelect", "DoubleClick", "Click"],
            ControlType::RichTextBox => vec!["TextChanged", "KeyDown", "KeyUp"],
            ControlType::TabControl => vec!["SelectedIndexChanged", "Click"],
            ControlType::ToolBar => vec!["Click", "ButtonClick"],
            ControlType::StatusBar => vec![],
            ControlType::UpDown => vec!["ValueChanged"],
        };
        for extra in [
            "GotFocus",
            "LostFocus",
            "KeyDown",
            "KeyUp",
            "MouseMove",
            "MouseEnter",
            "MouseLeave",
            "MouseWheel",
            "Paint",
            "DragDrop",
        ] {
            if !events.contains(&extra) {
                events.push(extra);
            }
        }
        events
    }
    
    pub fn default_properties(&self) -> HashMap<String, String> {
        let mut props = HashMap::new();
        match self {
            ControlType::Button => {
                props.insert("Text".to_string(), "Button".to_string());
                props.insert("Name".to_string(), "Button1".to_string());
                props.insert("Event_Click".to_string(), "Button1_Click".to_string());
            }
            ControlType::Label => {
                props.insert("Text".to_string(), "Label".to_string());
                props.insert("Name".to_string(), "Label1".to_string());
                props.insert("Event_Click".to_string(), "Label1_Click".to_string());
            }
            ControlType::TextBox => {
                props.insert("Text".to_string(), "".to_string());
                props.insert("Name".to_string(), "TextBox1".to_string());
                props.insert("Event_TextChanged".to_string(), "TextBox1_TextChanged".to_string());
            }
            ControlType::CheckBox => {
                props.insert("Text".to_string(), "CheckBox".to_string());
                props.insert("Name".to_string(), "CheckBox1".to_string());
                props.insert("Event_CheckedChanged".to_string(), "CheckBox1_CheckedChanged".to_string());
            }
            ControlType::RadioButton => {
                props.insert("Text".to_string(), "RadioButton".to_string());
                props.insert("Name".to_string(), "RadioButton1".to_string());
                props.insert("Event_CheckedChanged".to_string(), "RadioButton1_CheckedChanged".to_string());
            }
            ControlType::GroupBox => {
                props.insert("Text".to_string(), "GroupBox".to_string());
                props.insert("Name".to_string(), "GroupBox1".to_string());
            }
            ControlType::Panel => {
                props.insert("Name".to_string(), "Panel1".to_string());
            }
            ControlType::PictureBox => {
                props.insert("Name".to_string(), "PictureBox1".to_string());
                props.insert("Event_Click".to_string(), "PictureBox1_Click".to_string());
            }
            ControlType::ListBox => {
                props.insert("Name".to_string(), "ListBox1".to_string());
                props.insert("Event_SelectedIndexChanged".to_string(), "ListBox1_SelectedIndexChanged".to_string());
            }
            ControlType::ComboBox => {
                props.insert("Name".to_string(), "ComboBox1".to_string());
                props.insert("Event_SelectedIndexChanged".to_string(), "ComboBox1_SelectedIndexChanged".to_string());
            }
            ControlType::TrackBar => {
                props.insert("Name".to_string(), "TrackBar1".to_string());
                props.insert("Text".to_string(), "".to_string());
            }
            ControlType::ProgressBar => {
                props.insert("Name".to_string(), "ProgressBar1".to_string());
            }
            ControlType::DateTimePicker => {
                props.insert("Name".to_string(), "DateTimePicker1".to_string());
                props.insert("Text".to_string(), "".to_string());
            }
            ControlType::ListView => {
                props.insert("Name".to_string(), "ListView1".to_string());
            }
            ControlType::TreeView => {
                props.insert("Name".to_string(), "TreeView1".to_string());
            }
            ControlType::RichTextBox => {
                props.insert("Name".to_string(), "RichTextBox1".to_string());
                props.insert("Text".to_string(), "".to_string());
            }
            ControlType::TabControl => {
                props.insert("Name".to_string(), "TabControl1".to_string());
            }
            ControlType::ToolBar => {
                props.insert("Name".to_string(), "ToolBar1".to_string());
            }
            ControlType::StatusBar => {
                props.insert("Name".to_string(), "StatusBar1".to_string());
            }
            ControlType::UpDown => {
                props.insert("Name".to_string(), "UpDown1".to_string());
            }
        }
        props
    }
    
    pub fn native_type_name(&self) -> &'static str {
        match self {
            ControlType::Button => "Button",
            ControlType::Label => "Label",
            ControlType::TextBox => "TextBox",
            ControlType::CheckBox => "CheckBox",
            ControlType::RadioButton => "RadioButton",
            ControlType::GroupBox => "GroupBox",
            ControlType::Panel => "Panel",
            ControlType::PictureBox => "PictureBox",
            ControlType::ListBox => "ListBox",
            ControlType::ComboBox => "ComboBox",
            ControlType::TrackBar => "TrackBar",
            ControlType::ProgressBar => "ProgressBar",
            ControlType::DateTimePicker => "DateTimePicker",
            ControlType::ListView => "ListView",
            ControlType::TreeView => "TreeView",
            ControlType::RichTextBox => "RichTextBox",
            ControlType::TabControl => "TabControl",
            ControlType::ToolBar => "ToolBar",
            ControlType::StatusBar => "StatusBar",
            ControlType::UpDown => "UpDown",
        }
    }
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct FormDesign {
    pub name: String,
    pub width: f32,
    pub height: f32,
    pub controls: Vec<FormControl>,
    pub selected_control: Option<String>,
}

impl Default for FormDesign {
    fn default() -> Self {
        Self {
            name: "Form1".to_string(),
            width: 640.0,
            height: 480.0,
            controls: Vec::new(),
            selected_control: None,
        }
    }
}

impl FormDesign {
    pub fn load(path: &str) -> Option<Self> {
        fs::read_to_string(path).ok().and_then(|c| serde_json::from_str(&c).ok())
    }

    pub fn save(&self, path: &str) -> std::io::Result<()> {
        let json = serde_json::to_string_pretty(self)?;
        fs::write(path, json)?;
        Ok(())
    }
}

pub struct FormDesigner {
    pub form: FormDesign,
    pub snap_to_grid: bool,
    pub grid_size: f32,
    pub drag_offset: Option<(f32, f32)>,
    pub dragged_control: Option<String>,
    pub form_path: Option<String>,
    pub dirty: bool,
    pub show_events: bool,
    pub event_filter: String,
}

impl FormDesigner {
    pub fn new() -> Self {
        Self {
            form: FormDesign::default(),
            snap_to_grid: true,
            grid_size: 8.0,
            drag_offset: None,
            dragged_control: None,
            form_path: None,
            dirty: false,
            show_events: false,
            event_filter: String::new(),
        }
    }

    pub fn new_form(&mut self, name: &str) {
        self.form = FormDesign::default();
        self.form.name = name.to_string();
        self.form.controls.clear();
        self.form.selected_control = None;
        self.form_path = None;
        self.dirty = false;
        self.dragged_control = None;
        self.drag_offset = None;
    }

    pub fn open_form(&mut self, path: &str) {
        match FormDesign::load(path) {
            Some(form) => {
                self.form = form;
                self.form_path = Some(path.to_string());
                self.dirty = false;
                self.dragged_control = None;
                self.drag_offset = None;
            }
            None => {
                eprintln!("Failed to open form: {}", path);
            }
        }
    }

    pub fn save_form(&mut self) -> std::io::Result<()> {
        let path = self.form_path.clone().unwrap_or_else(|| format!("{}.bfform", self.form.name));
        self.form.save(&path)?;
        self.form_path = Some(path);
        self.dirty = false;
        Ok(())
    }

    pub fn save_form_as(&mut self, path: &str) -> std::io::Result<()> {
        self.form.save(path)?;
        self.form_path = Some(path.to_string());
        self.dirty = false;
        Ok(())
    }

    fn add_control(&mut self, control_type: ControlType) {
        let id = format!("{}_{}", 
            control_type.display_name().to_lowercase(), 
            self.form.controls.len() + 1);
        
        let (default_w, default_h) = control_type.default_size();
     
        let control = FormControl {
            id: id.clone(),
            control_type: control_type,
            x: 20.0 + (self.form.controls.len() as f32 * 30.0) % (self.form.width - 100.0),
            y: 20.0 + (self.form.controls.len() as f32 * 25.0) % (self.form.height - 100.0),
            width: default_w,
            height: default_h,
            properties: control_type.default_properties(),
            events: HashMap::new(),
        };
     
        self.form.controls.push(control);
        self.form.selected_control = Some(id);
        self.dirty = true;
    }

    pub fn ui(&mut self, ui: &mut egui::Ui, _project: &Project, _editor: &mut Editor, _output: &mut OutputWindow) {
        ui.heading("Form Designer");
        ui.colored_label(
            egui::Color32::from_rgb(80, 180, 100),
            "Generates compilable BasicForge Form code (Form / controls / events).",
        );
        ui.separator();
        
        // Top toolbar
        ui.horizontal(|ui| {
            if ui.button("New Form").clicked() {
                self.new_form("Form1");
            }
            if ui.button("Open Form").clicked() {
                _output.log("File dialog not yet implemented - use Project Explorer");
            }
            if ui.button("Save Form").clicked() {
                let _ = self.save_form();
            }
            if ui.button("Generate Code").clicked() {
                let code = self.generate_code();
                ui.ctx().output_mut(|o| o.copied_text = code.clone());
                _output.log(&format!("Form code generated ({} bytes), copied to clipboard.", code.len()));
            }
            ui.checkbox(&mut self.snap_to_grid, "Snap to Grid");
            ui.separator();
            ui.checkbox(&mut self.show_events, "Show Events");
        });
        
        ui.separator();
        
        // Tab bar for Properties/Events
        ui.horizontal(|ui| {
            ui.selectable_value(&mut self.show_events, false, "Properties");
            ui.selectable_value(&mut self.show_events, true, "Events");
        });
        
        ui.separator();
        
        // Split: Toolbox | Canvas | Properties/Events
        egui::ScrollArea::vertical().show(ui, |ui| {
            egui::Grid::new("designer_grid").num_columns(3).min_col_width(120.0).show(ui, |ui| {
                // Column 1: Toolbox
                ui.vertical(|ui| {
                    ui.label("Toolbox");
                    ui.separator();
                    for control_type in ControlType::all() {
                        let label = format!("{} {}", control_type.icon(), control_type.display_name());
                        if ui.selectable_label(false, label).clicked() {
                            self.add_control(control_type);
                        }
                    }
                });
                
                ui.vertical(|ui| {
                    ui.label("Canvas");
                    ui.separator();
                    self.render_canvas(ui);
                });
                
                ui.vertical(|ui| {
                    if self.show_events {
                        ui.label("Event Designer");
                        ui.separator();
                        self.render_event_designer(ui);
                    } else {
                        ui.label("Properties");
                        ui.separator();
                        self.render_properties(ui);
                    }
                });
            });
        });
    }
    
    fn render_properties(&mut self, ui: &mut egui::Ui) {
        let selected_id = self.form.selected_control.clone();
        if let Some(selected_id) = selected_id {
            if let Some(idx) = self.form.controls.iter().position(|c| c.id == selected_id) {
                let mut control = self.form.controls[idx].clone();
                ui.label(format!("ID: {}", control.id));
                ui.label(format!("Type: {}", control.control_type.display_name()));
                
                egui::Grid::new("properties_grid").num_columns(2).show(ui, |ui| {
                    ui.label("X:");
                    let mut x = control.x;
                    if ui.add(egui::DragValue::new(&mut x).speed(1.0)).changed() {
                        if let Some(c) = self.form.controls.get_mut(idx) {
                            c.x = if self.snap_to_grid { 
                                (x / self.grid_size).round() * self.grid_size 
                            } else { x };
                        }
                    }
                    ui.end_row();
                    
                    ui.label("Y:");
                    let mut y = control.y;
                    if ui.add(egui::DragValue::new(&mut y).speed(1.0)).changed() {
                        if let Some(c) = self.form.controls.get_mut(idx) {
                            c.y = if self.snap_to_grid { 
                                (y / self.grid_size).round() * self.grid_size 
                            } else { y };
                        }
                    }
                    ui.end_row();
                    
                    ui.label("Width:");
                    let mut w = control.width;
                    if ui.add(egui::DragValue::new(&mut w).speed(1.0).range(1.0..=2000.0)).changed() {
                        if let Some(c) = self.form.controls.get_mut(idx) {
                            c.width = w;
                        }
                    }
                    ui.end_row();
                    
                    ui.label("Height:");
                    let mut h = control.height;
                    if ui.add(egui::DragValue::new(&mut h).speed(1.0).range(1.0..=2000.0)).changed() {
                        if let Some(c) = self.form.controls.get_mut(idx) {
                            c.height = h;
                        }
                    }
                    ui.end_row();
                    
                    // Separate regular properties from events
                    let props: Vec<_> = control.properties.iter()
                        .filter(|(k, _)| !k.starts_with("Event_"))
                        .map(|(k, v)| (k.clone(), v.clone()))
                        .collect();
                    
                    for (key, value) in props {
                        ui.label(&key);
                        let mut v = value;
                        if ui.text_edit_singleline(&mut v).changed() {
                            if let Some(c) = self.form.controls.get_mut(idx) {
                                c.properties.insert(key, v);
                            }
                        }
                        ui.end_row();
                    }
                });
            }
        } else {
            ui.label("Form Properties:");
            egui::Grid::new("form_properties_grid").num_columns(2).show(ui, |ui| {
                ui.label("Name:");
                ui.text_edit_singleline(&mut self.form.name);
                ui.end_row();
                ui.label("Width:");
                let mut w = self.form.width;
                if ui.add(egui::DragValue::new(&mut w).speed(1.0).range(100.0..=5000.0)).changed() {
                    self.form.width = w;
                }
                ui.end_row();
                ui.label("Height:");
                let mut h = self.form.height;
                if ui.add(egui::DragValue::new(&mut h).speed(1.0).range(100.0..=5000.0)).changed() {
                    self.form.height = h;
                }
                ui.end_row();
            });
        }
    }
    
    fn render_event_designer(&mut self, ui: &mut egui::Ui) {
        let selected_id = self.form.selected_control.clone();
        if let Some(selected_id) = selected_id {
            if let Some(idx) = self.form.controls.iter().position(|c| c.id == selected_id) {
                // Collect event data before borrowing self mutably in the closure
                let control_type = self.form.controls[idx].control_type;
                let control_id = self.form.controls[idx].id.clone();
                let properties = self.form.controls[idx].properties.clone();
                
                ui.label(format!("Control: {} ({})", control_id, control_type.display_name()));
                ui.separator();
                
                // Event filter
                ui.horizontal(|ui| {
                    ui.label("Filter:");
                    ui.text_edit_singleline(&mut self.event_filter);
                });
                ui.separator();
                
                let available_events = control_type.available_events();
                let filter = self.event_filter.to_lowercase();
                
                egui::ScrollArea::vertical().max_height(300.0).show(ui, |ui| {
                    for event_name in available_events {
                        if !filter.is_empty() && !event_name.to_lowercase().contains(&filter) {
                            continue;
                        }
                        
                        let event_key = format!("Event_{}", event_name);
                        let current_handler = properties.get(&event_key)
                            .cloned()
                            .unwrap_or_else(|| format!("{}_{}", control_id, event_name));
                        
                        ui.horizontal(|ui| {
                            ui.label(event_name);
                            ui.separator();
                            let mut handler = current_handler;
                            if ui.text_edit_singleline(&mut handler).changed() {
                                if let Some(c) = self.form.controls.get_mut(idx) {
                                    c.properties.insert(event_key, handler);
                                }
                            }
                        });
                    }
                });
                
                ui.separator();
                ui.label("Double-click an event to create handler in code editor");
            }
        } else {
            ui.label("Select a control to edit its events");
        }
    }

    fn render_canvas(&mut self, ui: &mut egui::Ui) {
        let canvas_size = egui::Vec2::new(self.form.width, self.form.height);
        
        let (rect, _) = ui.allocate_exact_size(canvas_size, egui::Sense::click_and_drag());
        
        // Draw form background
        ui.painter().rect_filled(rect, 0.0, egui::Color32::from_rgb(240, 240, 240));
        ui.painter().rect_stroke(rect, 0.0, egui::Stroke::new(1.0, egui::Color32::from_rgb(180, 180, 180)));
        
        // Draw grid
        if self.snap_to_grid {
            let grid = self.grid_size;
            let mut x = 0.0;
            while x < self.form.width {
                ui.painter().line_segment(
                    [rect.left_top() + egui::Vec2::new(x, 0.0), rect.left_top() + egui::Vec2::new(x, self.form.height)],
                    egui::Stroke::new(0.5, egui::Color32::from_rgb(220, 220, 220)),
                );
                x += grid;
            }
            let mut y = 0.0;
            while y < self.form.height {
                ui.painter().line_segment(
                    [rect.left_top() + egui::Vec2::new(0.0, y), rect.left_top() + egui::Vec2::new(self.form.width, y)],
                    egui::Stroke::new(0.5, egui::Color32::from_rgb(220, 220, 220)),
                );
                y += grid;
            }
        }
        
        // Handle drag from toolbox to canvas (we check if a control type was just added)
        // Handle dragging existing controls
        if let Some(ref dragged_id) = self.dragged_control {
            if let Some(pointer_pos) = ui.input(|i| i.pointer.hover_pos()) {
                let local_pos = pointer_pos - rect.min;
                if let Some(idx) = self.form.controls.iter().position(|c| c.id == *dragged_id) {
                    let control = &self.form.controls[idx];
                    let offset = self.drag_offset.unwrap_or((0.0, 0.0));
                    let new_x = local_pos.x - offset.0;
                    let new_y = local_pos.y - offset.1;
                    if let Some(c) = self.form.controls.get_mut(idx) {
                        c.x = if self.snap_to_grid { 
                            (new_x / self.grid_size).round() * self.grid_size 
                        } else { new_x };
                        c.y = if self.snap_to_grid { 
                            (new_y / self.grid_size).round() * self.grid_size 
                        } else { new_y };
                        c.x = c.x.clamp(0.0, self.form.width - c.width);
                        c.y = c.y.clamp(0.0, self.form.height - c.height);
                    }
                }
            }
        }
        
        // Draw controls
        for control in &self.form.controls {
            let ctrl_rect = egui::Rect::from_min_size(
                rect.left_top() + egui::Vec2::new(control.x, control.y),
                egui::Vec2::new(control.width, control.height),
            );
            
            let is_selected = self.form.selected_control.as_ref() == Some(&control.id);
            let fill = if is_selected { egui::Color32::from_rgb(0, 120, 215) } else { egui::Color32::from_rgb(255, 255, 255) };
            let stroke = egui::Stroke::new(1.0, if is_selected { egui::Color32::from_rgb(0, 90, 160) } else { egui::Color32::from_rgb(150, 150, 150) });
            
            ui.painter().rect_filled(ctrl_rect, 2.0, fill);
            ui.painter().rect_stroke(ctrl_rect, 2.0, stroke);
            
            // Control label
            let label = control
            .properties
            .get("Text")
            .map(|s| s.as_str())
            .unwrap_or(&control.id);
            ui.painter().text(
                ctrl_rect.center(),
                egui::Align2::CENTER_CENTER,
                &label,
                egui::FontId::default(),
                egui::Color32::from_rgb(0, 0, 0),
            );
            
            // Handle click selection
            let response = ui.interact(ctrl_rect, ui.id().with(&control.id), egui::Sense::click_and_drag());
            if response.clicked() {
                self.form.selected_control = Some(control.id.clone());
                self.dirty = true;
            }
            if response.drag_started() {
                self.dragged_control = Some(control.id.clone());
                let pointer_pos = ui.input(|i| i.pointer.hover_pos().unwrap_or(rect.left_top()));
                let local_pos = pointer_pos - rect.min;
                self.drag_offset = Some((local_pos.x - control.x, local_pos.y - control.y));
            }
            if response.drag_stopped() {
                self.dragged_control = None;
                self.drag_offset = None;
                self.dirty = true;
            }
        }
        
        // Handle clicking on empty canvas to deselect
        if ui.input(|i| i.pointer.primary_clicked()) && !rect.contains(ui.input(|i| i.pointer.hover_pos().unwrap_or(rect.left_top()))) {
            self.form.selected_control = None;
            self.dragged_control = None;
            self.drag_offset = None;
        }
    }

    fn default_properties(control_type: &ControlType) -> HashMap<String, String> {
        let mut props = HashMap::new();
        match control_type {
            ControlType::Button => {
                props.insert("Text".to_string(), "Button".to_string());
                props.insert("Name".to_string(), "Button1".to_string());
                props.insert("Click".to_string(), "Button1_Click".to_string());
            }
            ControlType::Label => {
                props.insert("Text".to_string(), "Label".to_string());
                props.insert("Name".to_string(), "Label1".to_string());
                props.insert("Click".to_string(), "Label1_Click".to_string());
            }
            ControlType::TextBox => {
                props.insert("Text".to_string(), "".to_string());
                props.insert("Name".to_string(), "TextBox1".to_string());
                props.insert("TextChanged".to_string(), "TextBox1_TextChanged".to_string());
            }
            ControlType::CheckBox => {
                props.insert("Text".to_string(), "CheckBox".to_string());
                props.insert("Name".to_string(), "CheckBox1".to_string());
                props.insert("CheckedChanged".to_string(), "CheckBox1_CheckedChanged".to_string());
            }
            ControlType::RadioButton => {
                props.insert("Text".to_string(), "RadioButton".to_string());
                props.insert("Name".to_string(), "RadioButton1".to_string());
                props.insert("CheckedChanged".to_string(), "RadioButton1_CheckedChanged".to_string());
            }
            ControlType::GroupBox => {
                props.insert("Text".to_string(), "GroupBox".to_string());
                props.insert("Name".to_string(), "GroupBox1".to_string());
            }
            ControlType::Panel => {
                props.insert("Name".to_string(), "Panel1".to_string());
            }
            ControlType::PictureBox => {
                props.insert("Name".to_string(), "PictureBox1".to_string());
                props.insert("Click".to_string(), "PictureBox1_Click".to_string());
            }
            ControlType::ListBox => {
                props.insert("Name".to_string(), "ListBox1".to_string());
                props.insert("SelectedIndexChanged".to_string(), "ListBox1_SelectedIndexChanged".to_string());
            }
            ControlType::ComboBox => {
                props.insert("Name".to_string(), "ComboBox1".to_string());
                props.insert("SelectedIndexChanged".to_string(), "ComboBox1_SelectedIndexChanged".to_string());
            }
            ControlType::TrackBar => {
                props.insert("Name".to_string(), "TrackBar1".to_string());
                props.insert("Text".to_string(), "".to_string());
            }
            ControlType::ProgressBar => {
                props.insert("Name".to_string(), "ProgressBar1".to_string());
            }
            ControlType::DateTimePicker => {
                props.insert("Name".to_string(), "DateTimePicker1".to_string());
                props.insert("Text".to_string(), "".to_string());
            }
            ControlType::ListView => {
                props.insert("Name".to_string(), "ListView1".to_string());
            }
            ControlType::TreeView => {
                props.insert("Name".to_string(), "TreeView1".to_string());
            }
            ControlType::RichTextBox => {
                props.insert("Name".to_string(), "RichTextBox1".to_string());
                props.insert("Text".to_string(), "".to_string());
            }
            ControlType::TabControl => {
                props.insert("Name".to_string(), "TabControl1".to_string());
            }
            ControlType::ToolBar => {
                props.insert("Name".to_string(), "ToolBar1".to_string());
            }
            ControlType::StatusBar => {
                props.insert("Name".to_string(), "StatusBar1".to_string());
            }
            ControlType::UpDown => {
                props.insert("Name".to_string(), "UpDown1".to_string());
            }
        }
        props
    }

    pub fn generate_code(&self) -> String {
        let esc = |s: &str| s.replace('\\', "\\\\").replace('"', "\\\"");
        let mut code = String::new();
        let mut handlers: Vec<String> = Vec::new();

        code.push_str(&format!("Form {}\n", self.form.name));
        code.push_str(&format!("    Width = {}\n", self.form.width as i32));
        code.push_str(&format!("    Height = {}\n", self.form.height as i32));
        code.push_str(&format!("    Text = \"{}\"\n\n", esc(&self.form.name)));

        for control in &self.form.controls {
            let kind = control.control_type.display_name();
            code.push_str(&format!("    {} {}\n", kind, control.id));

            if let Some(text) = control.properties.get("Text") {
                code.push_str(&format!("        Text = \"{}\"\n", esc(text)));
            }
            code.push_str(&format!("        Left = {}\n", control.x as i32));
            code.push_str(&format!("        Top = {}\n", control.y as i32));
            code.push_str(&format!("        Width = {}\n", control.width as i32));
            code.push_str(&format!("        Height = {}\n", control.height as i32));

            for (key, value) in &control.properties {
                if let Some(event) = key.strip_prefix("Event_") {
                    code.push_str(&format!("        On{} = {}\n", event, value));
                    handlers.push(value.clone());
                }
            }
            code.push_str(&format!("    End {}\n\n", kind));
        }

        code.push_str("End Form\n\n");

        for handler in handlers {
            code.push_str(&format!("Sub {}\n", handler));
            code.push_str("    ' TODO: handle event\n");
            code.push_str("End Sub\n\n");
        }

        code
    }
}

impl Default for FormDesigner {
    fn default() -> Self {
        Self::new()
    }
}