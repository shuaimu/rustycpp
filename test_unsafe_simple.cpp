// @safe

void unsafe_begin();
void unsafe_end();

void test() {
    int value = 42;
    int& ref1 = value;
    
    unsafe_begin();
    int& ref2 = value;  // Should not be checked
    unsafe_end();
}