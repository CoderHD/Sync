#pragma once

#define sizebytes(type) sizeof(type) / 8

#define null nullptr
using ushort = unsigned short;
using uint = unsigned int;
using ulong = unsigned long;

#if   defined(LIVE)
using s8  = signed   char;
using s16 = signed   short;
using u8  = unsigned char;
using u16 = unsigned short;
using s32 = signed   int;
using u32 = unsigned int;
using s64 = signed   long;
using u64 = unsigned long;
#elif defined(EMBEDDED)
using s16 = signed   int16_t;
using u16 = unsigned int16_t;
using s32 = signed   int32_t;
using u32 = unsigned int32_t;
using s64 = signed   int64_t;
using u64 = unsigned int64_t;
#endif
