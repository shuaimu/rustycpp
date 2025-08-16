// Example file with both safe and unsafe code

#include <cstdlib>
#include <memory>

// Unsafe by default - uses raw pointers
void* allocate_raw(size_t size) {
    return malloc(size);
}

void deallocate_raw(void* ptr) {
    free(ptr);
}

// @safe
namespace memory {
    
    // @safe
    std::unique_ptr<int[]> allocate_array(size_t count) {
        return std::make_unique<int[]>(count);
    }
    
    // @safe
    void process_array(const int* arr, size_t size) {
        // This should trigger an error - dereferencing raw pointer in safe code
        for (size_t i = 0; i < size; ++i) {
            int value = arr[i];  // Error: pointer dereference requires unsafe
            // Process value...
        }
    }
    
    // @unsafe
    void unsafe_process(int* ptr) {
        // This is OK - we're in unsafe context
        *ptr = 42;
    }
    
    // @safe
    void safe_wrapper() {
        int x = 10;
        // This should error - calling unsafe function from safe context
        unsafe_process(&x);  // Error: requires unsafe annotation
    }
}

// Example of gradual adoption - this function remains unchecked
void legacy_function() {
    int* ptr = (int*)malloc(sizeof(int));
    *ptr = 100;
    free(ptr);
    // Use after free not detected in unchecked code
    *ptr = 200;  // Bug, but not checked by default
}