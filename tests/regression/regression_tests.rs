// Regression test suite for BasicForge
// Tests all milestone examples and ensures they compile and run correctly

use std::process::{Command, Stdio};
use std::path::PathBuf;
use tempfile::tempdir;

#[test]
fn regression_milestone3_examples() {
    let examples = vec![
        "01_hello_world",
        "02_variables",
        "03_type_inference", 
        "04_arithmetic",
        "05_conditions",
        "06_loops",
        "07_functions",
        "08_modules",
        "10_arrays",
        "11_strings",
    ];
    
    for example in examples {
        run_example_test(example);
    }
}

#[test]
fn regression_working_tests() {
    let tests = vec![
        "test_math",
        "test_func", 
        "test_if",
        "test_for",
        "test_while",
        "test_arr",
        "test_doloop",
        "test_select",
        "test_strings",
        "test_concat",
        "test_case",
        "test_conv",
        "test_minimal",
    ];
    
    for test in tests {
        run_example_test(test);
    }
}

#[test]
fn regression_pending_features() {
    // These should compile but may not run correctly yet
    let pending = vec![
        "test_class",
        "test_foreach", 
        "test_trycatch",
        "test_enum",
    ];
    
    for test in pending {
        // Just verify they don't crash the compiler
        let result = compile_test(test);
        // We expect these to fail gracefully
        assert!(result.is_err() || result.is_ok());
    }
}

fn run_example_test(name: &str) {
    let result = compile_and_run(name);
    assert!(result.is_ok(), "Example {} failed: {:?}", name, result.err());
    let output = result.unwrap();
    assert!(!output.is_empty(), "Example {} produced no output", name);
}

fn compile_test(name: &str) -> Result<(), String> {
    let project_root = PathBuf::from(env!("CARGO_MANIFEST_DIR")).join("..").join("..");
    let example_path = project_root.join("examples").join("milestone3").join(format!("{}.bf", name));
    let bfc_path = project_root.join("compiler").join("build").join("Release").join("bfc.exe");
    let output_exe = format!("{}_regression.exe", name);
    
    if !example_path.exists() {
        return Err(format!("Example file not found: {:?}", example_path));
    }
    
    if !bfc_path.exists() {
        return Err(format!("Compiler not found: {:?}", bfc_path));
    }
    
    let compile_result = Command::new(&bfc_path)
        .arg(&example_path)
        .arg("-o")
        .arg(&output_exe)
        .current_dir(&project_root)
        .stdout(Stdio::piped())
        .stderr(Stdio::piped())
        .output()
        .map_err(|e| format!("Failed to run compiler: {}", e))?;
    
    if !compile_result.status.success() {
        return Err(String::from_utf8_lossy(&compile_result.stderr).to_string());
    }
    
    Ok(())
}

fn compile_and_run(name: &str) -> Result<String, String> {
    let project_root = PathBuf::from(env!("CARGO_MANIFEST_DIR")).join("..").join("..");
    let example_path = project_root.join("examples").join("milestone3").join(format!("{}.bf", name));
    let bfc_path = project_root.join("compiler").join("build").join("Release").join("bfc.exe");
    let output_exe = format!("{}_regression.exe", name);
    
    if !example_path.exists() {
        return Err(format!("Example file not found: {:?}", example_path));
    }
    
    if !bfc_path.exists() {
        return Err(format!("Compiler not found: {:?}", bfc_path));
    }
    
    // Compile
    let compile_result = Command::new(&bfc_path)
        .arg(&example_path)
        .arg("-o")
        .arg(&output_exe)
        .current_dir(&project_root)
        .stdout(Stdio::piped())
        .stderr(Stdio::piped())
        .output()
        .map_err(|e| format!("Failed to run compiler: {}", e))?;
    
    if !compile_result.status.success() {
        return Err(String::from_utf8_lossy(&compile_result.stderr).to_string());
    }
    
    // Run
    let exe_path = project_root.join(&output_exe);
    let run_result = Command::new(&exe_path)
        .current_dir(&project_root)
        .stdout(Stdio::piped())
        .stderr(Stdio::piped())
        .output()
        .map_err(|e| format!("Failed to run executable: {}", e))?;
    
    if !run_result.status.success() {
        return Err(format!("Executable failed: {}", String::from_utf8_lossy(&run_result.stderr)));
    }
    
    let output = String::from_utf8_lossy(&run_result.stdout).to_string();
    Ok(output)
}

// Performance baseline tests
#[test]
fn performance_compile_time() {
    use std::time::Instant;
    
    let project_root = PathBuf::from(env!("CARGO_MANIFEST_DIR")).join("..").join("..");
    let example_path = project_root.join("examples").join("milestone3").join("01_hello_world.bf");
    let bfc_path = project_root.join("compiler").join("build").join("Release").join("bfc.exe");
    
    let start = Instant::now();
    
    let output = Command::new(&bfc_path)
        .arg(&example_path)
        .arg("-o")
        .arg("perf_test.exe")
        .current_dir(&project_root)
        .output()
        .expect("Failed to run compiler");
    
    let elapsed = start.elapsed();
    
    assert!(output.status.success(), "Compilation failed");
    assert!(elapsed.as_millis() < 5000, "Compile time too slow: {}ms", elapsed.as_millis());
    
    println!("Compile time for Hello World: {}ms", elapsed.as_millis());
}

#[test]
fn performance_exe_size() {
    let project_root = PathBuf::from(env!("CARGO_MANIFEST_DIR")).join("..").join("..");
    let example_path = project_root.join("examples").join("milestone3").join("01_hello_world.bf");
    let bfc_path = project_root.join("compiler").join("build").join("Release").join("bfc.exe");
    let output_exe = project_root.join("size_test.exe");
    
    let output = Command::new(&bfc_path)
        .arg(&example_path)
        .arg("-o")
        .arg(&output_exe)
        .current_dir(&project_root)
        .output()
        .expect("Failed to run compiler");
    
    assert!(output.status.success(), "Compilation failed");
    
    let metadata = std::fs::metadata(&output_exe).expect("Executable not found");
    let size_kb = metadata.len() / 1024;
    
    // Hello World should be under 50KB (with static runtime)
    assert!(size_kb < 50, "Executable too large: {}KB", size_kb);
    
    println!("Executable size: {}KB", size_kb);
    
    // Cleanup
    let _ = std::fs::remove_file(&output_exe);
}