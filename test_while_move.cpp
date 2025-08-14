
#include <memory>

void test() {
    std::unique_ptr<int> ptr(new int(42));
    int count = 0;
    
    while (count < 2) {
        auto moved = std::move(ptr);  // Error on second iteration
        count++;
    }
}
