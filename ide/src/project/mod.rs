use serde::{Deserialize, Serialize};
use std::fs;
use std::path::Path;

#[derive(Debug, Serialize, Deserialize)]
pub struct ProjectFile {
    pub name: String,
    pub version: String,
    pub files: Vec<String>,
    pub build_config: BuildConfig,
}

#[derive(Debug, Serialize, Deserialize, Clone)]
pub struct BuildConfig {
    pub output_name: String,
    pub target: String,
    pub debug: bool,
    pub optimize: bool,
}

impl Default for BuildConfig {
    fn default() -> Self {
        Self {
            output_name: "MyApp.exe".to_string(),
            target: "x64".to_string(),
            debug: true,
            optimize: false,
        }
    }
}

pub struct Project {
    pub name: String,
    pub path: String,
    pub files: Vec<String>,
    pub config: BuildConfig,
}

impl Project {
    pub fn new() -> Self {
        Self {
            name: String::new(),
            path: String::new(),
            files: Vec::new(),
            config: BuildConfig::default(),
        }
    }

    pub fn load(&mut self, path: &str) {
        match fs::read_to_string(path) {
            Ok(content) => {
                match serde_json::from_str::<ProjectFile>(&content) {
                    Ok(proj_file) => {
                        self.name = proj_file.name;
                        self.path = path.to_string();
                        self.files = proj_file.files;
                        self.config = proj_file.build_config;
                    }
                    Err(e) => {
                        eprintln!("Parse project error: {}", e);
                    }
                }
            }
            Err(e) => {
                eprintln!("Open project error: {}", e);
            }
        }
    }

    pub fn save(&self) -> Result<(), Box<dyn std::error::Error>> {
        let proj_file = ProjectFile {
            name: self.name.clone(),
            version: "0.1.0".to_string(),
            files: self.files.clone(),
            build_config: self.config.clone(),
        };
        let json = serde_json::to_string_pretty(&proj_file)?;
        fs::write(&self.path, json)?;
        Ok(())
    }

    pub fn create_new(&mut self, name: &str, path: &str) {
        self.name = name.to_string();
        self.path = path.to_string();
        self.files = Vec::new();
        self.config = BuildConfig::default();
    }

    pub fn add_file(&mut self, file_path: &str) {
        if !self.files.contains(&file_path.to_string()) {
            self.files.push(file_path.to_string());
        }
    }

    pub fn remove_file(&mut self, file_path: &str) {
        self.files.retain(|f| f != file_path);
    }

    pub fn ui(&mut self, ui: &mut eframe::egui::Ui) {
        ui.heading("Project Explorer");
        ui.separator();

        ui.label(format!("Name: {}", self.name));
        ui.text_edit_singleline(&mut self.name);
        ui.separator();

        ui.label("Build Output:");
        ui.text_edit_singleline(&mut self.config.output_name);
        ui.checkbox(&mut self.config.debug, "Debug");
        ui.checkbox(&mut self.config.optimize, "Optimize");
        ui.separator();

        ui.label("Files:");
        let files_to_remove: Vec<usize> = self
            .files
            .iter()
            .enumerate()
            .filter_map(|(i, f)| {
                let file_name = Path::new(f).file_name().unwrap_or_default().to_string_lossy().to_string();
                let removed = ui.button(format!("✕ {}", file_name)).clicked();
                if removed {
                    Some(i)
                } else {
                    None
                }
            })
            .collect();

        for idx in files_to_remove.iter().rev() {
            self.files.remove(*idx);
        }

        ui.separator();
        if ui.button("Add File").clicked() {
            // TODO: file dialog
        }
        if ui.button("Build").clicked() {
            // Build will be handled by parent
        }
        if ui.button("Run").clicked() {
            // Run will be handled by parent
        }
    }
}

impl Default for Project {
    fn default() -> Self {
        Self::new()
    }
}
