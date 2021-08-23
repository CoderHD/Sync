#pragma once
#include "Types.h"
#include <memory>

enum SeekPositions
{
  SEEK_POS_START, SEEK_POS_END
};

struct BaseStream
{
  virtual uint read(char *out, uint size) = 0;
  virtual bool write(char *in, uint size) = 0;
  virtual bool available() = 0;
  virtual bool seek(uint pos) = 0;
};

class MemoryStream : public BaseStream
{
 private:
  char *buff;
  uint size, pointer;
 public:
  MemoryStream(char *buff, uint size);
  uint read(char *out, uint size) override;
  bool write(char *in, uint size) override;
  bool available() override;
  bool seek(uint pos) override;
};

template <typename T>
inline MemoryStream makeMemoryStream(T t) {
  uint size = sizebytes(T);
  char buff[size];
  MemoryStream stream(buff, size);
  stream.write(t, size);
  return stream;
}

#if defined(LIVE)
#include <cstdio>

class FileStream : public BaseStream
{
 private:
  std::FILE *file;
  long size;
  bool readMode, writeMode;
 public:
  FileStream(const char *path, const char *mode);
  void open(const char *path, const char *mode);
  uint read(char *out, uint outSize) override;
  bool write(char *in, uint inSize) override;
  bool available() override;
  bool seek(uint pos) override;
};
#endif
