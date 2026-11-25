use std::io::{BufRead, BufReader, Read, Write};
use std::process::{Child, ChildStdin, Command, Stdio};
use std::sync::mpsc;

pub struct LspClient {
    child: Child,
    stdin: ChildStdin,
    rx: mpsc::Receiver<String>,
}

impl LspClient {
    pub fn start(bfc: &str) -> std::io::Result<Self> {
        let mut child = Command::new(bfc)
            .arg("--lsp")
            .stdin(Stdio::piped())
            .stdout(Stdio::piped())
            .spawn()?;
        let stdin = child.stdin.take().unwrap();
        let stdout = child.stdout.take().unwrap();
        let (tx, rx) = mpsc::channel();
        std::thread::spawn(move || {
            let mut reader = BufReader::new(stdout);
            loop {
                let mut content_length = 0usize;
                let mut line = String::new();
                if reader.read_line(&mut line).unwrap_or(0) == 0 {
                    break;
                }
                if line.to_ascii_lowercase().starts_with("content-length:") {
                    content_length = line[15..].trim().parse().unwrap_or(0);
                }
                if line == "\r\n" || line == "\n" {
                    if content_length > 0 {
                        let mut buf = vec![0u8; content_length];
                        if reader.read_exact(&mut buf).is_ok() {
                            if let Ok(text) = String::from_utf8(buf) {
                                if tx.send(text).is_err() {
                                    break;
                                }
                            }
                        }
                    }
                    content_length = 0;
                }
            }
        });
        Ok(Self { child, stdin, rx })
    }

    pub fn send(&mut self, msg: &str) {
        let _ = write!(
            self.stdin,
            "Content-Length: {}\r\n\r\n{}",
            msg.as_bytes().len(),
            msg
        );
        let _ = self.stdin.flush();
    }

    pub fn notify(&mut self, method: &str, params: &str) {
        let msg = format!(
            r#"{{"jsonrpc":"2.0","method":"{}","params":{}}}"#,
            method, params
        );
        self.send(&msg);
    }

    pub fn open(&mut self, uri: &str, text: &str) {
        let params = format!(
            r#"{{"textDocument":{{"uri":"{}","text":"{}"}}}}"#,
            uri,
            text.replace('\\', "\\\\").replace('"', "\\\"").replace('\n', "\\n")
        );
        self.notify("textDocument/didOpen", &params);
    }

    pub fn poll(&self) -> Vec<String> {
        let mut out = Vec::new();
        while let Ok(msg) = self.rx.try_recv() {
            out.push(msg);
        }
        out
    }

    pub fn stop(&mut self) {
        let _ = self.child.kill();
    }
}