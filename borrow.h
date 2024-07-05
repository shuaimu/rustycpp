#pragma once
#include <execinfo.h>

#include <atomic>
#include <cassert>
#include <csignal>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <utility>

#include "./external/dbg.h"

namespace borrow {

// Macros for custom error handling
#define PRINT_STACK_TRACE()                                            \
	do {                                                               \
		void* buffer[30];                                              \
		int size = backtrace(buffer, 30);                              \
		char** symbols = backtrace_symbols(buffer, size);              \
		if (symbols == nullptr) {                                      \
			std::cerr << "Failed to obtain stack trace." << std::endl; \
			break;                                                     \
		}                                                              \
		std::cerr << "Stack trace:" << std::endl;                      \
		for (int i = 0; i < size; ++i) {                               \
			std::cerr << symbols[i] << std::endl;                      \
		}                                                              \
		free(symbols);                                                 \
	} while (0)

#ifndef borrow_verify
#ifdef BORROW_INFER_CHECK
#define borrow_verify(x)               \
	{                                  \
		if (!(x)) {                    \
			volatile int* a = nullptr; \
			*a;                        \
		}                              \
	}
// #define borrow_verify(x) nullptr
#else

#define DBG_MACRO_NO_WARNING
#define borrow_verify(x, errmsg) \
	do {                         \
		if (!(x)) {              \
			dbg(x, errmsg);      \
			PRINT_STACK_TRACE(); \
			std::abort();        \
		}                        \
	} while (0)

#endif
#endif

// define RefCell before Ref and RefMut
template <class T>
class RefCell;

template <class T>
class Ref {
	friend class RefCell<T>;

   public:
	Ref(const Ref&) = delete;
	Ref(Ref&& p) : raw_(p.raw_), p_cnt_(p.p_cnt_) {
		p.raw_ = nullptr;
		p.p_cnt_ = nullptr;
	};
	Ref(Ref& p) : raw_(p.raw_), p_cnt_(p.p_cnt_) {
		auto i = (*p_cnt_)++;
		borrow_verify(i > 0, "error in Ref constructor");
	}
	const T* operator->() const { return raw_; }

	const T& operator*() const { return *raw_; }

	void reset() {
		auto before_sub = p_cnt_->fetch_sub(1);
		borrow_verify(before_sub > 0,
					  "Trying to reset a reference of null pointer");
		raw_ = nullptr;
		p_cnt_ = nullptr;
	}
	~Ref() {
		if (p_cnt_ != nullptr) {
			auto before_sub = p_cnt_->fetch_sub(1);
			borrow_verify(before_sub > 0, "Trying to deference null pointer");
			// if the RefCell have not been deleted (not nullptr)
			// then the RefCell should have at least one reference
		}
	}

   private:
	Ref() = default;
	const T* raw_{nullptr};
	std::atomic<int32_t>* p_cnt_{nullptr};
};

template <typename T>
class RefMut {
	friend class RefCell<T>;

   public:
	RefMut(const RefMut&) = delete;
	RefMut(RefMut&& p) : raw_(p.raw_), p_cnt_(p.p_cnt_) {
		p.raw_ = nullptr;
		p.p_cnt_ = nullptr;
	}

	T* operator->() { return raw_; }

	T& operator*() { return *raw_; }
	void reset() {
		auto i = p_cnt_->fetch_add(1);
		borrow_verify(i == -1,
					  "error in checking just single reference of RefMut");
		p_cnt_ = nullptr;
		raw_ = nullptr;
	}
	~RefMut() {
		if (p_cnt_ != nullptr) {
			auto i = p_cnt_->fetch_add(1);
			borrow_verify(i == -1,
						  "error in checking just single reference of RefMut");
		}
	}

   private:
	RefMut() = default;
	T* raw_;
	std::atomic<int32_t>* p_cnt_;
};

template <typename T>
class Rc {
   public:
	class Weak {
		friend class Rc<T>;

	   public:
		Weak() = default;
		Weak(const Weak&) = delete;
		Weak(Weak&& p) : rc_(p.rc_) { p.rc_ = nullptr; }
		Weak& operator=(Weak&& p) {
			borrow_verify(rc_ == nullptr,
						  "the Weak smart pointer is already initialized");
			rc_ = p.rc_;
			p.rc_ = nullptr;
			return *this;
		}

		std::optional<Rc<T>> upgrade() const {
			if (rc_ == nullptr || rc_->strong_cnt_ == 0) {
				return std::nullopt;
			}
			auto ret = rc_->clone();
			ret.weak_cnt_--;
			return std::make_optional(std::move(ret));
		}

		~Weak() {
			if (rc_ != nullptr && rc_->strong_cnt_ == 0 &&
				--rc_->weak_cnt_ == 0) {
				rc_->~Rc();
			}
		}

	   private:
		Rc<T>* rc_{nullptr};
		// it is guaranteed that rc_ is not null in the lifetime
		// of Weak (implemented by weak_cnt_ in Rc)
	};

	Rc(const Rc&) = delete;
	Rc(T* p) : raw_(p), strong_cnt_(new int32_t(1)), weak_cnt_(new int32_t(0)) {
		borrow_verify(p != nullptr, "trying to create Rc with null pointer");
	}
	Rc(const T& p)
		: raw_(new T(p)),
		  strong_cnt_(new int32_t(1)),
		  weak_cnt_(new int32_t(0)) {}
	Rc(T&& p)
		: raw_(new T(std::move(p))),
		  strong_cnt_(new int32_t(1)),
		  weak_cnt_(new int32_t(0)) {}
	Rc(Rc&& p)
		: raw_(p.raw_), strong_cnt_(p.strong_cnt_), weak_cnt_(p.weak_cnt_) {
		p.raw_ = nullptr;
		p.strong_cnt_ = p.weak_cnt_ = nullptr;
	}

	T* operator->() { return raw_; }

	const T* as_ptr() const {
		// Rc is not mutable (cannot change where the pointer points to)
		return raw_;
	}

	T& operator*() { return *raw_; }

	Rc clone() {
		Rc<T> rc;
		rc.raw_ = raw_;
		rc.strong_cnt_ = strong_cnt_;
		rc.weak_cnt_ = weak_cnt_;
		(*strong_cnt_)++;
		return weak;
	}

	Weak downgrade() {
		Weak weak;
		weak.rc_ = this;
		(*weak_cnt_)++;
		return Weak(std::move(weak)); 
		// have to do this because weak only have explicit move constructor 
	}

	int32_t strong_count() const { return *strong_cnt_; }

	int32_t weak_count() const { return *weak_cnt_; }

	~Rc() {
		if (strong_cnt_ != nullptr && --*strong_cnt_ == 0) {
			delete raw_;
			if (*weak_cnt_ == 0) {
				delete strong_cnt_;
				delete weak_cnt_;
			}
		}
	}

   private:
	Rc() = default;
	T* raw_{nullptr};
	int32_t *strong_cnt_{nullptr}, *weak_cnt_{nullptr};
};

template <class T>
class RefCell {
   public:
	RefCell(const RefCell&) = delete;
	RefCell() = default;
	explicit RefCell(T* p) : raw_(p), cnt_(0) {
		// using explicit constructor for any pointers owning resources
		// i.e. can be constructed from either T or T*
		borrow_verify(p != nullptr,
					  "trying to create RefCell with null pointer");
	}
	explicit RefCell(T&& p) : raw_(new T(std::move(p))), cnt_(0) {}

	explicit RefCell(RefCell&& p) {
		auto i = p.cnt_.exchange(-2);
		// -2 indicates that the moved RefCell have two mutable
		// references, thus invalidating it
		borrow_verify(i == 0, "verify failed in RefCell move constructor");
		borrow_verify(cnt_ == 0, "verify failed in RefCell move constructor");
		cnt_ = i;
		raw_ = p.raw_;
		p.raw_ = nullptr;
		p.cnt_ = 0;
	}

	inline void reset(T* p) {
		borrow_verify(cnt_ == 0, "error in RefCell reset");
		raw_ = p;
		borrow_verify(
			cnt_ == 0,
			"error in RefCell reset");	// is this enough to capture data race?
	}

	inline RefMut<T> borrow_mut() {
		RefMut<T> mut;
		borrow_verify(cnt_ == 0, "verify failed in borrow_mut");
		cnt_--;
		mut.p_cnt_ = &cnt_;
		mut.raw_ = raw_;
		return mut;
	}

	inline Ref<T> borrow() const {
		// *raw_; // for refer static analysis
		auto i = cnt_++;
		borrow_verify(i >= 0, "verify failed in borrow");
		Ref<T> ref;
		ref.raw_ = raw_;
		ref.p_cnt_ = &cnt_;
		return ref;
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
	mutable std::atomic<int32_t> cnt_{0};
	// negative values indicate RefMut, positive values indicate Ref
};

template <typename T>
inline RefMut<T> borrow_mut(RefCell<T>& RefCell) {
	return std::forward<RefMut<T>>(RefCell.borrow_mut());
}

template <typename T>
inline Ref<T> borrow(RefCell<T>& RefCell) {
	return std::forward<Ref<T>>(RefCell.borrow());
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

}  // namespace borrow

// infer run --pulse-only -- clang++ -x c++ -std=c++11 -O0 borrow.h -D
// BORROW_TEST=1 -D BORROW_INFER_CHECK=1
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

#endif