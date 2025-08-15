// @safe

// Declare move function
int move(int x);

void test() {
    int value = 42;
    int value2 = move(value);  // Should trigger move detection
    
    int x = value;  // Should error if move was detected
}