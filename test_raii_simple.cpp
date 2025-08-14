// What happens without understanding destructors

class Box {
    int* data;
public:
    Box(int* p) : data(p) {}
    ~Box() { delete data; }
    int* get() { return data; }
};

void problem1() {
    int* ptr = new int(42);
    
    {
        Box b(ptr);  // b now owns ptr
    } // b's destructor runs here, deleting ptr
    
    // This is use-after-free but we don't catch it!
    *ptr = 100;  // CRASH - ptr was deleted
}

void problem2() {
    int* ptr = new int(42);
    Box b1(ptr);
    Box b2(ptr);  // Double ownership - both will try to delete!
    // When b1 and b2 go out of scope, ptr is deleted twice
} // CRASH - double delete

void problem3() {
    Box* b = new Box(new int(42));
    delete b;  // Destructor called, inner int is freed
    
    // But we don't track that the destructor ran
    // So we don't know the internal pointer is now invalid
}