#pragma once
#include "Endian.h"
#include "Types.h"

namespace Sync {
    struct Buffer {
        u8* buff;
        u32 i;
        u32 size;
    
        Buffer(u32 size);
        ~Buffer();
        
        u64 finish();
        void offset(u64 offset);
    };

    bool bendian_read_bool(Buffer* buff);
    u8  bendian_read_u8(Buffer* buff);
    u16 bendian_read_u16(Buffer* buff);
    u32 bendian_read_u32(Buffer* buff);
    u64 bendian_read_u64(Buffer* buff);
    f32 bendian_read_f32(Buffer* buff);
    f64 bendian_read_f64(Buffer* buff);
    u8* bendian_read_ptr(Buffer* buff, u32 len);
    char* bendian_read_string(Buffer* buff, u32& len);
    void bendian_read(Buffer* buff, void* to, u32 len);

    template<typename T>
    void bendian_write(Buffer* buff, T val) {
        makeBEndian(val);
        memcpy(buff->buff + buff->i, &val, sizeof(T));
        buff->i += sizeof(T);
    }
    template<> 
    void bendian_write<u8>(Buffer* buff, u8 val);
    template<>
    void bendian_write<bool>(Buffer* buff, bool val);
    void bendian_write(Buffer* buff, char* data, u32 len);
    void bendian_write(Buffer* buff, void* data, u32 len);
};