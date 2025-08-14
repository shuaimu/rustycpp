// Examples that SHOULD be errors but might not be caught due to control flow limitations

// Mock std::move
namespace std {
    template<typename T>
    T&& move(T& t) { return static_cast<T&&>(t); }
}

// CASE 1: Double borrow should be caught but might not be due to scope issues
void false_negative_double_borrow() {
    int value = 42;
    
    // Create two mutable references to the same value
    // This SHOULD be an error (violates Rust's borrowing rules)
    int& ref1 = value;
    int& ref2 = value;  // ERROR: Second mutable borrow while first is active!
    
    ref1 = 100;
    ref2 = 200;  // Both references used - definitely a problem
}

// CASE 2: Move in a loop - should error on second iteration
void false_negative_loop_move() {
    int x = 42;
    
    // This loop runs twice
    for (int i = 0; i < 2; i++) {
        int y = std::move(x);  // First iteration OK, second iteration is use-after-move!
        // But checker probably doesn't understand loop iterations
    }
}

// CASE 3: Conditional move that definitely happens
void false_negative_conditional_move() {
    int x = 42;
    bool always_true = true;  // This is always true
    
    if (always_true) {
        int y = std::move(x);  // x is definitely moved
    }
    
    int z = x;  // ERROR: Use after move (since condition is always true)
    // But checker can't do path analysis
}

// CASE 4: False positive - mutually exclusive paths
void false_positive_exclusive_paths() {
    int value = 42;
    bool condition = true;
    
    if (condition) {
        int& ref1 = value;
        ref1 = 100;
        // ref1 scope ends here
    }
    
    if (!condition) {
        // This branch never executes when above branch does
        int& ref2 = value;  // Should be OK (mutually exclusive)
        ref2 = 200;
        // But checker might think both refs exist simultaneously
    }
}

// CASE 5: Scope confusion with blocks
void scope_confusion() {
    int value = 42;
    
    {
        int& ref1 = value;
        ref1 = 100;
    }  // ref1 definitely out of scope
    
    {
        int& ref2 = value;  // Should be OK - ref1 is gone
        ref2 = 200;
    }
    
    // But if checker doesn't understand block scopes, it sees both borrows
}

int main() {
    false_negative_double_borrow();
    false_negative_loop_move();
    false_negative_conditional_move();
    false_positive_exclusive_paths();
    scope_confusion();
    return 0;
}