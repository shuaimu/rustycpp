// This file demonstrates the safety annotation rules:
// 1. @safe/@unsafe only applies to the NEXT element
// 2. Functions are unsafe by default
// 3. To make a whole file safe, wrap in a namespace

// Example 1: Single annotation affects only next element
// @safe
int move(int x);  // This declaration is marked safe

void func1() {
    // This function is unsafe (default)
    int x = 1;
    int& ref1 = x;
    int& ref2 = x;  // Not checked
}

// Example 2: Explicit function annotations  
// @safe
void func2() {
    // This function is safe
    int x = 1;
    int& ref1 = x;
    int& ref2 = x;  // ERROR: multiple mutable borrows
}

// Example 3: Namespace makes all contents follow the annotation
// @safe
namespace safe_code {
    void func3() {
        // Safe by namespace default
        int value = 42;
        int value2 = move(value);
        int x = value;  // ERROR: use after move
    }
    
    void func4() {
        // Also safe by namespace default
        int a = 1;
        const int& ref1 = a;
        const int& ref2 = a;  // OK: multiple immutable borrows
    }
    
    // @unsafe
    void func5() {
        // Explicitly unsafe within safe namespace
        int x = 1;
        int& ref1 = x;
        int& ref2 = x;  // Not checked
    }
}

// Example 4: Blocks within safe functions
// @safe  
void func6() {
    int x = 42;
    int& ref = x;  // OK
    
    // Can't have inline unsafe blocks anymore - @unsafe only applies to next element
    // If you need unsafe code, put it in a separate function marked @unsafe
    
    const int& ref2 = x;  // ERROR: mixing mutable and immutable borrows
}