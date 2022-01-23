#pragma once
#include "Types.h"

// debug breaks
#if defined(DEBUG)
void BREAK();
#else
#define BREAK()
#endif

// assert
#if defined(DEBUG)
#define DYNAMIC_ASSERT(expr) if(!(expr)) BREAK()
#endif

// log_error with function sig.
#if defined(_MSC_VER)
#define LOG_ERROR(what) Sync::log_error(__FUNCSIG__, what)
#elif defined(__GNUG__)
#define LOG_ERROR(what) Sync::log_error(__PRETTY_FUNCTION__, what)
#endif

// special null return
#define NULL_FALSE(expr) if(!expr) return Sync::ERROR_POINTER_NULL;

namespace Sync {
    void vprint(const char* str, va_list args);
    void print(const char* str, ...);
    void log_error(const char* where, const char* what);
    void log_info(const char* what, ...);

    // Error Codes
    enum ErrorCodes : u32 {
            SUCCESS = 0, ERROR_UNKNOWN = 1, ERROR_POINTER_NULL = 2
    };
    using err = u32;
};