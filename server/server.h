#pragma once

#include <memory>
#include <openssl/ssl.h>

struct Server {
  virtual ~Server() = default;
  virtual void start() = 0;
  static std::shared_ptr<Server> create(SSL *);
};
