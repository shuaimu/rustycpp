// @safe

#define unsafe_begin() ((void)0)
#define unsafe_end() ((void)0)

void test() {
    int value = 42;
    int& ref1 = value;
    
    unsafe_begin();
    int& ref2 = value;  // Should not be checked
    unsafe_end();
}