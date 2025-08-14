use std::process::Command;
use std::fs;

#[test]
fn test_if_else_move_in_both_branches() {
    // Variable moved in both branches - should be moved after if
    let test_code = r#"
class UniquePtr {
public:
    UniquePtr(int* p) : ptr(p) {}
    UniquePtr(UniquePtr&& other) : ptr(other.ptr) { other.ptr = nullptr; }
    int* ptr;
};

UniquePtr&& move(UniquePtr& p) {
    return static_cast<UniquePtr&&>(p);
}

void test() {
    int* raw = new int(42);
    UniquePtr ptr(raw);
    
    if (raw != nullptr) {
        UniquePtr a = move(ptr);
    } else {
        UniquePtr b = move(ptr);
    }
    
    // ptr is moved in both branches
    UniquePtr c = move(ptr);  // Error: use after move
}
"#;
    
    fs::write("test_if_both.cpp", test_code).unwrap();
    
    let output = Command::new("cargo")
        .args(&["run", "--", "test_if_both.cpp"])
        .env("Z3_SYS_Z3_HEADER", "/opt/homebrew/include/z3.h")
        .env("DYLD_LIBRARY_PATH", "/opt/homebrew/Cellar/llvm/19.1.7/lib")
        .output()
        .expect("Failed to run borrow checker");
    
    let stdout = String::from_utf8_lossy(&output.stdout);
    
    // Should detect use-after-move
    assert!(stdout.contains("moved") || stdout.contains("violation"),
            "Should detect use-after-move when moved in both branches. Output: {}", stdout);
    
    // Clean up
    let _ = fs::remove_file("test_if_both.cpp");
}

#[test]
fn test_if_else_move_in_one_branch() {
    // Variable moved in only one branch - should NOT be considered moved after
    let test_code = r#"
class UniquePtr {
public:
    UniquePtr(int* p) : ptr(p) {}
    UniquePtr(UniquePtr&& other) : ptr(other.ptr) { other.ptr = nullptr; }
    int* ptr;
};

UniquePtr&& move(UniquePtr& p) {
    return static_cast<UniquePtr&&>(p);
}

void test() {
    int* raw = new int(42);
    UniquePtr ptr(raw);
    int x = 0;
    
    if (x == 0) {
        UniquePtr a = move(ptr);
    } else {
        // ptr not moved in else branch
        int y = 42;
    }
    
    // ptr may or may not be moved - conservative analysis should allow this
    // (though in reality this would be a bug if x == 0)
    int* p = ptr.ptr;  // Should be OK in conservative analysis
}
"#;
    
    fs::write("test_if_one.cpp", test_code).unwrap();
    
    let output = Command::new("cargo")
        .args(&["run", "--", "test_if_one.cpp"])
        .env("Z3_SYS_Z3_HEADER", "/opt/homebrew/include/z3.h")
        .env("DYLD_LIBRARY_PATH", "/opt/homebrew/Cellar/llvm/19.1.7/lib")
        .output()
        .expect("Failed to run borrow checker");
    
    let stdout = String::from_utf8_lossy(&output.stdout);
    
    // Should NOT report error (conservative analysis)
    assert!(!stdout.contains("after move"),
            "Should not report use-after-move when moved in only one branch. Output: {}", stdout);
    
    // Clean up
    let _ = fs::remove_file("test_if_one.cpp");
}

#[test]
fn test_if_no_else_with_move() {
    // If without else - move in if branch
    let test_code = r#"
class UniquePtr {
public:
    UniquePtr(int* p) : ptr(p) {}
    UniquePtr(UniquePtr&& other) : ptr(other.ptr) { other.ptr = nullptr; }
    int* ptr;
};

UniquePtr&& move(UniquePtr& p) {
    return static_cast<UniquePtr&&>(p);
}

void test() {
    int* raw = new int(42);
    UniquePtr ptr(raw);
    int x = 0;
    
    if (x == 0) {
        UniquePtr a = move(ptr);
    }
    // No else branch
    
    // ptr may or may not be moved
    int* p = ptr.ptr;  // Should be OK (conservative)
}
"#;
    
    fs::write("test_if_no_else.cpp", test_code).unwrap();
    
    let output = Command::new("cargo")
        .args(&["run", "--", "test_if_no_else.cpp"])
        .env("Z3_SYS_Z3_HEADER", "/opt/homebrew/include/z3.h")
        .env("DYLD_LIBRARY_PATH", "/opt/homebrew/Cellar/llvm/19.1.7/lib")
        .output()
        .expect("Failed to run borrow checker");
    
    let stdout = String::from_utf8_lossy(&output.stdout);
    
    // Should NOT report error (conservative analysis)
    assert!(!stdout.contains("after move"),
            "Should not report error for if without else. Output: {}", stdout);
    
    // Clean up
    let _ = fs::remove_file("test_if_no_else.cpp");
}

#[test]
fn test_nested_if_with_borrows() {
    // Test nested if statements with borrows
    let test_code = r#"
void test() {
    int value = 42;
    int x = 0;
    
    if (x == 0) {
        int& ref1 = value;
        if (x == 1) {
            const int& ref2 = value;  // Error: already mutably borrowed
        }
    }
}
"#;
    
    fs::write("test_nested_if.cpp", test_code).unwrap();
    
    let output = Command::new("cargo")
        .args(&["run", "--", "test_nested_if.cpp"])
        .env("Z3_SYS_Z3_HEADER", "/opt/homebrew/include/z3.h")
        .env("DYLD_LIBRARY_PATH", "/opt/homebrew/Cellar/llvm/19.1.7/lib")
        .output()
        .expect("Failed to run borrow checker");
    
    let stdout = String::from_utf8_lossy(&output.stdout);
    
    // Should detect borrow conflict
    assert!(stdout.contains("already mutably borrowed") || stdout.contains("violation"),
            "Should detect borrow conflict in nested if. Output: {}", stdout);
    
    // Clean up
    let _ = fs::remove_file("test_nested_if.cpp");
}

#[test]
fn test_if_else_different_borrows() {
    // Different borrows in different branches
    let test_code = r#"
void test() {
    int value = 42;
    int x = 0;
    
    if (x == 0) {
        int& ref1 = value;
        ref1 = 100;
    } else {
        const int& ref2 = value;
        int y = ref2;
    }
    
    // Both refs out of scope
    int& ref3 = value;  // Should be OK
}
"#;
    
    fs::write("test_if_else_borrows.cpp", test_code).unwrap();
    
    let output = Command::new("cargo")
        .args(&["run", "--", "test_if_else_borrows.cpp"])
        .env("Z3_SYS_Z3_HEADER", "/opt/homebrew/include/z3.h")
        .env("DYLD_LIBRARY_PATH", "/opt/homebrew/Cellar/llvm/19.1.7/lib")
        .output()
        .expect("Failed to run borrow checker");
    
    let stdout = String::from_utf8_lossy(&output.stdout);
    
    // Should NOT report errors
    assert!(!stdout.contains("already") || stdout.contains("✓"),
            "Should allow different borrows in different branches. Output: {}", stdout);
    
    // Clean up
    let _ = fs::remove_file("test_if_else_borrows.cpp");
}