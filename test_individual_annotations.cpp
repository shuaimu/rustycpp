// File starts unsafe by default

int move(int x);  // Declare move function

void unsafe_func() {
    int value = 42;
    int value2 = move(value);
    int x = value;  // Not checked - function is unsafe
}

// @safe
void safe_func() {
    int value = 42;  
    int value2 = move(value);
    int x = value;  // Should error - use after move
}

// @unsafe
void explicitly_unsafe() {
    int a = 1;
    int& ref1 = a;
    int& ref2 = a;  // Not checked - explicitly unsafe
}

// @safe  
void another_safe_func() {
    int a = 1;
    int& ref1 = a;
    int& ref2 = a;  // Should error - multiple mutable refs
}