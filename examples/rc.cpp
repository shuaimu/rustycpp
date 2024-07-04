#include "../borrow.h"
#include <vector>
using namespace borrow;
template <typename T>
using Weak = Rc<T>::Weak;
using namespace std;

// reference: https://doc.rust-lang.org/book/ch15-06-reference-cycles.html

struct Node{
    int val;
    RefCell<Weak<Node>> parent; 
    RefCell<vector<Rc<Node>>> children;
};


int main(){
	// auto test_parent = RefCell(Weak<Node>());

	auto leaf = Rc<Node>(Node(
        3, RefCell(Weak<Node>()), RefCell(vector<Rc<Node>>())
    )
    );

    cout << "leaf parent" << (leaf->parent.borrow()->upgrade()).has_value()<< endl;
}