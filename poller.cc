#include "poller.h"
#include "common.h"
#include "file.h"

namespace impl {
class Poller : public ::Poller {
public:
  Poller() { _fd = epoll_create1(0); }
  ~Poller() { close(_fd); }
  void add(std::shared_ptr<File>, uint32_t) override;
  int wait(std::vector<epoll_event> &) override;

private:
  int _fd;
};

void Poller::add(std::shared_ptr<File> f, uint32_t u) {
  epoll_event e;
  e.data.ptr = f.get();
  e.events = u;
  auto a = epoll_ctl(_fd, EPOLL_CTL_ADD, f->fd(), &e);
  assert(!a);
}

int Poller::wait(std::vector<epoll_event> &v) {
  auto a = epoll_wait(_fd, v.data(), v.size(), -1);
  assert(a > 0);
  return a;
}
} // namespace impl

std::shared_ptr<Poller> Poller::create() {
  return std::make_shared<impl::Poller>();
}
