#include "Log.h"
#include "Types.h"
#include <stdarg.h>

#if defined(ESP8266)
#include <Arduino.h>
#elif defined(LIVE)
#include <stdio.h>
#endif 

#if defined(DEBUG)
#include <intrin.h>
void BREAK() {
    __debugbreak();
}
#endif

void Sync::vprint(const char* str, va_list args)
{
#if defined(LIVE)
    vprintf(str, args);
#else 
#error "Not implemented yet."
#endif
}

void Sync::print(const char *str, ...)
{
    va_list args;
    va_start(args, str);
    vprint(str, args);
    va_end(args);
}

void Sync::log_error(const char* where, const char* what) {
    print("[Error] %s: %s\n", where, what);
    BREAK();
}

void Sync::log_info(const char* what, ...) {
    print("[Info]: ");
    va_list args;
    va_start(args, what);
    vprint(what, args);
    va_end(args);
    print("\n");
}