#pragma once

#include <memory>

struct Helper {
  virtual ~Helper() = default;
  virtual uint32_t id() = 0;
  static std::shared_ptr<Helper> create();
};
