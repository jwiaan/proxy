#pragma once

#include "../file.h"
#include <memory>
#include <netinet/in.h>

struct Server : File {
  virtual uint32_t id() const = 0;
  virtual bool writeAll() = 0;
  virtual bool recvData(std::string &) = 0;
  virtual bool sendData(const std::string &) = 0;
  static std::shared_ptr<Server> create(uint32_t, const std::string &, int);
};
