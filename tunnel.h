#pragma once

#include <memory>
#include <openssl/ssl.h>

namespace common {
struct Msg;
}

struct Tunnel {
  virtual ~Tunnel() = default;
  virtual int fd() const = 0;
  virtual bool read(common::Msg &) = 0;
  virtual bool write(const common::Msg &) = 0;
  static std::shared_ptr<Tunnel> create(SSL *);
};
