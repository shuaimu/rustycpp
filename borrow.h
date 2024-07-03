#pragma once
#include <cstdint>
#include <memory>
#include <utility>
#include <atomic>
#include <cassert>
#include <cstdlib>
#include <csignal>
#include <execinfo.h>
#include "./external/dbg.h"
#include <iostream>

namespace borrow {

// Macros for custom error handling
#define PRINT_STACK_TRACE() \
    do { \
        void* buffer[30]; \
        int size = backtrace(buffer, 30); \
        char** symbols = backtrace_symbols(buffer, size); \
        if (symbols == nullptr) { \
            std::cerr << "Failed to obtain stack trace." << std::endl; \
            break; \
        } \
        std::cerr << "Stack trace:" << std::endl; \
        for (int i = 0; i < size; ++i) { \
            std::cerr << symbols[i] << std::endl; \
        } \
        free(symbols); \
    } while(0)

#ifndef borrow_verify
#ifdef BORROW_INFER_CHECK
#define borrow_verify(x) {if (!(x)) {volatile int* a = nullptr ; *a;}}
//#define borrow_verify(x) nullptr
#else

#define DBG_MACRO_NO_WARNING
#define borrow_verify(x, errmsg) \
    do { \
        if (!(x)) { \
            dbg(x, errmsg);\
            PRINT_STACK_TRACE(); \
            std::abort(); \
        } \
    } while(0) 
  
#endif
#endif


// define RefCell before Ref and RefMut
template<class T>
class RefCell;

template<class T>
class Ref {
friend class RefCell<T>;
public:
  Ref() = default;
  Ref(const Ref&) = delete;
  Ref(Ref&& p) : raw_(p.raw_), p_cnt_(p.p_cnt_){
    p.raw_ = nullptr;
    p.p_cnt_ = nullptr;
  };
  Ref(Ref& p) : raw_(p.raw_), p_cnt_(p.p_cnt_){
    auto i = (*p_cnt_)++;
    borrow_verify(i > 0, "error in Ref constructor");
  }
  const T* operator->() {
    return raw_;
  }

  const T& operator*() {
    return *raw_;
  }

  void reset() {
    auto before_sub = p_cnt_->fetch_sub(1);
    borrow_verify(before_sub > 0, "Trying to reset a reference of null pointer");
    raw_ = nullptr;
    p_cnt_ = nullptr;
  }
  ~Ref() {
    if (p_cnt_ != nullptr) {
      auto before_sub = p_cnt_->fetch_sub(1);
      borrow_verify(before_sub > 0, "Trying to deference null pointer"); // failure means - count became negative which is not possible
    } 
  }

private:
  const T* raw_{nullptr};
  std::atomic<int32_t>* p_cnt_{nullptr};
};

template <typename T>
class RefMut {
friend class RefCell<T>;
 public:
  RefMut() = default;
  RefMut(const RefMut&) = delete;
  RefMut(RefMut&& p) : raw_(p.raw_), p_cnt_(p.p_cnt_) {
    p.raw_ = nullptr;
    p.p_cnt_ = nullptr;
  }

  T* operator->() {
    return raw_;
  }

  T& operator*() {
    return *raw_;
  }
  void reset() {
    auto i = p_cnt_->fetch_add(1);
	borrow_verify(i == -1, "error in checking just single reference of RefMut");
	  p_cnt_ = nullptr;
    raw_ = nullptr;
  }
  ~RefMut() {
    if (p_cnt_ != nullptr) {
      auto i = p_cnt_->fetch_add(1);
      borrow_verify(i == -1, "error in checking just single reference of RefMut");
    }
  }
private:
  T* raw_;
  std::atomic<int32_t>* p_cnt_;
};


template <typename T> 
class Rc{
class Weak {
public: 
  Weak(const Weak&) = delete;
  Weak() = default;
  Weak(Rc<T>& rc) : rc_(&rc) {
    rc_->weak_cnt_++;
  }
  std::optional<Rc<T>> upgrade() {
    if (rc_ != nullptr && rc_->strong_cnt_ == 0) {
      return std::nullopt;
    }
    rc_->weak_cnt_--;
    rc_->strong_cnt_++;
    return *rc_;
  }

  ~Weak() {
    if (rc_->strong_cnt_ == 0 && --rc_->weak_cnt_ == 0) {
      delete rc_;
    }
  }
private:
  Rc<T>* rc_{nullptr}; // it is guaranteed that rc_ is not null in the lifetime of Weak (implemented by weak_cnt_ in Rc)
};


public:
  Rc(const Rc&) = delete;
  Rc() = default;
  explicit Rc(T* p) : raw_(p), strong_cnt_(new int32_t(1)), weak_cnt_(new int32_t(0)) {}

  Rc(Rc&& p) : raw_(p.raw_), strong_cnt_(p.strong_cnt_), weak_cnt_(p.weak_cnt_) {
    p.raw_ = p.strong_cnt_ = p.weak_cnt_ = nullptr;
  }

  T* operator->() {
    return raw_;
  }

  T& operator*() {
    return *raw_;
  }

  Rc clone() {
    Rc<T> rc;
    rc.raw_ = raw_;
    rc.strong_cnt_ = strong_cnt_;
    rc.weak_cnt_ = weak_cnt_;
    (*strong_cnt_)++;
    return rc;
  }

  Weak downgrade() {
    return Weak(*this);
  }

  ~Rc() { 
    if (--(*strong_cnt_) == 0) {
      delete raw_;
      if (*weak_cnt_ == 0){
        delete strong_cnt_;
        delete weak_cnt_;
      }
    }
  }

 private:
  T* raw_{nullptr};
  int32_t *strong_cnt_{nullptr}, *weak_cnt_{nullptr};
};

template <class T>
class RefCell {
 public:
  RefCell(const RefCell&) = delete;
  RefCell() = default;
  explicit RefCell(T* p) : raw_(p), cnt_(0) {
  };
  RefCell(RefCell&& p) {
    auto i = p.cnt_.exchange(-2);
    borrow_verify(i==0, "verify failed in RefCell move constructor");
    borrow_verify(cnt_ == 0, "verify failed in RefCell move constructor");
    cnt_ = i;
    raw_ = p.raw_;
    p.raw_ = nullptr;
    p.cnt_ = 0;
  };

  inline void reset(T* p) {
    borrow_verify(cnt_ == 0, "error in RefCell reset");
    raw_ = p;
    borrow_verify(cnt_ == 0, "error in RefCell reset"); // is this enough to capture data race?
  }

  inline RefMut<T> borrow_mut() {
    RefMut<T> mut;
    borrow_verify(cnt_ == 0, "verify failed in borrow_mut");
    cnt_--;
    mut.p_cnt_ = &cnt_;
    mut.raw_ = raw_;
    raw_ = nullptr;
    return mut;
  }

  inline Ref<T> borrow_const() {
    // *raw_; // for refer static analysis
    auto i = cnt_++;
    borrow_verify(i >= 0, "verify failed in borrow_const");
    Ref<T> ref;
    ref.raw_ = raw_;
    ref.p_cnt_ = &cnt_;
    return ref;
  }

  T* operator->() {
    borrow_verify(cnt_==0, "verify failed in ->");
    return raw_;
  }

  void reset() {
    borrow_verify(cnt_ == 0, "verify failed in RefCell reset");
    delete raw_;
    raw_ = nullptr;
  }

  ~RefCell() {
    if (raw_ != nullptr) {
      reset();
    }
  }

private:
  T* raw_{nullptr}; 
  std::atomic<int32_t> cnt_{0}; // negative values indicate RefMut, positive values indicate Ref
};

template <typename T>
inline RefMut<T> borrow_mut(RefCell<T>& RefCell) {
  return std::forward<RefMut<T>>(RefCell.borrow_mut());
}

template <typename T>
inline Ref<T> borrow_const(RefCell<T>& RefCell) {
  return std::forward<Ref<T>>(RefCell.borrow_const());
}

template <typename T>
inline void reset_ptr(RefCell<T>& ptr) {
  return ptr.reset();
}

template <typename T>
inline void reset_ptr(RefMut<T>& ptr) {
  return ptr.reset();
}

template <typename T>
inline void reset_ptr(Ref<T>& ptr) {
  return ptr.reset();
}

} // namespace borrow;

// infer run --pulse-only -- clang++ -x c++ -std=c++11 -O0 borrow.h -D BORROW_TEST=1 -D BORROW_INFER_CHECK=1
#ifdef BORROW_TEST
using namespace borrow;
void test1() {
  RefCell<int> owner; 
  owner.reset(new int(5));
  auto x = borrow_mut(owner);
  auto y = borrow_mut(owner);
}

void test2() {
  RefCell<int> owner; 
  owner.reset(new int(5));
  {
    auto x = borrow_mut(owner);
  }
  auto y = borrow_mut(owner);
  {
    auto z = borrow_mut(owner);
  }
}

void test3() {
  RefCell<int> owner; 
  owner.reset(new int(5));
  auto x = borrow_const(owner);
  auto y = borrow_const(owner);
  auto z = borrow_mut(owner);
}

int main() {
  test1();
  test2();
  test3();
}


#endif