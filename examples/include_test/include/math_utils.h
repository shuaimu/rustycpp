#ifndef MATH_UTILS_H
#define MATH_UTILS_H

// @lifetime: (&'a, &'b) -> &'a
const int& max(const int& a, const int& b);

// @lifetime: owned
int square(int x);

// @lifetime: &'a mut
int& increment(int& value);

#endif // MATH_UTILS_H