// Debugger DAP (Debug Adapter Protocol) stub
// This provides a basic DAP implementation for the IDE

use std::collections::HashMap;
use std::sync::Arc;
use tokio::sync::RwLock;

pub struct DebugAdapter {
    breakpoints: Arc<RwLock<HashMap<String, Vec<Breakpoint>>>>,
    stack_frames: Arc<RwLock<Vec<StackFrame>>>,
    variables: Arc<RwLock<HashMap<String, String>>>,
}

#[derive(Debug, Clone)]
pub struct Breakpoint {
    pub line: usize,
    pub verified: bool,
}

#[derive(Debug, Clone)]
pub struct StackFrame {
    pub id: usize,
    pub name: String,
    pub file: String,
    pub line: usize,
}

impl DebugAdapter {
    pub fn new() -> Self {
        Self {
            breakpoints: Arc::new(RwLock::new(HashMap::new())),
            stack_frames: Arc::new(RwLock::new(Vec::new())),
            variables: Arc::new(RwLock::new(HashMap::new())),
        }
    }

    pub async fn set_breakpoint(&self, file: &str, line: usize) {
        let mut bps = self.breakpoints.write().await;
        bps.entry(file.to_string()).or_default().push(Breakpoint { line, verified: true });
    }

    pub async fn remove_breakpoint(&self, file: &str, line: usize) {
        let mut bps = self.breakpoints.write().await;
        if let Some(bps) = bps.get_mut(file) {
            bps.retain(|bp| bp.line != line);
        }
    }

    pub async fn get_breakpoints(&self, file: &str) -> Vec<Breakpoint> {
        let bps = self.breakpoints.read().await;
        bps.get(file).cloned().unwrap_or_default()
    }

    pub async fn set_variable(&self, name: &str, value: &str) {
        let mut vars = self.variables.write().await;
        vars.insert(name.to_string(), value.to_string());
    }

    pub async fn get_variable(&self, name: &str) -> Option<String> {
        let vars = self.variables.read().await;
        vars.get(name).cloned()
    }

    pub async fn push_frame(&self, frame: StackFrame) {
        let mut frames = self.stack_frames.write().await;
        frames.push(frame);
    }

    pub async fn clear_frames(&self) {
        let mut frames = self.stack_frames.write().await;
        frames.clear();
    }

    pub async fn get_frames(&self) -> Vec<StackFrame> {
        let frames = self.stack_frames.read().await;
        frames.clone()
    }
}

impl Default for DebugAdapter {
    fn default() -> Self {
        Self::new()
    }
}