// Test move semantics with rusty types

extern "C" int printf(const char*, ...);

// Our move function
template<typename T>
T&& move(T& x) {
    return static_cast<T&&>(x);
}

// Simple unique pointer
class UniqueInt {
    int* ptr;
public:
    UniqueInt(int val) : ptr(new int(val)) {}
    ~UniqueInt() { delete ptr; }
    
    // Delete copy
    UniqueInt(const UniqueInt&) = delete;
    UniqueInt& operator=(const UniqueInt&) = delete;
    
    // Move constructor
    UniqueInt(UniqueInt&& other) : ptr(other.ptr) {
        other.ptr = nullptr;
    }
    
    int get() const { return ptr ? *ptr : 0; }
};

// @safe
void test_unique_move() {
    UniqueInt p1(42);
    UniqueInt p2 = move(p1);
    
    // Use after move - should be caught
    int val = p1.get();  // ERROR: use after move
    printf("Value: %d\n", val);
}

// @safe
void test_correct_move() {
    UniqueInt p1(100);
    printf("Before move: %d\n", p1.get());
    
    UniqueInt p2 = move(p1);
    printf("After move: %d\n", p2.get());
    // Don't use p1 after move - correct
}

int main() {
    test_correct_move();
    test_unique_move();
    return 0;
}