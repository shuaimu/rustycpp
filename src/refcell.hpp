#pragma once
#include "utils.hpp"

namespace rsptr {
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
}