#include <vector>
#include <iostream>
#include <array> 
#include "../src/rsptr.hpp"
using namespace rsptr;
// #define Rc Arc
template <typename T>
using Weak = Rc<T>::Weak;
using namespace std;

// reference: https://doc.rust-lang.org/book/ch15-06-reference-cycles.html

struct Node {
	int val;
	RefCell<Weak<Node>> parent;
	RefCell<vector<Rc<Node>>> children;
};

int main() {
    auto test_leaf = Rc(RefCell(1));
    auto test_mutref = test_leaf->borrow_mut();


	auto leaf = Rc(Node(3, RefCell(Weak<Node>()), RefCell(vector<Rc<Node>>())));

   
	cout << "leaf parent has value " << leaf->parent.borrow()->upgrade().has_value()
		 << endl;
        
    vector<Rc<Node>> branch_children_vec; branch_children_vec.push_back(leaf.clone());
	// cannot directy construct the vector using initializer list as Rc deleted
	// copy constructor see
	// https://stackoverflow.com/questions/78709325/initializing-vector-and-array-with-initializing-list-of-objects-that-only-have-m
	auto branch = Rc(Node(5, RefCell(Weak<Node>()),
						  RefCell(std::move(branch_children_vec))));

    *leaf->parent.borrow_mut() = branch.downgrade();

    cout << "leaf parent has value " << leaf->parent.borrow()->upgrade().has_value() 
         << " leaf parent value " << leaf->parent.borrow()->upgrade().value()->val << endl;
}