#include "buffer.h"
#include "common.h"

Buffer::Buffer(int fd) : _fd(fd) { BIO_socket_nbio(_fd, 1); }

Buffer::~Buffer() { close(_fd); }

bool Buffer::read() {
  while (true) {
    char b[1024 * 1024];
    auto n = ::read(_fd, b, sizeof(b));
    if (n < 0) {
      if (errno == EAGAIN)
        return true;

      ERR("read: ", strerror(errno));
      return false;
    }

    if (n == 0)
      return false;

    _in += std::string(b, n);
  }
}

bool Buffer::write() {
  while (!_out.empty()) {
    auto n = ::write(_fd, _out.data(), _out.size());
    if (n < 0) {
      if (errno == EAGAIN)
        return true;

      ERR("write ", strerror(errno));
      return false;
    }

    _out.erase(0, n);
  }

  return true;
}
