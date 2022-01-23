#include "Buffer.h"
#include "Log.h"
#include "Compatability.h"
#include "Endian.h"
#include <cmath>
#include <memory>

using namespace Sync;

Sync::Buffer::Buffer(u32 size) {
	buff = new u8[size];
	i = 0;
	memset(buff, 0, size);
}

Sync::Buffer::~Buffer() {
	delete[] buff;
}

u64 Sync::Buffer::finish() {
	u64 _i = i;
	i = 0;
	return _i;
}

void Sync::Buffer::offset(u64 offset) {
	i += offset;
}

bool Sync::bendian_read_bool(Buffer* buff) {
	bool val = (bool)buff->buff[buff->i];
	buff->i += sizeof(bool);
	return val;
}

u8 Sync::bendian_read_u8(Buffer* buff) {
	u8 val = (u8)buff->buff[buff->i];
	buff->i += sizeof(u8);
	return val;
}

u16 Sync::bendian_read_u16(Buffer* buff) {
	u16 val;
	memcpy(&val, buff->buff + buff->i, sizeof(u16));
	makeBEndian(val);
	buff->i += sizeof(u16);
	return val;
}

u32 Sync::bendian_read_u32(Buffer* buff) {
	u32 val;
	memcpy(&val, buff->buff + buff->i, sizeof(u32));
	makeBEndian(val);
	buff->i += sizeof(u32);
	return val;
}

u64 Sync::bendian_read_u64(Buffer* buff) {
	u64 val;
	memcpy(&val, buff->buff + buff->i, sizeof(u64));
	makeBEndian(val);
	buff->i += sizeof(u64);
	return val;
}

f32 Sync::bendian_read_f32(Buffer* buff) {
	f32 val;
	memcpy(&val, buff->buff + buff->i, sizeof(f32));
	buff->i += sizeof(f32);
	return val;
}

f64 Sync::bendian_read_f64(Buffer* buff) {
	f64 val;
	memcpy(&val, buff->buff + buff->i, sizeof(f64));
	buff->i += sizeof(f64);
	return val;
}

u8* Sync::bendian_read_ptr(Buffer* buff, u32 len) {
	u8* ptr = buff->buff + buff->i;
	buff->i += len;
	return ptr;
}

char* Sync::bendian_read_string(Buffer* buff, u32& len) {
	len = bendian_read_u32(buff);
	char* str = (char*)(buff->buff + buff->i);
	buff->i += len;
	return str;
}

void Sync::bendian_read(Buffer* buff, void* to, u32 len) {
	memcpy(to, buff->buff + buff->i, len); 
	buff->i += len;
}

template<>
void Sync::bendian_write<u8>(Buffer* buff, u8 val) {
	buff->buff[buff->i] = val;
	buff->i++;
}

template<>
void Sync::bendian_write<bool>(Buffer* buff, bool val) {
	buff->buff[buff->i] = val;
	buff->i++;
}

void Sync::bendian_write(Buffer* buff, char* data, u32 len) {
	bendian_write(buff, len);
	memcpy(buff->buff + buff->i, data, len);
	buff->i += len;
}

void Sync::bendian_write(Buffer* buff, void* data, u32 len) {
	memcpy(buff->buff + buff->i, data, len);
	buff->i += len;
}