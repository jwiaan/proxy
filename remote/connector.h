#pragma once

#include "../handler.h"

struct Connector : Handler {
  virtual void connect(uint32_t, const std::string &, const std::string &) = 0;
  virtual int connect(uint32_t &, std::string &) = 0;
  static std::shared_ptr<Connector> create();
};