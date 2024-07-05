#include "../src/rsptr.hpp"
using namespace rsptr;
// infer run --pulse-only -- clang++ -x c++ -std=c++11 -O0 borrow.h -D
// BORROW_TEST=1 -D BORROW_INFER_CHECK=1
void test1() {
	RefCell<int> owner;
	owner.reset(new int(5));
	auto x = borrow_mut(owner);
	auto y = borrow_mut(owner);
}

void test2() {
	RefCell<int> owner;
	owner.reset(new int(5));
	{ auto x = borrow_mut(owner); }
	auto y = borrow_mut(owner);
	{ auto z = borrow_mut(owner); }
}

void test3() {
	RefCell<int> owner;
	owner.reset(new int(5));
	auto x = borrow(owner);
	auto y = borrow(owner);
	auto z = borrow_mut(owner);
}

int main() {
	test1();
	test2();
	test3();
}
