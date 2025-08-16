#ifndef SAFE_STRING_H
#define SAFE_STRING_H

#include <string>
#include <memory>

// @safe
namespace safe {

// A string wrapper with explicit ownership
class SafeString {
private:
    std::unique_ptr<std::string> data;
    
public:
    SafeString(const char* str);
    ~SafeString();
    
    // @lifetime: (&'a) -> &'a
    const std::string& get() const;
    
    // Transfer ownership
    std::unique_ptr<std::string> release();
    
    // No copy constructor (explicit ownership)
    SafeString(const SafeString&) = delete;
    SafeString& operator=(const SafeString&) = delete;
    
    // Move constructor
    SafeString(SafeString&& other) noexcept;
    SafeString& operator=(SafeString&& other) noexcept;
};

} // namespace safe

#endif // SAFE_STRING_H