#pragma once
#include <atomic>
#include <optional>

#include "utils.hpp"

namespace rsptr {
template <typename T>
class Arc {
   public:
	class Weak {
		friend class Arc<T>;

	   public:
		Weak() = default;
		Weak(const Weak&) = delete;
		Weak(Weak&& p) : arc_(p.arc_) { p.arc_ = nullptr; }
		Weak& operator=(Weak&& p) {
			borrow_verify(arc_ == nullptr,
						  "the Weak smart pointer is already initialized");
			arc_ = p.arc_;
			p.arc_ = nullptr;
			return *this;
		}

		std::optional<Arc<T>> upgrade() const {
			if (arc_ == nullptr ||
				arc_->strong_cnt_->load(std::memory_order_relaxed) == 0) {
				return std::nullopt;
			}
			auto ret = arc_->clone();
			ret.weak_cnt_->fetch_sub(1, std::memory_order_relaxed);
			return std::make_optional(std::move(ret));
		}

		~Weak() {
			if (arc_ != nullptr) {
				arc_->weak_cnt_->fetch_sub(1, std::memory_order_relaxed);
			}
		}

	   private:
		Arc<T>* arc_{nullptr};
	};

	Arc(const Arc&) = delete;
	explicit Arc(T* p)
		: raw_(p),
		  strong_cnt_(new std::atomic<int>(1)),
		  weak_cnt_(new std::atomic<int>(0)) {
		borrow_verify(p != nullptr, "trying to create Arc with null pointer");
	}
	explicit Arc(const T& p)
		: raw_(new T(p)),
		  strong_cnt_(new std::atomic<int>(1)),
		  weak_cnt_(new std::atomic<int>(0)) {}
	explicit Arc(T&& p)
		: raw_(new T(std::move(p))),
		  strong_cnt_(new std::atomic<int>(1)),
		  weak_cnt_(new std::atomic<int>(0)) {}
	explicit Arc(Arc&& p)
		: raw_(p.raw_), strong_cnt_(p.strong_cnt_), weak_cnt_(p.weak_cnt_) {
		p.raw_ = nullptr;
		p.strong_cnt_ = p.weak_cnt_ = nullptr;
	}

	T* operator->() { return raw_; }

	const T* as_ptr() const { return raw_; }

	T& operator*() { return *raw_; }

	Arc clone() {
		Arc<T> arc;
		arc.raw_ = raw_;
		arc.strong_cnt_ = strong_cnt_;
		arc.weak_cnt_ = weak_cnt_;
		(*strong_cnt_)++;
		return Arc(std::move(arc));
	}

	Weak downgrade() {
		Weak weak;
		weak.arc_ = this;
		(*weak_cnt_)++;
		return weak;
	}

	int32_t strong_count() const {
		return strong_cnt_->load(std::memory_order_relaxed);
	}

	int32_t weak_count() const {
		return weak_cnt_->load(std::memory_order_relaxed);
	}

	~Arc() {
		if (strong_cnt_ != nullptr &&
			strong_cnt_->fetch_sub(1, std::memory_order_relaxed) == 1) {
			delete raw_;
			if (weak_cnt_->load(std::memory_order_relaxed) == 0) {
				delete strong_cnt_;
				delete weak_cnt_;
			}
		}
	}

   private:
	Arc() = default;
	T* raw_{nullptr};
	std::atomic<int>* strong_cnt_{nullptr};
	std::atomic<int>* weak_cnt_{nullptr};
};

}  // namespace rsptr
