#include "buffer.h"
#include "common.h"

namespace impl {
class Buffer : public ::Buffer {
public:
  Buffer(int fd) : _fd(fd) { BIO_socket_nbio(_fd, 1); }
  ~Buffer() { close(_fd); }
  int fd() const override { return _fd; }
  bool empty() const override { return _in.empty(); }
  bool read() override;
  bool write() override;
  bool write(const std::string &) override;
  void pop(size_t n) override { _in.erase(0, n); }
  std::string pop() override;
  std::optional<std::string> peek(size_t) const override;

private:
  int _fd;
  std::string _in, _out;
};

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

bool Buffer::write(const std::string &s) {
  _out += s;
  return write();
}

std::string Buffer::pop() {
  auto s = std::move(_in);
  _in.clear();
  return s;
}

std::optional<std::string> Buffer::peek(size_t n) const {
  if (_in.size() < n)
    return std::nullopt;

  return _in.substr(0, n);
}
} // namespace impl

std::shared_ptr<Buffer> Buffer::create(int fd) {
  return std::make_shared<impl::Buffer>(fd);
}
