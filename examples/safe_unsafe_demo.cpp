// Example demonstrating safe/unsafe annotations for gradual adoption
// By default, C++ files are unsafe (no checking) for compatibility

#include <iostream>

// Legacy code - not checked by default
void legacy_function() {
    int* ptr = new int(42);
    int* alias = ptr;
    delete ptr;
    // *alias = 100;  // Use-after-free but not caught (unsafe by default)
    std::cout << "Legacy code runs without checks\n";
}

// @safe
// New code with safety checking enabled
void modern_safe_function() {
    int value = 42;
    int& ref1 = value;
    // int& ref2 = value;  // ERROR: Would be caught - double mutable borrow
    ref1 = 100;
    std::cout << "Safe function with borrow checking\n";
}

// @safe
// Function that needs to do something unsafe
void mixed_function() {
    int value = 42;
    int& safe_ref = value;
    safe_ref = 100;
    
    // Sometimes you need to bypass checks for performance or FFI
    // @unsafe {
    //     int* raw = &value;
    //     int* alias = raw;  // Multiple aliases allowed in unsafe block
    //     *alias = 200;
    // }
    
    std::cout << "Mixed safe/unsafe code\n";
}

// @unsafe
// Explicitly unsafe function even if file is compiled with --safe
void explicitly_unsafe() {
    int* ptr = new int(42);
    delete ptr;
    // *ptr = 100;  // Use-after-free not caught in @unsafe function
    std::cout << "Explicitly unsafe function\n";
}

int main() {
    std::cout << "Safe/Unsafe Demo\n";
    std::cout << "================\n\n";
    
    std::cout << "1. Legacy code (unsafe by default):\n";
    legacy_function();
    
    std::cout << "\n2. Modern safe code (with @safe):\n";
    modern_safe_function();
    
    std::cout << "\n3. Mixed safe/unsafe code:\n";
    mixed_function();
    
    std::cout << "\n4. Explicitly unsafe function:\n";
    explicitly_unsafe();
    
    std::cout << "\nTo enable checking for entire file, compile with: --safe flag\n";
    
    return 0;
}

/*
Usage:
------
1. Default (unsafe, no checking):
   cargo run -- examples/safe_unsafe_demo.cpp
   
2. Safe mode (check entire file except @unsafe functions):
   cargo run -- --safe examples/safe_unsafe_demo.cpp
   
3. Annotations:
   - @safe on function: Check this function even in unsafe file
   - @unsafe on function: Don't check even in safe file
   - Future: @unsafe { ... } blocks within safe functions

Benefits:
---------
- Gradual adoption: Start with unsafe, add @safe to new code
- Compatibility: Existing C++ projects work without modification
- Flexibility: Mix safe and unsafe as needed
- Performance: Skip checks in performance-critical sections
*/