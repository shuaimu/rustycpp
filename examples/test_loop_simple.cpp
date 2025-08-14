// Simple test to see how loops are handled

// Mock std::move
namespace std {
    template<typename T>
    T&& move(T& t) { return static_cast<T&&>(t); }
}

void test_loop_borrow() {
    int value = 42;
    
    // Take a reference in a loop
    for (int i = 0; i < 3; i++) {
        int& ref = value;  // Borrow in each iteration
        ref += i;
    }
    // ref should be out of scope here
    
    // This should be OK - previous borrow ended
    int& ref2 = value;
    ref2 = 100;
}

void test_if_else_borrow() {
    int value = 42;
    bool condition = true;
    
    if (condition) {
        int& ref1 = value;  // Borrow in true branch
        ref1 = 100;
    } else {
        int& ref2 = value;  // Borrow in false branch  
        ref2 = 200;
    }
    // Both refs out of scope
    
    // This should be OK
    int& ref3 = value;
    ref3 = 300;
}

void test_moved_in_condition() {
    int x = 42;
    bool condition = false;
    
    if (condition) {
        int y = std::move(x);  // Move only if condition is true
    }
    
    // x might or might not be moved
    int z = x;  // What does checker say?
}

void test_nested_scopes() {
    int value = 42;
    
    {
        int& ref1 = value;  // Borrow in inner scope
        ref1 = 100;
    }  // ref1 goes out of scope
    
    // This should be OK
    int& ref2 = value;
    ref2 = 200;
}

int main() {
    test_loop_borrow();
    test_if_else_borrow();
    test_moved_in_condition();
    test_nested_scopes();
    return 0;
}