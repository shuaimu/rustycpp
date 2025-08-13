void test_simple() {
    int x = 10;
    int& ref = x;  // Mutable borrow
    
    // This should error - can't have immutable while mutable exists
    const int& const_ref = x;
}