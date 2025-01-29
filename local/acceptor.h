#pragma once

#include "../handler.h"

struct Acceptor : Handler {
  virtual int accept(uint32_t &) = 0;
  static std::shared_ptr<Acceptor> create(int);
};
