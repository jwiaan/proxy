#pragma once

#include <memory>

struct Reader {
  virtual ~Reader() = default;
  virtual bool read() = 0;
  virtual std::string get(size_t n) = 0;
  static std::shared_ptr<Reader> create(int);
};