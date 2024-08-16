#pragma once
#include <execinfo.h>
#include <atomic>
#include <cassert>
#include <csignal>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <utility>
#include "../external/dbg.h"
namespace rsptr {
// Macros for custom error handling
#define PRINT_STACK_TRACE()                                            \
	do {                                                               \
		void* buffer[30];                                              \
		int size = backtrace(buffer, 30);                              \
		char** symbols = backtrace_symbols(buffer, size);              \
		if (symbols == nullptr) {                                      \
			std::cerr << "Failed to obtain stack trace." << std::endl; \
			break;                                                     \
		}                                                              \
		std::cerr << "Stack trace:" << std::endl;                      \
		for (int i = 0; i < size; ++i) {                               \
			std::cerr << symbols[i] << std::endl;                      \
		}                                                              \
		free(symbols);                                                 \
	} while (0)

#ifndef borrow_verify
#ifdef BORROW_INFER_CHECK
#define borrow_verify(x)               \
	{                                  \
		if (!(x)) {                    \
			volatile int* a = nullptr; \
			*a;                        \
		}                              \
	}
// #define borrow_verify(x) nullptr
#else

#define DBG_MACRO_NO_WARNING
#define borrow_verify(x, errmsg) \
	do {                         \
		if (!(x)) {              \
			dbg(x, errmsg);      \
			PRINT_STACK_TRACE(); \
			std::abort();        \
		}                        \
	} while (0)

#endif
#endif
}  // namespace rsptr