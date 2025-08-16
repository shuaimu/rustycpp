// Simple demo of Rusty types that our checker can parse
// Avoids complex template features

extern "C" int printf(const char*, ...);

// Simple move function (our checker understands this)
template<typename T>
T&& move(T& x) {
    return static_cast<T&&>(x);
}

// Simplified Box for demo (no templates to simplify parsing)
// @safe
namespace rusty_simple {
    
    class BoxInt {
    private:
        int* ptr;
        
    public:
        BoxInt() : ptr(nullptr) {}
        explicit BoxInt(int value) : ptr(new int(value)) {}
        
        // No copy
        BoxInt(const BoxInt&) = delete;
        BoxInt& operator=(const BoxInt&) = delete;
        
        // Move constructor
        BoxInt(BoxInt&& other) : ptr(other.ptr) {
            other.ptr = nullptr;
        }
        
        // Move assignment
        BoxInt& operator=(BoxInt&& other) {
            if (this != &other) {
                delete ptr;
                ptr = other.ptr;
                other.ptr = nullptr;
            }
            return *this;
        }
        
        ~BoxInt() {
            delete ptr;
        }
        
        // @lifetime: (&'a) -> &'a
        int& operator*() {
            return *ptr;
        }
        
        // @lifetime: (&'a) -> &'a
        const int& operator*() const {
            return *ptr;
        }
        
        bool is_valid() const {
            return ptr != nullptr;
        }
    };
    
    // Factory function
    // @lifetime: owned
    // @safe
    BoxInt make_box(int value) {
        return BoxInt(value);
    }
}

// @safe
namespace demo {
    
    // Test 1: Proper move semantics (should pass)
    // @safe
    void test_box_move_correct() {
        auto box1 = rusty_simple::make_box(42);
        printf("Box1 value: %d\n", *box1);
        
        // Move ownership
        auto box2 = move(box1);
        printf("Box2 value after move: %d\n", *box2);
        
        // box1 is now invalid, but we don't use it
    }
    
    // Test 2: Use after move (should fail)
    // @safe
    void test_box_use_after_move() {
        auto box1 = rusty_simple::make_box(100);
        auto box2 = move(box1);
        
        // ERROR: This should be caught by the checker
        int value = *box1;  // Use after move!
        printf("Invalid access: %d\n", value);
    }
    
    // Test 3: Multiple ownership attempts (move prevents copies)
    // @safe
    void test_no_double_ownership() {
        auto box1 = rusty_simple::make_box(200);
        
        // This won't compile due to deleted copy constructor:
        // auto box2 = box1;  // Error: copy constructor deleted
        
        // Only move is allowed
        auto box3 = move(box1);
        printf("Box3 has ownership: %d\n", *box3);
    }
    
    // Test 4: Borrow checking with references
    // @safe
    void test_borrow_checking() {
        auto box = rusty_simple::make_box(42);
        
        // Multiple immutable borrows - OK
        const int& ref1 = *box;
        const int& ref2 = *box;
        printf("Immutable refs: %d, %d\n", ref1, ref2);
        
        // This would violate borrow rules if uncommented:
        // int& mut_ref = *box;  // Can't have mutable with immutable
    }
    
    // Test 5: Lifetime safety
    // @safe
    const int& get_box_ref(const rusty_simple::BoxInt& box) {
        return *box;  // Lifetime tied to box parameter
    }
    
    // @safe
    void test_lifetime_safety() {
        auto box = rusty_simple::make_box(300);
        const int& ref = get_box_ref(box);
        printf("Reference value: %d\n", ref);
        
        // ref is valid as long as box exists
        // When box goes out of scope, ref becomes invalid
    }
}

int main() {
    printf("Simple Rusty Types Demo\n");
    printf("=======================\n");
    
    // Run safe tests
    demo::test_box_move_correct();
    demo::test_no_double_ownership();
    demo::test_borrow_checking();
    demo::test_lifetime_safety();
    
    // Test use-after-move detection:
    demo::test_box_use_after_move();  // Should error
    
    printf("\nDemo complete\n");
    return 0;
}