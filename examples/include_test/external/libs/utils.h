#ifndef UTILS_H
#define UTILS_H

// @lifetime: (&'a) -> &'a
const char* echo(const char* str);

// @lifetime: owned
int compute(int x, int y);

#endif // UTILS_H