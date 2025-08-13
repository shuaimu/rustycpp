// Simulate std::move without including headers
namespace std {
    template<typename T>
    T&& move(T& t) { return static_cast<T&&>(t); }
}

class UniquePtr {
public:
    int* ptr;
    UniquePtr(int* p) : ptr(p) {}
};

void test_basic_move() {
    UniquePtr ptr1(new int(42));
    
    // Move ptr1 to ptr2 using std::move
    UniquePtr ptr2 = std::move(ptr1);
    
    // This should be flagged as use-after-move
    int* p = ptr1.ptr;  // ERROR: ptr1 has been moved
}

void consume(UniquePtr p) {
    // Function that takes ownership
}

void test_move_in_call() {
    UniquePtr ptr(new int(42));
    
    // Move ptr into function
    consume(std::move(ptr));
    
    // This should be flagged as use-after-move
    int* p = ptr.ptr;  // ERROR: ptr has been moved
}

void test_multiple_moves() {
    UniquePtr ptr1(new int(42));
    UniquePtr ptr2 = std::move(ptr1);
    
    // Try to move again - should error
    UniquePtr ptr3 = std::move(ptr1);  // ERROR: ptr1 already moved
}

int main() {
    test_basic_move();
    test_move_in_call();
    test_multiple_moves();
    return 0;
}