#pragma once
#include "utils.hpp"

namespace rsptr {
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
	explicit Rc(T* p) : raw_(p), strong_cnt_(new int32_t(1)), weak_cnt_(new int32_t(0)) {
		borrow_verify(p != nullptr, "trying to create Rc with null pointer");
	}
	explicit Rc(const T& p)
		: raw_(new T(p)),
		  strong_cnt_(new int32_t(1)),
		  weak_cnt_(new int32_t(0)) {}
	explicit Rc(T&& p)
		: raw_(new T(std::move(p))),
		  strong_cnt_(new int32_t(1)),
		  weak_cnt_(new int32_t(0)) {}
	explicit Rc(Rc&& p)
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
		return Rc(std::move(rc));
	}

	Weak downgrade() {
		Weak weak;
		weak.rc_ = this;
		(*weak_cnt_)++;
		return weak;
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
}