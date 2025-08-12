void test() {
    int x = 10;
    const int& ref1 = x;
    const int& ref2 = x;  // Should be OK
    const int& ref3 = x;  // Should be OK
}