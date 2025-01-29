#pragma once

#include <memory>

struct Handler {
  virtual ~Handler() = default;
  virtual int fd() const = 0;
};
