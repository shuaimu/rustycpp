// Test unsafe propagation - safe functions cannot call unmarked/unsafe functions

// External functions (simulate system calls)
extern "C" int printf(const char*, ...);
void process_data(int x);  // No safety annotation - unsafe by default

// @unsafe
void explicitly_unsafe_operation(int* ptr) {
    *ptr = 42;  // Pointer dereference allowed in unsafe
}

// @safe
namespace safe_zone {
    
    void helper_unmarked() {
        // This function has no safety annotation
        printf("Helper\n");  // printf is whitelisted
    }
    
    // @safe
    void safe_function_bad() {
        // This should trigger errors:
        process_data(10);           // Error: calling unmarked function
        explicitly_unsafe_operation(nullptr);  // Error: calling unsafe function
        helper_unmarked();          // Error: calling unmarked function (even in same namespace)
    }
    
    // @unsafe
    void unsafe_wrapper() {
        // Unsafe functions can call anything:
        process_data(10);           // OK in unsafe context
        explicitly_unsafe_operation(nullptr);  // OK in unsafe context
        helper_unmarked();          // OK in unsafe context
    }
    
    // @safe
    void safe_function_good() {
        // Only safe operations:
        printf("Safe operation\n"); // OK: printf is whitelisted
        int x = 10;
        int y = x;                  // OK: normal assignment
        // Note: Cannot call unsafe_wrapper() here either
    }
}

// Default namespace (unsafe by default)
namespace default_ns {
    void unmarked_can_call_anything() {
        // No safety annotation = unsafe by default
        process_data(10);           // OK: unsafe calling unsafe
        explicitly_unsafe_operation(nullptr);  // OK: unsafe calling unsafe
    }
}

int main() {
    // Main is unmarked, so it's unsafe by default
    safe_zone::safe_function_bad();   // Will check this function
    safe_zone::unsafe_wrapper();      // Won't check this (it's unsafe)
    safe_zone::safe_function_good();  // Will check this function
    
    return 0;
}