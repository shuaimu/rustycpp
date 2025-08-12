void test() {
    int x = 10;
    const int& ref1 = x;
    int& ref2 = x;  // Should fail
}

int main() {
    test();
    return 0;
}