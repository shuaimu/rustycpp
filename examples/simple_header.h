#ifndef SIMPLE_HEADER_H
#define SIMPLE_HEADER_H

// @lifetime: &'a
const int& getGlobalRef();

// @lifetime: (&'a) -> &'a
const int& identity(const int& x);

// @lifetime: owned
int getValue();

// @lifetime: &'a mut
int& getMutableRef();

// @lifetime: (&'a, &'b) -> &'a where 'a: 'b
const int& selectFirst(const int& a, const int& b);

#endif // SIMPLE_HEADER_H