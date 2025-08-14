// @safe
// This entire file is checked for memory safety

// All functions in this file are checked by default
void checked_by_default() {
    int value = 42;
    int& ref1 = value;
    // int& ref2 = value;  // ERROR: double mutable borrow
    ref1 = 100;
}

// Another checked function
void also_checked() {
    int* ptr = new int(42);
    // delete ptr;
    // *ptr = 100;  // Would be caught as use-after-free
}

// @unsafe
// This function opts out of checking even in a safe file  
void performance_critical() {
    int* ptr = new int(42);
    delete ptr;
    // *ptr = 100;  // Would be use-after-free but not caught (@unsafe)
}

// Back to safe by default
void safe_again() {
    int value = 10;
    const int& cref = value;
    // int& mref = value;  // ERROR: can't have mutable ref with const ref
}