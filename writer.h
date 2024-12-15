#pragma once

#include <memory>

struct Writer {
  virtual ~Writer() = default;
  virtual bool write(const std::string &) = 0;
  static std::shared_ptr<Writer> create(int);
};
