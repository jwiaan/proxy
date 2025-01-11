#pragma once

#include <memory>
#include <openssl/ssl.h>

struct Local {
  virtual ~Local() = default;
  virtual void start() = 0;
  static std::shared_ptr<Local> create(int, int, SSL_CTX *);
};
