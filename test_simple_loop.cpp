void test() {
    int value = 42;
    
    for (int i = 0; i < 2; i++) {
        int& ref = value;
        ref = i;
    }
}