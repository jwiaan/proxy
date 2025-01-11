#pragma once

#include "../file.h"
#include <memory>

struct Connector : File {
  virtual void connect(uint32_t, const std::string &, const std::string &) = 0;
  virtual int connect(uint32_t &) = 0;
  static std::shared_ptr<Connector> create();
};