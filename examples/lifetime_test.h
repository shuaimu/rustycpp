#ifndef LIFETIME_TEST_H
#define LIFETIME_TEST_H

// @lifetime: &'static
const int& getRef();

// @lifetime: (&'a) -> &'a
const int& identity(const int& x);

// @lifetime: (&'a, &'b) -> &'a where 'a: 'b
const int& selectFirst(const int& first, const int& second);

// @lifetime: owned
int getValue();

// @lifetime: &'local
const int& returnLocal();

#endif // LIFETIME_TEST_H