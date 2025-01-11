#include "acceptor.h"
#include "../common.h"

namespace impl {
class Acceptor : public ::Acceptor {
public:
  Acceptor(int fd) : _fd(fd) {}
  int fd() const override { return _fd; }
  int accept(uint32_t &) override;

private:
  int _fd;
  uint32_t _id = 0;
};

int Acceptor::accept(uint32_t &id) {
  auto fd = ::accept(_fd, nullptr, nullptr);
  if (fd < 0) {
    ERR("accept: ", strerror(errno));
    return -1;
  }

  id = _id++;
  return fd;
}
} // namespace impl

std::shared_ptr<Acceptor> Acceptor::create(int fd) {
  return std::make_shared<impl::Acceptor>(fd);
}
