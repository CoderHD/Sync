#include "Stream.h"
#include "Backend.h"

MemoryStream::MemoryStream(char* buff, uint size) : buff(buff), size(size), pointer(0)
{
}

uint MemoryStream::read(void* out, uint outSize)
{
	uint free = size - pointer - 1;
	if (free < outSize) {
		memcpy(out, buff + pointer, free);
		return free;
	}
	else {
		memcpy(out, buff + pointer, outSize);
		return outSize;
	}
}

bool MemoryStream::write(void* in, uint inSize)
{
	return false;
}

bool MemoryStream::available()
{
	return size - pointer - 1 == 0;
}

bool MemoryStream::seek(uint pos)
{
	switch (pos) {
	case SeekPositions::SEEK_POS_START:
		pointer = 0;
		break;
	case SeekPositions::SEEK_POS_END:
		pointer = size - 1;
		break;
	default:
		return false;
	}
	return true;
}

WaitingStream::WaitingStream(uint size) : index(0), size(size), data(null), waiting(true) {
}

void WaitingStream::fill(Buffer* data) {
	this->data = data;
	waiting = false;
}

uint WaitingStream::read(void* out, uint size) {
	while (waiting) {}
	uint length = std::min(data->size, size);
	memcpy(out, data->data, length);
	return length;
}

bool WaitingStream::write(void* in, uint size) {
	return false;
}

bool WaitingStream::available() {
	return index < size;
}

bool WaitingStream::seek(uint pos) {
	return false;
}

#if defined(LIVE)
FileStream::FileStream(const char* path, const char* mode)
{
	open(path, mode);
}

void FileStream::open(const char* path, const char* mode)
{
	file = fopen(path, mode);
	fseek(file, 0, SEEK_END);
	size = ftell(file);
	fseek(file, 0, SEEK_SET);
	if (mode[0] == 'r') { readMode = true; writeMode = false; }
	if (mode[0] == 'w') { readMode = false; writeMode = true; }
	if (mode[0] == 'r' && mode[1] == 'w' || mode[0] == 'w' && mode[1] == 'r') { readMode = true; writeMode = true; }
}

bool FileStream::close() {
	return fclose(file);
}

uint FileStream::read(void* out, uint outSize)
{
	if (readMode) return fread(&out, outSize, 1, file);
	else return -1;
}

bool FileStream::write(void* in, uint inSize)
{
	if (writeMode)
	{
		fwrite(&in, inSize, 1, file);
		return true;
	}
	else return false;
}

bool FileStream::available()
{
	return size - ftell(file) - 1 == 0;
}

bool FileStream::seek(uint pos)
{
	switch (pos) {
	case SeekPositions::SEEK_POS_START:
		fseek(file, 0, SEEK_SET);
		break;
	case SeekPositions::SEEK_POS_END:
		fseek(file, 0, SEEK_END);
		break;
	default:
		return false;
	}
	return true;
}
#endif
