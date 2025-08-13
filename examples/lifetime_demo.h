#ifndef LIFETIME_DEMO_H
#define LIFETIME_DEMO_H

// @lifetime: owned
int getValue();

// @lifetime: &'static
const int& getStaticRef();

// @lifetime: (&'a) -> &'a
const int& identity(const int& x);

// @lifetime: owned
int* createInt();

#endif // LIFETIME_DEMO_H