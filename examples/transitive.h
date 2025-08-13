#ifndef TRANSITIVE_H
#define TRANSITIVE_H

// @lifetime: (&'a, &'b) -> &'a where 'a: 'b
const int& requires_outlives(const int& longer, const int& shorter);

// @lifetime: (&'a, &'b, &'c) -> &'a where 'a: 'b, 'b: 'c
const int& requires_transitive(const int& a, const int& b, const int& c);

#endif // TRANSITIVE_H