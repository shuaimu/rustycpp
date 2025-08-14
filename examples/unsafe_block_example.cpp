// @safe
// Example demonstrating unsafe blocks within safe functions

// Functions for marking unsafe regions
// These are just markers for the analyzer, not real functions
void unsafe_begin();
void unsafe_end();

// Alternative: use extern "C" to avoid name mangling
extern "C" {
    void UNSAFE_BEGIN();
    void UNSAFE_END();
}

// @safe
void mixed_safety_function() {
    int value = 42;
    
    // Safe code - checked for borrow rules
    int& ref1 = value;
    ref1 = 100;
    
    // Begin unsafe block
    unsafe_begin();
    {
        // Unsafe code - no checking
        int* ptr = &value;
        int* alias = ptr;  // Multiple aliases OK in unsafe
        *alias = 200;
        
        // Even dangerous operations not caught
        int* heap = new int(42);
        delete heap;
        // *heap = 300;  // Use-after-free not caught in unsafe
    }
    unsafe_end();
    
    // Back to safe code
    // int& ref2 = value;  // ERROR: would be caught (already borrowed)
}

void single_unsafe_statement() {
    int value = 42;
    int& ref = value;
    
    // Unsafe for a single statement
    unsafe_begin();
    int* ptr = &value;  // Not checked
    unsafe_end();
    
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
    
    // Unsafe optimization - skip bounds checking
    unsafe_begin();
    {
        int* ptr = array;
        // Fast, unchecked pointer arithmetic
        for (int i = 0; i < 1000; i++) {
            *(ptr++) *= 2;  // No bounds checking
        }
    }
    unsafe_end();
}

// Example: interfacing with C code
extern "C" {
    void* legacy_c_function(void* data);
}

void interface_with_c() {
    int value = 42;
    
    // Need unsafe to work with raw pointers for C interop
    unsafe_begin();
    {
        void* raw_ptr = &value;
        void* result = legacy_c_function(raw_ptr);
        // Cast back - unsafe but necessary for C interop
        int* typed = static_cast<int*>(result);
    }
    unsafe_end();
    
    // Safe code continues
    int& ref = value;
}