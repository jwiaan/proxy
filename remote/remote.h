#pragma once

#include <memory>
#include <openssl/ssl.h>

struct Remote {
  virtual ~Remote() = default;
  virtual void start() = 0;
  static std::shared_ptr<Remote> create(int, SSL_CTX *);
};
