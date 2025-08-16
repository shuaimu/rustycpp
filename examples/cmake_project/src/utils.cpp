// Utility functions - mix of safe and unsafe code

#include <vector>
#include <algorithm>

// Unsafe by default (no annotation)
void sort_raw_array(int* arr, size_t size) {
    // Uses raw pointer - OK because function is unsafe by default
    std::sort(arr, arr + size);
}

// @safe
namespace utils {
    
    // @safe
    void sort_vector(std::vector<int>& vec) {
        std::sort(vec.begin(), vec.end());
        // Safe - no raw pointers
    }
    
    // @safe
    int sum_vector(const std::vector<int>& vec) {
        int total = 0;
        for (int val : vec) {
            total += val;
        }
        return total;
    }
    
    // @safe
    void unsafe_caller() {
        int arr[5] = {5, 2, 8, 1, 9};
        // This should error - calling unsafe function from safe context
        sort_raw_array(arr, 5);  // Error: requires unsafe annotation
    }
}