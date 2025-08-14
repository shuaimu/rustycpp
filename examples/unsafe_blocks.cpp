// @safe
// Example showing unsafe blocks within safe functions

// Define UNSAFE macro for marking unsafe blocks
#define UNSAFE if(true)

void safe_function_with_unsafe_block() {
    int value = 42;
    int& ref = value;
    ref = 100;  // This is checked
    
    // Unsafe block - checks are disabled here
    UNSAFE {
        int* ptr = &value;
        int* alias = ptr;  // Multiple aliases allowed in unsafe
        *alias = 200;
        
        // Even use-after-free is not caught in unsafe block
        int* temp = new int(42);
        delete temp;
        *temp = 300;  // Not caught
    }
    
    // Back to safe code
    // int& ref2 = value;  // ERROR: Would be caught (double borrow)
}

// Another approach: using comments (future enhancement)
void future_syntax() {
    int value = 42;
    
    // @unsafe {
    //     int* ptr = &value;
    //     // unchecked code
    // }
    
    int& ref = value;  // checked
}