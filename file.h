#pragma once

struct File {
  virtual ~File() = default;
  virtual int fd() const = 0;
};
