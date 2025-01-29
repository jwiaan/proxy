#pragma once

#include <memory>
#include <sys/epoll.h>
#include <vector>

struct Handler;

class Poller {
protected:
  Poller();
  ~Poller();
  void add(std::shared_ptr<Handler>, uint32_t);
  int wait(std::vector<epoll_event> &);

private:
  int _fd;
};
