class UniquePtr {
public:
    UniquePtr(int* p) : ptr(p) {}
    UniquePtr(UniquePtr&& other) : ptr(other.ptr) { other.ptr = nullptr; }
    int* ptr;
};

UniquePtr&& move(UniquePtr& p) {
    return static_cast<UniquePtr&&>(p);
}

void test() {
    int* raw = new int(42);
    UniquePtr ptr(raw);
    int count = 0;
    
    while (count < 2) {
        UniquePtr moved = move(ptr);
        count++;
    }
}