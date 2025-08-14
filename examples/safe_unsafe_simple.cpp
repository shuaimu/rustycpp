// Example demonstrating safe/unsafe annotations
// By default, C++ code is unsafe (no checking)
// To make entire file safe, add: // @safe

// Legacy function - not checked by default
void legacy_code() {
    int* ptr = new int(42);
    int* alias = ptr;
    delete ptr;
    *alias = 100;  // Use-after-free but NOT caught (unsafe by default)
}

// @safe
// Modern function with safety checking
void safe_code() {
    int value = 42;
    int& ref1 = value;
    // Uncomment to see error:
    // int& ref2 = value;  // ERROR: double mutable borrow
    ref1 = 100;
}

// @unsafe
// Explicitly unsafe even with --safe flag
void performance_critical() {
    int* ptr = new int(42);
    delete ptr;
    *ptr = 100;  // Use-after-free NOT caught (@unsafe)
}

/*
Usage:
  Default:    cargo run -- examples/safe_unsafe_simple.cpp
  Safe mode:  cargo run -- --safe examples/safe_unsafe_simple.cpp
*/