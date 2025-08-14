use std::process::Command;
use std::fs;

#[test]
fn test_unsafe_block_in_safe_function() {
    let test_code = r#"// @safe

void unsafe_begin();
void unsafe_end();

void test() {
    int value = 42;
    int& ref1 = value;  // Checked
    
    unsafe_begin();
    {
        // This should NOT be checked
        int& ref2 = value;  // Not caught - in unsafe block
        int& ref3 = value;  // Not caught - in unsafe block
    }
    unsafe_end();
    
    // Back to safe - this should be caught
    // int& ref4 = value;  // Would be caught
}
"#;
    
    fs::write("test_unsafe_block.cpp", test_code).unwrap();
    
    let output = Command::new("cargo")
        .args(&["run", "--", "test_unsafe_block.cpp"])
        .env("Z3_SYS_Z3_HEADER", "/opt/homebrew/include/z3.h")
        .env("DYLD_LIBRARY_PATH", "/opt/homebrew/Cellar/llvm/19.1.7/lib")
        .output()
        .expect("Failed to run borrow checker");
    
    let stdout = String::from_utf8_lossy(&output.stdout);
    
    // Should NOT find violations in the unsafe block
    assert!(stdout.contains("No borrow checking violations") || 
            stdout.contains("✓"),
            "Should not check code in unsafe blocks. Output: {}", stdout);
    
    // Clean up
    let _ = fs::remove_file("test_unsafe_block.cpp");
}

#[test]
fn test_unsafe_block_with_errors_outside() {
    let test_code = r#"// @safe

void unsafe_begin();
void unsafe_end();

void test() {
    int value = 42;
    int& ref1 = value;
    
    unsafe_begin();
    {
        int& ref2 = value;  // Not checked
    }
    unsafe_end();
    
    // This SHOULD be caught
    int& ref3 = value;  // ERROR: already borrowed by ref1
}
"#;
    
    fs::write("test_unsafe_with_error.cpp", test_code).unwrap();
    
    let output = Command::new("cargo")
        .args(&["run", "--", "test_unsafe_with_error.cpp"])
        .env("Z3_SYS_Z3_HEADER", "/opt/homebrew/include/z3.h")
        .env("DYLD_LIBRARY_PATH", "/opt/homebrew/Cellar/llvm/19.1.7/lib")
        .output()
        .expect("Failed to run borrow checker");
    
    let stdout = String::from_utf8_lossy(&output.stdout);
    
    // Should detect the error outside the unsafe block
    assert!(stdout.contains("already mutably borrowed") || 
            stdout.contains("violation"),
            "Should detect errors outside unsafe blocks. Output: {}", stdout);
    
    // Clean up
    let _ = fs::remove_file("test_unsafe_with_error.cpp");
}

#[test]
fn test_nested_unsafe_blocks() {
    let test_code = r#"// @safe

void unsafe_begin();
void unsafe_end();

void test() {
    int value = 42;
    
    unsafe_begin();
    {
        int& ref1 = value;
        
        // Nested unsafe (should still be unsafe)
        unsafe_begin();
        {
            int& ref2 = value;  // Not checked
        }
        unsafe_end();
        
        int& ref3 = value;  // Not checked (still in outer unsafe)
    }
    unsafe_end();
}
"#;
    
    fs::write("test_nested_unsafe.cpp", test_code).unwrap();
    
    let output = Command::new("cargo")
        .args(&["run", "--", "test_nested_unsafe.cpp"])
        .env("Z3_SYS_Z3_HEADER", "/opt/homebrew/include/z3.h")
        .env("DYLD_LIBRARY_PATH", "/opt/homebrew/Cellar/llvm/19.1.7/lib")
        .output()
        .expect("Failed to run borrow checker");
    
    let stdout = String::from_utf8_lossy(&output.stdout);
    
    // Should not detect violations in nested unsafe blocks
    assert!(stdout.contains("No borrow checking violations") || 
            stdout.contains("✓"),
            "Nested unsafe blocks should work. Output: {}", stdout);
    
    // Clean up
    let _ = fs::remove_file("test_nested_unsafe.cpp");
}

#[test]
fn test_unsafe_in_unsafe_function() {
    // Unsafe blocks in already unsafe functions should work
    let test_code = r#"
void unsafe_begin();
void unsafe_end();

// @unsafe
void already_unsafe() {
    int value = 42;
    int& ref1 = value;
    int& ref2 = value;  // Not checked - function is unsafe
    
    unsafe_begin();
    {
        int& ref3 = value;  // Still not checked
    }
    unsafe_end();
}
"#;
    
    fs::write("test_unsafe_in_unsafe.cpp", test_code).unwrap();
    
    let output = Command::new("cargo")
        .args(&["run", "--", "test_unsafe_in_unsafe.cpp"])
        .env("Z3_SYS_Z3_HEADER", "/opt/homebrew/include/z3.h")
        .env("DYLD_LIBRARY_PATH", "/opt/homebrew/Cellar/llvm/19.1.7/lib")
        .output()
        .expect("Failed to run borrow checker");
    
    let stdout = String::from_utf8_lossy(&output.stdout);
    
    // Should not check anything in unsafe function
    assert!(stdout.contains("No borrow checking violations") || 
            stdout.contains("✓"),
            "Unsafe function should not be checked. Output: {}", stdout);
    
    // Clean up
    let _ = fs::remove_file("test_unsafe_in_unsafe.cpp");
}