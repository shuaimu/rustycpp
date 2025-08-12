void test() {
    int x = 10;
    int& ref1 = x;
    int& ref2 = x;  // Should fail
}