## RustyCpp: Writing C++ that is easier to debug.

This is a style guide to help write C++ that is easier to debug (or have fewer bugs to begin with) by mimicking how Rust does it. 

## Being explicit

This is one of the most important philosophies of Rust. It helps avoid errors caused by overlooking the hidden or implicit code.  

### No computation in constructors/destructors 
The constructors are limited to initialize (the memory layout of) member variables and that is all. If there is extra to do, write an Init function. If Init needs to be called on member variables, do not do it in the constructor. Instead, do it in an Init function.

If you have computation in destructor, such as setting a flag or stop a thread, put it in a Destroy function and have it explicitly called.  

### Composition over inheritance
Avoid using inheritance.  

We still want polymorphism. In this case, limit the inheritance to just one layer: abstract class and implementation class. 
The abstract class has no member variables and all its member functions are pure virtual with =0.
The implementation class is final and cannot be inherited further. 

### Use move and disallow copy assignment/constructor
Except primitive types, use `move` instead of copy. There are multiple ways to disallow copy constructor, our convention is inheriting the `boost::noncopyable` class.
```
class X: private boost::noncopyable {};
```

If copy from an object is necessary, implement move constructor and a `Clone` function. 

```
Object obj1 = move(obj2.Clone()); // move can be omitted because it is already a right value. 
```

### Use POD types  
Try to use [POD](https://en.cppreference.com/w/cpp/named_req/PODType) types if possible. POD means "plain old data". A class is POD if:  
* No user-defined copy assignment
* No virtual functions
* No destructor 

## Memory safety, pointers, and references

### No raw pointers

Do not use raw pointers unless it is required in system calls, in which case wrapping all pointers in a separate class. 

### Ownership model: unique_ptr and shared_ptr

Try to program with the "owenership" model, that each object is owned by another object or function for its lifetime. 

To transfer ownership, use unique_ptr to wrap the object. 

Try to avoid shared ownership. This is the hardest part with the ownership model, but in most cases it is achievable. If shared ownership is really necessary, consider using shared_ptr but remember that shared_ptr has non-neglible performance cost because it uses atomic instructions for reference counting (it is like Arc not Rc).   

### Borrow checking
The closest thing that the C++ has to the borrow checking would be the [Circle C++](https://www.circle-lang.org/site/index.html). But it is still experimental and not open sourced. If we stick to original C++, compile time borrow checking seems impossible, [link](https://docs.google.com/document/d/e/2PACX-1vSt2VB1zQAJ6JDMaIA9PlmEgBxz2K5Tx6w2JqJNeYCy0gU4aoubdTxlENSKNSrQ2TXqPWcuwtXe6PlO/pub). 

The next best thing we can do is a runtime checking, basically the `RefCell` smart pointer in Rust. 
The repo has an implementation of it in C++. 
Try to use it over shared_ptr.   


### TODO the RefCell smart pointers


## Incrementally migrate to Rust (C++/Rust interop)
Some language (like D, Zig, Swift) has seamless integration support for C++. It then would be easier to adopt the new language in a C++ project. You will just need to write new code in that language and interop with existing C++ seamlessly. 

Unfortunately, Rust does not support this (perhaps intentionally because it risks downgrading Rust to a second-class citizen in C++ ecosystem), [link](https://internals.rust-lang.org/t/true-c-interop-built-into-the-language/19175/5).
The current best way to do C++/Rust interop is through cxx/autocxx crates. 
The interop is a semi-automated process based on the C FFIs that both C++ and Rust support. 
However, if the rest of the C++ code follow this guideline, especially if all types are POD, the interop experience should be very close to a seamless integration that other languages have (yet to verify).  


## TODO 

* Investigate microsoft [proxy](https://github.com/microsoft/proxy). It looks like a promising approach to add polymorphism to POD types. But can it be integrated with cxx/autocxx?  
* Investigate autocxx. It provides an interesting feature to implement a C++ subclass in Rust. Can it do the reverse (implement a Rust trait in C++)?
* Multi threading? 