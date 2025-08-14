// @safe
// Example demonstrating unsafe blocks using comment annotations

// @safe
void mixed_safety_function() {
    int value = 42;
    
    // Safe code - checked for borrow rules
    int& ref1 = value;
    ref1 = 100;
    
    // @unsafe
    // Unsafe code - no checking
    int* ptr = &value;
    int* alias = ptr;  // Multiple aliases OK in unsafe
    *alias = 200;
    
    // Even dangerous operations not caught
    int* heap = new int(42);
    delete heap;
    // @endunsafe
    
    // Back to safe code
    // int& ref2 = value;  // ERROR: would be caught (already borrowed)
}

void single_unsafe_statement() {
    int value = 42;
    int& ref = value;
    
    // @unsafe
    int* ptr = &value;  // Not checked
    // @endunsafe
    
    // This is checked again
    // int& ref2 = value;  // ERROR: already borrowed
}

// Example with performance-critical code
void performance_critical_with_unsafe() {
    int array[1000];
    
    // Safe initialization
    for (int i = 0; i < 1000; i++) {
        array[i] = i;
    }
    
    // @unsafe
    // Fast, unchecked pointer arithmetic
    int* ptr = array;
    for (int i = 0; i < 1000; i++) {
        *(ptr++) *= 2;  // No bounds checking
    }
    // @endunsafe
}

// Example: interfacing with C code
extern "C" {
    void* legacy_c_function(void* data);
}

void interface_with_c() {
    int value = 42;
    
    // @unsafe
    // Need unsafe to work with raw pointers for C interop
    void* raw_ptr = &value;
    void* result = legacy_c_function(raw_ptr);
    // Cast back - unsafe but necessary for C interop
    int* typed = static_cast<int*>(result);
    // @endunsafe
    
    // Safe code continues
    int& ref = value;
}