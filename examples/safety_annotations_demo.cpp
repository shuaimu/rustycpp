// Comprehensive demonstration of unified @safe/@unsafe annotation system
// Rule: Annotations attach to the next statement/block/function/namespace

// ============================================================================
// Example 1: Namespace-level safety (applies to entire namespace)
// ============================================================================

// @safe
namespace safe_namespace {
    // All functions in this namespace are checked by default
    
    void checked_by_default() {
        int value = 42;
        int& ref1 = value;
        // int& ref2 = value;  // ERROR: would violate borrow rules
        
        const int& cref1 = value;  // OK: immutable borrow with existing mutable
    }
    
    // @unsafe
    void explicitly_unsafe_function() {
        // This function opts out of checking
        int value = 42;
        int& ref1 = value;
        int& ref2 = value;  // OK: not checked
        int& ref3 = value;  // OK: not checked
    }
    
    void another_checked_function() {
        // Inherits @safe from namespace
        int array[10];
        int& ref = array[0];
        // int& ref2 = array[0];  // ERROR: would be caught
    }
}

// ============================================================================
// Example 2: Function-level annotations (default namespace is unsafe)
// ============================================================================

namespace function_level_example {
    // No @safe on namespace, so default is unsafe
    
    void unchecked_function() {
        // Not checked by default
        int value = 42;
        int& ref1 = value;
        int& ref2 = value;  // OK: not checked
    }
    
    // @safe
    void checked_function() {
        // Explicitly marked as safe
        int value = 42;
        const int& ref1 = value;
        const int& ref2 = value;  // OK: multiple const refs allowed
        // int& mut_ref = value;  // ERROR: would violate borrow rules
    }
    
    // @safe
    void function_with_unsafe_block() {
        int value = 42;
        int& ref1 = value;
        
        // Unsafe block within safe function
        // @unsafe
        {
            // This block is not checked
            int& ref2 = value;  // OK in unsafe block
            int& ref3 = value;  // OK in unsafe block
            
            // Can do dangerous operations
            int* raw_ptr = &value;
            int* alias = raw_ptr;  // Multiple aliases OK
        }
        // @endunsafe
        
        // Back to safe context
        // int& ref4 = value;  // ERROR: would be caught (ref1 still exists)
    }
}

// ============================================================================
// Example 3: First code element rule
// ============================================================================

// @safe
// Attaches to the first code element (global variable)
// Makes the entire file safe from this point
int global_config = 100;

void global_function1() {
    // Checked because file is marked safe
    int value = 42;
    int& ref = value;
    // int& ref2 = value;  // ERROR: would be caught
}

// @unsafe
void explicitly_unsafe_global() {
    // Opts out of checking
    int value = 42;
    int& ref1 = value;
    int& ref2 = value;  // OK: not checked
}

// ============================================================================
// Example 4: Mixed safety in classes (parsed as functions)
// ============================================================================

// @safe
class SafeClass {
public:
    void safe_method() {
        // Inherits safety from class/file context
        int value = 42;
        const int& ref = value;
        // int& mut_ref = value;  // ERROR: would be caught
    }
    
    // @unsafe
    void unsafe_method() {
        // Explicitly unsafe method
        int value = 42;
        int& ref1 = value;
        int& ref2 = value;  // OK: not checked
    }
    
    // @safe
    void method_with_unsafe_regions() {
        int data[100];
        
        // Safe processing
        for (int i = 0; i < 100; i++) {
            const int& ref = data[i];
            // Process safely
        }
        
        // @unsafe
        // Performance-critical section
        int* ptr = data;
        for (int i = 0; i < 100; i++) {
            *(ptr++) *= 2;  // Unchecked pointer arithmetic
        }
        // @endunsafe
    }
};

// ============================================================================
// Example 5: Real-world patterns
// ============================================================================

// @safe
namespace production_code {
    
    // Safe by default - good for new code
    void process_data(int* data, size_t size) {
        if (!data) return;
        
        // Safe iteration
        for (size_t i = 0; i < size; i++) {
            const int& value = data[i];
            // Process value safely
        }
    }
    
    // @unsafe
    void legacy_api_wrapper(void* raw_data) {
        // Interfacing with C code - needs unsafe
        char* buffer = static_cast<char*>(raw_data);
        // Do unsafe operations with raw pointers
    }
    
    // @safe
    void mixed_safety_example() {
        int resources[10];
        
        // Safe initialization
        for (int& r : resources) {
            r = 0;
        }
        
        // @unsafe
        {
            // Low-level optimization
            int* start = resources;
            int* end = resources + 10;
            while (start < end) {
                *start++ = 42;
            }
        }
        // @endunsafe
        
        // Safe usage
        const int& first = resources[0];
        // Use first safely
    }
}

// ============================================================================
// Usage Summary:
// - @safe/@unsafe before namespace: applies to all contents
// - @safe/@unsafe before function: applies to that function only  
// - @safe before first code element: applies to entire file
// - @unsafe ... @endunsafe: unsafe block within safe context
// - Default is unsafe for backward compatibility
// ============================================================================