#pragma once

#include "../file.h"
#include <memory>

struct Acceptor : File {
  virtual int accept(uint32_t &) = 0;
  static std::shared_ptr<Acceptor> create(int);
};
