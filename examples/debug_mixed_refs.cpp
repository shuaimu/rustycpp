void test() {
    int x = 10;
    const int& ref1 = x;  // Immutable borrow
    int& mut_ref = x;     // Mutable borrow - should fail
}