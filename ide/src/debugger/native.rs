use std::ffi::OsStr;
use std::os::windows::ffi::OsStrExt;
use std::ptr;

use windows_sys::Win32::Foundation::{CloseHandle, HANDLE};
use windows_sys::Win32::System::Diagnostics::Debug::{
    ContinueDebugEvent, WaitForDebugEvent, DEBUG_EVENT, CREATE_PROCESS_DEBUG_EVENT,
    EXCEPTION_DEBUG_EVENT, EXIT_PROCESS_DEBUG_EVENT,
};
use windows_sys::Win32::System::Threading::{
    CreateProcessW, TerminateProcess, DEBUG_ONLY_THIS_PROCESS, PROCESS_INFORMATION, STARTUPINFOW,
};

pub struct NativeDebugger {
    pub pid: u32,
    pub running: bool,
    process: HANDLE,
}

impl NativeDebugger {
    pub fn start(exe: &str) -> std::io::Result<Self> {
        let exe_w: Vec<u16> = OsStr::new(exe).encode_wide().chain(Some(0)).collect();
        let mut si: STARTUPINFOW = unsafe { std::mem::zeroed() };
        si.cb = std::mem::size_of::<STARTUPINFOW>() as u32;
        let mut pi: PROCESS_INFORMATION = unsafe { std::mem::zeroed() };
        let ok = unsafe {
            CreateProcessW(
                exe_w.as_ptr(),
                ptr::null_mut(),
                ptr::null(),
                ptr::null(),
                0,
                DEBUG_ONLY_THIS_PROCESS,
                ptr::null(),
                ptr::null(),
                &mut si,
                &mut pi,
            )
        };
        if ok == 0 {
            return Err(std::io::Error::last_os_error());
        }
        Ok(Self { pid: pi.dwProcessId, running: true, process: pi.hProcess })
    }

    pub fn wait(&mut self) -> Option<String> {
        let mut event: DEBUG_EVENT = unsafe { std::mem::zeroed() };
        if unsafe { WaitForDebugEvent(&mut event, 1000) } == 0 {
            return None;
        }
        let mut msg = String::new();
        match event.dwDebugEventCode {
            CREATE_PROCESS_DEBUG_EVENT => msg = "process created".into(),
            EXCEPTION_DEBUG_EVENT => {
                if (unsafe { event.u.Exception.ExceptionRecord.ExceptionCode } as u32) == 0x80000003u32 {
                    msg = "system breakpoint".into();
                }
            }
            EXIT_PROCESS_DEBUG_EVENT => {
                self.running = false;
                msg = "exited".into();
            }
            _ => {}
        }
        unsafe {
            ContinueDebugEvent(event.dwProcessId, event.dwThreadId, 1);
        }
        if msg.is_empty() { None } else { Some(msg) }
    }

    pub fn stop(&mut self) {
        if self.running {
            unsafe {
                TerminateProcess(self.process, 0);
                CloseHandle(self.process);
            }
            self.running = false;
        }
    }
}

pub fn disassemble(exe: &str) -> String {
    match std::process::Command::new("llvm-objdump")
        .args(["-d", "--no-show-raw-insn", exe])
        .output()
    {
        Ok(output) => String::from_utf8_lossy(&output.stdout).to_string(),
        Err(_) => String::new(),
    }
}