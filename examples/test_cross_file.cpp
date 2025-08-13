#include "string_utils.h"
#include <iostream>

void test_lifetime_annotations() {
    // Test 1: Basic immutable borrow
    const std::string& static_str = getStaticString();
    std::cout << static_str << std::endl;
    
    // Test 2: Identity function preserves lifetime
    std::string local = "hello";
    const std::string& ref = identity(local);
    // ref has same lifetime as local
    
    // Test 3: Selecting first with lifetime bounds
    std::string str1 = "first";
    std::string str2 = "second";
    const std::string& selected = selectFirst(str1, str2);
    // selected borrows from str1
    
    // Test 4: Ownership transfer
    std::string owned = copyString(str1);
    // owned is a new string, doesn't borrow from str1
    
    // Test 5: Mutable borrow
    std::string& mut_ref = getMutableString();
    mut_ref = "modified";
    
    // Test 6: Mutable parameter and return
    std::string mut_str = "original";
    std::string& modified = modifyString(mut_str);
    modified += " changed";
}

void test_borrow_violations() {
    std::string value = "test";
    
    // Multiple immutable borrows - OK
    const std::string& ref1 = value;
    const std::string& ref2 = value;
    
    // Can't have mutable borrow while immutable borrows exist
    // std::string& mut_ref = value; // This should be an error
    
    // Can't have multiple mutable borrows
    std::string& mut_ref1 = value;
    // std::string& mut_ref2 = value; // This should be an error
}

int main() {
    test_lifetime_annotations();
    test_borrow_violations();
    return 0;
}