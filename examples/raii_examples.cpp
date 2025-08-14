// Examples of RAII patterns that the borrow checker doesn't understand

#include <memory>
#include <fstream>

// Example 1: Basic RAII - Destructor releases resources
class FileHandle {
    FILE* file;
public:
    FileHandle(const char* name) : file(fopen(name, "r")) {}
    
    ~FileHandle() { 
        if (file) fclose(file);  // Destructor cleans up
    }
    
    // Move constructor transfers ownership
    FileHandle(FileHandle&& other) : file(other.file) {
        other.file = nullptr;  // Source loses ownership
    }
};

void example1() {
    FileHandle f1("data.txt");
    FileHandle f2 = std::move(f1);  
    // Borrow checker sees the move but doesn't understand that:
    // 1. f1's destructor won't close the file (it's nullptr)
    // 2. f2's destructor WILL close the file
    // 3. The file is safely managed throughout
}

// Example 2: Scope-based cleanup
void example2() {
    {
        std::unique_ptr<int> ptr(new int(42));
        // When ptr goes out of scope, destructor deletes the memory
    } // <- Destructor called here automatically
    
    // Borrow checker doesn't understand that the memory is freed
    // It just knows ptr is out of scope, not that resources were released
}

// Example 3: Custom RAII lock guard
class MutexGuard {
    Mutex& mutex;
public:
    MutexGuard(Mutex& m) : mutex(m) {
        mutex.lock();  // Constructor acquires
    }
    
    ~MutexGuard() {
        mutex.unlock();  // Destructor releases
    }
    
    // Delete copy to prevent double-unlock
    MutexGuard(const MutexGuard&) = delete;
};

void example3() {
    Mutex m;
    
    {
        MutexGuard guard(m);  // Lock acquired
        // ... critical section ...
    } // <- Lock automatically released here
    
    // Borrow checker doesn't understand the lock is released
    // Can't detect double-lock or use-while-locked issues
}

// Example 4: Object with internal ownership
class String {
    char* data;
    size_t len;
    
public:
    String(const char* s) {
        len = strlen(s);
        data = new char[len + 1];
        strcpy(data, s);
    }
    
    ~String() {
        delete[] data;  // Destructor frees memory
    }
    
    // Copy constructor - deep copy
    String(const String& other) {
        len = other.len;
        data = new char[len + 1];
        strcpy(data, other.data);
    }
    
    // Move constructor - transfer ownership
    String(String&& other) {
        data = other.data;
        len = other.len;
        other.data = nullptr;  // Source loses ownership
        other.len = 0;
    }
};

void example4() {
    String s1("hello");
    String s2 = s1;  // Copy - both own separate memory
    String s3 = std::move(s1);  // Move - s3 takes s1's memory
    
    // Borrow checker doesn't understand:
    // - s2 has its own copy (deep copy via copy constructor)
    // - s1 is now empty but safe (move constructor nulled it)
    // - All three will properly clean up via destructors
}

// Example 5: Problems this causes for analysis
class UniquePtr {
    int* ptr;
public:
    UniquePtr(int* p) : ptr(p) {}
    ~UniquePtr() { delete ptr; }
    
    UniquePtr(UniquePtr&& other) : ptr(other.ptr) {
        other.ptr = nullptr;
    }
    
    int* get() { return ptr; }
};

void example5_false_negative() {
    int* raw = new int(42);
    {
        UniquePtr p1(raw);
    } // <- Destructor deletes raw
    
    *raw = 100;  // Use after free! But borrow checker doesn't catch this
    // because it doesn't understand p1's destructor freed the memory
}

void example5_false_positive() {
    UniquePtr p1(new int(42));
    int* raw = p1.get();
    *raw = 100;  // This is actually safe
    
    // But if borrow checker treats get() as borrowing,
    // it might think p1 is borrowed when it's just returning a pointer
    // The destructor will still run and that's OK
}

// Example 6: Temporary objects
int process(String s) {
    return s.length();
}

void example6() {
    int len = process(String("temporary"));
    // Temporary String is created, passed to process, then destroyed
    // Borrow checker doesn't track temporary object lifetime
}

// What we need:
// 1. Track constructor calls (including copy/move constructors)
// 2. Track destructor calls (automatic at scope end)
// 3. Understand that moved-from objects won't release resources
// 4. Model temporary object lifetimes
// 5. Understand RAII patterns (lock guards, smart pointers, etc.)