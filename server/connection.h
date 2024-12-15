#pragma once

#include <memory>

namespace common {
struct Msg;
}

struct Connection {
  virtual ~Connection() = default;
  virtual uint32_t id() const = 0;
  virtual bool read(common::Msg &) = 0;
  virtual bool write(const common::Msg &) = 0;
  static std::shared_ptr<Connection> create(int, uint32_t, uint32_t);
};
