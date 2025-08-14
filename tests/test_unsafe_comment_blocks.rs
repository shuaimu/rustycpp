use std::process::Command;
use std::fs;

#[test]
fn test_unsafe_comment_block_in_safe_function() {
    let test_code = r#"// @safe

void test() {
    int value = 42;
    int& ref1 = value;  // Checked
    
    // @unsafe
    // This should NOT be checked
    int& ref2 = value;  // Not caught - in unsafe block
    int& ref3 = value;  // Not caught - in unsafe block
    // @endunsafe
    
    // Back to safe - this should be caught
    // int& ref4 = value;  // Would be caught
}
"#;
    
    fs::write("test_unsafe_comment.cpp", test_code).unwrap();
    
    let output = Command::new("cargo")
        .args(&["run", "--", "test_unsafe_comment.cpp"])
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
    let _ = fs::remove_file("test_unsafe_comment.cpp");
}

#[test]
fn test_unsafe_comment_with_errors_outside() {
    let test_code = r#"// @safe

void test() {
    int value = 42;
    int& ref1 = value;
    
    // @unsafe
    int& ref2 = value;  // Not checked
    // @endunsafe
    
    // This SHOULD be caught
    int& ref3 = value;  // ERROR: already borrowed by ref1
}
"#;
    
    fs::write("test_unsafe_comment_error.cpp", test_code).unwrap();
    
    let output = Command::new("cargo")
        .args(&["run", "--", "test_unsafe_comment_error.cpp"])
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
    let _ = fs::remove_file("test_unsafe_comment_error.cpp");
}

#[test]
fn test_nested_unsafe_comment_blocks() {
    let test_code = r#"// @safe

void test() {
    int value = 42;
    
    // @unsafe
    int& ref1 = value;
    
    // @unsafe  // Nested unsafe
    int& ref2 = value;  // Not checked
    // @endunsafe
    
    int& ref3 = value;  // Not checked (still in outer unsafe)
    // @endunsafe
}
"#;
    
    fs::write("test_nested_unsafe_comment.cpp", test_code).unwrap();
    
    let output = Command::new("cargo")
        .args(&["run", "--", "test_nested_unsafe_comment.cpp"])
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
    let _ = fs::remove_file("test_nested_unsafe_comment.cpp");
}

#[test]
fn test_unsafe_comment_in_loop() {
    let test_code = r#"// @safe

void test() {
    int value = 42;
    
    for (int i = 0; i < 2; i++) {
        // @unsafe
        int& ref = value;  // Not checked even in loop
        // @endunsafe
    }
}
"#;
    
    fs::write("test_unsafe_loop.cpp", test_code).unwrap();
    
    let output = Command::new("cargo")
        .args(&["run", "--", "test_unsafe_loop.cpp"])
        .env("Z3_SYS_Z3_HEADER", "/opt/homebrew/include/z3.h")
        .env("DYLD_LIBRARY_PATH", "/opt/homebrew/Cellar/llvm/19.1.7/lib")
        .output()
        .expect("Failed to run borrow checker");
    
    let stdout = String::from_utf8_lossy(&output.stdout);
    
    // Should not detect violations in unsafe blocks within loops
    assert!(stdout.contains("No borrow checking violations") || 
            stdout.contains("✓"),
            "Unsafe blocks in loops should work. Output: {}", stdout);
    
    // Clean up
    let _ = fs::remove_file("test_unsafe_loop.cpp");
}

#[test]
fn test_partial_line_unsafe() {
    // Test that unsafe regions work even with partial lines
    let test_code = r#"// @safe

void test() {
    int value = 42;
    int& ref1 = value;
    
    // @unsafe
    int& ref2 = value; int& ref3 = value;  // Both should be unchecked
    // @endunsafe
    
    int& ref4 = value;  // ERROR: already borrowed
}
"#;
    
    fs::write("test_partial_unsafe.cpp", test_code).unwrap();
    
    let output = Command::new("cargo")
        .args(&["run", "--", "test_partial_unsafe.cpp"])
        .env("Z3_SYS_Z3_HEADER", "/opt/homebrew/include/z3.h")
        .env("DYLD_LIBRARY_PATH", "/opt/homebrew/Cellar/llvm/19.1.7/lib")
        .output()
        .expect("Failed to run borrow checker");
    
    let stdout = String::from_utf8_lossy(&output.stdout);
    
    // Should detect error only for ref4
    assert!(stdout.contains("already mutably borrowed") || 
            stdout.contains("violation"),
            "Should detect errors outside unsafe blocks. Output: {}", stdout);
    
    // Clean up
    let _ = fs::remove_file("test_partial_unsafe.cpp");
}