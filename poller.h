#pragma once

#include <memory>
#include <sys/epoll.h>
#include <vector>

struct File;

struct Poller {
  virtual ~Poller() = default;
  virtual void add(std::shared_ptr<File>, uint32_t) = 0;
  virtual int wait(std::vector<epoll_event> &) = 0;
  static std::shared_ptr<Poller> create();
};
