
#include <memory>

void test() {
    std::unique_ptr<int> ptr(new int(42));
    
    for (int i = 0; i < 2; i++) {
        if (i == 0) {
            auto moved = std::move(ptr);  // Moves on first iteration
        } else {
            // ptr is already moved on second iteration
            auto value = *ptr;  // Error: use after move
        }
    }
}
