#pragma once

#include "handler.h"
#include <openssl/ssl.h>
#include <vector>

struct Tunnel : Handler {
  virtual bool writeAll() = 0;
  virtual bool recvMsgs(std::vector<std::string> &) = 0;
  virtual bool sendMsg(const std::string &) = 0;
  static std::shared_ptr<Tunnel> create(int, SSL_CTX *, bool);
};
