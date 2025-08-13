#include <memory>
#include <utility>
#include <string>

void test_std_move_basic() {
    std::unique_ptr<int> ptr = std::make_unique<int>(42);
    
    // Move the unique_ptr using std::move
    std::unique_ptr<int> ptr2 = std::move(ptr);
    
    // This should be flagged as use-after-move
    if (ptr) {  // ERROR: ptr has been moved
        *ptr = 100;
    }
}

void test_std_move_with_strings() {
    std::string s1 = "hello";
    std::string s2 = std::move(s1);
    
    // Use after move - should be an error
    s1.length();  // ERROR: s1 has been moved
}

void test_move_in_function_call() {
    std::unique_ptr<int> ptr = std::make_unique<int>(42);
    
    // Function that takes ownership
    void consume(std::unique_ptr<int> p);
    
    consume(std::move(ptr));
    
    // Use after move
    if (ptr) {  // ERROR: ptr has been moved
        *ptr = 100;
    }
}

void test_conditional_move() {
    std::unique_ptr<int> ptr = std::make_unique<int>(42);
    bool condition = true;
    
    if (condition) {
        std::unique_ptr<int> ptr2 = std::move(ptr);
    }
    
    // This may or may not be valid depending on control flow
    // For now, we should flag it as potentially problematic
    if (ptr) {  // WARNING: ptr may have been moved
        *ptr = 100;
    }
}

void test_move_and_reassign() {
    std::unique_ptr<int> ptr = std::make_unique<int>(42);
    std::unique_ptr<int> ptr2 = std::move(ptr);
    
    // Reassign after move - this is OK
    ptr = std::make_unique<int>(100);
    
    // Now ptr is valid again
    if (ptr) {  // OK: ptr has been reassigned
        *ptr = 200;
    }
}

int main() {
    test_std_move_basic();
    test_std_move_with_strings();
    test_move_in_function_call();
    test_conditional_move();
    test_move_and_reassign();
    return 0;
}