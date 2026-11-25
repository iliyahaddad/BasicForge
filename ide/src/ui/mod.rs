use eframe::egui;

pub struct OutputWindow {
    lines: Vec<String>,
    auto_scroll: bool,
}

impl OutputWindow {
    pub fn new() -> Self {
        Self {
            lines: Vec::new(),
            auto_scroll: true,
        }
    }

    pub fn log(&mut self, message: &str) {
        self.lines.push(message.to_string());
        if self.lines.len() > 1000 {
            self.lines.remove(0);
        }
    }

    pub fn clear(&mut self) {
        self.lines.clear();
    }

    pub fn ui(&mut self, ui: &mut eframe::egui::Ui) {
        ui.horizontal(|ui| {
            if ui.button("Clear").clicked() {
                self.clear();
            }
            ui.checkbox(&mut self.auto_scroll, "Auto-scroll");
        });
        ui.separator();

        egui::ScrollArea::both().id_salt("output_scroll").show(ui, |ui| {
            for line in &self.lines {
                ui.label(line);
            }
            if self.auto_scroll {
                ui.ctx().request_repaint_after(std::time::Duration::from_millis(100));
            }
        });
    }
}

impl Default for OutputWindow {
    fn default() -> Self {
        Self::new()
    }
}
