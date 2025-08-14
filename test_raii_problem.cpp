// Simple example showing what we miss without constructor/destructor tracking

class SmartPtr {
    int* data;
public:
    // Constructor takes ownership
    SmartPtr(int* p) : data(p) {}
    
    // Destructor releases ownership
    ~SmartPtr() { 
        delete data; 
    }
    
    // Move constructor transfers ownership
    SmartPtr(SmartPtr&& other) : data(other.data) {
        other.data = nullptr;  // Critical: source no longer owns
    }
    
    int* get() { return data; }
};

void test() {
    int* raw = new int(42);
    
    // Problem 1: Constructor takes ownership but we don't track it
    SmartPtr ptr1(raw);
    // At this point, ptr1 owns raw, but borrow checker doesn't know
    
    // Problem 2: Move constructor transfers ownership but we don't fully understand it
    SmartPtr ptr2 = std::move(ptr1);
    // ptr2 now owns the memory, ptr1 doesn't, but we only track "moved" state
    
    // Problem 3: We don't know ptr1's destructor won't delete anything (it's null)
    // and ptr2's destructor WILL delete the memory
    
    // This would be a use-after-free if we could detect it:
    if (false) {  // Just to show the issue
        SmartPtr temp(raw);  // Two owners of same memory!
    } // temp's destructor would delete raw
    
    *raw = 100;  // This would crash if temp existed
}