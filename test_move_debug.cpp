// @safe
// Test if our move detection is working at all

void test() {
    int value = 42;
    // Try different ways to trigger move detection
    int value2 = move(value);  // plain move
    
    int x = value;  // Should error if move was detected
}