#include "poller.h"
#include "common.h"

class PollerImpl : public Poller {
public:
  PollerImpl() { _fd = epoll_create1(0); }
  ~PollerImpl() { close(_fd); }
  int add(int, void *) override;
  int wait(epoll_event *, int) override;
  std::string print(uint32_t) const override;

private:
  int _fd;
};

int PollerImpl::add(int fd, void *ptr) {
  epoll_event e;
  e.events = EPOLLIN;
  e.data.ptr = ptr;
  return epoll_ctl(_fd, EPOLL_CTL_ADD, fd, &e);
}

int PollerImpl::wait(epoll_event *e, int n) {
  return epoll_wait(_fd, e, n, -1);
}

std::string PollerImpl::print(uint32_t e) const {
  std::string s;
  if (e & EPOLLIN)
    s += "EPOLLIN|";
  if (e & EPOLLPRI)
    s += "EPOLLPRI|";
  if (e & EPOLLOUT)
    s += "EPOLLOUT|";
  if (e & EPOLLRDNORM)
    s += "EPOLLRDNORM|";
  if (e & EPOLLRDBAND)
    s += "EPOLLRDBAND|";
  if (e & EPOLLWRNORM)
    s += "EPOLLWRNORM|";
  if (e & EPOLLWRBAND)
    s += "EPOLLWRBAND|";
  if (e & EPOLLMSG)
    s += "EPOLLMSG|";
  if (e & EPOLLERR)
    s += "EPOLLERR|";
  if (e & EPOLLHUP)
    s += "EPOLLHUP|";
  if (e & EPOLLRDHUP)
    s += "EPOLLRDHUP|";
  if (e & EPOLLEXCLUSIVE)
    s += "EPOLLEXCLUSIVE|";
  if (e & EPOLLWAKEUP)
    s += "EPOLLWAKEUP|";
  if (e & EPOLLONESHOT)
    s += "EPOLLONESHOT|";
  if (e & EPOLLET)
    s += "EPOLLET|";
  s.pop_back();
  return s;
}

std::shared_ptr<Poller> Poller::create() {
  return std::make_shared<PollerImpl>();
}
