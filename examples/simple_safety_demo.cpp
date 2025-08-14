// Simple demonstration of unified @safe/@unsafe annotations
// No standard library dependencies

// Example 1: Namespace-level @safe
// @safe
namespace app {
    void func1() {
        int value = 42;
        int& ref1 = value;
        // int& ref2 = value;  // ERROR: would be caught
    }
    
    // @unsafe
    void unsafe_func() {
        int value = 42;
        int& ref1 = value;
        int& ref2 = value;  // OK - not checked
    }
}

// Example 2: Function-level (default unsafe)
void default_unsafe() {
    int value = 42;
    int& ref1 = value;
    int& ref2 = value;  // OK - not checked
}

// @safe
void safe_function() {
    int value = 42;
    int& ref1 = value;
    // int& ref2 = value;  // ERROR: would be caught
}

// Example 3: First element makes file safe
/*
// @safe
int global = 42;  // Would make entire file safe

void all_functions_checked() {
    int value = 42;
    int& ref1 = value;
    // int& ref2 = value;  // ERROR: would be caught
}
*/