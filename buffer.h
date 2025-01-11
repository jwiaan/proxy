#pragma once

#include <memory>
#include <optional>

struct Buffer {
  virtual ~Buffer() = default;
  virtual int fd() const = 0;
  virtual bool empty() const = 0;
  virtual bool read() = 0;
  virtual bool write() = 0;
  virtual bool write(const std::string &) = 0;
  virtual void pop(size_t) = 0;
  virtual std::string pop() = 0;
  virtual std::optional<std::string> peek(size_t) const = 0;
  static std::shared_ptr<Buffer> create(int);
};
