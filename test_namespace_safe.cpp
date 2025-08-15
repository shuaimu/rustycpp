// @safe
namespace myapp {

int move(int x);

void test() {
    int value = 42;
    int value2 = move(value);  // Should trigger move detection
    
    int x = value;  // Should error - use after move
}

void test2() {
    int a = 1;
    int& ref1 = a;
    int& ref2 = a;  // Should error - multiple mutable borrows
}

}  // namespace myapp