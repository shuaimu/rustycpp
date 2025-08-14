// Example demonstrating unified @safe/@unsafe annotation rules
// Annotations attach to the next statement/block/function/namespace

// Example 1: Namespace-level safety (whole file)
// @safe
namespace myapp {
    
    void func1() {
        int value = 42;
        int& ref1 = value;
        // int& ref2 = value;  // ERROR: would be caught
    }
    
    // @unsafe
    void unsafe_func() {
        // This function is explicitly unsafe, no checking
        int value = 42;
        int& ref1 = value;
        int& ref2 = value;  // OK - not checked
    }
    
    void func2() {
        // Inherits @safe from namespace
        int value = 42;
        const int& ref1 = value;
        const int& ref2 = value;  // OK - multiple const refs
    }
}

// Example 2: Function-level annotations
namespace example2 {
    
    // Default is unsafe (no annotation)
    void unchecked_func() {
        int value = 42;
        int& ref1 = value;
        int& ref2 = value;  // Not checked
    }
    
    // @safe
    void checked_func() {
        int value = 42;
        int& ref1 = value;
        // int& ref2 = value;  // ERROR: would be caught
        
        // Unsafe block within safe function
        // @unsafe
        {
            int& ref2 = value;  // OK in unsafe block
            int& ref3 = value;  // OK in unsafe block
        }
        // @endunsafe
        
        // Back to safe
        // int& ref4 = value;  // ERROR: would be caught
    }
}

// Example 3: First code element rule
// @safe
int global_var = 42;  // Makes whole file safe

void global_func() {
    int value = 42;
    int& ref1 = value;
    // int& ref2 = value;  // ERROR: would be caught
}

// Example 4: Class-level (future enhancement)
// @safe
class SafeClass {
public:
    void method1() {
        // Inherits @safe from class
        int value = 42;
        int& ref1 = value;
        // int& ref2 = value;  // ERROR: would be caught
    }
    
    // @unsafe
    void unsafe_method() {
        // Explicitly unsafe method
        int value = 42;
        int& ref1 = value;
        int& ref2 = value;  // OK - not checked
    }
};