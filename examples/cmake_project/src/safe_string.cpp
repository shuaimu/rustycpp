#include "safe_string.h"
#include <utility>

// @safe
namespace safe {

SafeString::SafeString(const char* str) 
    : data(std::make_unique<std::string>(str)) {
}

SafeString::~SafeString() = default;

// @safe
const std::string& SafeString::get() const {
    return *data;
}

// @safe
std::unique_ptr<std::string> SafeString::release() {
    return std::move(data);  // Transfer ownership
}

// @safe
SafeString::SafeString(SafeString&& other) noexcept
    : data(std::move(other.data)) {
}

// @safe
SafeString& SafeString::operator=(SafeString&& other) noexcept {
    if (this != &other) {
        data = std::move(other.data);
    }
    return *this;
}

} // namespace safe