#pragma once

#include <memory>
#include <openssl/ssl.h>

struct Client {
  virtual ~Client() = default;
  virtual void start() = 0;
  static std::shared_ptr<Client> create(int, SSL *);
};
