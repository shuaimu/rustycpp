#ifndef LOCAL_HEADER_H
#define LOCAL_HEADER_H

// @lifetime: &'a
const char* getLocalString();

// @lifetime: (&'a) -> &'a
const int& passThrough(const int& val);

#endif // LOCAL_HEADER_H