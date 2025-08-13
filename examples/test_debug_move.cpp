// Simple test to debug std::move detection
namespace std {
    template<typename T>
    T&& move(T& t);
}

void test() {
    int x = 42;
    int y = std::move(x);  // Should be detected as a move
    
    // Use after move
    int z = x;  // Should error
}