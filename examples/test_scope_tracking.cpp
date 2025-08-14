// Test that scope tracking eliminates false positives

void test_nested_scopes() {
    int value = 42;
    
    // First scope with a reference
    {
        int& ref1 = value;
        ref1 = 100;
    }  // ref1 goes out of scope here
    
    // Second scope with another reference
    // This should NOT error - ref1 is gone
    {
        int& ref2 = value;
        ref2 = 200;
    }  // ref2 goes out of scope here
    
    // Third reference outside any nested scope
    // This should also be OK
    int& ref3 = value;
    ref3 = 300;
}

void test_multiple_scopes() {
    int x = 10;
    int y = 20;
    
    {
        int& rx = x;  // Borrow x in this scope
        rx = 15;
    }  // rx out of scope
    
    {
        int& ry = y;  // Borrow y in this scope
        ry = 25;
    }  // ry out of scope
    
    // Should be able to borrow both again
    int& rx2 = x;
    int& ry2 = y;
    rx2 = 30;
    ry2 = 40;
}

void test_nested_blocks() {
    int value = 100;
    
    {
        int& ref1 = value;
        {
            // Nested block - ref1 still exists
            // This SHOULD error (double borrow)
            int& ref2 = value;
            ref2 = 200;
        }
        ref1 = 150;
    }
}

void test_const_ref_scopes() {
    int value = 42;
    
    {
        const int& cref1 = value;
        const int& cref2 = value;  // Multiple const refs OK
        int sum = cref1 + cref2;
    }  // Both const refs out of scope
    
    // Now can take mutable ref
    int& mref = value;
    mref = 100;
}

// Mock std::move for testing
namespace std {
    template<typename T>
    T&& move(T& t) { return static_cast<T&&>(t); }
}

void test_move_in_scope() {
    int x = 42;
    
    {
        int y = std::move(x);  // x is moved
        // x is moved but y is only in this scope
    }  // y goes out of scope
    
    // x is still moved though!
    // int z = x;  // This should still error (use after move)
}

int main() {
    test_nested_scopes();
    test_multiple_scopes();
    test_nested_blocks();
    test_const_ref_scopes();
    test_move_in_scope();
    return 0;
}