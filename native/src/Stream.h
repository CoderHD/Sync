#pragma once
#include "Types.h"
#include <memory>
#include <cmath>

class Buffer;

enum SeekPositions
{
	SEEK_POS_START, SEEK_POS_END
};

struct Stream
{
	virtual uint read(void* out, uint size) = 0;
	virtual bool write(void* in, uint size) = 0;
	virtual bool available() = 0;
	virtual bool seek(uint pos) = 0;
};

class MemoryStream : public Stream
{
private:
	char* buff;
	uint size, pointer;
public:
	MemoryStream(char* buff, uint size);
	uint read(void* out, uint size) override;
	bool write(void* in, uint size) override;
	bool available() override;
	bool seek(uint pos) override;
};

class WaitingStream : public Stream
{
private:
	uint index, size;
	Buffer* data;
	bool waiting;
public:
	WaitingStream(uint size);
	void fill(Buffer* data);
	uint read(void* out, uint size) override;
	bool write(void* in, uint size) override;
	bool available() override;
	bool seek(uint pos) override;
};

#if defined(LIVE)
#include <cstdio>

class FileStream : public Stream
{
private:
	std::FILE* file;
	long size;
	bool readMode, writeMode;
public:
	FileStream(const char* path, const char* mode);
	void open(const char* path, const char* mode);
	uint read(void* out, uint outSize) override;
	bool write(void* in, uint inSize) override;
	bool available() override;
	bool seek(uint pos) override;
};
#endif
