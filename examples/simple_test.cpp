void test() {
    int value = 42;
    int* ptr = &value;
    *ptr = 10;
}

int main() {
    test();
    return 0;
}
