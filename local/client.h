#pragma once

#include "../file.h"
#include <memory>
#include <optional>

struct Client : File {
  virtual uint32_t id() const = 0;
  virtual bool writeAll() = 0;
  virtual bool recvMsg(std::optional<std::string> &) = 0;
  virtual bool sendResp(bool, uint32_t, uint16_t) = 0;
  virtual bool sendData(const std::string &) = 0;
  static std::shared_ptr<Client> create(uint32_t, int);
};
