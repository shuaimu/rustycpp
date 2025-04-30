# RustyCpp: Writing C++ that is easier to debug

This style guide aims to help developers write C++ code that is easier to debug and less prone to bugs by adopting principles from Rust.

## Being explicit

Explicitness is one of Rust's core philosophies. It helps prevent errors that arise from overlooking hidden or implicit code behaviors.

### No computation in constructors/destructors
Constructors should be limited to initializing member variables and establishing the object's memory layout—nothing more. For additional initialization steps, create a separate `Init()` function. When member variables require initialization, handle this in the `Init()` function rather than in the constructor.

Similarly, if you need computation in a destructor (such as setting flags or stopping threads), implement a `Destroy()` function that must be explicitly called before destruction.

### Composition over inheritance
Avoid inheritance whenever possible.

When polymorphism is necessary, limit inheritance to a single layer: an abstract base class and its implementation class. The abstract class should contain no member variables, and all its member functions should be pure virtual (declared with `= 0`). The implementation class should be marked as `final` to prevent further inheritance.

### Use move and disallow copy assignment/constructor
Except for primitive types, prefer using `move` instead of copy operations. There are multiple ways to disallow copy constructors; our convention is to inherit from the `boost::noncopyable` class:

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
Avoid using raw pointers except when required by system calls, in which case wrap them in a dedicated class.

### Ownership model: unique_ptr and shared_ptr

Program with the ownership model, where each object is owned by another object or function throughout its lifetime.

To transfer ownership, wrap objects in `unique_ptr`.

Avoid shared ownership whenever possible. While this can be challenging, it's achievable in most cases. If shared ownership is truly necessary, consider using `shared_ptr`, but be aware that it incurs a non-negligible performance cost due to atomic reference counting operations (similar to Rust's `Arc` rather than `Rc`).

### Borrow checking
The closest thing that the C++ has to the borrow checking would be the [Circle C++](https://www.circle-lang.org/site/index.html). But it is still experimental and not open sourced. If we stick to original C++, compile time borrow checking seems impossible, [link](https://docs.google.com/document/d/e/2PACX-1vSt2VB1zQAJ6JDMaIA9PlmEgBxz2K5Tx6w2JqJNeYCy0gU4aoubdTxlENSKNSrQ2TXqPWcuwtXe6PlO/pub). 

The next best alternative is runtime checking, similar to Rust's `RefCell` smart pointer. 
This repository includes a C++ implementation of this concept.
Consider using it instead of `shared_ptr` when appropriate.


## Incrementally migrate to Rust (C++/Rust interop)
Some languages (like D, Zig, and Swift) offer seamless integration with C++. This makes it easier to adopt these languages in existing C++ projects, as you can simply write new code in the chosen language and interact with existing C++ code without friction.

Unfortunately, Rust does not support this level of integration (perhaps intentionally to avoid becoming a secondary option in the C++ ecosystem), as discussed [here](https://internals.rust-lang.org/t/true-c-interop-built-into-the-language/19175/5).
Currently, the best approach for C++/Rust interoperability is through the cxx/autocxx crates.
This interoperability is implemented as a semi-automated process based on C FFIs (Foreign Function Interfaces) that both C++ and Rust support.
However, if your C++ code follows the guidelines in this document, particularly if all types are POD, the interoperability experience can approach the seamless integration offered by other languages (though this remains to be verified).


## TODO 

* Investigate microsoft [proxy](https://github.com/microsoft/proxy). It looks like a promising approach to add polymorphism to POD types. But can it be integrated with cxx/autocxx?  
* Investigate autocxx. It provides an interesting feature to implement a C++ subclass in Rust. Can it do the reverse (implement a Rust trait in C++)?
* Multi threading? 
* Make the RefCell implementation has the same memory layout and APIs as the Rust standard library. Then integrate it into autocxx.  