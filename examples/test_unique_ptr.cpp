#include <memory>
#include <iostream>

void use_after_move_example() {
    std::unique_ptr<int> ptr1 = std::make_unique<int>(42);
    std::unique_ptr<int> ptr2 = std::move(ptr1);
    
    // This should trigger a use-after-move error
    *ptr1 = 10;  // ERROR: ptr1 has been moved
}

void valid_transfer() {
    std::unique_ptr<int> ptr1 = std::make_unique<int>(42);
    std::unique_ptr<int> ptr2 = std::move(ptr1);
    
    // This is fine - using ptr2, not ptr1
    *ptr2 = 10;
}

void borrow_checking_example() {
    int value = 42;
    int& ref1 = value;  // Immutable borrow
    int& ref2 = value;  // Another immutable borrow - OK
    
    // This would be an error if we had mutable borrow checking
    // int& mut_ref = value;  // ERROR: Cannot have mutable borrow while immutable borrows exist
}

// Example with annotations (parsed from comments for MVP)
// @owns
std::unique_ptr<int> create_value() {
    return std::make_unique<int>(42);
}

// @borrows
void use_value(const int& value) {
    std::cout << value << std::endl;
}

int main() {
    valid_transfer();
    use_after_move_example();
    borrow_checking_example();
    
    auto ptr = create_value();
    use_value(*ptr);
    
    return 0;
}