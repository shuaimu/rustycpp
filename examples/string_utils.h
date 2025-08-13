#ifndef STRING_UTILS_H
#define STRING_UTILS_H

#include <string>

// @lifetime: &'a
const std::string& getStaticString();

// @lifetime: (&'a) -> &'a
const std::string& identity(const std::string& str);

// @lifetime: (&'a, &'b) -> &'a where 'a: 'b
const std::string& selectFirst(const std::string& first, const std::string& second);

// @lifetime: owned
std::string copyString(const std::string& str);

// @lifetime: &'a mut
std::string& getMutableString();

// @lifetime: (&'a mut) -> &'a mut
std::string& modifyString(std::string& str);

#endif // STRING_UTILS_H