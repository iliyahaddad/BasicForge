// Form Designer native UI runtime
// Win32-based UI rendering for forms

use std::collections::HashMap;
use std::sync::Arc;
use tokio::sync::RwLock;

pub struct NativeForm {
    pub controls: Arc<RwLock<Vec<NativeControl>>>,
    pub width: i32,
    pub height: i32,
}

#[derive(Debug, Clone)]
pub struct NativeControl {
    pub id: String,
    pub control_type: ControlType,
    pub x: i32,
    pub y: i32,
    pub width: i32,
    pub height: i32,
    pub text: String,
}

#[derive(Debug, Clone, PartialEq)]
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
}

impl NativeForm {
    pub fn new(name: &str, width: i32, height: i32) -> Self {
        Self {
            controls: Arc::new(RwLock::new(Vec::new())),
            width,
            height,
        }
    }

    pub async fn add_control(&self, control: NativeControl) {
        let mut controls = self.controls.write().await;
        controls.push(control);
    }

    pub async fn remove_control(&self, id: &str) {
        let mut controls = self.controls.write().await;
        controls.retain(|c| c.id != id);
    }

    pub async fn get_controls(&self) -> Vec<NativeControl> {
        let controls = self.controls.read().await;
        controls.clone()
    }

    pub async fn render(&self) -> String {
        let controls = self.controls.read().await;
        let mut html = format!("<form width=\"{}\" height=\"{}\">", self.width, self.height);
        for control in controls.iter() {
            html += &format!("<control type=\"{:?}\" x=\"{}\" y=\"{}\" w=\"{}\" h=\"{}\" text=\"{}\"/>",
                control.control_type, control.x, control.y, control.width, control.height, control.text);
        }
        html += "</form>";
        html
    }
}