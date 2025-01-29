#pragma once

#include "../handler.h"

struct Server : Handler {
  virtual uint32_t id() const = 0;
  virtual bool writeAll() = 0;
  virtual bool recvData(std::string &) = 0;
  virtual bool sendData(const std::string &) = 0;
  static std::shared_ptr<Server> create(int, uint32_t, const std::string &);
};
