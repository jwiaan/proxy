#pragma once

#include <string>

class Buffer {
protected:
  Buffer(int);
  ~Buffer();
  bool read();
  bool write();

protected:
  int _fd;
  std::string _in, _out;
};
