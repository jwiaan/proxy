#include "poller.h"
#include "common.h"
#include "handler.h"

Poller::Poller() { _fd = epoll_create1(0); }

Poller::~Poller() { close(_fd); }

void Poller::add(std::shared_ptr<Handler> h, uint32_t u) {
  epoll_event e;
  e.data.ptr = h.get();
  e.events = u;
  auto a = epoll_ctl(_fd, EPOLL_CTL_ADD, h->fd(), &e);
  assert(!a);
}

int Poller::wait(std::vector<epoll_event> &v) {
  auto n = epoll_wait(_fd, v.data(), v.size(), -1);
  assert(n > 0);
  return n;
}
